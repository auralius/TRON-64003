################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/sysdepend/stm32_cube/lib/libtm/nucleo_stm32f411/tm_com.c 

C_DEPS += \
./mtk3_bsp2/sysdepend/stm32_cube/lib/libtm/nucleo_stm32f411/tm_com.d 

OBJS += \
./mtk3_bsp2/sysdepend/stm32_cube/lib/libtm/nucleo_stm32f411/tm_com.o 


# Each subdirectory must supply rules for building sources it contributes
mtk3_bsp2/sysdepend/stm32_cube/lib/libtm/nucleo_stm32f411/tm_com.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/sysdepend/stm32_cube/lib/libtm/nucleo_stm32f411/tm_com.c mtk3_bsp2/sysdepend/stm32_cube/lib/libtm/nucleo_stm32f411/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-mtk3_bsp2-2f-sysdepend-2f-stm32_cube-2f-lib-2f-libtm-2f-nucleo_stm32f411

clean-mtk3_bsp2-2f-sysdepend-2f-stm32_cube-2f-lib-2f-libtm-2f-nucleo_stm32f411:
	-$(RM) ./mtk3_bsp2/sysdepend/stm32_cube/lib/libtm/nucleo_stm32f411/tm_com.cyclo ./mtk3_bsp2/sysdepend/stm32_cube/lib/libtm/nucleo_stm32f411/tm_com.d ./mtk3_bsp2/sysdepend/stm32_cube/lib/libtm/nucleo_stm32f411/tm_com.o ./mtk3_bsp2/sysdepend/stm32_cube/lib/libtm/nucleo_stm32f411/tm_com.su

.PHONY: clean-mtk3_bsp2-2f-sysdepend-2f-stm32_cube-2f-lib-2f-libtm-2f-nucleo_stm32f411

