################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../qpcpp/source/qep_hsm.cpp \
../qpcpp/source/qep_msm.cpp \
../qpcpp/source/qf_act.cpp \
../qpcpp/source/qf_actq.cpp \
../qpcpp/source/qf_defer.cpp \
../qpcpp/source/qf_dyn.cpp \
../qpcpp/source/qf_mem.cpp \
../qpcpp/source/qf_ps.cpp \
../qpcpp/source/qf_qact.cpp \
../qpcpp/source/qf_qeq.cpp \
../qpcpp/source/qf_qmact.cpp \
../qpcpp/source/qf_time.cpp \
../qpcpp/source/qxk.cpp \
../qpcpp/source/qxk_mutex.cpp \
../qpcpp/source/qxk_sema.cpp \
../qpcpp/source/qxk_xthr.cpp 

OBJS += \
./qpcpp/source/qep_hsm.o \
./qpcpp/source/qep_msm.o \
./qpcpp/source/qf_act.o \
./qpcpp/source/qf_actq.o \
./qpcpp/source/qf_defer.o \
./qpcpp/source/qf_dyn.o \
./qpcpp/source/qf_mem.o \
./qpcpp/source/qf_ps.o \
./qpcpp/source/qf_qact.o \
./qpcpp/source/qf_qeq.o \
./qpcpp/source/qf_qmact.o \
./qpcpp/source/qf_time.o \
./qpcpp/source/qxk.o \
./qpcpp/source/qxk_mutex.o \
./qpcpp/source/qxk_sema.o \
./qpcpp/source/qxk_xthr.o 

CPP_DEPS += \
./qpcpp/source/qep_hsm.d \
./qpcpp/source/qep_msm.d \
./qpcpp/source/qf_act.d \
./qpcpp/source/qf_actq.d \
./qpcpp/source/qf_defer.d \
./qpcpp/source/qf_dyn.d \
./qpcpp/source/qf_mem.d \
./qpcpp/source/qf_ps.d \
./qpcpp/source/qf_qact.d \
./qpcpp/source/qf_qeq.d \
./qpcpp/source/qf_qmact.d \
./qpcpp/source/qf_time.d \
./qpcpp/source/qxk.d \
./qpcpp/source/qxk_mutex.d \
./qpcpp/source/qxk_sema.d \
./qpcpp/source/qxk_xthr.d 


# Each subdirectory must supply rules for building sources it contributes
qpcpp/source/%.o: ../qpcpp/source/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C++ Compiler'
	arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb -O3 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -ffreestanding -fno-move-loop-invariants -Wall -Wextra  -g3 -DDEBUG -DTRACE -DSTM32L476xx -I"../include" -I"../system/include" -I"../system/include/cmsis" -I"../system/include/stm32l4xx" -I"../system/BSP/STM32L4xx-Nucleo" -I"../system/BSP/Components" -I../qpcpp/ports/arm-cm/qxk/gnu -I../qpcpp/include -I"../framework/include" -I"../src/Sample" -I"../src/Sample/SampleReg" -I"../src/System" -I"../src/UartAct" -I"../src/UartAct/UartIn" -I"../src/UartAct/UartOut" -I"../src/BtnGrp" -I"../src/BtnGrp/Btn" -I"../src/LedGrp" -I"../src/LedGrp/Led" -std=gnu++11 -fabi-version=0 -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


