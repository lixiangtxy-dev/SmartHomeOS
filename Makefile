# ==========================================
# SmartHomeOS Makefile (支持多目录架构)
# ==========================================

# 1. 编译器设置
CC = riscv64-unknown-elf-gcc
# 核心改动：加入 -I./include 告诉编译器头文件在哪里
CFLAGS = -march=rv32imac_zicsr -mabi=ilp32 -mcmodel=medany -ffreestanding -O0 -g -Wall -I./include

# 2. 模拟器设置
QEMU = qemu-system-riscv32
QEMU_FLAGS = -nographic -machine virt -bios none -kernel kernel.elf 

# 3. 源码目录配置 (把你新建的文件夹列在这里)
SRC_DIRS = core fs lib

# 4. 自动搜集源文件 (黑魔法：自动找到所有 .c 和 .S)
C_SRCS = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
S_SRCS = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.S))

# 5. 生成对应的 .o 目标文件列表
OBJS = $(C_SRCS:.c=.o) $(S_SRCS:.S=.o)
LIBGCC = $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

# 默认动作
all: run

# 链接步骤
kernel.elf: kernel.ld $(OBJS)
	$(CC) -T kernel.ld -o kernel.elf $(CFLAGS) -nostdlib $(OBJS) $(LIBGCC)

# 编译 C 语言文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 编译汇编文件
%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

# 运行 QEMU
run: kernel.elf
	@echo "------------------"
	@echo "按 Ctrl+A 然后按 X 退出 QEMU"
	@echo "------------------"
	$(QEMU) $(QEMU_FLAGS)

# 清理垃圾 (会清理根目录和所有子目录下的 .o 文件)
clean:
	rm -f kernel.elf
	rm -f $(OBJS)