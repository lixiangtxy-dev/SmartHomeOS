#ifndef __OS_MSGQUEUE_H
#define __OS_MSGQUEUE_H

#include <stdint.h>
#include <stddef.h>
#include "mutex.h" // 引入你之前写好的互斥锁

// 消息队列控制块 (Message Queue Control Block)
typedef struct {
    uint8_t  *buffer;      // 指向队列内存池的指针
    uint16_t item_size;    // 每个消息块的大小 (比如 sizeof(float))
    uint16_t capacity;     // 队列最多能容纳的消息个数
    uint16_t head;         // 读指针 (出队位置)
    uint16_t tail;         // 写指针 (入队位置)
    uint16_t count;        // 当前队列中的消息数量
    mutex_t  lock;         // 保护队列的专属互斥锁
} os_mq_t;

// --- 内核态 API ---

/**
 * @brief 初始化消息队列
 * @param mq        消息队列控制块指针
 * @param buffer    用于存储数据的实际内存数组
 * @param item_size 单个消息的字节数
 * @param capacity  最多存放的消息个数
 */
void os_mq_init(os_mq_t *mq, void *buffer, uint16_t item_size, uint16_t capacity);

/**
 * @brief 向队列发送消息 (入队)
 * @param mq   消息队列句柄
 * @param msg  指向要发送数据的指针
 * @return 0: 成功, -1: 队列已满
 */
int os_mq_send(os_mq_t *mq, const void *msg);

/**
 * @brief 从队列接收消息 (出队)
 * @param mq   消息队列句柄
 * @param msg  指向用于接收数据的缓冲区的指针
 * @return 0: 成功, -1: 队列为空
 */
int os_mq_recv(os_mq_t *mq, void *msg);

#endif // __OS_MSGQUEUE_H