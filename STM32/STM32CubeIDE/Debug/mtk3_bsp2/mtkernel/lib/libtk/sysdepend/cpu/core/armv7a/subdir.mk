################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/int_armv7a.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/wusec_armv7a.c 

C_DEPS += \
./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/int_armv7a.d \
./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/wusec_armv7a.d 

OBJS += \
./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/int_armv7a.o \
./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/wusec_armv7a.o 


# Each subdirectory must supply rules for building sources it contributes
mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/int_armv7a.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/int_armv7a.c mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/wusec_armv7a.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/wusec_armv7a.c mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-mtk3_bsp2-2f-mtkernel-2f-lib-2f-libtk-2f-sysdepend-2f-cpu-2f-core-2f-armv7a

clean-mtk3_bsp2-2f-mtkernel-2f-lib-2f-libtk-2f-sysdepend-2f-cpu-2f-core-2f-armv7a:
	-$(RM) ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/int_armv7a.cyclo ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/int_armv7a.d ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/int_armv7a.o ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/int_armv7a.su ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/wusec_armv7a.cyclo ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/wusec_armv7a.d ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/wusec_armv7a.o ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/armv7a/wusec_armv7a.su

.PHONY: clean-mtk3_bsp2-2f-mtkernel-2f-lib-2f-libtk-2f-sysdepend-2f-cpu-2f-core-2f-armv7a

