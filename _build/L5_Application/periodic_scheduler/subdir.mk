################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../L5_Application/periodic_scheduler/period_callbacks.cpp \
../L5_Application/periodic_scheduler/prd_dispatcher.cpp 

OBJS += \
./L5_Application/periodic_scheduler/period_callbacks.o \
./L5_Application/periodic_scheduler/prd_dispatcher.o 

CPP_DEPS += \
./L5_Application/periodic_scheduler/period_callbacks.d \
./L5_Application/periodic_scheduler/prd_dispatcher.d 


# Each subdirectory must supply rules for building sources it contributes
L5_Application/periodic_scheduler/%.o: ../L5_Application/periodic_scheduler/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C++ Compiler'
	arm-none-eabi-g++ -mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -ffunction-sections -fdata-sections -Wall -Wshadow -Wlogical-op -Wfloat-equal -DBUILD_CFG_MPU=0 -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\newlib" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L0_LowLevel" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L1_FreeRTOS" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L1_FreeRTOS\include" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L1_FreeRTOS\portable" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L2_Drivers" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L2_Drivers\base" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L3_Utils" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L3_Utils\tlm" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L4_IO" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L4_IO\fat" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L4_IO\wireless" -I"E:\School\cmpe146\SJSU_Dev\SJSU_Dev\projects\lpc1758_freertos\L5_Application" -std=gnu++11 -fabi-version=0 -fno-exceptions -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


