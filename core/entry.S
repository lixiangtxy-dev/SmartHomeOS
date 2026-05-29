.section .text.entry
.global _start
.global trap_entry

_start:
    # 1. 设置栈指针 (128MB 处)
    li sp, 0x88000000
    # 2. 跳转到 main
    call main
loop:
    j loop

# ---------------- 中断入口 ----------------
.align 4
trap_entry:
    # 1. 保存“调用者保存寄存器” (Caller Saved)
    # 因为中断可能打断任何计算，必须保存临时寄存器
    addi sp, sp, -112
    sw ra, 108(sp)
    sw t0, 104(sp)
    sw t1, 100(sp)
    sw t2, 96(sp)
    sw t3, 92(sp)
    sw t4, 88(sp)
    sw t5, 84(sp)
    sw t6, 80(sp)
    sw a0, 76(sp)
    sw a1, 72(sp)
    sw a2, 68(sp)
    sw a3, 64(sp)
    sw a4, 60(sp)
    sw a5, 56(sp)
    sw a6, 52(sp)
    sw a7, 48(sp)

    # 2. 读取中断原因和异常PC，作为参数传递给 C 函数
    csrr a0, mcause
    csrr a1, mepc
    call trap_handler

    # 3. 恢复寄存器
    lw ra, 108(sp)
    lw t0, 104(sp)
    lw t1, 100(sp)
    lw t2, 96(sp)
    lw t3, 92(sp)
    lw t4, 88(sp)
    lw t5, 84(sp)
    lw t6, 80(sp)
    lw a0, 76(sp)
    lw a1, 72(sp)
    lw a2, 68(sp)
    lw a3, 64(sp)
    lw a4, 60(sp)
    lw a5, 56(sp)
    lw a6, 52(sp)
    lw a7, 48(sp)
    addi sp, sp, 112

    # 4. 中断返回
    mret