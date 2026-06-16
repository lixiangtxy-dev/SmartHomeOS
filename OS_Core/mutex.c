#include "mutex.h"
#include "task.h" // 引入任务模块，以便调用 task_yield()

void mutex_init(mutex_t *mux) {
    mux->is_locked = 0;
}

void mutex_lock(mutex_t *mux) {
    while (mux->is_locked == 1) {
        task_yield(); 
    }
    mux->is_locked = 1;
}

void mutex_unlock(mutex_t *mux) {
    mux->is_locked = 0;
}