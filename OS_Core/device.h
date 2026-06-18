#ifndef __OS_DEVICE_H
#define __OS_DEVICE_H

#include <stdint.h>
#include <stddef.h>

struct os_device; 

// 设备操作方法表 (面向对象接口)
typedef struct {
    int (*init) (struct os_device *dev);
    int (*open) (struct os_device *dev);
    int (*read) (struct os_device *dev, void *buffer, uint32_t size);
    int (*write)(struct os_device *dev, const void *buffer, uint32_t size);
} os_device_ops_t;

// 设备控制块 (节点)
typedef struct os_device {
    const char            *name;      // 设备唯一名称
    const os_device_ops_t *ops;       // 操作方法
    void                  *user_data; // 留给底层的私有数据 (如引脚配置表)
    struct os_device      *next;      // 链表指针
} os_device_t;

// --- 内核态 API (供驱动层调用) ---
int os_device_register(os_device_t *dev);

// --- 用户态 API (供应用任务调用) ---
os_device_t* os_device_open(const char *name);
int os_device_read(os_device_t *dev, void *buffer, uint32_t size);
int os_device_write(os_device_t *dev, const void *buffer, uint32_t size);

#endif // __OS_DEVICE_H