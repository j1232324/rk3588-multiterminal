/**
 * protocol.h - 自定义通信协议定义
 * 基于RK3588的嵌入式多终端通信系统
 */

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <stdint.h>

/* 帧头标识 */
#define FRAME_HEADER        0xA5A5
#define PROTOCOL_VERSION    0x01

/* 帧结构 */
typedef struct {
    uint16_t header;        /* 帧头 0xA5A5 */
    uint8_t  version;       /* 协议版本 */
    uint8_t  msg_type;      /* 消息类型 */
    uint16_t src_terminal;  /* 源终端ID */
    uint16_t dst_terminal;  /* 目标终端ID */
    uint16_t data_len;      /* 数据载荷长度 */
    uint8_t  data[1024];    /* 数据载荷 */
    uint16_t crc16;         /* CRC16校验 */
} __attribute__((packed)) protocol_frame_t;

/* 帧头+版本+类型+源+目标+长度 = 2+1+1+2+2+2 = 10字节头部 */
#define FRAME_HEADER_SIZE   10
#define FRAME_MAX_PAYLOAD   1024
#define FRAME_MAX_SIZE      (FRAME_HEADER_SIZE + FRAME_MAX_PAYLOAD + 2) /* +2 for CRC */

/* 消息类型 */
#define MSG_HEARTBEAT       0x01    /* 心跳包 */
#define MSG_REGISTER        0x02    /* 终端注册 */
#define MSG_DATA            0x03    /* 数据消息 */
#define MSG_ACK             0x04    /* 确认应答 */
#define MSG_BROADCAST       0x05    /* 广播消息 */
#define MSG_CONFIG          0x06    /* 配置下发 */
#define MSG_STATUS          0x07    /* 状态上报 */
#define MSG_VOICE           0x08    /* 语音数据（预留LD3320） */
#define MSG_ERROR           0xFF    /* 错误响应 */

/* 终端ID */
#define TERMINAL_ROUTER     0x0000
#define TERMINAL_ALL        0xFFFF
#define TERMINAL_MIN_DYNAMIC 0x0001
#define TERMINAL_MAX_DYNAMIC 0x00FE

/* 协议函数 */
int protocol_encode(protocol_frame_t *frame, uint8_t *buf, size_t *len);
int protocol_decode(const uint8_t *buf, size_t len, protocol_frame_t *frame);
uint16_t protocol_crc16(const uint8_t *data, size_t len);

#endif /* __PROTOCOL_H__ */
