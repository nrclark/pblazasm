#
# makefile for compiling for Windoze using MinGW w/o eclipse
# 16-jul-09	hvk	initial
# 24-jul-09	hvk	fixed echo
# 13-apr-10	hvk	using 'MinGW/gcc' of Qt and 'rm' of Xilinx/cygwin
# 15-oct-10	hvk	using 'tcc' of http://bellard.org/tcc/ and 'rm' of gnu fileutils
#

## defines

RM = "C:\MinGW\msys\1.0\bin\rm"
#MP = "C:/tcc/inc"
#CC = "C:/tcc/tcc.exe"
MP = "C:/MinGW/include"
CC = "C:/MinGW/bin/gcc"

C_SRCS += \
./pBlazASM.c \
./pbLexer.c \
./pbLibgen.c \
./pbParser.c \
./pbSymbols.c \
./getopt.c

EXE = "./pBlzASM6.exe"
OBJS = $(C_SRCS:.c=.o)
LIBS = 
INCS = -I$(MP)
CFLAGS = $(INCS) -g3 -DWINDOWS -Wall -c -O3 -DHAVE_STRING_H

## rules

# All Target
all:	test

test:	$(EXE)
#	./pBlzASM6.exe -l -m../pBlazMRG/test.mem -s../pBlazMRG/test_data.mem pBlzASM6_Syntax.psm

# Tool invocations
$(EXE):	$(OBJS)
	@echo Building: $(EXE)
	$(CC) -o $@ $^ $(LIBS)
	
%.o: %.c %.h
	@echo Building: "$@"
	$(CC) $(CFLAGS) -o"$@" "$<"

# Other Targets
clean:
	$(RM) $(OBJS)
	$(RM) $(EXE)

.PHONY: all clean test
.SECONDARY:
