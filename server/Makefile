CROSS_COMPILE = 
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror -g -pthread
	
all: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) $(CFLAGS) aesdsocket.c -o aesdsocket
	
executible = aesdsocket
	
clean:
	rm -f $(executible) *.o
