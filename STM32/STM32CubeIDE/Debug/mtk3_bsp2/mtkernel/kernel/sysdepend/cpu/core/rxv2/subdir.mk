################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/dispatch.S \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/int_asm.S \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/reset_hdl.S 

C_SRCS += \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/cpu_cntl.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/exc_hdr.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/interrupt.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/reset_main.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/vector_tbl.c 

C_DEPS += \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/cpu_cntl.d \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/exc_hdr.d \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/interrupt.d \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/reset_main.d \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/vector_tbl.d 

OBJS += \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/cpu_cntl.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/dispatch.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/exc_hdr.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/int_asm.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/interrupt.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/reset_hdl.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/reset_main.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/vector_tbl.o 

S_UPPER_DEPS += \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/dispatch.d \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/int_asm.d \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/reset_hdl.d 


# Each subdirectory must supply rules for building sources it contributes
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/cpu_cntl.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/cpu_cntl.c mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/dispatch.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/dispatch.S mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m7 -DDEBUG -D_STM32CUBE_NUCLEO_H723_ -c -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/exc_hdr.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/exc_hdr.c mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/int_asm.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/int_asm.S mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m7 -DDEBUG -D_STM32CUBE_NUCLEO_H723_ -c -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/interrupt.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/interrupt.c mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/reset_hdl.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/reset_hdl.S mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m7 -DDEBUG -D_STM32CUBE_NUCLEO_H723_ -c -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/reset_main.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/reset_main.c mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/vector_tbl.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/vector_tbl.c mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-mtk3_bsp2-2f-mtkernel-2f-kernel-2f-sysdepend-2f-cpu-2f-core-2f-rxv2

clean-mtk3_bsp2-2f-mtkernel-2f-kernel-2f-sysdepend-2f-cpu-2f-core-2f-rxv2:
	-$(RM) ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/cpu_cntl.cyclo ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/cpu_cntl.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/cpu_cntl.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/cpu_cntl.su ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/dispatch.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/dispatch.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/exc_hdr.cyclo ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/exc_hdr.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/exc_hdr.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/exc_hdr.su ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/int_asm.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/int_asm.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/interrupt.cyclo ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/interrupt.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/interrupt.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/interrupt.su ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/reset_hdl.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/reset_hdl.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/reset_main.cyclo ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/reset_main.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/reset_main.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/reset_main.su ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/vector_tbl.cyclo ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/vector_tbl.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/vector_tbl.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/rxv2/vector_tbl.su

.PHONY: clean-mtk3_bsp2-2f-mtkernel-2f-kernel-2f-sysdepend-2f-cpu-2f-core-2f-rxv2

