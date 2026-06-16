#ifndef __MUTEX_H
#define __MUTEX_H

#include <stdint.h>

typedef struct {
    volatile uint8_t is_locked; 
} mutex_t;

// 互斥锁 API 变得极其干净
void mutex_init(mutex_t *mux);
void mutex_lock(mutex_t *mux);
void mutex_unlock(mutex_t *mux);

#endif