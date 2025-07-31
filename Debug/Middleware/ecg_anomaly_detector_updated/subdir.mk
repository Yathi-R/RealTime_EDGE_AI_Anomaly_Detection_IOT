################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middleware/ecg_anomaly_detector_updated/network.c \
../Middleware/ecg_anomaly_detector_updated/network_data.c \
../Middleware/ecg_anomaly_detector_updated/network_data_params.c 

C_DEPS += \
./Middleware/ecg_anomaly_detector_updated/network.d \
./Middleware/ecg_anomaly_detector_updated/network_data.d \
./Middleware/ecg_anomaly_detector_updated/network_data_params.d 

OBJS += \
./Middleware/ecg_anomaly_detector_updated/network.o \
./Middleware/ecg_anomaly_detector_updated/network_data.o \
./Middleware/ecg_anomaly_detector_updated/network_data_params.o 


# Each subdirectory must supply rules for building sources it contributes
Middleware/ecg_anomaly_detector_updated/%.o Middleware/ecg_anomaly_detector_updated/%.su Middleware/ecg_anomaly_detector_updated/%.cyclo: ../Middleware/ecg_anomaly_detector_updated/%.c Middleware/ecg_anomaly_detector_updated/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H563xx -c -I../Core/Inc -I"D:/STM_Workspace_1.19/Final_RT_Edge_Anomaly/Middleware/ecg_anomaly_detector_updated" -I"D:/STM_Workspace_1.19/Final_RT_Edge_Anomaly/Middleware/ecg_anomaly_detector_updated/Inc" -I"D:/STM_Workspace_1.19/Final_RT_Edge_Anomaly/Middleware/ecg_anomaly_detector" -I"D:/STM_Workspace_1.19/Final_RT_Edge_Anomaly/Middleware/ecg_anomaly_detector/Inc" -I"D:/STM_Workspace_1.19/Final_RT_Edge_Anomaly/Middleware/ecg_anomaly_detector/Lib" -I../Drivers/STM32H5xx_HAL_Driver/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H5xx/Include -I"D:/STM_Workspace_1.19/Final_RT_Edge_Anomaly/ThirdParty/FreeRTOS/include" -I"D:/STM_Workspace_1.19/Final_RT_Edge_Anomaly/ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure" -I"../Drivers/CMSIS/Device/ST/STM32H5xx/Include " -I../Drivers/CMSIS/Include -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middleware-2f-ecg_anomaly_detector_updated

clean-Middleware-2f-ecg_anomaly_detector_updated:
	-$(RM) ./Middleware/ecg_anomaly_detector_updated/network.cyclo ./Middleware/ecg_anomaly_detector_updated/network.d ./Middleware/ecg_anomaly_detector_updated/network.o ./Middleware/ecg_anomaly_detector_updated/network.su ./Middleware/ecg_anomaly_detector_updated/network_data.cyclo ./Middleware/ecg_anomaly_detector_updated/network_data.d ./Middleware/ecg_anomaly_detector_updated/network_data.o ./Middleware/ecg_anomaly_detector_updated/network_data.su ./Middleware/ecg_anomaly_detector_updated/network_data_params.cyclo ./Middleware/ecg_anomaly_detector_updated/network_data_params.d ./Middleware/ecg_anomaly_detector_updated/network_data_params.o ./Middleware/ecg_anomaly_detector_updated/network_data_params.su

.PHONY: clean-Middleware-2f-ecg_anomaly_detector_updated

