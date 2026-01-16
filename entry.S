.section .text.entry
.global _start

_start:
    # 1. 设置栈指针 (Stack Pointer)
    # 栈是向下增长的，我们把栈顶设在内存 128MB 处
    li sp, 0x88000000

    # 2. 跳转到 C 语言的 main 函数 (kernel.c)
    call main

    # 3. 如果 main 返回了，就死循环
loop:
    j loop