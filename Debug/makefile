################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Utilities/subdir.mk
-include Peripheral/Src/subdir.mk
-include Drivers/STM32H7xx_HAL_Driver/Src/subdir.mk
-include Drivers/BSP/STM32H750B-Discovery/subdir.mk
-include Drivers/BSP/Components/wm8994/subdir.mk
-include Drivers/BSP/Components/mt48lc4m32b2/subdir.mk
-include Drivers/BSP/Components/ft5336/subdir.mk
-include Core/Src/subdir.mk
-include Application/Src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
CISYNTH.elf \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
CISYNTH.list \

OBJCOPY_BIN += \
CISYNTH.bin \


# All Target
all: CISYNTH.elf secondary-outputs

# Tool invocations
CISYNTH.elf: $(OBJS) $(USER_OBJS) /mnt/data/Documents/Workspace_Cube/CISYNTH/CISYNTH.ld
	arm-none-eabi-gcc -o "CISYNTH.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m7 -T"/mnt/data/Documents/Workspace_Cube/CISYNTH/CISYNTH.ld" --specs=nosys.specs -Wl,-Map="CISYNTH.map" -Wl,--gc-sections -static -L"/mnt/data/Documents/Workspace_Cube/CISYNTH/Addons/PDM/Lib" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

CISYNTH.list: $(EXECUTABLES)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "CISYNTH.list"
	@echo 'Finished building: $@'
	@echo ' '

CISYNTH.bin: $(EXECUTABLES)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "CISYNTH.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) *
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_BIN)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
