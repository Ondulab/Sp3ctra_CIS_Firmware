################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middleware/Src/synth.c \
../Middleware/Src/wave_generation.c \
../Middleware/Src/wave_sommation.c 

OBJS += \
./Middleware/Src/synth.o \
./Middleware/Src/wave_generation.o \
./Middleware/Src/wave_sommation.o 

C_DEPS += \
./Middleware/Src/synth.d \
./Middleware/Src/wave_generation.d \
./Middleware/Src/wave_sommation.d 


# Each subdirectory must supply rules for building sources it contributes
Middleware/Src/synth.o: ../Middleware/Src/synth.c Middleware/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Middleware/Src/synth.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Middleware/Src/wave_generation.o: ../Middleware/Src/wave_generation.c Middleware/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Middleware/Src/wave_generation.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Middleware/Src/wave_sommation.o: ../Middleware/Src/wave_sommation.c Middleware/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Middleware/Src/wave_sommation.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

