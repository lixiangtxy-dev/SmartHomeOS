#include "msgqueue.h"

static void os_memcpy(void *dest, const void *src, uint32_t len) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    while (len--) {
        *d++ = *s++;
    }
}

void os_mq_init(os_mq_t *mq, void *buffer, uint16_t item_size, uint16_t capacity) {
    if (mq == NULL || buffer == NULL || item_size == 0 || capacity == 0) return;

    mq->buffer = (uint8_t *)buffer;
    mq->item_size = item_size;
    mq->capacity = capacity;
    mq->head = 0;
    mq->tail = 0;
    mq->count = 0;
    
    // 初始化队列的专属门卫 (互斥锁)
    mutex_init(&mq->lock);
}

int os_mq_send(os_mq_t *mq, const void *msg) {
    if (mq == NULL || msg == NULL) return -1;

    // 1. 获取互斥锁，防止其他任务同时写/读
    mutex_lock(&mq->lock);

    // 2. 检查队列是否已满
    if (mq->count >= mq->capacity) {
        mutex_unlock(&mq->lock);
        return -1; // Queue Full
    }

    // 3. 计算实际写入的内存偏移量，并拷贝数据
    uint32_t offset = mq->tail * mq->item_size;
    os_memcpy(&(mq->buffer[offset]), msg, mq->item_size);

    // 4. 更新写指针和计数器 (环形逻辑)
    mq->tail = (mq->tail + 1) % mq->capacity;
    mq->count++;

    // 5. 释放互斥锁
    mutex_unlock(&mq->lock);
    return 0;
}

int os_mq_recv(os_mq_t *mq, void *msg) {
    if (mq == NULL || msg == NULL) return -1;

    mutex_lock(&mq->lock);

    // 检查队列是否为空
    if (mq->count == 0) {
        mutex_unlock(&mq->lock);
        return -1; // Queue Empty
    }

    // 计算实际读取的内存偏移量，并拷贝数据到接收者的指针中
    uint32_t offset = mq->head * mq->item_size;
    os_memcpy(msg, &(mq->buffer[offset]), mq->item_size);

    // 更新读指针和计数器 (环形逻辑)
    mq->head = (mq->head + 1) % mq->capacity;
    mq->count--;

    mutex_unlock(&mq->lock);
    return 0;
}