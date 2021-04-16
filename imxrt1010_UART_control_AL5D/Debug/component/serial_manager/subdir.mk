################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../component/serial_manager/serial_manager.c \
../component/serial_manager/serial_port_uart.c 

OBJS += \
./component/serial_manager/serial_manager.o \
./component/serial_manager/serial_port_uart.o 

C_DEPS += \
./component/serial_manager/serial_manager.d \
./component/serial_manager/serial_port_uart.d 


# Each subdirectory must supply rules for building sources it contributes
component/serial_manager/%.o: ../component/serial_manager/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MIMXRT1011DAE5A -DCPU_MIMXRT1011DAE5A_cm7 -DSDK_DEBUGCONSOLE=1 -DXIP_EXTERNAL_FLASH=1 -DXIP_BOOT_HEADER_ENABLE=1 -DSERIAL_PORT_TYPE_UART=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Users\garret\Documents\IOT\git_lib_EGA\imxrt1010_uart_robotic_arm\board" -I"C:\Users\garret\Documents\IOT\git_lib_EGA\imxrt1010_uart_robotic_arm\source" -I"C:\Users\garret\Documents\IOT\git_lib_EGA\imxrt1010_uart_robotic_arm" -I"C:\Users\garret\Documents\IOT\git_lib_EGA\imxrt1010_uart_robotic_arm\drivers" -I"C:\Users\garret\Documents\IOT\git_lib_EGA\imxrt1010_uart_robotic_arm\device" -I"C:\Users\garret\Documents\IOT\git_lib_EGA\imxrt1010_uart_robotic_arm\CMSIS" -I"C:\Users\garret\Documents\IOT\git_lib_EGA\imxrt1010_uart_robotic_arm\utilities" -I"C:\Users\garret\Documents\IOT\git_lib_EGA\imxrt1010_uart_robotic_arm\component\serial_manager" -I"C:\Users\garret\Documents\IOT\git_lib_EGA\imxrt1010_uart_robotic_arm\component\lists" -I"C:\Users\garret\Documents\IOT\git_lib_EGA\imxrt1010_uart_robotic_arm\component\uart" -I"C:\Users\garret\Documents\IOT\git_lib_EGA\imxrt1010_uart_robotic_arm\xip" -O0 -fno-common -g3 -Wall -c  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m7 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


