################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../L4_IO/wireless/src/mesh.c \
../L4_IO/wireless/src/nrf24L01Plus.c \
../L4_IO/wireless/src/wireless.c 

OBJS += \
./L4_IO/wireless/src/mesh.o \
./L4_IO/wireless/src/nrf24L01Plus.o \
./L4_IO/wireless/src/wireless.o 

C_DEPS += \
./L4_IO/wireless/src/mesh.d \
./L4_IO/wireless/src/nrf24L01Plus.d \
./L4_IO/wireless/src/wireless.d 


# Each subdirectory must supply rules for building sources it contributes
L4_IO/wireless/src/%.o: ../L4_IO/wireless/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -ffunction-sections -fdata-sections -Wall -Wshadow -Wlogical-op -Wfloat-equal -DBUILD_CFG_MPU=0 -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\newlib" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L0_LowLevel" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L1_FreeRTOS" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L1_FreeRTOS\include" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L1_FreeRTOS\portable" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L2_Drivers" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L2_Drivers\base" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L3_Utils" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L3_Utils\tlm" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L4_IO" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L4_IO\fat" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L4_IO\wireless" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L5_Application" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


