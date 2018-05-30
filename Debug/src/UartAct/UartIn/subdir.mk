################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/UartAct/UartIn/UartIn.cpp 

OBJS += \
./src/UartAct/UartIn/UartIn.o 

CPP_DEPS += \
./src/UartAct/UartIn/UartIn.d 


# Each subdirectory must supply rules for building sources it contributes
src/UartAct/UartIn/%.o: ../src/UartAct/UartIn/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C++ Compiler'
	arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb -O3 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -ffreestanding -fno-move-loop-invariants -Wall -Wextra  -g3 -DDEBUG -DTRACE -DSTM32L476xx -I"../include" -I"../system/include" -I"../system/include/cmsis" -I"../system/include/stm32l4xx" -I"../system/BSP/STM32L4xx-Nucleo" -I"../system/BSP/Components" -I../qpcpp/ports/arm-cm/qxk/gnu -I../qpcpp/include -I"../framework/include" -I"../src/Sample" -I"../src/Sample/SampleReg" -I"../src/System" -I"../src/UartAct" -I"../src/UartAct/UartIn" -I"../src/UartAct/UartOut" -I"../src/BtnGrp" -I"../src/BtnGrp/Btn" -I"../src/LedGrp" -I"../src/LedGrp/Led" -std=gnu++11 -fabi-version=0 -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


