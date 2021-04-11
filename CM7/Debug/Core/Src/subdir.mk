################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/adc.c \
../Core/Src/dma.c \
../Core/Src/eth.c \
../Core/Src/fmc.c \
../Core/Src/gpio.c \
../Core/Src/main.c \
../Core/Src/rng.c \
../Core/Src/sai.c \
../Core/Src/spi.c \
../Core/Src/stm32h7xx_hal_msp.c \
../Core/Src/stm32h7xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/tim.c \
../Core/Src/usart.c 

OBJS += \
./Core/Src/adc.o \
./Core/Src/dma.o \
./Core/Src/eth.o \
./Core/Src/fmc.o \
./Core/Src/gpio.o \
./Core/Src/main.o \
./Core/Src/rng.o \
./Core/Src/sai.o \
./Core/Src/spi.o \
./Core/Src/stm32h7xx_hal_msp.o \
./Core/Src/stm32h7xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/tim.o \
./Core/Src/usart.o 

C_DEPS += \
./Core/Src/adc.d \
./Core/Src/dma.d \
./Core/Src/eth.d \
./Core/Src/fmc.d \
./Core/Src/gpio.d \
./Core/Src/main.d \
./Core/Src/rng.d \
./Core/Src/sai.d \
./Core/Src/spi.d \
./Core/Src/stm32h7xx_hal_msp.d \
./Core/Src/stm32h7xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/tim.d \
./Core/Src/usart.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/adc.o: ../Core/Src/adc.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/adc.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/dma.o: ../Core/Src/dma.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/dma.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/eth.o: ../Core/Src/eth.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/eth.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/fmc.o: ../Core/Src/fmc.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/fmc.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/gpio.o: ../Core/Src/gpio.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/gpio.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/main.o: ../Core/Src/main.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/main.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/rng.o: ../Core/Src/rng.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/rng.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/sai.o: ../Core/Src/sai.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/sai.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/spi.o: ../Core/Src/spi.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/spi.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/stm32h7xx_hal_msp.o: ../Core/Src/stm32h7xx_hal_msp.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/stm32h7xx_hal_msp.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/stm32h7xx_it.o: ../Core/Src/stm32h7xx_it.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/stm32h7xx_it.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/syscalls.o: ../Core/Src/syscalls.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/syscalls.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/sysmem.o: ../Core/Src/sysmem.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/sysmem.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/tim.o: ../Core/Src/tim.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/tim.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/usart.o: ../Core/Src/usart.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -D__FPU_PRESENT -DARM_MATH_CM7 -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -c -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Application/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Core/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Middleware/Inc" -I"/home/zhonx/Documents/Workspace_Cube2/SSS_CIS/CM7/Peripheral/Inc" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/usart.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

