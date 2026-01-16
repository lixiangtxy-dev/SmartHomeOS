# switch.S - 任务切换的核心汇编
.text
.global switch_to

# 函数原型：void switch_to(uint32_t **old_sp_ptr, uint32_t *new_sp);
# 参数 a0: 指向“旧任务栈指针变量”的指针 (用来保存当前的 sp)
# 参数 a1: 新任务的栈指针 (用来恢复 sp)

switch_to:
    # 1. 在当前任务的栈中腾出空间保存寄存器
    # 我们需要保存 ra (返回地址), sp, s0~s11 (被调用者保存寄存器)
    # 共 14 个寄存器 * 4 字节 = 56 字节
    addi sp, sp, -56

    # 2. 保存当前寄存器到栈里
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

    # 3. 将当前的栈指针 sp 保存到 old_task->sp 变量中
    sw sp, 0(a0)

    # -------------------------------------------
    #    此刻，CPU 切换到了新任务的世界！
    # -------------------------------------------

    # 4. 从新任务中加载栈指针
    mv sp, a1

    # 5. 从新栈中恢复寄存器
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

    # 6. 恢复栈空间
    addi sp, sp, 56

    # 7. 跳转回新任务的代码 (ra 寄存器里存着新任务的函数地址)
    ret