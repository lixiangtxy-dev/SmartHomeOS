#include "mempool.h"
#include <stdio.h>

// 定义空闲块的节点结构
typedef struct FreeBlock {
    struct FreeBlock *next;
} FreeBlock;

// 实际的内存池空间
// 注意：RISC-V 架构要求栈必须是 16 字节对齐，因此加上 aligned(16) 属性
__attribute__((aligned(16))) 
static uint8_t stack_memory_pool[MAX_TASK_COUNT][TASK_STACK_SIZE_BYTES];

// 指向第一个空闲块的指针
static FreeBlock *free_list_head = NULL;

/**
 * @brief 初始化内存池，将所有块串联成一个单向链表
 */
void stack_pool_init(void) {
    // 将第一个块作为链表头
    free_list_head = (FreeBlock*)&stack_memory_pool[0][0];
    
    FreeBlock *current = free_list_head;
    
    // 将后续的块逐一链接起来
    for (int i = 1; i < MAX_TASK_COUNT; i++) {
        FreeBlock *next_block = (FreeBlock*)&stack_memory_pool[i][0];
        current->next = next_block;
        current = next_block;
    }
    
    // 最后一个块的 next 置为 NULL
    current->next = NULL;
    
    printf("[Mempool] Initialized %d stacks of %d bytes.\r\n", MAX_TASK_COUNT, TASK_STACK_SIZE_BYTES);
}

/**
 * @brief 分配一个栈空间，时间复杂度 O(1)
 */
void* stack_pool_alloc(void) {
    // 如果没有空闲块了，返回 NULL
    if (free_list_head == NULL) {
        printf("[Mempool] Error: Out of memory!\r\n");
        return NULL;
    }
    
    // 取出链表头部的块
    void *allocated_stack = (void*)free_list_head;
    
    // 链表头指针后移
    free_list_head = free_list_head->next;
    
    return allocated_stack;
}

/**
 * @brief 释放一个栈空间，回收到内存池中，时间复杂度 O(1)
 */
void stack_pool_free(void* stack_ptr) {
    if (stack_ptr == NULL) return;
    
    FreeBlock *block_to_free = (FreeBlock*)stack_ptr;
    
    // 将释放的块插入到空闲链表的头部（头插法最快）
    block_to_free->next = free_list_head;
    free_list_head = block_to_free;
}