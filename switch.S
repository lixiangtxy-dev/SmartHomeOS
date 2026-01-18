.text
.global switch_to

# void switch_to(uint32_t **old_sp, uint32_t *new_sp);
switch_to:
    # 1. 保存当前任务的 Callee-Saved 寄存器
    addi sp, sp, -56
    sw ra,  52(sp)
    sw s0,  48(sp)
    sw s1,  44(sp)
    sw s2,  40(sp)
    sw s3,  36(sp)
    sw s4,  32(sp)
    sw s5,  28(sp)
    sw s6,  24(sp)
    sw s7,  20(sp)
    sw s8,  16(sp)
    sw s9,  12(sp)
    sw s10, 8(sp)
    sw s11, 4(sp)

    # 2. 保存旧栈指针
    sw sp, 0(a0)

    # 3. 切换到新栈
    mv sp, a1

    # 4. 恢复新任务的寄存器
    lw ra,  52(sp)
    lw s0,  48(sp)
    lw s1,  44(sp)
    lw s2,  40(sp)
    lw s3,  36(sp)
    lw s4,  32(sp)
    lw s5,  28(sp)
    lw s6,  24(sp)
    lw s7,  20(sp)
    lw s8,  16(sp)
    lw s9,  12(sp)
    lw s10, 8(sp)
    lw s11, 4(sp)
    addi sp, sp, 56

    ret