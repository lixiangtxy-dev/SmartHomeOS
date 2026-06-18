#include "device.h"

// 设备链表头指针
static os_device_t *device_list_head = NULL;

// 简易字符串比较
static int os_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int os_device_register(os_device_t *dev) {
    if (dev == NULL || dev->name == NULL) return -1;
    
    os_device_t *current = device_list_head;
    while (current != NULL) {
        if (os_strcmp(current->name, dev->name) == 0) return -2; 
        current = current->next;
    }

    if (dev->ops && dev->ops->init) dev->ops->init(dev);

    dev->next = device_list_head;
    device_list_head = dev;
    return 0; 
}

os_device_t* os_device_open(const char *name) {
    os_device_t *current = device_list_head;
    while (current != NULL) {
        if (os_strcmp(current->name, name) == 0) {
            if (current->ops && current->ops->open) current->ops->open(current);
            return current; 
        }
        current = current->next;
    }
    return NULL; 
}

int os_device_read(os_device_t *dev, void *buffer, uint32_t size) {
    if (dev == NULL || dev->ops == NULL || dev->ops->read == NULL) return -1;
    return dev->ops->read(dev, buffer, size);
}

int os_device_write(os_device_t *dev, const void *buffer, uint32_t size) {
    if (dev == NULL || dev->ops == NULL || dev->ops->write == NULL) return -1;
    return dev->ops->write(dev, buffer, size);
}