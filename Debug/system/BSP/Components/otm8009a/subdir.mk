################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../system/BSP/Components/otm8009a/otm8009a.c 

OBJS += \
./system/BSP/Components/otm8009a/otm8009a.o 

C_DEPS += \
./system/BSP/Components/otm8009a/otm8009a.d 


# Each subdirectory must supply rules for building sources it contributes
system/BSP/Components/otm8009a/%.o: ../system/BSP/Components/otm8009a/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -O3 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -ffreestanding -fno-move-loop-invariants -Wall -Wextra  -g3 -DDEBUG -DTRACE -DSTM32L476xx -I"../include" -I"../system/include" -I"../system/include/cmsis" -I"../system/include/stm32l4xx" -I"../system/BSP/STM32L4xx-Nucleo" -I"../system/BSP/Components" -I"../framework/include" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


