################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../pBlazASM.c \
../pbLexer.c \
../pbLibgen.c \
../pbParser.c \
../pbSymbols.c 

OBJS += \
./pBlazASM.o \
./pbLexer.o \
./pbLibgen.o \
./pbParser.o \
./pbSymbols.o 

C_DEPS += \
./pBlazASM.d \
./pbLexer.d \
./pbLibgen.d \
./pbParser.d \
./pbSymbols.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


