#include "os.h"

// 链接脚本里定义的内核结束地址
extern char end[]; 

struct run {
    struct run *next;
};

struct {
    struct spinlock lock;
    struct run *freelist;
} kmem;

void kinit() {
    spin_init(&kmem.lock, "kmem");
    kmem.freelist = 0;
    
    char *p = (char *)PGROUNDUP((uintptr_t)end);
    
    for (; p + PGSIZE <= (char *)HEAP_STOP; p += PGSIZE) {
        kfree(p);
    }
}

// 释放物理页
void kfree(void *pa) {
    struct run *r;

    // 1. 安全检查
    if (((uintptr_t)pa % PGSIZE) != 0 || (char *)pa < end || (uintptr_t)pa >= PHYSTOP) {
        uart_puts("PANIC: kfree invalid address\n");
        while(1);
    }

    // 2. 填充垃圾数据 (为了尽早暴露 Use-After-Free bug)
    // 这里的 1 是随便填的，只要不是 0 就行
    char *p = (char*)pa;
    for(int i = 0; i < PGSIZE; i++)
        p[i] = 1;

    r = (struct run *)pa;

    // 3. 加锁并头插法归还到链表
    spin_lock(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    spin_unlock(&kmem.lock);
}

// 分配物理页
void *kalloc(void) {
    struct run *r;

    spin_lock(&kmem.lock);
    r = kmem.freelist;
    if (r) {
        kmem.freelist = r->next;
    }
    spin_unlock(&kmem.lock);

    if (r) {
        // 分配出去前清零内存 (防止读到旧数据)
        uart_puts("kalloc: allocated a page\n");
        char *p = (char*)r;
        for(int i=0; i<PGSIZE; i++)
            p[i] = 0;
    }

    return (void *)r;
}