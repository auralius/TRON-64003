################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/CUTE/cute_bridge.cpp \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/CUTE/cute_model_manager.cpp \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/CUTE/cute_postprocess.cpp \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/CUTE/cute_runner.cpp 

OBJS += \
./CUTE/cute_bridge.o \
./CUTE/cute_model_manager.o \
./CUTE/cute_postprocess.o \
./CUTE/cute_runner.o 

CPP_DEPS += \
./CUTE/cute_bridge.d \
./CUTE/cute_model_manager.d \
./CUTE/cute_postprocess.d \
./CUTE/cute_runner.d 


# Each subdirectory must supply rules for building sources it contributes
CUTE/cute_bridge.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/CUTE/cute_bridge.cpp CUTE/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -DNOODLE_USE_INT8 -DNOODLE_USE_NONE -DNOODLE_POOL_MODE=NOODLE_POOL_NONE -DNOODLE_BUFFER_ARENA_INITIAL_BYTES=131072u -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../Noodle/src -O3 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
CUTE/cute_model_manager.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/CUTE/cute_model_manager.cpp CUTE/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -DNOODLE_USE_INT8 -DNOODLE_USE_NONE -DNOODLE_POOL_MODE=NOODLE_POOL_NONE -DNOODLE_BUFFER_ARENA_INITIAL_BYTES=131072u -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../Noodle/src -O3 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
CUTE/cute_postprocess.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/CUTE/cute_postprocess.cpp CUTE/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -DNOODLE_USE_INT8 -DNOODLE_USE_NONE -DNOODLE_POOL_MODE=NOODLE_POOL_NONE -DNOODLE_BUFFER_ARENA_INITIAL_BYTES=131072u -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../Noodle/src -O3 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
CUTE/cute_runner.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/CUTE/cute_runner.cpp CUTE/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -DNOODLE_USE_INT8 -DNOODLE_USE_NONE -DNOODLE_POOL_MODE=NOODLE_POOL_NONE -DNOODLE_BUFFER_ARENA_INITIAL_BYTES=131072u -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../Noodle/src -O3 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-CUTE

clean-CUTE:
	-$(RM) ./CUTE/cute_bridge.cyclo ./CUTE/cute_bridge.d ./CUTE/cute_bridge.o ./CUTE/cute_bridge.su ./CUTE/cute_model_manager.cyclo ./CUTE/cute_model_manager.d ./CUTE/cute_model_manager.o ./CUTE/cute_model_manager.su ./CUTE/cute_postprocess.cyclo ./CUTE/cute_postprocess.d ./CUTE/cute_postprocess.o ./CUTE/cute_postprocess.su ./CUTE/cute_runner.cyclo ./CUTE/cute_runner.d ./CUTE/cute_runner.o ./CUTE/cute_runner.su

.PHONY: clean-CUTE

