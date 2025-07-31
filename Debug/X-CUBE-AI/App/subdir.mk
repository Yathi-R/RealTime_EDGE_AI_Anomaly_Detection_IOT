################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../X-CUBE-AI/App/app_x-cube-ai.c \
../X-CUBE-AI/App/ecg_network.c \
../X-CUBE-AI/App/ecg_network_data.c \
../X-CUBE-AI/App/ecg_network_data_params.c 

C_DEPS += \
./X-CUBE-AI/App/app_x-cube-ai.d \
./X-CUBE-AI/App/ecg_network.d \
./X-CUBE-AI/App/ecg_network_data.d \
./X-CUBE-AI/App/ecg_network_data_params.d 

OBJS += \
./X-CUBE-AI/App/app_x-cube-ai.o \
./X-CUBE-AI/App/ecg_network.o \
./X-CUBE-AI/App/ecg_network_data.o \
./X-CUBE-AI/App/ecg_network_data_params.o 


# Each subdirectory must supply rules for building sources it contributes
X-CUBE-AI/App/%.o X-CUBE-AI/App/%.su X-CUBE-AI/App/%.cyclo: ../X-CUBE-AI/App/%.c X-CUBE-AI/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H563xx -c -I../Core/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H5xx/Include -I"D:/STM_Workspace_1.19/Final_RT_Edge_Anomaly/ThirdParty/FreeRTOS/include" -I"D:/STM_Workspace_1.19/Final_RT_Edge_Anomaly/ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure" -I"../Drivers/CMSIS/Device/ST/STM32H5xx/Include " -I../Drivers/CMSIS/Include -I../X-CUBE-AI/App -I../X-CUBE-AI -I../Middlewares/ST/AI/Inc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-X-2d-CUBE-2d-AI-2f-App

clean-X-2d-CUBE-2d-AI-2f-App:
	-$(RM) ./X-CUBE-AI/App/app_x-cube-ai.cyclo ./X-CUBE-AI/App/app_x-cube-ai.d ./X-CUBE-AI/App/app_x-cube-ai.o ./X-CUBE-AI/App/app_x-cube-ai.su ./X-CUBE-AI/App/ecg_network.cyclo ./X-CUBE-AI/App/ecg_network.d ./X-CUBE-AI/App/ecg_network.o ./X-CUBE-AI/App/ecg_network.su ./X-CUBE-AI/App/ecg_network_data.cyclo ./X-CUBE-AI/App/ecg_network_data.d ./X-CUBE-AI/App/ecg_network_data.o ./X-CUBE-AI/App/ecg_network_data.su ./X-CUBE-AI/App/ecg_network_data_params.cyclo ./X-CUBE-AI/App/ecg_network_data_params.d ./X-CUBE-AI/App/ecg_network_data_params.o ./X-CUBE-AI/App/ecg_network_data_params.su

.PHONY: clean-X-2d-CUBE-2d-AI-2f-App

