################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/dispatch.S \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/exc_entry.S \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/int_asm.S \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/reset_hdl.S \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/vector_tbl.S 

C_SRCS += \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/cpu_cntl.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/exc_hdl.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/interrupt.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/reset_main.c 

C_DEPS += \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/cpu_cntl.d \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/exc_hdl.d \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/interrupt.d \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/reset_main.d 

OBJS += \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/cpu_cntl.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/dispatch.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/exc_entry.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/exc_hdl.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/int_asm.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/interrupt.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/reset_hdl.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/reset_main.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/vector_tbl.o 

S_UPPER_DEPS += \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/dispatch.d \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/exc_entry.d \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/int_asm.d \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/reset_hdl.d \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/vector_tbl.d 


# Each subdirectory must supply rules for building sources it contributes
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/cpu_cntl.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/cpu_cntl.c mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/dispatch.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/dispatch.S mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m7 -DDEBUG -D_STM32CUBE_NUCLEO_H723_ -c -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/exc_entry.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/exc_entry.S mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m7 -DDEBUG -D_STM32CUBE_NUCLEO_H723_ -c -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/exc_hdl.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/exc_hdl.c mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/int_asm.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/int_asm.S mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m7 -DDEBUG -D_STM32CUBE_NUCLEO_H723_ -c -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/interrupt.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/interrupt.c mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/reset_hdl.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/reset_hdl.S mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m7 -DDEBUG -D_STM32CUBE_NUCLEO_H723_ -c -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/reset_main.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/reset_main.c mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/vector_tbl.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/vector_tbl.S mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m7 -DDEBUG -D_STM32CUBE_NUCLEO_H723_ -c -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"

clean: clean-mtk3_bsp2-2f-mtkernel-2f-kernel-2f-sysdepend-2f-cpu-2f-core-2f-armv7a

clean-mtk3_bsp2-2f-mtkernel-2f-kernel-2f-sysdepend-2f-cpu-2f-core-2f-armv7a:
	-$(RM) ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/cpu_cntl.cyclo ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/cpu_cntl.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/cpu_cntl.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/cpu_cntl.su ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/dispatch.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/dispatch.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/exc_entry.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/exc_entry.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/exc_hdl.cyclo ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/exc_hdl.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/exc_hdl.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/exc_hdl.su ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/int_asm.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/int_asm.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/interrupt.cyclo ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/interrupt.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/interrupt.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/interrupt.su ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/reset_hdl.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/reset_hdl.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/reset_main.cyclo ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/reset_main.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/reset_main.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/reset_main.su ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/vector_tbl.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/core/armv7a/vector_tbl.o

.PHONY: clean-mtk3_bsp2-2f-mtkernel-2f-kernel-2f-sysdepend-2f-cpu-2f-core-2f-armv7a

