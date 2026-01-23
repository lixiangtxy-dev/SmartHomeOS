// [fs.h] 新建文件
#ifndef __FS_H__
#define __FS_H__

#include <stdint.h>
#include "os.h"

// 1. 超级块 (Block 0)
struct superblock {
    uint32_t magic;      // 魔数，用于识别是否已格式化
    uint32_t size;       // 总块数
    uint32_t nblocks;    // 数据块数量
    uint32_t ninodes;    // Inode 数量
};

// 2. Inode (索引节点)
// 每个 Inode 代表一个文件或目录
struct inode {
    uint16_t type;       // T_DIR 或 T_FILE
    uint16_t major;      //如果是设备文件
    uint16_t minor;
    uint16_t nlink;      // 链接数 (简单起见暂未充分利用)
    uint32_t size;       // 文件实际大小 (字节)
    uint32_t addrs[12];  // 12 个直接数据块指针 (每个块 4KB，最大支持 48KB 文件)
};

// 3. 目录项
// 目录文件的“内容”就是一系列 directory_entry
struct dirent {
    uint16_t inum;       // Inode 编号
    char name[14];       // 文件名 (固定长度简化处理)
};

// 定义一些常量用于计算位置
#define ROOT_INODE 1     // 根目录的 Inode 号
#define MAX_INODES 64    // 简易版：最多 64 个文件
#define BPB        (FS_BLOCK_SIZE * 8) // Bits Per Block

#endif