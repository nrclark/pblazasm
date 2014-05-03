
## defines

RM = "rm"
MP = "C:/MinGW32/include"
CC = "g++"

C_SRCS += \
./pBlazSIMcl.cpp \
./pBlaze.cpp

EXE = "./pBlazSIMcl.exe"
OBJS = $(C_SRCS:.cpp=.o)
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
	
%.o: %.cpp %.h
	@echo Building: "$@"
	$(CC) $(CFLAGS) -o"$@" "$<"

# Other Targets
clean:
	$(RM) $(OBJS)
	$(RM) $(EXE)

.PHONY: all clean test
.SECONDARY:
