#CROSS_COMPILE = 

#CC = $(CROSS_COMPILE)gcc

ifeq ($(CC),)
	CC= $(CROSS_COMPILE)gcc
endif


ifeq ($(CFLAGS),)
	CFLAGS= -g -Wall -Werror
endif

#CFLAGS = -Wall -Werror -g -pthread

#linker flags
ifeq ($(LDFLAGS),)
	LDFLAGS= -pthread -lrt
endif

#LDFLAGS = -pthread -lrt 

# Source files
SRCS = aesdsocket.c

# Object files
OBJS = $(SRCS:.c=.o)

TARGET = aesdsocket

all: $(TARGET)
default: $(TARGET)


$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@


.PHONY: clean	

clean:
	rm -f $(TARGET) *.o *.elf *.map
