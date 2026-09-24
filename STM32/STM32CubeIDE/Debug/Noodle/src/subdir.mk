################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_buffer.cpp \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_conv.cpp \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_dw.cpp \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_fcn.cpp \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_globals.cpp \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_int8_api.cpp \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_int8_layers.cpp \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_int8_math.cpp \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_internal.cpp \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_io.cpp \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_math.cpp \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_memory.cpp \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_shape.cpp \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_tensor.cpp 

OBJS += \
./Noodle/src/noodle_buffer.o \
./Noodle/src/noodle_conv.o \
./Noodle/src/noodle_dw.o \
./Noodle/src/noodle_fcn.o \
./Noodle/src/noodle_globals.o \
./Noodle/src/noodle_int8_api.o \
./Noodle/src/noodle_int8_layers.o \
./Noodle/src/noodle_int8_math.o \
./Noodle/src/noodle_internal.o \
./Noodle/src/noodle_io.o \
./Noodle/src/noodle_math.o \
./Noodle/src/noodle_memory.o \
./Noodle/src/noodle_shape.o \
./Noodle/src/noodle_tensor.o 

CPP_DEPS += \
./Noodle/src/noodle_buffer.d \
./Noodle/src/noodle_conv.d \
./Noodle/src/noodle_dw.d \
./Noodle/src/noodle_fcn.d \
./Noodle/src/noodle_globals.d \
./Noodle/src/noodle_int8_api.d \
./Noodle/src/noodle_int8_layers.d \
./Noodle/src/noodle_int8_math.d \
./Noodle/src/noodle_internal.d \
./Noodle/src/noodle_io.d \
./Noodle/src/noodle_math.d \
./Noodle/src/noodle_memory.d \
./Noodle/src/noodle_shape.d \
./Noodle/src/noodle_tensor.d 


# Each subdirectory must supply rules for building sources it contributes
Noodle/src/noodle_buffer.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_buffer.cpp Noodle/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -DNOODLE_USE_INT8 -DNOODLE_USE_NONE -DNOODLE_POOL_MODE=NOODLE_POOL_NONE -DNOODLE_BUFFER_ARENA_INITIAL_BYTES=131072u -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../Noodle/src -O3 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Noodle/src/noodle_conv.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_conv.cpp Noodle/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -DNOODLE_USE_INT8 -DNOODLE_USE_NONE -DNOODLE_POOL_MODE=NOODLE_POOL_NONE -DNOODLE_BUFFER_ARENA_INITIAL_BYTES=131072u -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../Noodle/src -O3 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Noodle/src/noodle_dw.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_dw.cpp Noodle/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -DNOODLE_USE_INT8 -DNOODLE_USE_NONE -DNOODLE_POOL_MODE=NOODLE_POOL_NONE -DNOODLE_BUFFER_ARENA_INITIAL_BYTES=131072u -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../Noodle/src -O3 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Noodle/src/noodle_fcn.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_fcn.cpp Noodle/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -DNOODLE_USE_INT8 -DNOODLE_USE_NONE -DNOODLE_POOL_MODE=NOODLE_POOL_NONE -DNOODLE_BUFFER_ARENA_INITIAL_BYTES=131072u -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../Noodle/src -O3 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Noodle/src/noodle_globals.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_globals.cpp Noodle/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -DNOODLE_USE_INT8 -DNOODLE_USE_NONE -DNOODLE_POOL_MODE=NOODLE_POOL_NONE -DNOODLE_BUFFER_ARENA_INITIAL_BYTES=131072u -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../Noodle/src -O3 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Noodle/src/noodle_int8_api.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_int8_api.cpp Noodle/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -DNOODLE_USE_INT8 -DNOODLE_USE_NONE -DNOODLE_POOL_MODE=NOODLE_POOL_NONE -DNOODLE_BUFFER_ARENA_INITIAL_BYTES=131072u -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../Noodle/src -O3 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Noodle/src/noodle_int8_layers.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_int8_layers.cpp Noodle/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -DNOODLE_USE_INT8 -DNOODLE_USE_NONE -DNOODLE_POOL_MODE=NOODLE_POOL_NONE -DNOODLE_BUFFER_ARENA_INITIAL_BYTES=131072u -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../Noodle/src -O3 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Noodle/src/noodle_int8_math.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_int8_math.cpp Noodle/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -DNOODLE_USE_INT8 -DNOODLE_USE_NONE -DNOODLE_POOL_MODE=NOODLE_POOL_NONE -DNOODLE_BUFFER_ARENA_INITIAL_BYTES=131072u -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../Noodle/src -O3 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Noodle/src/noodle_internal.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_internal.cpp Noodle/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -DNOODLE_USE_INT8 -DNOODLE_USE_NONE -DNOODLE_POOL_MODE=NOODLE_POOL_NONE -DNOODLE_BUFFER_ARENA_INITIAL_BYTES=131072u -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../Noodle/src -O3 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Noodle/src/noodle_io.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_io.cpp Noodle/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -DNOODLE_USE_INT8 -DNOODLE_USE_NONE -DNOODLE_POOL_MODE=NOODLE_POOL_NONE -DNOODLE_BUFFER_ARENA_INITIAL_BYTES=131072u -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../Noodle/src -O3 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Noodle/src/noodle_math.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_math.cpp Noodle/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -DNOODLE_USE_INT8 -DNOODLE_USE_NONE -DNOODLE_POOL_MODE=NOODLE_POOL_NONE -DNOODLE_BUFFER_ARENA_INITIAL_BYTES=131072u -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../Noodle/src -O3 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Noodle/src/noodle_memory.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_memory.cpp Noodle/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -DNOODLE_USE_INT8 -DNOODLE_USE_NONE -DNOODLE_POOL_MODE=NOODLE_POOL_NONE -DNOODLE_BUFFER_ARENA_INITIAL_BYTES=131072u -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../Noodle/src -O3 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Noodle/src/noodle_shape.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_shape.cpp Noodle/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -DNOODLE_USE_INT8 -DNOODLE_USE_NONE -DNOODLE_POOL_MODE=NOODLE_POOL_NONE -DNOODLE_BUFFER_ARENA_INITIAL_BYTES=131072u -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../Noodle/src -O3 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Noodle/src/noodle_tensor.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Noodle/src/noodle_tensor.cpp Noodle/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -DNOODLE_USE_INT8 -DNOODLE_USE_NONE -DNOODLE_POOL_MODE=NOODLE_POOL_NONE -DNOODLE_BUFFER_ARENA_INITIAL_BYTES=131072u -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../Noodle/src -O3 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Noodle-2f-src

clean-Noodle-2f-src:
	-$(RM) ./Noodle/src/noodle_buffer.cyclo ./Noodle/src/noodle_buffer.d ./Noodle/src/noodle_buffer.o ./Noodle/src/noodle_buffer.su ./Noodle/src/noodle_conv.cyclo ./Noodle/src/noodle_conv.d ./Noodle/src/noodle_conv.o ./Noodle/src/noodle_conv.su ./Noodle/src/noodle_dw.cyclo ./Noodle/src/noodle_dw.d ./Noodle/src/noodle_dw.o ./Noodle/src/noodle_dw.su ./Noodle/src/noodle_fcn.cyclo ./Noodle/src/noodle_fcn.d ./Noodle/src/noodle_fcn.o ./Noodle/src/noodle_fcn.su ./Noodle/src/noodle_globals.cyclo ./Noodle/src/noodle_globals.d ./Noodle/src/noodle_globals.o ./Noodle/src/noodle_globals.su ./Noodle/src/noodle_int8_api.cyclo ./Noodle/src/noodle_int8_api.d ./Noodle/src/noodle_int8_api.o ./Noodle/src/noodle_int8_api.su ./Noodle/src/noodle_int8_layers.cyclo ./Noodle/src/noodle_int8_layers.d ./Noodle/src/noodle_int8_layers.o ./Noodle/src/noodle_int8_layers.su ./Noodle/src/noodle_int8_math.cyclo ./Noodle/src/noodle_int8_math.d ./Noodle/src/noodle_int8_math.o ./Noodle/src/noodle_int8_math.su ./Noodle/src/noodle_internal.cyclo ./Noodle/src/noodle_internal.d ./Noodle/src/noodle_internal.o ./Noodle/src/noodle_internal.su ./Noodle/src/noodle_io.cyclo ./Noodle/src/noodle_io.d ./Noodle/src/noodle_io.o ./Noodle/src/noodle_io.su ./Noodle/src/noodle_math.cyclo ./Noodle/src/noodle_math.d ./Noodle/src/noodle_math.o ./Noodle/src/noodle_math.su ./Noodle/src/noodle_memory.cyclo ./Noodle/src/noodle_memory.d ./Noodle/src/noodle_memory.o ./Noodle/src/noodle_memory.su ./Noodle/src/noodle_shape.cyclo ./Noodle/src/noodle_shape.d ./Noodle/src/noodle_shape.o ./Noodle/src/noodle_shape.su ./Noodle/src/noodle_tensor.cyclo ./Noodle/src/noodle_tensor.d ./Noodle/src/noodle_tensor.o ./Noodle/src/noodle_tensor.su

.PHONY: clean-Noodle-2f-src

