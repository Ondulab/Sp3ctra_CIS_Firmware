################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Application/Src/cisynth_ifft.c \
../Application/Src/cisynth_imagePlay.c \
../Application/Src/menu.c 

OBJS += \
./Application/Src/cisynth_ifft.o \
./Application/Src/cisynth_imagePlay.o \
./Application/Src/menu.o 

C_DEPS += \
./Application/Src/cisynth_ifft.d \
./Application/Src/cisynth_imagePlay.d \
./Application/Src/menu.d 


# Each subdirectory must supply rules for building sources it contributes
Application/Src/cisynth_ifft.o: ../Application/Src/cisynth_ifft.c Application/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/Src/cisynth_ifft.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/Src/cisynth_imagePlay.o: ../Application/Src/cisynth_imagePlay.c Application/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/Src/cisynth_imagePlay.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/Src/menu.o: ../Application/Src/menu.c Application/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/Src/menu.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

