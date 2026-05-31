# 基于RK3588的嵌入式多终端通信系统

## 一、项目概述

### 1.1 项目名称
基于RK3588的嵌入式多终端通信系统

### 1.2 项目背景
在工业控制、智能终端等领域，多设备之间的实时通信是核心需求。本项目基于RK3588高性能嵌入式平台，设计并实现一套多终端通信系统，涵盖进程间通信（IPC）的全部核心机制，展示嵌入式Linux系统下的分布式微服务架构设计能力。

### 1.3 项目目标
- 实现6个独立进程的微服务架构
- 覆盖全部IPC通信方式：共享内存、消息队列、管道、互斥锁
- 设计自定义通信协议
- SQLite数据库持久化存储
- 达到工业级可靠性标准
- 预留语音芯片（LD3320）扩展接口

### 1.4 技术平台
- 硬件：RK3588开发板（ARM Cortex-A76 + Cortex-A55）
- 操作系统：Linux（Debian/Ubuntu-based）
- 编程语言：C（核心通信模块）+ Python（管理界面）
- 数据库：SQLite 3
- IPC机制：POSIX共享内存、System V消息队列、命名管道、互斥锁

---

## 二、系统架构

### 2.1 架构总览

```
┌─────────────────────────────────────────────────┐
│                  管理控制台（Console）              │
│              Python / CLI 管理界面                 │
└───────────┬─────────────────────────┬───────────┘
            │                         │
    ┌───────▼───────┐         ┌───────▼───────┐
    │  消息路由中心   │◄───────►│  协议解析引擎  │
    │  (Router)     │         │  (Protocol)   │
    └───┬───┬───┬───┘         └───────────────┘
        │   │   │
   ┌────┘   │   └────┐
   ▼        ▼        ▼
┌──────┐ ┌──────┐ ┌──────┐
│终端A │ │终端B │ │终端C │
│(Dev) │ │(Dev) │ │(Dev) │
└──────┘ └──────┘ └──────┘
```

### 2.2 六大微服务进程

| 进程名称 | 功能描述 | 通信方式 |
|---------|---------|---------|
| router | 消息路由中心，负责所有消息的转发与分发 | 共享内存 + 消息队列 |
| protocol | 协议解析引擎，编解码自定义协议 | 共享内存 |
| storage | 数据存储服务，SQLite读写 | 管道 + 互斥锁 |
| monitor | 系统监控，进程状态检测 | 共享内存（只读） |
| terminal | 终端接入服务，管理设备连接 | 消息队列 + 管道 |
| console | 管理控制台，提供CLI操作界面 | 管道 + 信号 |

---

## 三、IPC通信机制设计

### 3.1 共享内存（Shared Memory）
- 用途：进程间高速数据交换（router ↔ protocol, monitor只读）
- 实现：POSIX shm_open() + mmap()
- 同步：配合互斥锁防止竞态
- 容量：设计环形缓冲区，容量64KB

### 3.2 消息队列（Message Queue）
- 用途：异步消息传递（terminal ↔ router）
- 实现：System V msgget() + msgsnd() + msgrcv()
- 特性：支持优先级排序，FIFO + 紧急消息通道
- 队列深度：最大100条消息

### 3.3 管道（Pipe）
- 用途：单向数据流（storage读写, console命令下发）
- 实现：命名管道（FIFO）mkfifo()
- 场景：日志流、配置下发、数据库查询结果回传

### 3.4 互斥锁（Mutex）
- 用途：保护共享资源（共享内存区域、数据库连接）
- 实现：POSIX pthread_mutex + 命名互斥锁
- 策略：读写锁分离（monitor只读，storage读写）

---

## 四、自定义通信协议

### 4.1 协议帧格式

