#
# makefile for compiling for Windoze using MinGW w/o eclipse
# 16-jul-09	hvk	initial
# 24-jul-09	hvk	fixed echo
# 15-oct-10	hvk	using ''tcc' of http://bellard.org/tcc/ and 'rm' of Xilinx/cygwin
#

## defines

RM = rm
MP = "C:/tcc/inc"
CC = "C:/tcc/tcc"
 
C_SRCS += \
./pBlazMRG.c \
./pbLibgen.c \
./getopt.c

EXE = "./pBlazMRG.exe"
OBJS = $(C_SRCS:.c=.o)
LIBS = 
INCS = -I$(MP)"/include" -I"./inc"
CFLAGS = $(INCS) -g3 -DWINDOWS -Wall -c -O3 -DTCC -DHAVE_STRING_H

MGFLAGS =
TEMPLATE = template
ENTITY = test

## rules

# All Target
all:	test

test:	$(EXE)
	$(EXE) $(MGFLAGS) -e$(ENTITY) $(ENTITY).mem $(TEMPLATE).vhd $(ENTITY).vhd 

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
