#
# makefile for compiling for Windoze using MinGW w/o eclipse
# 16-jul-09	hvk	initial
# 24-jul-09	hvk	fixed echo
# 13-apr-10	hvk	using 'MinGW/gcc' of Qt and 'rm' of Xilinx/cygwin
#

## defines

RM = rm
MP = "C:/Qt/2010.04/mingw/include"
CC = "C:/Qt/2010.04/mingw/bin/gcc"


C_SRCS += \
./pBlazASM.c \
./pbLexer.c \
./pbLibgen.c \
./pbParser.c \
./pbSymbols.c 

EXE = "./pBlazASM.exe"
OBJS = $(C_SRCS:.c=.o)
LIBS = 
INCS = -I$(MP) -I"../inc"
#CFLAGS = $(INCS) -g3 -DWINDOWS -Wall -std=c99 -c -O3
CFLAGS = $(INCS) -g3 -DWINDOWS -Wall -c -O3

## rules

# All Target
all:	$(EXE)

# Tool invocations
$(EXE):	$(OBJS)
	@echo Building: $(EXE)
	$(CC) -o $@ $^ $(LIBS)
	
%.o: %.c %.h
	@echo Building: "$@"
	$(CC) $(CFLAGS) -o"$@" "$<"

# Other Targets
clean:
	$(RM) $(EXE)
	$(RM) $(OBJS)

.PHONY: all clean
.SECONDARY:
