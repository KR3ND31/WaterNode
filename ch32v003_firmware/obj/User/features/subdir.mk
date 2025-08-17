################################################################################
# MRS Version: 2.1.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/features/moisture.c \
../User/features/valve.c 

C_DEPS += \
./User/features/moisture.d \
./User/features/valve.d 

OBJS += \
./User/features/moisture.o \
./User/features/valve.o 



# Each subdirectory must supply rules for building sources it contributes
User/features/%.o: ../User/features/%.c
	@	riscv-none-embed-gcc -march=rv32ecxw -mabi=ilp32e -msmall-data-limit=0 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"e:/Projects/WaterNode/ch32v003_firmware/Debug" -I"e:/Projects/WaterNode/ch32v003_firmware/Core" -I"e:/Projects/WaterNode/ch32v003_firmware/User" -I"e:/Projects/WaterNode/ch32v003_firmware/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
