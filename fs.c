// [fs.c] 新建文件
#include "os.h"
#include "fs.h"

// ---------------- 底层驱动层 (Block Device) ----------------

// 将 Ramdisk 视为一个大数组，通过 memcpy 读写
void disk_rw(uint32_t block_no, uint8_t *buf, int write) {
    uint8_t *addr = (uint8_t *)(RAMDISK_START + block_no * FS_BLOCK_SIZE);
    if (write) {
        // 简易 memcpy
        for(int i=0; i<FS_BLOCK_SIZE; i++) addr[i] = buf[i];
    } else {
        for(int i=0; i<FS_BLOCK_SIZE; i++) buf[i] = addr[i];
    }
}

uint8_t tmp_buf[FS_BLOCK_SIZE]; 

// ---------------- 文件系统层 (FS Layer) ----------------

struct superblock sb;

// 简单的字符串比较
int streq(char *a, char *b) {
    while(*a && *b) { if(*a != *b) return 0; a++; b++; }
    return *a == *b;
}

// 简单的内存清零
void bzero(void *p, int len) {
    char *c = (char*)p;
    while(len--) *c++ = 0;
}

// 简单的字符串拷贝
void mstrcpy(char *dst, char *src, int n) {
    while(n-- && *src) *dst++ = *src++;
    *dst = 0;
}

// ---------------- Inode 操作 ----------------

// 读取 Inode
void read_inode(uint32_t inum, struct inode *ip) {
    disk_rw(1, tmp_buf, 0); // 读 Block 1
    struct inode *inodes = (struct inode *)tmp_buf;
    *ip = inodes[inum];
}

// 写入 Inode
void write_inode(uint32_t inum, struct inode *ip) {
    disk_rw(1, tmp_buf, 0); // 先读出来
    struct inode *inodes = (struct inode *)tmp_buf;
    inodes[inum] = *ip;     // 修改对应的 Inode
    disk_rw(1, tmp_buf, 1); // 写回去
}

// 分配一个新的 Inode
uint32_t ialloc(uint16_t type) {
    disk_rw(1, tmp_buf, 0);
    struct inode *inodes = (struct inode *)tmp_buf;
    
    // 从 1 开始找 (0保留)
    for(int i=1; i<MAX_INODES; i++) {
        if(inodes[i].type == 0) { // 空闲
            inodes[i].type = type;
            inodes[i].size = 0;
            inodes[i].nlink = 1;
            disk_rw(1, tmp_buf, 1); // 写回
            return i;
        }
    }
    uart_puts("FS: No free inodes!\n");
    return 0;
}

uint32_t balloc() {
    // 重新读取 SB 确保最新
    disk_rw(0, (uint8_t*)&sb, 0); 
    
    if (sb.nblocks < 2) sb.nblocks = 2; // 初始化
    
    uint32_t block = sb.nblocks++;
    
    // 更新 SB
    disk_rw(0, (uint8_t*)&sb, 1);
    
    // 清零新块
    bzero(tmp_buf, FS_BLOCK_SIZE);
    disk_rw(block, tmp_buf, 1);
    
    return block;
}

// ---------------- 目录与文件操作 ----------------

// 在根目录查找文件
uint32_t dir_lookup(char *name) {
    struct inode root;
    read_inode(ROOT_INODE, &root);
    
    for(int i=0; i<12; i++) {
        if(root.addrs[i] == 0) continue;
        disk_rw(root.addrs[i], tmp_buf, 0);
        struct dirent *de = (struct dirent *)tmp_buf;
        // 遍历块内的所有目录项
        for(int j=0; j < FS_BLOCK_SIZE / sizeof(struct dirent); j++) {
            if(de[j].inum == 0) continue;
            if(streq(name, de[j].name)) return de[j].inum;
        }
    }
    return 0;
}

