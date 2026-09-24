################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/Camera/camera.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/Camera/ov2640.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/Camera/ov2640_regs.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/Camera/ov5640.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/Camera/ov5640_regs.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/Camera/ov7670.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/Camera/ov7670_regs.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/Camera/ov7725.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/Camera/ov7725_regs.c 

C_DEPS += \
./BSP/Camera/camera.d \
./BSP/Camera/ov2640.d \
./BSP/Camera/ov2640_regs.d \
./BSP/Camera/ov5640.d \
./BSP/Camera/ov5640_regs.d \
./BSP/Camera/ov7670.d \
./BSP/Camera/ov7670_regs.d \
./BSP/Camera/ov7725.d \
./BSP/Camera/ov7725_regs.d 

OBJS += \
./BSP/Camera/camera.o \
./BSP/Camera/ov2640.o \
./BSP/Camera/ov2640_regs.o \
./BSP/Camera/ov5640.o \
./BSP/Camera/ov5640_regs.o \
./BSP/Camera/ov7670.o \
./BSP/Camera/ov7670_regs.o \
./BSP/Camera/ov7725.o \
./BSP/Camera/ov7725_regs.o 


# Each subdirectory must supply rules for building sources it contributes
BSP/Camera/camera.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/Camera/camera.c BSP/Camera/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
BSP/Camera/ov2640.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/Camera/ov2640.c BSP/Camera/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
BSP/Camera/ov2640_regs.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/Camera/ov2640_regs.c BSP/Camera/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
BSP/Camera/ov5640.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/Camera/ov5640.c BSP/Camera/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
BSP/Camera/ov5640_regs.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/Camera/ov5640_regs.c BSP/Camera/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
BSP/Camera/ov7670.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/Camera/ov7670.c BSP/Camera/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
BSP/Camera/ov7670_regs.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/Camera/ov7670_regs.c BSP/Camera/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
BSP/Camera/ov7725.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/Camera/ov7725.c BSP/Camera/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
BSP/Camera/ov7725_regs.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/Camera/ov7725_regs.c BSP/Camera/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-BSP-2f-Camera

clean-BSP-2f-Camera:
	-$(RM) ./BSP/Camera/camera.cyclo ./BSP/Camera/camera.d ./BSP/Camera/camera.o ./BSP/Camera/camera.su ./BSP/Camera/ov2640.cyclo ./BSP/Camera/ov2640.d ./BSP/Camera/ov2640.o ./BSP/Camera/ov2640.su ./BSP/Camera/ov2640_regs.cyclo ./BSP/Camera/ov2640_regs.d ./BSP/Camera/ov2640_regs.o ./BSP/Camera/ov2640_regs.su ./BSP/Camera/ov5640.cyclo ./BSP/Camera/ov5640.d ./BSP/Camera/ov5640.o ./BSP/Camera/ov5640.su ./BSP/Camera/ov5640_regs.cyclo ./BSP/Camera/ov5640_regs.d ./BSP/Camera/ov5640_regs.o ./BSP/Camera/ov5640_regs.su ./BSP/Camera/ov7670.cyclo ./BSP/Camera/ov7670.d ./BSP/Camera/ov7670.o ./BSP/Camera/ov7670.su ./BSP/Camera/ov7670_regs.cyclo ./BSP/Camera/ov7670_regs.d ./BSP/Camera/ov7670_regs.o ./BSP/Camera/ov7670_regs.su ./BSP/Camera/ov7725.cyclo ./BSP/Camera/ov7725.d ./BSP/Camera/ov7725.o ./BSP/Camera/ov7725.su ./BSP/Camera/ov7725_regs.cyclo ./BSP/Camera/ov7725_regs.d ./BSP/Camera/ov7725_regs.o ./BSP/Camera/ov7725_regs.su

.PHONY: clean-BSP-2f-Camera

