################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../framework/source/fw.cpp \
../framework/source/fw_active.cpp \
../framework/source/fw_bitset.cpp \
../framework/source/fw_defer.cpp \
../framework/source/fw_evt.cpp \
../framework/source/fw_hsm.cpp \
../framework/source/fw_log.cpp \
../framework/source/fw_region.cpp \
../framework/source/fw_timer.cpp 

OBJS += \
./framework/source/fw.o \
./framework/source/fw_active.o \
./framework/source/fw_bitset.o \
./framework/source/fw_defer.o \
./framework/source/fw_evt.o \
./framework/source/fw_hsm.o \
./framework/source/fw_log.o \
./framework/source/fw_region.o \
./framework/source/fw_timer.o 

CPP_DEPS += \
./framework/source/fw.d \
./framework/source/fw_active.d \
./framework/source/fw_bitset.d \
./framework/source/fw_defer.d \
./framework/source/fw_evt.d \
./framework/source/fw_hsm.d \
./framework/source/fw_log.d \
./framework/source/fw_region.d \
./framework/source/fw_timer.d 


# Each subdirectory must supply rules for building sources it contributes
framework/source/%.o: ../framework/source/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C++ Compiler'
	arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb -O3 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -ffreestanding -fno-move-loop-invariants -Wall -Wextra  -g3 -DDEBUG -DTRACE -DSTM32L476xx -I"../include" -I"../system/include" -I"../system/include/cmsis" -I"../system/include/stm32l4xx" -I"../system/BSP/STM32L4xx-Nucleo" -I"../system/BSP/Components" -I../qpcpp/ports/arm-cm/qxk/gnu -I../qpcpp/include -I"../framework/include" -I"../src/Sample" -I"../src/Sample/SampleReg" -I"../src/System" -I"../src/UartAct" -I"../src/UartAct/UartIn" -I"../src/UartAct/UartOut" -I"../src/BtnGrp" -I"../src/BtnGrp/Btn" -I"../src/LedGrp" -I"../src/LedGrp/Led" -std=gnu++11 -fabi-version=0 -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