```
┌──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│  帧头     │  版本     │  消息类型 │  源终端   │  目标终端 │  数据长度 │
│  2 bytes │  1 byte  │  1 byte  │  2 bytes │  2 bytes │  2 bytes │
├──────────┴──────────┴──────────┴──────────┴──────────┴──────────┤
│                        数据载荷（Payload）                       │
│                      0 ~ 1024 bytes                            │
├─────────────────────────────────────────────────────────────────┤
│                        CRC16 校验                              │
│                       2 bytes                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 4.2 消息类型定义

| 类型码 | 名称 | 说明 |
|-------|------|------|
| 0x01 | MSG_HEARTBEAT | 心跳包 |
| 0x02 | MSG_REGISTER | 终端注册 |
| 0x03 | MSG_DATA | 数据消息 |
| 0x04 | MSG_ACK | 确认应答 |
| 0x05 | MSG_BROADCAST | 广播消息 |
| 0x06 | MSG_CONFIG | 配置下发 |
| 0x07 | MSG_STATUS | 状态上报 |
| 0x08 | MSG_VOICE | 语音数据（预留LD3320扩展） |
| 0xFF | MSG_ERROR | 错误响应 |

### 4.3 终端编号

| 编号 | 名称 | 说明 |
|------|------|------|
| 0x0000 | TERMINAL_ROUTER | 路由中心 |
| 0x0001~0x00FF | 终端动态分配 | 系统自动分配 |
| 0xFFFF | TERMINAL_ALL | 广播目标 |

---

## 五、数据库设计

### 5.1 SQLite表结构

```sql
-- 终端信息表
CREATE TABLE terminals (
    id INTEGER PRIMARY KEY,
    terminal_id INTEGER UNIQUE NOT NULL,
    name TEXT NOT NULL,
    ip_address TEXT,
    status INTEGER DEFAULT 0,
    registered_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    last_heartbeat DATETIME
);

-- 消息日志表
CREATE TABLE message_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    msg_type INTEGER NOT NULL,
    src_terminal INTEGER,
    dst_terminal INTEGER,
    payload_size INTEGER,
    payload_data BLOB,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 系统配置表
CREATE TABLE config (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
```

---

## 六、项目文件结构

```
rk3588-multiterminal/
├── doc/                    # 项目文档
│   └── README.md           # 本文档
├── src/                    # 源代码
│   ├── common/             # 公共模块
│   │   ├── protocol.h      # 协议定义
│   │   ├── protocol.c      # 协议编解码
│   │   ├── ipc.h           # IPC封装
│   │   └── ipc.c           # IPC实现
│   ├── router/             # 消息路由中心
│   │   └── router.c
│   ├── protocol_engine/    # 协议解析引擎
│   │   └── protocol_engine.c
│   ├── storage/            # 数据存储服务
│   │   └── storage.c
│   ├── monitor/            # 系统监控
│   │   └── monitor.c
│   ├── terminal/           # 终端接入服务
│   │   └── terminal.c
│   └── console/            # 管理控制台
│       └── console.py      # Python CLI
├── include/                # 头文件
│   └── ...
├── sql/                    # 数据库脚本
│   └── init.sql
├── config/                 # 配置文件
│   └── system.conf
├── tests/                  # 测试用例
│   └── ...
├── Makefile                # 构建脚本
└── README.md               # 项目说明
```

---

## 七、开发计划

### 阶段一：基础框架（第1周）
- [ ] 搭建RK3588交叉编译环境
- [ ] 实现IPC封装层（共享内存、消息队列、管道、互斥锁）
- [ ] 定义自定义协议格式
- [ ] 实现协议编解码模块

### 阶段二：核心服务（第2周）
- [ ] 实现消息路由中心（router）
- [ ] 实现协议解析引擎（protocol）
- [ ] 实现终端接入服务（terminal）
- [ ] 实现数据存储服务（storage）+ SQLite

### 阶段三：监控与管理（第3周）
- [ ] 实现系统监控进程（monitor）
- [ ] 实现管理控制台（console）
- [ ] 端到端通信测试
- [ ] 性能压测与优化

### 阶段四：扩展与完善（第4周）
- [ ] 预留LD3320语音芯片接口
- [ ] 添加日志系统
- [ ] 编写测试用例
- [ ] 整理文档与代码Review

---

## 八、技术亮点

1. **IPC全覆盖**：一个项目涵盖Linux下全部4种IPC机制，展示系统编程能力
2. **微服务架构**：6个独立进程，进程间松耦合，符合工业级设计
3. **自定义协议**：完整的帧格式设计，含校验、分包、重组
4. **SQLite持久化**：轻量级嵌入式数据库，无需额外服务
5. **可扩展性**：预留语音芯片接口，可无缝扩展为语音通信系统
6. **工业级标准**：心跳检测、超时重传、异常恢复、日志审计

---

## 九、扩展规划

### 语音通信扩展（LD3320）
- LD3320为关键词识别芯片，通过串口发送识别结果
- 预留消息类型 MSG_VOICE (0x08) 用于语音数据传输
- 后续可扩展为语音指令控制系统

### 多板互联
- 支持多块RK3588开发板通过以太网/WiFi互联
- 路由中心支持跨板消息转发
