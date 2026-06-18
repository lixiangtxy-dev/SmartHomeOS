#include "shadow.h"
#include "mutex.h"
#include <string.h>

// 全局唯一的影子内存实例 (被 static 保护，外部无法直接访问)
static smarthome_shadow_t g_device_shadow;

// 保护影子的专属互斥锁
static mutex_t shadow_mutex;

void os_shadow_init(void) {
    // 内存清零
    memset(&g_device_shadow, 0, sizeof(smarthome_shadow_t));
    
    // 初始化读写锁
    mutex_init(&shadow_mutex);
}

void os_shadow_update_temp(float temp) {
    mutex_lock(&shadow_mutex);     // 【加锁】进入临界区
    g_device_shadow.room_temp = temp;
    mutex_unlock(&shadow_mutex);   // 【解锁】退出临界区
}

void os_shadow_update_light(uint32_t voltage) {
    mutex_lock(&shadow_mutex);
    g_device_shadow.light_voltage = voltage;
    mutex_unlock(&shadow_mutex);
}

void os_shadow_update_led(uint8_t state) {
    mutex_lock(&shadow_mutex);
    g_device_shadow.alarm_led_state = state;
    mutex_unlock(&shadow_mutex);
}

// 获取状态快照 (极速内存拷贝，绝不阻塞太久)
void os_shadow_get_snapshot(smarthome_shadow_t *snapshot) {
    if (snapshot == NULL) return;
    
    mutex_lock(&shadow_mutex);
    // 将当前的全局影子数据，完整地拷贝给调用者提供的本地变量
    *snapshot = g_device_shadow; 
    mutex_unlock(&shadow_mutex);
}