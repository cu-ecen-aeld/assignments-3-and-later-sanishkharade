CROSS_COMPILE = 
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror -g -pthread

#linker flags
LDFLAGS = -pthread -lrt 

# Source files
SRCS = aesdsocket.c

# Object files
OBJS = $(SRCS:.c=.o)

TARGET = aesdsocket

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@


.PHONY: clean	

clean:
	rm -f $(TARGET) *.o
