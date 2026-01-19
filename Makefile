# 编译器设置
CC = riscv64-unknown-elf-gcc
CFLAGS = -march=rv32imac_zicsr -mabi=ilp32 -mcmodel=medany -ffreestanding -O0 -g -Wall

# 模拟器设置
QEMU = qemu-system-riscv32
QEMU_FLAGS = -nographic -machine virt -bios none -kernel kernel.elf 

# 目标文件
OBJS = entry.o kernel.o switch.o trap.o lock.o

# 默认动作：编译并运行
all: run

# 链接步骤：把所有 .o 文件打包成 kernel.elf
kernel.elf: kernel.ld $(OBJS)
	$(CC) -T kernel.ld -o kernel.elf $(CFLAGS) -nostdlib $(OBJS)

# 编译步骤：把 .c 变成 .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 汇编步骤：把 .S 变成 .o
%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

# 运行 QEMU
run: kernel.elf
	@echo "------------------"
	@echo "按 Ctrl+A 然后按 X 退出 QEMU"
	@echo "------------------"
	$(QEMU) $(QEMU_FLAGS)

# 清理垃圾
clean:
	rm -f *.o *.elf