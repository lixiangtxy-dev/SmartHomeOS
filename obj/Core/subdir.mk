################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/core_riscv.c 

C_DEPS += \
./Core/core_riscv.d 

OBJS += \
./Core/core_riscv.o 

DIR_OBJS += \
./Core/*.o \

DIR_DEPS += \
./Core/*.d \

DIR_EXPANDS += \
./Core/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
Core/%.o: ../Core/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/Users/Administrator/Desktop/SmartHome/SmartHome-mcu/CH32V307RCT/Debug" -I"c:/Users/Administrator/Desktop/SmartHome/SmartHome-mcu/CH32V307RCT/Core" -I"c:/Users/Administrator/Desktop/SmartHome/SmartHome-mcu/CH32V307RCT/User" -I"c:/Users/Administrator/Desktop/SmartHome/SmartHome-mcu/CH32V307RCT/Peripheral/inc" -I"c:/Users/Administrator/Desktop/SmartHome/SmartHome-mcu/CH32V307RCT/OS_Core" -I"c:/Users/Administrator/Desktop/SmartHome/SmartHome-mcu/CH32V307RCT/OS_Drivers" -I"c:/Users/Administrator/Desktop/SmartHome/SmartHome-mcu/CH32V307RCT/OS_HAL" -I"c:/Users/Administrator/Desktop/SmartHome/SmartHome-mcu/CH32V307RCT/LittleFS" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

