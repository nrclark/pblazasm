
## defines

RM = "rm"
MP = "C:/MinGW32/include"
CC = "gcc"

C_SRCS += \
./pBlazASM.c \
./pbLexer.c \
./pbLibgen.c \
./pbParser.c \
./pbSymbols.c

#./getopt.c

EXE = "./pBlazASM.exe"
OBJS = $(C_SRCS:.c=.o)
LIBS = 
INCS = -I$(MP)
CFLAGS = $(INCS) -g3 -DWINDOWS -Wall -c -O3 -DHAVE_STRING_H

## rules

# All Target
all:	test

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
