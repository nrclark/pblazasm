
## defines

RM = "rm"
MP = "C:/MinGW32/include"
CC = "gcc"

C_SRCS += \
./pBlazASM.c \
./pbLexer.c \
./pbParser.c \
./pbSymbols.c \
../common/dbuf.c \
../common/dbuf_string.c \
../common/pbLibgen.c

#../common/getopt.c

EXE = "./pBlazASM.exe"
OBJS = $(C_SRCS:.c=.o)
LIBS = 
INCS = -I$(MP) -I../common
CFLAGS = $(INCS) -g3 -DWINDOWS -Wall -c -O3 -DHAVE_STRING_H

## rules

# All Target
all:	$(EXE) test

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
