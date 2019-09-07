################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Peripheral/Src/cis.c \
../Peripheral/Src/synth.c \
../Peripheral/Src/times_base.c \
../Peripheral/Src/wave_generation.c \
../Peripheral/Src/wave_sommation.c 

OBJS += \
./Peripheral/Src/cis.o \
./Peripheral/Src/synth.o \
./Peripheral/Src/times_base.o \
./Peripheral/Src/wave_generation.o \
./Peripheral/Src/wave_sommation.o 

C_DEPS += \
./Peripheral/Src/cis.d \
./Peripheral/Src/synth.d \
./Peripheral/Src/times_base.d \
./Peripheral/Src/wave_generation.d \
./Peripheral/Src/wave_sommation.d 


# Each subdirectory must supply rules for building sources it contributes
Peripheral/Src/cis.o: ../Peripheral/Src/cis.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16 '-D__weak=__attribute__((weak))' -D__FPU_PRESENT -DARM_MATH_CM7 '-D__packed="__attribute__((__packed__))"' -DUSE_HAL_DRIVER -DSTM32H743xx -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Core/Inc" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Drivers/CMSIS/DSP/Include" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Middleware/Inc" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Peripheral/Inc" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Drivers/STM32H7xx_HAL_Driver/Inc" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Drivers/CMSIS/Device/ST/STM32H7xx/Include" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Drivers/CMSIS/Include" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Drivers/CMSIS/DSP/Include"  -O2 -Wall -fmessage-length=0 -ffunction-sections -fdata-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Peripheral/Src/%.o: ../Peripheral/Src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16 '-D__weak=__attribute__((weak))' -D__FPU_PRESENT -DARM_MATH_CM7 '-D__packed="__attribute__((__packed__))"' -DUSE_HAL_DRIVER -DSTM32H743xx -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Core/Inc" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Drivers/CMSIS/DSP/Include" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Middleware/Inc" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Peripheral/Inc" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Drivers/STM32H7xx_HAL_Driver/Inc" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Drivers/CMSIS/Device/ST/STM32H7xx/Include" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Drivers/CMSIS/Include" -I"/mnt/data/Documents/Workspace_AC6/CISYNTH/Drivers/CMSIS/DSP/Include"  -O3 -Ofast -Wall -fmessage-length=0 -ffunction-sections -fdata-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


