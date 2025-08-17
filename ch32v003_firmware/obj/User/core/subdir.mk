################################################################################
# MRS Version: 2.1.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/core/packet.c \
../User/core/protocol.c 

C_DEPS += \
./User/core/packet.d \
./User/core/protocol.d 

OBJS += \
./User/core/packet.o \
./User/core/protocol.o 



# Each subdirectory must supply rules for building sources it contributes
User/core/%.o: ../User/core/%.c
	@	riscv-none-embed-gcc -march=rv32ecxw -mabi=ilp32e -msmall-data-limit=0 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"e:/Projects/WaterNode/ch32v003_firmware/Debug" -I"e:/Projects/WaterNode/ch32v003_firmware/Core" -I"e:/Projects/WaterNode/ch32v003_firmware/User" -I"e:/Projects/WaterNode/ch32v003_firmware/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
