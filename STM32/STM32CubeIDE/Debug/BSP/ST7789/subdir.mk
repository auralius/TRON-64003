################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/ST7789/lcd.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/ST7789/st7789.c 

C_DEPS += \
./BSP/ST7789/lcd.d \
./BSP/ST7789/st7789.d 

OBJS += \
./BSP/ST7789/lcd.o \
./BSP/ST7789/st7789.o 


# Each subdirectory must supply rules for building sources it contributes
BSP/ST7789/lcd.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/ST7789/lcd.c BSP/ST7789/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
BSP/ST7789/st7789.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/Drivers/BSP/ST7789/st7789.c BSP/ST7789/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-BSP-2f-ST7789

clean-BSP-2f-ST7789:
	-$(RM) ./BSP/ST7789/lcd.cyclo ./BSP/ST7789/lcd.d ./BSP/ST7789/lcd.o ./BSP/ST7789/lcd.su ./BSP/ST7789/st7789.cyclo ./BSP/ST7789/st7789.d ./BSP/ST7789/st7789.o ./BSP/ST7789/st7789.su

.PHONY: clean-BSP-2f-ST7789

