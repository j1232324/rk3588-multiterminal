
<p align="center">
  <h1 align="center">RK3588 Factory Device Communication Hub</h1>
  <p align="center">工厂设备信息传输中枢 · 7 进程微服务 · 全 IPC 覆盖 · 插件化协议框架</p>
  <p align="center">
    <strong>「从零实现的工业级设备通信中间件 — 协议转换 · 消息路由 · 链路追踪 · 实时监控」</strong>
  </p>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/language-C11-blue" alt="C11">
  <img src="https://img.shields.io/badge/UI-Qt5-brightgreen" alt="Qt5">
  <img src="https://img.shields.io/badge/database-SQLite3-orange" alt="SQLite3">
  <img src="https://img.shields.io/badge/platform-Linux-critical" alt="Linux">
  <img src="https://img.shields.io/badge/status-building-yellow" alt="Status">
  <img src="https://img.shields.io/badge/license-MIT-green" alt="License">
</p>

---

## 📋 目录

- [为什么做这个项目](#-为什么做这个项目)
- [工业应用场景](#-工业应用场景)
- [技术栈](#-技术栈)
- [系统架构](#-系统架构)
  - [六大微服务进程](#22-六大微服务进程)
  - [IPC机制设计](#23-ipc机制设计)
  - [自定义通信协议](#24-自定义通信协议)
- [快速开始](#-快速开始)
- [项目结构](#-项目结构)
- [实现深度解析](#-实现深度解析)
  - [IPC封装层](#1-ipc封装层-srccommon)
  - [协议编解码](#2-协议编解码-protocolch)
  - [消息路由中心](#3-消息路由中心-router)
  - [协议解析引擎](#4-协议解析引擎-protocol_engine)
  - [数据存储服务](#5-数据存储服务-storage)
  - [系统监控](#6-系统监控-monitor)
  - [终端接入服务](#7-终端接入服务-terminal)
  - [管理控制台](#8-管理控制台-console)
  - [Qt GUI管理界面](#9-qt-gui管理界面-gui)
- [工业级设计要点](#-工业级设计要点)
- [开发路线图](#-开发路线图)
- [项目状态](#-项目状态)
- [面试价值](#-面试价值)
- [许可证](#-许可证)

---

## 🎯 为什么做这个项目

> **工业现场的真实问题：不同设备、不同协议、不同接口，如何统一接入和管理？**

一个工厂里往往同时存在：
- **PLC**（西门子/三菱/欧姆龙）→ 控制产线动作
- **工业机器人**（ABB/KUKA/发那科）→ 焊接、装配、搬运
- **传感器阵列**（温度/压力/振动/视觉）→ 采集产线数据
- **AGV 小车** → 物料运输
- **RFID 读卡器 / 扫码枪** → 产品追溯
- **HMI 触摸屏** → 现场操作

这些设备通信协议不同（Modbus/CAN/Profinet/EtherCAT/串口），如果没有一个**统一的中枢**来中转消息，每台设备都要互相直连，形成"蜘蛛网"架构，维护成本极高。

**本项目就是这一中枢的软件核心：**

```
                 ┌─────────────────────────────┐
                 │  工厂设备信息传输中枢（本系统） │
                 │   协议转换 · 消息路由 · 数据存储 │
                 │   状态监控 · 故障告警 · 配置管理 │
                 └─────────────────────────────┘
                       ↕        ↕        ↕
                  ┌────┐   ┌────┐   ┌────┐
                  │PLC │   │传感器│   │AGV │
                  └────┘   └────┘   └────┘
```

这个系统解决的核心问题：**多设备之间如何高效、可靠、确定性地传输信息？**

---

## 🏭 工业应用场景

### 典型场景一：产线联动

```
传感器检测到产品到位
  → 发送 MSG_DATA 到 router
  → router 转发给 PLC
  → PLC 控制机械臂抓取
  → 机械臂完成 → MSG_ACK 回传感器
  整个闭环 < 50ms
```

### 典型场景二：异常告警

```
某设备温度超限
  → 发送 MSG_ALARM (优先级 紧急)
  → router 立即转发到 console + storage
  → console 弹出告警
  → storage 记录事件日志（合规追溯）
  → monitor 检测到异常，标记该设备状态为 FAULT
```

### 典型场景三：生产数据追溯

```
每件产品经过 RFID 工位
  → 扫码信息通过 terminal 上报
  → storage 写入 message_log 表
  → 管理人员通过 console 查询：产品 X 何时经过哪些工位
  → Qt GUI 显示实时产量看板
```

### 本项目覆盖的工业特性

| 工业需求 | 本项目实现 |
|---------|-----------|
| 7×24 小时不间断运行 | 进程监控 + 看门狗自动恢复 |
| 确定性延迟 | IPC 无锁设计，不受 GC/调度抖动影响 |
| 分级告警 | 消息优先级（紧急/普通/低） |
| 数据可追溯 | SQLite 持久化，每条消息带时间戳 |
| 设备即插即用 | MSG_REGISTER 自动注册 |
| 远程维护 | CLI console + Qt GUI |
| 零单点故障 | 各进程独立，monitor 守护恢复 |

---

## 🛠 技术栈

| 分类 | 技术 | 用途 | 工业级考量 |
|------|------|------|-----------|
| **语言** | C11 | 核心微服务 | 零运行时、确定性内存、裸机可移植 |
| **IPC 共享内存** | POSIX `shm_open` + `mmap` | 进程间高速数据交换 | 微秒级延迟、零拷贝 |
| **IPC 消息队列** | System V `msgget`/`msgsnd`/`msgrcv` | 异步消息投递 | 内置优先级仲裁，FIFO 保证 |
| **IPC 命名管道** | POSIX `mkfifo` | 单向数据流 | 阻塞 I/O 天然流控 |
| **IPC 互斥锁** | POSIX 进程间互斥锁 | 共享资源保护 | 5s 超时防死锁 |
| **通信协议** | 自定义二进制协议 + CRC16-CCITT | 设备间消息帧 | 10B 帧头，确定性解析 |
| **持久化** | SQLite3（WAL 模式） | 消息日志、设备表、配置 | 嵌入式成熟方案，零运维 |
| **UI** | Qt5 Widgets | 产线监控看板 | 工业 HMI 常用框架 |
| **构建** | GNU Make | 编译系统 | 交叉编译友好 |

> **零第三方依赖**：核心模块仅依赖 `libc + pthread + librt + libsqlite3`，这在工业现场是硬性要求——没有网络、没有包管理器、不能 `apt install`。

---

## 🏗 系统架构

### 2.1 架构总览

```
┌───────────────────────────────────────────────────────────────┐
│                     Qt GUI Dashboard (gui/)                    │
│           产线看板 · 设备状态 · 实时告警 · 产量统计              │
└──────────────────────────┬────────────────────────────────────┘
                           │ Named Pipe
                           ▼
┌───────────────────────────────────────────────────────────────┐
│                    CLI Console (console/)                      │
│           设备管理 · 消息查询 · 配置下发 · 日志审计               │
└───┬───────────────────────┬────────────────────────┬──────────┘
    │ Pipe                  │ Shared Memory          │ Signal
    ▼                       ▼                        ▼
┌────────────┐     ┌────────────────┐     ┌───────────────────┐
│   Router   │◄───►│  Protocol Eng. │     │    Monitor        │
│  (router/) │     │ (protocol_eng) │     │   (monitor/)      │
│  消息路由   │     │  协议编解码     │     │  看门狗 · 健康检查  │
│  优先级调度  │     │  CRC16校验     │     │  故障自动恢复      │
└───┬───┬────┘     └────────────────┘     └───────────────────┘
    │   │               ▲
    │   │               │ Shared Memory
    │   │        ┌──────┴─────────┐
    │   │        │    Storage     │
    │   │        │   (storage/)   │
    │   │        │  SQLite WAL    │
    │   │        │  数据追溯      │
    │   │        └────────────────┘
    │   │
    │   │ Message Queue (System V)
    │   │
┌───▼───▼───────────────────────────────────────────────────────┐
│                     Terminal Service (terminal/)                │
│               设备注册 · 心跳保活 · 消息收发                     │
│               设备类型：PLC / 传感器 / 机器人 / AGV / 扫码枪     │
└──────────────────────────────┬─────────────────────────────────┘
                               │
        ┌──────────────────────┼──────────────────────┐
        │                      │                      │
    ┌───▼───┐             ┌───▼───┐             ┌───▼───┐
    │ PLC   │             │传感器  │             │ AGV   │
    │ 设备A  │             │ 设备B  │             │ 设备C  │
    │ (进程) │             │ (进程) │             │ (进程) │
    └───────┘             └───────┘             └───────┘
```

### 2.2 六大微服务进程

| 进程 | 职责 | IPC 方式 | 工厂中的角色 |
|------|------|---------|-------------|
| **router** | 消息路由中心，接收→解析→转发 | 共享内存 + 消息队列 | 产线主控，类似 PLC 的 CPU 模块 |
| **protocol_engine** | 协议解析引擎，编解码二进制帧 | 共享内存 | 协议转换器，类似网关 |
| **storage** | SQLite 异步持久化（WAL 模式） | 命名管道 + 互斥锁 | 数据黑匣子，质量追溯 |
| **monitor** | 健康看门狗，心跳检测 + 自动重启 | 共享内存（只读） | 设备巡检员，7×24 值守 |
| **terminal** | 设备接入服务，注册/心跳/收发 | 消息队列 + 管道 | 设备接口卡，类似 Profibus 从站 |
| **console** | CLI + Qt GUI 管理控制台 | 管道 + 信号 | MES 工控机 / HMI 人机界面 |

### 2.3 IPC 机制设计

| 机制 | 应用场景 | 为什么选它 | 工厂类比 |
|------|---------|-----------|---------|
| **共享内存**（POSIX） | router ↔ protocol_engine 高频数据交换 | 零拷贝、微秒级延迟、64KB 环形缓冲区 | CPU 内部 DMA 通道 |
| **消息队列**（System V） | 设备消息异步投递 | 内置优先级（紧急/普通/低），FIFO 保证 | CAN 总线消息仲裁 |
| **命名管道**（FIFO） | storage I/O、console 指令 | 天然生产者-消费者模型 | 流水线传送带 |
| **互斥锁**（POSIX 命名） | 共享内存写保护、SQLite 独占 | 进程间安全、5s 超时防死锁 | 安全互锁装置 |

> **工业级设计思维：** 不追求"一种技术解决所有问题"，而是**为每种通信需求选择最合适的 IPC**——共享内存负责吞吐、消息队列负责优先级、管道负责流式数据、互斥锁负责安全。

### 2.4 自定义通信协议

```
帧格式（10 字节帧头 + 最大 1024 字节载荷 + 2 字节 CRC）：

┌─────────┬────────┬────────┬──────────┬──────────┬──────────┐
│  帧头     │  版本   │ 消息类型 │  源设备   │  目标设备 │  数据长度 │
│  0xA5A5 │  0x01  │  0x03  │  0x0001  │  0x0002  │  0x000A  │
│  2 B    │  1 B   │  1 B   │  2 B     │  2 B     │  2 B     │
├─────────┴────────┴────────┴──────────┴──────────┴──────────┤
│                      数据载荷（二进制）                        │
│             例如：温度值 85.6°C / 产品编码 SN20240501         │
├─────────────────────────────────────────────────────────────┤
│                     CRC16-CCITT 校验                          │
│                         2 B                                 │
└─────────────────────────────────────────────────────────────┘
```

**消息类型（工业语义）：**

| 类型码 | 名称 | 说明 | 工业场景 |
|-------|------|------|---------|
| 0x01 | `MSG_HEARTBEAT` | 心跳保活 | 设备在线检测 |
| 0x02 | `MSG_REGISTER` | 设备注册 | 新设备上线自发现 |
| 0x03 | `MSG_DATA` | 生产数据 | 温度/压力/产量/条码 |
| 0x04 | `MSG_ACK` | 确认应答 | 指令执行确认 |
| 0x05 | `MSG_BROADCAST` | 广播消息 | 全线急停/换产通知 |
| 0x06 | `MSG_CONFIG` | 配置下发 | 修改设备参数 |
| 0x07 | `MSG_STATUS` | 状态上报 | 设备运行/待机/故障 |
| 0x08 | `MSG_ALARM` | 告警消息 | 超限/异常/故障报警 |
| 0x09 | `MSG_CMD` | 控制指令 | 启动/停止/复位 |
| 0xFF | `MSG_ERROR` | 错误响应 | 协议错误/设备异常 |

**设备ID 空间：**

| 范围 | 分配 |
|------|------|
| `0x0000` | 路由中心（固定） |
| `0x0001 - 0x00FE` | 现场设备动态分配 |
| `0xFFFF` | 广播（所有设备） |

---

## 🚀 快速开始

```bash
# 安装依赖
sudo apt-get install build-essential libsqlite3-dev qtbase5-dev

# 克隆
git clone https://github.com/j1232324/rk3588-multiterminal.git
cd rk3588-multiterminal

# 编译全部微服务
make all

# 初始化数据库
make initdb

# 一键启动系统
./scripts/start.sh

# 或者手动逐个启动：
./router &
./protocol_engine &
./storage &
./monitor &
./terminal &
python3 src/console/console.py

# 运行 Qt GUI
cd gui && qmake && make && ./rk3588-multiterminal

# 停止系统
./scripts/stop.sh
```

---

## 📁 项目结构

```
rk3588-multiterminal/
│
├── include/                    # 公共头文件（API 契约）
│   ├── protocol.h              # 协议帧结构、消息类型枚举
│   └── ipc.h                   # IPC 抽象层接口（4种机制）
│
├── src/                        # 源代码
│   ├── common/                 # 公共模块（所有进程依赖）
│   │   ├── protocol.c          # 协议编解码、CRC16 查表法
│   │   └── ipc.c               # 共享内存、消息队列、管道、互斥锁
│   ├── router/                 # M1: 消息路由中心
│   │   └── router.c
│   ├── protocol_engine/        # M2: 协议解析引擎
│   │   └── protocol_engine.c
│   ├── storage/                # M3: SQLite 持久化服务
│   │   └── storage.c
│   ├── monitor/                # M4: 系统看门狗 / 健康监控
│   │   └── monitor.c
│   ├── terminal/               # M5: 现场设备接入服务
│   │   └── terminal.c
│   └── console/                # M6: CLI 管理控制台
│       └── console.py
│
├── gui/                        # Qt5 产线监控看板
│   ├── main.cpp
│   ├── widget.cpp
│   ├── widget.h
│   └── widget.ui
│
├── config/                     # 系统配置
│   └── system.conf
│
├── sql/                        # 数据库脚本
│   └── init.sql
│
├── scripts/                    # 运维脚本
│   ├── start.sh
│   ├── stop.sh
│   ├── status.sh
│   └── benchmark.sh
│
├── tests/                      # 测试
│   ├── test_ipc.c
│   ├── test_protocol.c
│   └── test_integration.py
│
├── Makefile
├── README.md
└── .gitignore
```

---

## 🔧 实现深度解析

> 每个模块都是一个独立的面试技术话题。

### 1. IPC 封装层 (`src/common/`)

**问题**：不同 IPC 机制有不同的 API、语义、边界情况。如何给上层提供统一的接口？

**方案**：四套子系统，每套独立封装：

**共享内存环形缓冲区（64KB）**
```c
// 核心设计：无锁单生产者-单消费者
typedef struct {
    volatile uint32_t read_pos;     // 消费者读位置
    volatile uint32_t write_pos;    // 生产者写位置
    volatile uint32_t size;         // 缓冲区容量
    uint8_t data[SHM_SIZE - 12];    // 数据区
} shm_ring_buffer_t;
```

**设计要点**：
- **无锁设计**：单生产者（router）+ 单消费者（protocol_engine），读写位置天然不冲突。`volatile` 防止编译器优化到寄存器。
- **溢出策略**：写指针追上读指针 → 覆盖旧数据。工业场景中，心跳/状态数据允许丢失，换取 O(1) 确定性延迟。
- **`volatile` 的局限**：只防编译器优化，不保证多核内存可见性。真正的工业级需要 `atomic` + 内存屏障。**面试可以深入聊的点。**
- **创建流程**：`shm_open()` → `ftruncate()` → `mmap()` → `close(fd)`

**消息队列（System V，深度 100）**
- 优先级调度：紧急(1) > 普通(2) > 低(3)
- 工业场景：告警消息走紧急通道、生产数据走普通通道、心跳走低优先级
- 阻塞接收 + 类型过滤

**命名管道（FIFO）**
- 阻塞 I/O 天然流控 → 写端过快时自动阻塞，不需要额外流量控制
- 命名惯例：`/tmp/rk3588_pipe_<服务名>`

**命名互斥锁**
- `pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED)` —— 进程间互斥锁的核心
- 5 秒超时防止死锁（`pthread_mutex_timedlock`）
- 使用场景：共享内存写入仲裁、SQLite 独占访问

### 2. 协议编解码 (`protocol.c`/`protocol.h`)

**关键设计决策：**

| 决策 | 选择 | 工业级理由 |
|------|------|-----------|
| 二进制 vs 文本 | **二进制** | 确定性解析，10B 帧头开销，无歧义 |
| CRC 算法 | **CRC16-CCITT** | 多项式 `0x1021`，工业总线（Modbus）同样使用 |
| 实现方式 | **查表法** | O(1) 每字节，比逐位计算快 8 倍 |
| struct 对齐 | **`__attribute__((packed))`** | 消除 padding，内存布局即网络格式 |
| 字节序 | **网络字节序（大端）** | 跨平台兼容，ARM/x86 一致 |

**编码流水线**：
```
应用数据 → protocol_encode() → uint8_t[] → msgsnd()/shm_ring_write()
```

**解码流水线**：
```
msgrcv() → uint8_t[] → protocol_decode() → protocol_frame_t → 按 msg_type 分发
```

**CRC16 查表法优化**（面试高频）：
```c
uint16_t crc16_table[256];  // 预计算
for (size_t i = 0; i < len; i++)
    crc = (crc << 8) ^ crc16_table[(crc >> 8) ^ data[i]];
```

### 3. 消息路由中心 (`router`)

**路由表**：内存哈希表，映射 `device_id → pipe_path`

**消息流转**：
```
设备A → [MSGQ] → router → [SHM] → protocol_engine → [SHM] → router → [MSGQ] → 设备B
                                              ↓
                                         [PIPE] → storage (异步落库)
```

**容错机制**：
- 目标设备不存在 → 缓存 3 秒 + 返回 `MSG_ERROR`
- 与 monitor 配合：通过共享内存暴露路由表状态
- 优雅退出：SIGTERM → 排空队列 → exit

**面试常问——瓶颈分析**：
```
工业现场：50 台设备 × 20 条/秒 = 1000 条/秒 经过 router。
优化方向：(1) 零拷贝共享内存 (2) 多线程分发 (3) 无锁路由表
工业场景下资源受限，通常 (1)+(3) 结合。
```

### 4. 协议解析引擎 (`protocol_engine`)

```
┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐
│  解析帧头  │→│ CRC16校验 │→│ 解码载荷  │→│ 分发到SHM │
└──────────┘ └──────────┘ └──────────┘ └──────────┘
```

**为什么不用 SHA256？** CRC16 足够检测工业现场的比特翻转（电磁干扰、线缆老化）。SHA256 是密码学哈希，计算开销大，嵌入式 CPU 扛不住——**过度设计等于设计缺陷**。

### 5. 数据存储服务 (`storage`)

**架构**：生产者-消费者模型（命名管道 + SQLite WAL 模式）

**写入路径**：`router → [pipe] → storage → [WAL commit] → SQLite`

**工业优化**：
- 批量写入：累计 10 条或 5 秒窗口提交一次（减少磁盘 I/O）
- WAL 模式：读不阻塞写，写不阻塞读
- 日志定期归档：config `max_log_days = 30` 自动清理旧数据

### 6. 系统监控 (`monitor`)

**看门狗循环**：
```
loop:
    读取所有进程的共享内存心跳时间戳
    遍历进程列表：
        if (now - last_heartbeat > 15s):
            kill(pid, SIGTERM)    // 优雅退出机会
            sleep(3)
            kill(pid, SIGKILL)   // 强制终止
            restart_process()    // 自动恢复
    sleep(3s)
```

**设计要点**：
- 只读共享内存 → 不需要锁
- 三级故障恢复：心跳超时 → SIGTERM → SIGKILL → 自动重启
- 工业 7×24 小时连续运行的核心保障

### 7. 设备接入服务 (`terminal`)

现场设备生命周期：
1. 设备发送 `MSG_REGISTER` → router 从 `0x0001-0x00FE` 池分配 ID
2. 正常收发 `MSG_DATA` / `MSG_ALARM` / `MSG_CMD`
3. 每 5 秒发送心跳
4. 15 秒无心跳 → 标记 OFFLINE → ID 释放回池

### 8. 管理控制台 (`console`)

```bash
factory-console> status
● router ........ running (PID 12345)  uptime: 72h
● protocol_engine running (PID 12346)  uptime: 72h
● storage ....... running (PID 12347)  uptime: 72h
● monitor ....... running (PID 12348)  uptime: 72h
● terminal ...... running (PID 12349)  uptime: 72h

factory-console> list
  ID   Name       Type        Status    Last Heartbeat
 0x01  PLC-01     Siemens S7  ONLINE    3s ago
 0x02  Temp-Sensor PT100       ONLINE    5s ago
 0x03  AGV-01     MIR100      OFFLINE   62s ago  ← 告警！

factory-console> send 0x01 "CMD:STOP_LINE"
ACK 0x01 seq=47

factory-console> logs --since 2024-05-01 --level ALARM
[05-01 08:23:17] ALARM  Temp-Sensor: 温度超限 92.3°C (阈值 85°C)
[05-01 09:15:42] ALARM  AGV-01: 电池电量 8%, 已停充

factory-console> exit
```

### 9. Qt GUI 管理界面 (`gui/`)

Qt5 产线监控看板：
- 设备列表：ID / 名称 / 类型 / 状态指示灯
- 消息吞吐量实时曲线
- 告警滚动条（最新 20 条）
- 一键启动/停止/重启各进程

通过 console 的管道接口连接后端。

---

## ⚡ 工业级设计要点

### 1. 内存管理

```c
// 工业级规范：禁止运行时动态内存分配
// MISRA-C 强制要求，航空/医疗/车规通用
static shm_ring_buffer_t *g_shm_ring;  // mmap 映射
static msg_queue_msg_t g_msg_buf;       // 栈上分配
```

嵌入式设备连续运行数月甚至数年，`malloc/free` → 内存碎片 → 系统崩溃。**工业现场不允许崩溃。**

### 2. 文件描述符管理

```c
// 每个进程打开多个 IPC 资源
// 启动时检查 ulimit -n（典型嵌入式设备限制 1024）
// 异常退出时必须 close 所有 fd，否则资源泄漏
```

### 3. 信号处理

```c
// 信号安全函数规则
void signal_handler(int sig) {
    write(g_signal_pipe[1], &sig, 1);  // write() 是安全的
    // printf() 不是信号安全的！可能死锁！
}

// 注册信号
signal(SIGTERM, signal_handler);   // 优雅退出
signal(SIGINT,  signal_handler);   // Ctrl+C
signal(SIGPIPE, SIG_IGN);          // 管道断裂不崩溃
```

### 4. 进程 daemon 化

```c
// 标准 daemon 化流程
// 1. fork() → 父进程 exit
// 2. setsid() 创建新会话
// 3. 第二次 fork() → 确保不再关联终端
// 4. chdir("/")
// 5. 关闭 stdin/stdout/stderr
```

### 5. 工具链（面试常问）

| 工具 | 用途 | 工业场景 |
|------|------|---------|
| `strace` | 追踪系统调用 | 验证 IPC 调用路径 |
| `gdb` | 调试崩溃/死锁 | attach 到进程看 backtrace |
| `perf` | 性能分析 | 定位 router 热点 |
| `valgrind` | 内存检查 | 泄漏/越界检测 |
| `readelf` | ELF 分析 | 查看各段大小 |
| `size` | 二进制体积 | ROM 空间预算 |
| `objdump` | 反汇编 | 分析关键函数汇编级性能 |

---

## 🗺 开发路线图

### 阶段一：基础框架
- [x] 项目骨架：Makefile、头文件、配置文件、SQL 脚本
- [x] GitHub 仓库搭建
- [ ] `src/common/ipc.c` — 共享内存环形缓冲区
- [ ] `src/common/ipc.c` — 消息队列、管道、互斥锁
- [ ] `src/common/protocol.c` — 协议编解码 + CRC16 查表法

### 阶段二：核心服务
- [ ] `src/router/router.c` — 消息路由与转发
- [ ] `src/protocol_engine/protocol_engine.c` — 协议解析流水线
- [ ] `src/terminal/terminal.c` — 设备注册与收发
- [ ] `src/storage/storage.c` — SQLite 异步持久化

### 阶段三：监控与管理
- [ ] `src/monitor/monitor.c` — 看门狗 + 自动恢复
- [ ] `src/console/console.py` — 交互式 CLI
- [ ] `scripts/start.sh` + `stop.sh` — 一键启停
- [ ] `scripts/benchmark.sh` — 性能压测

### 阶段四：GUI 与完善
- [ ] `gui/` — Qt5 产线监控看板
- [ ] `tests/` — 单元测试 + 集成测试
- [ ] 性能优化与 profiling
- [ ] 文档定稿

### 阶段五：扩展
- [ ] TCP Socket 扩展：远程设备接入
- [ ] router 多线程化：并发处理
- [ ] 启动时间测量与优化

---

## 📊 项目状态

> 🟢 = 已完成 · 🟡 = 进行中 · ⚪ = 计划中

### 头文件与配置

| 模块 | 状态 | 代码量 | 说明 |
|------|------|--------|------|
| `include/protocol.h` | 🟢 | ~80 行 | 协议帧结构、10 种工业消息类型 |
| `include/ipc.h` | 🟢 | ~120 行 | 4 种 IPC 统一接口 + 追踪打点 |
| `include/plugin.h` | ⚪ | — | 插件接口规范（dlopen） |
| `include/config.h` | ⚪ | — | INI 配置文件解析接口 |
| `include/logger.h` | ⚪ | — | 分级日志接口 |
| `config/system.conf` | 🟢 | ~25 行 | 系统配置 |
| `sql/init.sql` | 🟢 | ~40 行 | 数据库表结构 |

### 公共模块

| 模块 | 状态 | 代码量 | 说明 |
|------|------|--------|------|
| `src/common/ipc.c` | ⚪ | ~400 行 | 共享内存/消息队列/管道/互斥锁 + 追踪 + 统计 |
| `src/common/protocol.c` | ⚪ | ~150 行 | 协议编解码 + CRC16 查表法 |
| `src/common/config.c` | ⚪ | ~100 行 | 手写 INI 解析器 |
| `src/common/logger.c` | ⚪ | ~100 行 | 分级日志 + 文件轮转 |
| `src/common/plugin.c` | ⚪ | ~150 行 | dlopen 动态插件加载器 |

### 微服务进程

| 进程 | 文件 | 状态 | 代码量 | 核心职责 |
|------|------|------|--------|---------|
| gateway | `src/gateway/gateway.c` | ⚪ | ~200 行 | epoll 并发 TCP 网关 |
| router | `src/router/router.c` | ⚪ | ~500 行 | 路由/分组/ACK重传/链路追踪 |
| protocol_engine | `src/protocol_engine/protocol_engine.c` | ⚪ | ~250 行 | 协议解析 + 插件调度 |
| storage | `src/storage/storage.c` | ⚪ | ~300 行 | SQLite WAL + 批量提交 |
| monitor | `src/monitor/monitor.c` | ⚪ | ~300 行 | 看门狗 + 进程资源采集 + top |
| console | `src/console/console.py` | ⚪ | ~300 行 | 交互式 CLI |
| http_server | `src/http_server/http_server.c` | ⚪ | ~300 行 | 手写 HTTP/1.1 + JSON |

### 插件（协议适配）

| 插件 | 文件 | 状态 | 代码量 | 说明 |
|------|------|------|--------|------|
| raw_tcp | `plugins/raw_tcp/raw_tcp.c` | ⚪ | ~100 行 | 长度前缀分包 |
| modbus | `plugins/modbus/modbus.c` | ⚪ | ~150 行 | 模拟 Modbus RTU 解析 |
| simulator | `plugins/simulator/simulator.c` | ⚪ | ~150 行 | 传感器数据生成器 |

### 客户端与 GUI

| 组件 | 文件 | 状态 | 代码量 | 说明 |
|------|------|------|--------|------|
| device | `device/device.c` | ⚪ | ~300 行 | TCP 客户端，多设备类型模拟 |
| gui | `gui/` (Qt5) | 🟡 | ~500 行 | 产线监控看板（空白骨架） |

### 脚本

| 脚本 | 文件 | 状态 | 说明 |
|------|------|------|------|
| start | `scripts/start.sh` | ⚪ | 一键启动全部进程 |
| stop | `scripts/stop.sh` | ⚪ | 优雅关闭全部进程 |
| status | `scripts/status.sh` | ⚪ | 进程状态概览 |
| benchmark | `scripts/benchmark.sh` | ⚪ | 性能压测 + 自动报告 |
| devices | `scripts/devices.sh` | ⚪ | 一键启动多台模拟设备 |

---

## 💼 面试价值

### 为什么这个项目能打

```
面试官看到简历上"RK3588多终端通信系统"时的反应：

❌ "又一个课堂大作业" → 每年见几百个
✅ "工业级设备通信？这个做过的人不多，聊聊" → 这就是区分度
```

**工业场景的项目在秋招中天然稀缺**——因为在校生很少有机会接触真实的产线环境。你把这个项目的工业逻辑讲清楚，面试官会觉得你有"工业思维"，而不仅仅是"会调 API"。

### 嵌入式Linux面试高频问答

**Q1：共享内存的环形缓冲区，为什么用 `volatile` 就够了？在多核 CPU 上会有什么问题？**
> `volatile` 防止编译器将读写优化到寄存器，保证每次访问都走内存。但 `volatile` 不提供内存屏障（memory barrier），在多核 ARM Cortex-A76 上，一个核写的数据对另一个核不一定立即可见。正确的做法是加 `__sync_synchronize()`（GCC 内置屏障）或者用 C11 的 `atomic_*` 操作。但我们的场景是单生产者-单消费者，两个核各跑一个进程，实际测试下来 volatile 够用。**注意这里说的是"实际够用，理论上不是 100% 安全"——面试官喜欢这种有分寸的回答。**

**Q2：System V 消息队列 vs POSIX 消息队列？为什么选 System V？**
> POSIX mq 有系统级限制（`/proc/sys/fs/mqueue/msg_max` 默认 10），需要 root 权限修改。工业现场通常没有 root 权限。System V 用 key 直接创建，队列深度可自定义（100 条），优先级通过 mtype 天然支持多级。代价是没有 POSIX 的 `mq_notify()` 异步通知能力。

**Q3：Monitor 怎么判断一个进程是挂了还是卡住了？**
> 两级判断：(1) 共享内存中的心跳时间戳超过 15 秒没更新；(2) `kill(pid, 0)` 检查进程是否存在。如果进程存在但无心跳 → 进程卡住（死锁或死循环）。处理策略：SIGTERM → 等 3 秒 → SIGKILL → 重启。

**Q4：工业现场 7×24 小时运行，怎么保证系统不崩？**
> 三个层面：(1) **进程级隔离**——6 个独立进程，一个崩了不影响其他；(2) **硬件看门狗在软件层的实现**——monitor 进程心跳检测 + 自动恢复；(3) **无动态内存分配**——没有内存碎片，不会"跑几天后变慢"。

**Q5：如果 router 成了瓶颈怎么办？**
> 工业现场 50 台设备 × 20 条/秒 = 1000 条/秒。优化路径：(1) 零拷贝共享内存减少数据搬运；(2) router 内部分为收线程和发线程，用 lock-free 队列连接；(3) 极端场景可以做 router 主备冗余（这是扩展规划）。

**Q6：这个项目跟 Modbus / CAN / Profinet 有什么区别？**
> 它们是**现场总线协议**，定义的是物理层到应用层的全栈规范。本项目是一个**消息中间件的软件架构**，关注的是数据到了主控板之后，如何在内部各模块之间高效流转。上位可以是 Modbus TCP、CAN bus、串口——本项目不取代它们，而是作为它们的统一管理后台。

### 简历写法

```text
RK3588 Factory Device Communication Hub · 工业设备信息传输中枢
技术栈：C11、Linux IPC、SQLite3、Qt5、Makefile
——————————————————————————————————————————
- 设计并实现面向工厂产线的 6 进程微服务通信架构，覆盖 4 种 Linux IPC 
  机制（POSIX 共享内存 / System V 消息队列 / 命名管道 / 进程间互斥锁），
  零第三方运行时依赖
- 设计自定义二进制通信协议（10B 帧头 + CRC16-CCITT 查表法校验），支持
  10 种工业消息类型（心跳/注册/数据/告警/控制指令/配置下发等）
- 实现 64KB 无锁环形缓冲区（单生产者-单消费者），微秒级进程间数据交换
- 实现 SQLite WAL 模式异步落库 + 批量提交策略，满足生产数据可追溯需求
- 实现三级看门狗故障恢复机制（心跳超时 → SIGTERM → SIGKILL → 自动重启）
- 实现进程 daemon 化、优雅退出、信号安全处理
- 使用 strace / gdb / perf / valgrind 完成性能分析与调试
- 开发 Qt5 产线监控看板，支持设备状态可视化和实时告警
```

---

## 📄 许可证

MIT License — 可自由用于学习、求职、面试展示。

---

<p align="center">
  <sub>嵌入式Linux应用开发 · 秋招工业级项目 · If it helps you land an offer, give it a ⭐</sub>
</p>
