#
# makefile for compiling for Windoze using MinGW w/o eclipse
# 16-jul-09	hvk	initial
# 24-jul-09	hvk	fixed echo
#

## defines

RM = rm
MP = "C:/tcc/tcc.exe"
CC = "C:/tcc/tcc.exe"
 
C_SRCS += \
./pBlazDIS.c \
./pbLibgen.c \
./getopt.c

EXE = "./pBlazDIS.exe"
OBJS = $(C_SRCS:.c=.o)
LIBS = 
INCS = -I$(MP)"/include" -I"./inc"
CFLAGS = $(INCS) -g3 -DWINDOWS -Wall -c -O3 -DTCC -DHAVE_STRING_H

## rules

# All Target
all:	test

test:	$(EXE)
	$(EXE) ..\\\pBlazASM\\\pBlazASM_Syntax.mem

# Tool invocations
$(EXE):	$(OBJS)
	@echo Building: $(EXE)
	$(CC) -o $@ $^ $(LIBS)
	
%.o : %.c
	@echo Building: "$@"
	$(CC) $(CFLAGS) -o"$@" "$<"

# Other Targets
clean:
	-@echo Clean up: $(OBJS) $(EXE)
	-$(RM) $(OBJS) $(EXE)

.PHONY: all clean
.SECONDARY:
