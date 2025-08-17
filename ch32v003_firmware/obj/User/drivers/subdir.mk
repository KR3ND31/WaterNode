################################################################################
# MRS Version: 2.1.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/drivers/device.c \
../User/drivers/system.c \
../User/drivers/timer.c \
../User/drivers/uart.c 

C_DEPS += \
./User/drivers/device.d \
./User/drivers/system.d \
./User/drivers/timer.d \
./User/drivers/uart.d 

OBJS += \
./User/drivers/device.o \
./User/drivers/system.o \
./User/drivers/timer.o \
./User/drivers/uart.o 



# Each subdirectory must supply rules for building sources it contributes
User/drivers/%.o: ../User/drivers/%.c
	@	riscv-none-embed-gcc -march=rv32ecxw -mabi=ilp32e -msmall-data-limit=0 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"e:/Projects/WaterNode/ch32v003_firmware/Debug" -I"e:/Projects/WaterNode/ch32v003_firmware/Core" -I"e:/Projects/WaterNode/ch32v003_firmware/User" -I"e:/Projects/WaterNode/ch32v003_firmware/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
