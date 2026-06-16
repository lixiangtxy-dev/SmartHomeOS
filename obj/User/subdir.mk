################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/ch32v30x_it.c \
../User/main.c \
../User/mempool.c \
../User/mutex.c \
../User/system_ch32v30x.c \
../User/task.c 

C_DEPS += \
./User/ch32v30x_it.d \
./User/main.d \
./User/mempool.d \
./User/mutex.d \
./User/system_ch32v30x.d \
./User/task.d 

S_UPPER_SRCS += \
../User/switch.S 

S_UPPER_DEPS += \
./User/switch.d 

OBJS += \
./User/ch32v30x_it.o \
./User/main.o \
./User/mempool.o \
./User/mutex.o \
./User/switch.o \
./User/system_ch32v30x.o \
./User/task.o 

DIR_OBJS += \
./User/*.o \

DIR_DEPS += \
./User/*.d \

DIR_EXPANDS += \
./User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
User/%.o: ../User/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/Users/Administrator/Desktop/SmartHome/SmartHome-mcu/CH32V307RCT/Debug" -I"c:/Users/Administrator/Desktop/SmartHome/SmartHome-mcu/CH32V307RCT/Core" -I"c:/Users/Administrator/Desktop/SmartHome/SmartHome-mcu/CH32V307RCT/User" -I"c:/Users/Administrator/Desktop/SmartHome/SmartHome-mcu/CH32V307RCT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

User/%.o: ../User/%.S
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -x assembler-with-cpp -I"c:/Users/Administrator/Desktop/SmartHome/SmartHome-mcu/CH32V307RCT/Startup" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

