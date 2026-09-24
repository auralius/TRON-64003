################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/usermain/usermain.c 

C_DEPS += \
./mtk3_bsp2/mtkernel/kernel/usermain/usermain.d 

OBJS += \
./mtk3_bsp2/mtkernel/kernel/usermain/usermain.o 


# Each subdirectory must supply rules for building sources it contributes
mtk3_bsp2/mtkernel/kernel/usermain/usermain.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/usermain/usermain.c mtk3_bsp2/mtkernel/kernel/usermain/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-mtk3_bsp2-2f-mtkernel-2f-kernel-2f-usermain

clean-mtk3_bsp2-2f-mtkernel-2f-kernel-2f-usermain:
	-$(RM) ./mtk3_bsp2/mtkernel/kernel/usermain/usermain.cyclo ./mtk3_bsp2/mtkernel/kernel/usermain/usermain.d ./mtk3_bsp2/mtkernel/kernel/usermain/usermain.o ./mtk3_bsp2/mtkernel/kernel/usermain/usermain.su

.PHONY: clean-mtk3_bsp2-2f-mtkernel-2f-kernel-2f-usermain

