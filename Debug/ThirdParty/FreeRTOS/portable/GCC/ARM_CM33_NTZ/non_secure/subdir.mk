################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/mpu_wrappers_v2_asm.c \
../ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/port.c \
../ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c 

C_DEPS += \
./ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/mpu_wrappers_v2_asm.d \
./ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/port.d \
./ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.d 

OBJS += \
./ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/mpu_wrappers_v2_asm.o \
./ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/port.o \
./ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.o 


# Each subdirectory must supply rules for building sources it contributes
ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/%.o ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/%.su ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/%.cyclo: ../ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/%.c ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H563xx -c -I../Core/Inc -I"D:/STM_Workspace_1.19/Final_RT_Edge_Anomaly/Middleware/ecg_anomaly_detector" -I"D:/STM_Workspace_1.19/Final_RT_Edge_Anomaly/Middleware/ecg_anomaly_detector/Inc" -I"D:/STM_Workspace_1.19/Final_RT_Edge_Anomaly/Middleware/ecg_anomaly_detector/Lib" -I../Drivers/STM32H5xx_HAL_Driver/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H5xx/Include -I"D:/STM_Workspace_1.19/Final_RT_Edge_Anomaly/ThirdParty/FreeRTOS/include" -I"D:/STM_Workspace_1.19/Final_RT_Edge_Anomaly/ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure" -I"../Drivers/CMSIS/Device/ST/STM32H5xx/Include " -I../Drivers/CMSIS/Include -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-ThirdParty-2f-FreeRTOS-2f-portable-2f-GCC-2f-ARM_CM33_NTZ-2f-non_secure

clean-ThirdParty-2f-FreeRTOS-2f-portable-2f-GCC-2f-ARM_CM33_NTZ-2f-non_secure:
	-$(RM) ./ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/mpu_wrappers_v2_asm.cyclo ./ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/mpu_wrappers_v2_asm.d ./ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/mpu_wrappers_v2_asm.o ./ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/mpu_wrappers_v2_asm.su ./ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/port.cyclo ./ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/port.d ./ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/port.o ./ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/port.su ./ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.cyclo ./ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.d ./ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.o ./ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.su

.PHONY: clean-ThirdParty-2f-FreeRTOS-2f-portable-2f-GCC-2f-ARM_CM33_NTZ-2f-non_secure

