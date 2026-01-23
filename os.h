#ifndef __OS_H__
#define __OS_H__

#include <stdint.h>
#include <stddef.h>
// ---------------- 配置参数 ----------------
#define MAX_TASKS 10       // 最大支持 10 个任务
#define STACK_SIZE 1024    // 每个任务 1024 个 uint32 (4KB)
#define PHYSTOP 0x88000000 // 物理内存顶端 (128MB)
#define PGSIZE 4096        // 页大小 4KB

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))

#define HEAP_STOP  0x87000000 
#define RAMDISK_START 0x87000000
#define RAMDISK_SIZE  (4 * 1024 * 1024) // 4MB

// ---------------- 文件系统定义 ----------------
#define FS_BLOCK_SIZE 4096  // 块大小 (和页大小一致，方便)
#define FS_MAGIC      0x5A5A5A5A
// 文件类型
#define T_DIR  1
#define T_FILE 2

// 简易打开模式
#define O_READ  0
#define O_WRITE 1
#define O_CREATE 2

// 函数声明
void fs_init();
void fs_test();
int fs_open(char *name, int mode);
int fs_write(int inode_idx, char *buf, int n);
int fs_read(int inode_idx, char *buf, int n);
void fs_close(int inode_idx);
void fs_ls();
void *memcpy(void *dst, const void *src, size_t n);
// ---------------- 任务状态 ----------------
typedef enum {
    TASK_UNUSED = 0, // 空闲槽位
    TASK_RUNNABLE,   // 就绪 (等待 CPU)
    TASK_RUNNING,    // 正在运行
} task_state_t;

// ---------------- 任务结构体 (PCB) ----------------
struct task {
    uint32_t *sp;               // 栈指针
    task_state_t state;         // 任务状态
    char name[16];              // 任务名
    void *stack; // 任务专属栈 
};

// ---------------- CPU 结构体 ----------------
// 用于记录每个 CPU 的私有状态
struct cpu {
    int noff;       // 关中断的嵌套深度 (Nesting OFFset)
    int int_ena;    // 关中断之前的状态 (Interrupt ENAble)
};

// ---------------- 锁结构体 ----------------
struct spinlock {
    int locked;
    char *name;
    struct cpu *cpu;
};

// ---------------- 外部变量与函数 ----------------
extern struct task *current_task;
extern struct task tasks[MAX_TASKS];
extern struct spinlock uart_lock;
struct cpu* mycpu();

void kinit(void);
void *kalloc(void);
void kfree(void *pa);

// 任务管理函数
void task_os_init();
int task_create(void (*entry)(), char *name);
void task_yield();
void schedule();

// 其他辅助函数
void uart_puts(char *s);
void push_off(void);
void pop_off(void);
void uart_putc(char c);
void delay(volatile int count);
void spin_init(struct spinlock *lk, char *name);
void spin_lock(struct spinlock *lk);
void spin_unlock(struct spinlock *lk);
void enable_interrupts();
extern void switch_to(uint32_t **old_sp, uint32_t *new_sp);

#endif