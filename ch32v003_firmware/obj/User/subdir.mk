################################################################################
# MRS Version: 2.1.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/ch32v00x_it.c \
../User/device.c \
../User/main.c \
../User/moisture.c \
../User/packet.c \
../User/system_ch32v00x.c \
../User/timer.c \
../User/uart.c \
../User/valve.c 

C_DEPS += \
./User/ch32v00x_it.d \
./User/device.d \
./User/main.d \
./User/moisture.d \
./User/packet.d \
./User/system_ch32v00x.d \
./User/timer.d \
./User/uart.d \
./User/valve.d 

OBJS += \
./User/ch32v00x_it.o \
./User/device.o \
./User/main.o \
./User/moisture.o \
./User/packet.o \
./User/system_ch32v00x.o \
./User/timer.o \
./User/uart.o \
./User/valve.o 



# Each subdirectory must supply rules for building sources it contributes
User/%.o: ../User/%.c
	@	riscv-none-embed-gcc -march=rv32ecxw -mabi=ilp32e -msmall-data-limit=0 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/Users/dimaa/mounriver-studio-projects/Droplet_Module_CH32V003/Debug" -I"c:/Users/dimaa/mounriver-studio-projects/Droplet_Module_CH32V003/Core" -I"c:/Users/dimaa/mounriver-studio-projects/Droplet_Module_CH32V003/User" -I"c:/Users/dimaa/mounriver-studio-projects/Droplet_Module_CH32V003/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
