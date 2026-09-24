################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/dispatch.S 

C_SRCS += \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/cpu_cntl.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/interrupt.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/sys_start.c 

C_DEPS += \
./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/cpu_cntl.d \
./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/interrupt.d \
./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/sys_start.d 

OBJS += \
./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/cpu_cntl.o \
./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/dispatch.o \
./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/interrupt.o \
./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/sys_start.o 

S_UPPER_DEPS += \
./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/dispatch.d 


# Each subdirectory must supply rules for building sources it contributes
mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/cpu_cntl.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/cpu_cntl.c mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/dispatch.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/dispatch.S mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m7 -DDEBUG -D_STM32CUBE_NUCLEO_H723_ -c -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"
mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/interrupt.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/interrupt.c mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/sys_start.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/sys_start.c mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-mtk3_bsp2-2f-sysdepend-2f-nxp_mcux-2f-cpu-2f-core-2f-armv8m

clean-mtk3_bsp2-2f-sysdepend-2f-nxp_mcux-2f-cpu-2f-core-2f-armv8m:
	-$(RM) ./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/cpu_cntl.cyclo ./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/cpu_cntl.d ./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/cpu_cntl.o ./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/cpu_cntl.su ./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/dispatch.d ./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/dispatch.o ./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/interrupt.cyclo ./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/interrupt.d ./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/interrupt.o ./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/interrupt.su ./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/sys_start.cyclo ./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/sys_start.d ./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/sys_start.o ./mtk3_bsp2/sysdepend/nxp_mcux/cpu/core/armv8m/sys_start.su

.PHONY: clean-mtk3_bsp2-2f-sysdepend-2f-nxp_mcux-2f-cpu-2f-core-2f-armv8m

