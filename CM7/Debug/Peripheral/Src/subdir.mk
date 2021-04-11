################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Peripheral/Src/cis.c \
../Peripheral/Src/ssd1362.c \
../Peripheral/Src/times_base.c 

OBJS += \
./Peripheral/Src/cis.o \
./Peripheral/Src/ssd1362.o \
./Peripheral/Src/times_base.o 

C_DEPS += \
./Peripheral/Src/cis.d \
./Peripheral/Src/ssd1362.d \
./Peripheral/Src/times_base.d 


# Each subdirectory must supply rules for building sources it contributes
Peripheral/Src/cis.o: ../Peripheral/Src/cis.c Peripheral/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Peripheral/Src/cis.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Peripheral/Src/ssd1362.o: ../Peripheral/Src/ssd1362.c Peripheral/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Peripheral/Src/ssd1362.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Peripheral/Src/times_base.o: ../Peripheral/Src/times_base.c Peripheral/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Peripheral/Src/times_base.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

