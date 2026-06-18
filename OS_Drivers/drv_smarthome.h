#ifndef __DRV_SMARTHOME_H
#define __DRV_SMARTHOME_H

// 对外只暴露一个初始化函数，其他全部通过 device.h 的标准 API 调用
void drv_smarthome_init(void);

#endif