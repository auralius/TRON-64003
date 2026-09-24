################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/hllint_ent.S 

C_SRCS += \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/group_int.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/hllint_tbl.c \
/home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/intvect_tbl.c 

C_DEPS += \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/group_int.d \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/hllint_tbl.d \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/intvect_tbl.d 

OBJS += \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/group_int.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/hllint_ent.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/hllint_tbl.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/intvect_tbl.o 

S_UPPER_DEPS += \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/hllint_ent.d 


# Each subdirectory must supply rules for building sources it contributes
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/group_int.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/group_int.c mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/hllint_ent.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/hllint_ent.S mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m7 -DDEBUG -D_STM32CUBE_NUCLEO_H723_ -c -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/hllint_tbl.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/hllint_tbl.c mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/intvect_tbl.o: /home/auralius/works/STM32H743/08-DCMI2LCD_CubeIDE/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/intvect_tbl.c mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -DDEBUG -DTFT96 -DUSE_HAL_DRIVER -DSTM32H743xx -D_STM32CUBE_NUCLEO_H723_ -c -I../../Inc -I../../Drivers/BSP/Camera -I../../Drivers/BSP/ST7789 -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../CUTE -I../../mtk3_bsp2 -I../../mtk3_bsp2/config -I../../mtk3_bsp2/include -I../../mtk3_bsp2/mtkernel/kernel/knlinc -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-mtk3_bsp2-2f-mtkernel-2f-kernel-2f-sysdepend-2f-cpu-2f-rx65n

clean-mtk3_bsp2-2f-mtkernel-2f-kernel-2f-sysdepend-2f-cpu-2f-rx65n:
	-$(RM) ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/group_int.cyclo ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/group_int.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/group_int.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/group_int.su ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/hllint_ent.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/hllint_ent.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/hllint_tbl.cyclo ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/hllint_tbl.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/hllint_tbl.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/hllint_tbl.su ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/intvect_tbl.cyclo ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/intvect_tbl.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/intvect_tbl.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx65n/intvect_tbl.su

.PHONY: clean-mtk3_bsp2-2f-mtkernel-2f-kernel-2f-sysdepend-2f-cpu-2f-rx65n

