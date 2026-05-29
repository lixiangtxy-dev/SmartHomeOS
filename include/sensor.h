#ifndef __SENSOR_H__
#define __SENSOR_H__

// 传感器数据结构
struct sensor_data {
    int temperature; // 温度 (摄氏度)
    int humidity;    // 湿度 (%)
    int smoke_alarm; // 烟雾警报 (0: 正常, 1: 报警)
};

// 全局最新数据 (供多个任务共享读取)
extern struct sensor_data current_env;

void sensor_init(void);
void sensor_read_all(void);

#endif