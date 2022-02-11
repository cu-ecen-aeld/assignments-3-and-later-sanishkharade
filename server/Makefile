CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror -g
	
all: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) $(CFLAGS) aesdsocket.c -o aesdsocket
	
executible = aesdsocket
	
clean:
	rm -f $(executible) *.o
