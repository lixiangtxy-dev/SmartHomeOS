#ifndef __FS_PORT_H
#define __FS_PORT_H

#include "lfs.h"

// 暴露给全局的 LittleFS 句柄
extern lfs_t lfs;

// 初始化并挂载文件系统
void drv_fs_init(void);

#endif