// 添加文件到根目录
void dir_link(char *name, uint32_t inum) {
    struct inode root;
    read_inode(ROOT_INODE, &root);
    
    // 简化：假设根目录只有一个数据块 (root.addrs[0])
    if(root.addrs[0] == 0) {
        root.addrs[0] = balloc();
        write_inode(ROOT_INODE, &root);
    }
    
    disk_rw(root.addrs[0], tmp_buf, 0);
    struct dirent *de = (struct dirent *)tmp_buf;
    
    // 找空槽
    for(int j=0; j < FS_BLOCK_SIZE / sizeof(struct dirent); j++) {
        if(de[j].inum == 0) {
            de[j].inum = inum;
            mstrcpy(de[j].name, name, 13);
            disk_rw(root.addrs[0], tmp_buf, 1); // 写回
            return;
        }
    }
}

// ---------------- 格式化 (mkfs) ----------------
void mkfs() {
    uart_puts("FS: Formatting Ramdisk...\n");
    
    // 1. 初始化 Superblock
    sb.magic = FS_MAGIC;
    sb.size = 1024; // 假设 4MB / 4KB = 1024 Blocks
    sb.nblocks = 2; // Block 0, 1 used
    sb.ninodes = MAX_INODES;
    disk_rw(0, (uint8_t*)&sb, 1);
    
    // 2. 清空 Inode 表 (Block 1)
    bzero(tmp_buf, FS_BLOCK_SIZE);
    disk_rw(1, tmp_buf, 1);
    
    // 3. 创建根目录 (Inode 1)
    struct inode root;
    root.type = T_DIR;
    root.size = FS_BLOCK_SIZE; // 初始分配一个块
    root.nlink = 1;
    root.addrs[0] = balloc(); // 分配数据块
    // 清空其他指针
    for(int i=1; i<12; i++) root.addrs[i] = 0;
    
    write_inode(ROOT_INODE, &root); // 写入磁盘
}

// ---------------- 系统调用层 API ----------------

void fs_init() {
    disk_rw(0, (uint8_t*)&sb, 0);
    if(sb.magic != FS_MAGIC) {
        mkfs();
    } else {
        uart_puts("FS: Found existing filesystem.\n");
    }
}

int fs_open(char *name, int mode) {
    uint32_t inum = dir_lookup(name);
    if(inum == 0) {
        if(mode & O_CREATE) {
            inum = ialloc(T_FILE);
            dir_link(name, inum);
            return inum;
        }
        return 0; // Not found
    }
    return inum;
}

int fs_write(int inum, char *data, int n) {
    struct inode ip;
    read_inode(inum, &ip);
    
    if(ip.addrs[0] == 0) {
        ip.addrs[0] = balloc();
        write_inode(inum, &ip); // 更新 Inode 的数据块指针
    }
    
    disk_rw(ip.addrs[0], tmp_buf, 0);
    // Copy data
    for(int i=0; i<n && i<FS_BLOCK_SIZE; i++) {
        tmp_buf[i] = data[i];
    }
    disk_rw(ip.addrs[0], tmp_buf, 1);
    
    // 更新大小
    if(n > ip.size) {
        ip.size = n;
        write_inode(inum, &ip);
    }
    return n;
}

int fs_read(int inum, char *buf, int n) {
    struct inode ip;
    read_inode(inum, &ip);
    if(ip.addrs[0] == 0) return 0;
    
    disk_rw(ip.addrs[0], tmp_buf, 0);
    int len = n < ip.size ? n : ip.size;
    for(int i=0; i<len; i++) buf[i] = tmp_buf[i];
    return len;
}

void fs_ls() {
    struct inode root;
    read_inode(ROOT_INODE, &root);
    disk_rw(root.addrs[0], tmp_buf, 0);
    struct dirent *de = (struct dirent *)tmp_buf;
    
    uart_puts("--- LS ---\n");
    for(int j=0; j < FS_BLOCK_SIZE / sizeof(struct dirent); j++) {
        if(de[j].inum != 0) {
            uart_puts(de[j].name);
            uart_puts("\n");
        }
    }
    uart_puts("----------\n");
}