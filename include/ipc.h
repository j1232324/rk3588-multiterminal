/**
 * ipc.h - IPC通信机制封装
 * 共享内存、消息队列、管道、互斥锁
 */

#ifndef __IPC_H__
#define __IPC_H__

#include <stdint.h>
#include <stddef.h>

/* ======================== 共享内存 ======================== */

/* 共享内存区域定义 */
#define SHM_NAME_PREFIX     "/rk3588_shm_"
#define SHM_SIZE            (64 * 1024)     /* 64KB环形缓冲区 */

/* 环形缓冲区结构 */
typedef struct {
    volatile uint32_t read_pos;     /* 读指针 */
    volatile uint32_t write_pos;    /* 写指针 */
    volatile uint32_t size;         /* 缓冲区总大小 */
    uint8_t data[SHM_SIZE - 12];    /* 数据区（减去3个uint32_t） */
} shm_ring_buffer_t;

int  shm_create(const char *name, size_t size, void **addr);
int  shm_open_named(const char *name, void **addr, size_t *size);
int  shm_close(const char *name, void *addr, size_t size);
int  shm_ring_write(shm_ring_buffer_t *ring, const void *data, size_t len);
int  shm_ring_read(shm_ring_buffer_t *ring, void *buf, size_t len);

/* ======================== 消息队列 ======================== */

/* System V消息队列 */
#define MSG_KEY_PREFIX      0x4D53514D  /* "MSQM" */
#define MSG_MAX_SIZE        2048
#define MSG_QUEUE_DEPTH     100

/* 消息类型优先级 */
#define MSG_PRIO_URGENT     1           /* 紧急消息 */
#define MSG_PRIO_NORMAL     2           /* 普通消息 */
#define MSG_PRIO_LOW        3           /* 低优先级 */

/* 消息结构 */
typedef struct {
    long mtype;                 /* 消息类型（必须>0） */
    uint16_t src;               /* 源终端 */
    uint16_t dst;               /* 目标终端 */
    uint8_t  data[MSG_MAX_SIZE - 8]; /* 消息数据 */
} msg_queue_msg_t;

int  msgq_create(key_t key);
int  msgq_send(int msqid, const msg_queue_msg_t *msg, size_t len, int priority);
int  msgq_recv(int msqid, msg_queue_msg_t *msg, size_t len, int priority);
int  msgq_destroy(int msqid);

/* ======================== 管道 ======================== */

#define PIPE_PREFIX         "/tmp/rk3588_pipe_"
#define PIPE_BUFFER_SIZE    4096

int  pipe_create(const char *name);
int  pipe_open_read(const char *name);
int  pipe_open_write(const char *name);
int  pipe_send(int fd, const void *data, size_t len);
int  pipe_recv(int fd, void *buf, size_t len);
int  pipe_close(int fd);
int  pipe_destroy(const char *name);

/* ======================== 互斥锁 ======================== */

#define MUTEX_PREFIX        "/rk3588_mutex_"
#define MUTEX_TIMEOUT_MS    5000            /* 5秒超时 */

int  mutex_create(const char *name);
int  mutex_open(const char *name);
int  mutex_lock(int fd, int timeout_ms);
int  mutex_trylock(int fd);
int  mutex_unlock(int fd);
int  mutex_close(int fd);
int  mutex_destroy(const char *name);

/* ======================== 工具函数 ======================== */

/* 信号处理 */
typedef void (*signal_handler_t)(int signum);
int  signal_install(int signum, signal_handler_t handler);

/* 日志 */
void ipc_log(const char *level, const char *func, const char *fmt, ...);

#define LOG_INFO(fmt, ...)  ipc_log("INFO",  __func__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  ipc_log("WARN",  __func__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) ipc_log("ERROR", __func__, fmt, ##__VA_ARGS__)

#endif /* __IPC_H__ */
