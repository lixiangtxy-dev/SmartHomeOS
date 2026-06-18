#ifndef __OS_SHADOW_H
#define __OS_SHADOW_H

#include <stdint.h>

// 1. 定义设备影子结构体 (包含系统中所有的状态)
typedef struct {
    float    room_temp;       // 室内温度 (℃)
    uint32_t light_voltage;   // 光敏电压 (mV)
    uint8_t  alarm_led_state; // 报警灯状态 (1=ON, 0=OFF)
    
    // 进阶：未来可以加入 timestamp (时间戳)，用于判断数据是否过期
    // uint32_t last_update_tick; 
} smarthome_shadow_t;

// 2. 初始化影子数据中心
void os_shadow_init(void);

// 3. 生产者接口 (供底层传感器任务调用：写入最新数据)
void os_shadow_update_temp(float temp);
void os_shadow_update_light(uint32_t voltage);
void os_shadow_update_led(uint8_t state);

// 4. 消费者接口 (供顶层业务任务调用：获取全量快照)
// 传入一个结构体指针，系统会将最新的影子数据完整拷贝给你
void os_shadow_get_snapshot(smarthome_shadow_t *snapshot);

#endif // __OS_SHADOW_H