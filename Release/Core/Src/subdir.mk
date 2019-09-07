################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/adc.c \
../Core/Src/dac.c \
../Core/Src/debug.c \
../Core/Src/dma.c \
../Core/Src/gpio.c \
../Core/Src/hrtim.c \
../Core/Src/main.c \
../Core/Src/stm32h7xx_hal_msp.c \
../Core/Src/stm32h7xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/system_stm32h7xx.c \
../Core/Src/tim.c \
../Core/Src/usart.c 

OBJS += \
./Core/Src/adc.o \
./Core/Src/dac.o \
./Core/Src/debug.o \
./Core/Src/dma.o \
./Core/Src/gpio.o \
./Core/Src/hrtim.o \
./Core/Src/main.o \
./Core/Src/stm32h7xx_hal_msp.o \
./Core/Src/stm32h7xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/system_stm32h7xx.o \
./Core/Src/tim.o \
./Core/Src/usart.o 

C_DEPS += \
./Core/Src/adc.d \
./Core/Src/dac.d \
./Core/Src/debug.d \
./Core/Src/dma.d \
./Core/Src/gpio.d \
./Core/Src/hrtim.d \
./Core/Src/main.d \
./Core/Src/stm32h7xx_hal_msp.d \
./Core/Src/stm32h7xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/system_stm32h7xx.d \
./Core/Src/tim.d \
./Core/Src/usart.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o: ../Core/Src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16 '-D__weak=__attribute__((weak))' -D__FPU_PRESENT -DARM_MATH_CM7 '-D__packed="__attribute__((__packed__))"' -DUSE_HAL_DRIVER -DSTM32H743xx -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Core/Inc" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Drivers/CMSIS/DSP/Include" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Middleware/Inc" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Peripheral/Inc" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Drivers/STM32H7xx_HAL_Driver/Inc" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Drivers/CMSIS/Device/ST/STM32H7xx/Include" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Drivers/CMSIS/Include" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Drivers/CMSIS/DSP/Include"  -O3 -Ofast -Wall -fmessage-length=0 -ffunction-sections -fdata-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


