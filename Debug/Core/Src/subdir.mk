################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../Core/Src/startup_stm32h750xx.s 

C_SRCS += \
../Core/Src/adc.c \
../Core/Src/crc.c \
../Core/Src/dac.c \
../Core/Src/debug.c \
../Core/Src/dma2d.c \
../Core/Src/fmc.c \
../Core/Src/gpio.c \
../Core/Src/hrtim.c \
../Core/Src/ltdc.c \
../Core/Src/main.c \
../Core/Src/quadspi.c \
../Core/Src/rtc.c \
../Core/Src/sai.c \
../Core/Src/sdmmc.c \
../Core/Src/spi.c \
../Core/Src/stm32h7xx_hal_msp.c \
../Core/Src/stm32h7xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/system_stm32h7xx.c \
../Core/Src/tim.c \
../Core/Src/usart.c 

OBJS += \
./Core/Src/adc.o \
./Core/Src/crc.o \
./Core/Src/dac.o \
./Core/Src/debug.o \
./Core/Src/dma2d.o \
./Core/Src/fmc.o \
./Core/Src/gpio.o \
./Core/Src/hrtim.o \
./Core/Src/ltdc.o \
./Core/Src/main.o \
./Core/Src/quadspi.o \
./Core/Src/rtc.o \
./Core/Src/sai.o \
./Core/Src/sdmmc.o \
./Core/Src/spi.o \
./Core/Src/startup_stm32h750xx.o \
./Core/Src/stm32h7xx_hal_msp.o \
./Core/Src/stm32h7xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/system_stm32h7xx.o \
./Core/Src/tim.o \
./Core/Src/usart.o 

C_DEPS += \
./Core/Src/adc.d \
./Core/Src/crc.d \
./Core/Src/dac.d \
./Core/Src/debug.d \
./Core/Src/dma2d.d \
./Core/Src/fmc.d \
./Core/Src/gpio.d \
./Core/Src/hrtim.d \
./Core/Src/ltdc.d \
./Core/Src/main.d \
./Core/Src/quadspi.d \
./Core/Src/rtc.d \
./Core/Src/sai.d \
./Core/Src/sdmmc.d \
./Core/Src/spi.d \
./Core/Src/stm32h7xx_hal_msp.d \
./Core/Src/stm32h7xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/system_stm32h7xx.d \
./Core/Src/tim.d \
./Core/Src/usart.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/adc.o: ../Core/Src/adc.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/adc.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/crc.o: ../Core/Src/crc.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/crc.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/dac.o: ../Core/Src/dac.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/dac.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/debug.o: ../Core/Src/debug.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/debug.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/dma2d.o: ../Core/Src/dma2d.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/dma2d.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/fmc.o: ../Core/Src/fmc.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/fmc.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/gpio.o: ../Core/Src/gpio.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/gpio.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/hrtim.o: ../Core/Src/hrtim.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/hrtim.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/ltdc.o: ../Core/Src/ltdc.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/ltdc.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/main.o: ../Core/Src/main.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/main.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/quadspi.o: ../Core/Src/quadspi.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/quadspi.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/rtc.o: ../Core/Src/rtc.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/rtc.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/sai.o: ../Core/Src/sai.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/sai.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/sdmmc.o: ../Core/Src/sdmmc.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/sdmmc.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/spi.o: ../Core/Src/spi.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/spi.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/%.o: ../Core/Src/%.s
	arm-none-eabi-gcc -mcpu=cortex-m7 -g3 -c -x assembler-with-cpp --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"
Core/Src/stm32h7xx_hal_msp.o: ../Core/Src/stm32h7xx_hal_msp.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/stm32h7xx_hal_msp.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/stm32h7xx_it.o: ../Core/Src/stm32h7xx_it.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/stm32h7xx_it.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/syscalls.o: ../Core/Src/syscalls.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/syscalls.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/system_stm32h7xx.o: ../Core/Src/system_stm32h7xx.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32H750xx -DUSE_STM32H750B_DISCO -DTS_MULTI_TOUCH_SUPPORTED '-DAPPLICATION_ADDRESS=0x90000000' -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/system_stm32h7xx.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/tim.o: ../Core/Src/tim.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/tim.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/usart.o: ../Core/Src/usart.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DSTM32H750xx '-DAPPLICATION_ADDRESS=0x90000000' -DUSE_HAL_DRIVER -DDEBUG -c -I../Drivers/BSP/Components/rk043fn48h -I../Peripheral/Inc -I../Utilities -I../Drivers/BSP/Components/ft5336 -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/Components/mt48lc4m32b2 -I../Drivers/BSP/STM32H750B-Discovery -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/BSP/Components -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/BSP/Components/Common -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Application/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/usart.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

