################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Ecg_Driver_Manual.c \
../Core/Src/main.c \
../Core/Src/stm32h5xx_hal_msp.c \
../Core/Src/stm32h5xx_it.c \
../Core/Src/system_stm32h5xx.c 

C_DEPS += \
./Core/Src/Ecg_Driver_Manual.d \
./Core/Src/main.d \
./Core/Src/stm32h5xx_hal_msp.d \
./Core/Src/stm32h5xx_it.d \
./Core/Src/system_stm32h5xx.d 

OBJS += \
./Core/Src/Ecg_Driver_Manual.o \
./Core/Src/main.o \
./Core/Src/stm32h5xx_hal_msp.o \
./Core/Src/stm32h5xx_it.o \
./Core/Src/system_stm32h5xx.o 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H563xx -c -I../Core/Inc -I"D:/STM_Workspace_1.19/Final_RT_Edge_Anomaly/Middleware/ecg_anomaly_detector" -I"D:/STM_Workspace_1.19/Final_RT_Edge_Anomaly/Middleware/ecg_anomaly_detector/Inc" -I"D:/STM_Workspace_1.19/Final_RT_Edge_Anomaly/Middleware/ecg_anomaly_detector/Lib" -I../Drivers/STM32H5xx_HAL_Driver/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H5xx/Include -I"D:/STM_Workspace_1.19/Final_RT_Edge_Anomaly/ThirdParty/FreeRTOS/include" -I"D:/STM_Workspace_1.19/Final_RT_Edge_Anomaly/ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure" -I"../Drivers/CMSIS/Device/ST/STM32H5xx/Include " -I../Drivers/CMSIS/Include -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/Ecg_Driver_Manual.cyclo ./Core/Src/Ecg_Driver_Manual.d ./Core/Src/Ecg_Driver_Manual.o ./Core/Src/Ecg_Driver_Manual.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/stm32h5xx_hal_msp.cyclo ./Core/Src/stm32h5xx_hal_msp.d ./Core/Src/stm32h5xx_hal_msp.o ./Core/Src/stm32h5xx_hal_msp.su ./Core/Src/stm32h5xx_it.cyclo ./Core/Src/stm32h5xx_it.d ./Core/Src/stm32h5xx_it.o ./Core/Src/stm32h5xx_it.su ./Core/Src/system_stm32h5xx.cyclo ./Core/Src/system_stm32h5xx.d ./Core/Src/system_stm32h5xx.o ./Core/Src/system_stm32h5xx.su

.PHONY: clean-Core-2f-Src

