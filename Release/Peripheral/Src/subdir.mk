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
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Drivers/CMSIS/Include -I../Core/Inc -I"/mnt/data/Documents/Workspace_Cube/CISYNTH/Middleware/Inc" -I"/mnt/data/Documents/Workspace_Cube/CISYNTH/Peripheral/Inc" -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/STM32H7xx_HAL_Driver/Inc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Peripheral/Src/cis.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Peripheral/Src/synth.o: ../Peripheral/Src/synth.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Drivers/CMSIS/Include -I../Core/Inc -I"/mnt/data/Documents/Workspace_Cube/CISYNTH/Middleware/Inc" -I"/mnt/data/Documents/Workspace_Cube/CISYNTH/Peripheral/Inc" -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/STM32H7xx_HAL_Driver/Inc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Peripheral/Src/synth.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Peripheral/Src/times_base.o: ../Peripheral/Src/times_base.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Drivers/CMSIS/Include -I../Core/Inc -I"/mnt/data/Documents/Workspace_Cube/CISYNTH/Middleware/Inc" -I"/mnt/data/Documents/Workspace_Cube/CISYNTH/Peripheral/Inc" -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/STM32H7xx_HAL_Driver/Inc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Peripheral/Src/times_base.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Peripheral/Src/wave_generation.o: ../Peripheral/Src/wave_generation.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Drivers/CMSIS/Include -I../Core/Inc -I"/mnt/data/Documents/Workspace_Cube/CISYNTH/Middleware/Inc" -I"/mnt/data/Documents/Workspace_Cube/CISYNTH/Peripheral/Inc" -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/STM32H7xx_HAL_Driver/Inc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Peripheral/Src/wave_generation.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Peripheral/Src/wave_sommation.o: ../Peripheral/Src/wave_sommation.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Drivers/CMSIS/Include -I../Core/Inc -I"/mnt/data/Documents/Workspace_Cube/CISYNTH/Middleware/Inc" -I"/mnt/data/Documents/Workspace_Cube/CISYNTH/Peripheral/Inc" -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/STM32H7xx_HAL_Driver/Inc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Peripheral/Src/wave_sommation.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

