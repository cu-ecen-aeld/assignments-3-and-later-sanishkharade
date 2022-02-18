//#define _GNU_SOURCE 
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h> 	// for close socket
#include <sys/stat.h>
#include <fcntl.h>  	// for open
#include <linux/fs.h>	// for daemon
#include <signal.h>
#include <stdbool.h>

#include <syslog.h>		// for syslog

#define PORT "9000"
#define MAX_CONNECTIONS 10


int sockfd;
int clientfd;
char filepath[50] = "/var/tmp/aesdsocketdata";

// use power of 2 -> 100, 1000 didn't work
#define RECV_SIZE 128


static void signal_handler (int signo)
{
	// Gracefully exit when a SIGINT or SIGTERM is received
	if (signo == SIGINT)
	{
		syslog(LOG_INFO, "SIGINT received\n");
		
		close(sockfd);
		close(clientfd);
		unlink(filepath);
	}
	else if (signo == SIGTERM)
	{
		syslog(LOG_INFO, "SIGTERM received\n");
		
		close(sockfd);
		close(clientfd);
		unlink(filepath);
	}
	else
	{
		/* this should never happen */
		syslog(LOG_ERR, "Unknown Signal received\n");
		exit (EXIT_FAILURE);
	}
	exit (EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{	

	// Open the syslog for logging data
	//openlog("AESD Socket Application", LOG_CONS|LOG_PERROR|LOG_PID, LOG_USER);
	openlog("AESD Socket Application", LOG_PID, LOG_USER);

	// Status variable will store the return values of functions for error checking
	int status;

	// Create structures to get the value of addrinfo
	struct addrinfo hints;
	struct addrinfo *result, *rp;

	/*
	* Register signal_handler as our signal handler
	* for SIGINT and SIGTERM.
	*/
	if (signal (SIGINT, signal_handler) == SIG_ERR)
	{
		syslog(LOG_ERR, "Could not register SIGINT handler\n");
		exit (EXIT_FAILURE);
	}

	if (signal (SIGTERM, signal_handler) == SIG_ERR)
	{
		syslog(LOG_ERR, "Could not register SIGTERM handler\n");
		exit (EXIT_FAILURE);
	}

	printf("argc = %d\n", argc);
    if ((argc >= 2) && (strcmp("-d", argv[1]) == 0))
    {
    	syslog(LOG_DEBUG, "Running aesdsocket as a Daemon\n");
       	printf("Daemon\n");
		daemon(0,0);
    }
	//add daemon
	// if(argc > 1)
	// {
	// 	printf("Daemon\n");
	// 	daemon(0,0);
	// }


	// Set the values for members of the hints structure
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	status = getaddrinfo(NULL, PORT, &hints, &result);
	if(status != 0)
	{
		syslog(LOG_ERR, "ERROR: getaddrinfo()\n");
		printf("Error in getaddrinfo");
		exit(EXIT_FAILURE);
	}

	/* getaddrinfo() returns a list of address structures.
	  Try each address until we successfully bind.
	  If socket (or bind) fails, we (close the socket
	  and) try the next address.
	  Ref -  man page of getaddrinfo
	 */
	syslog(LOG_DEBUG, "Attempting to Bind\n");
	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sockfd = socket(AF_INET6, SOCK_STREAM, 0);
		if (sockfd == -1)
		{
			/* socket() failed */
			syslog(LOG_ERR, "ERROR: socket()\n");
			continue;
		}
		   
		if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
		{
			/* Successfully bound to socket */
			printf("rp->ai_addr = %p --- passed\n", rp->ai_addr);
			printf("sockfd = %d --- passed\n", sockfd);
			syslog(LOG_DEBUG, "Successfully Bound to socket\n");
			break;                  
		}
		   
		syslog(LOG_DEBUG, "FAILED: bind()\n");
		printf("rp->ai_addr = %p --- failed\n", rp->ai_addr);
		printf("sockfd = %d --- failed\n", sockfd);
		close(sockfd);
	}

	freeaddrinfo(result);           /* No longer needed */

	if (rp == NULL)
	{  	
		/* No address succeeded */
		// Socket is already closed so just exit
	   	syslog(LOG_ERR, "ERROR: bind() - No address succeeded\n");
	   	exit(EXIT_FAILURE);
	}

	//char filepath[50] = "/var/tmp/aesdsocketdata";

	// Listen for connections
	syslog(LOG_DEBUG, "Listening for connections on the socket\n");
	status = listen(sockfd, MAX_CONNECTIONS);
	if(status == -1)
	{
		// listen() failed
		syslog(LOG_ERR, "ERROR: listen()\n");
		printf("Listen failed\n");
		
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	syslog(LOG_DEBUG, "Listen Passed\n");
	
	char recv_buffer[RECV_SIZE] = {0};
	int total_data_size = 0;
	ssize_t nread = 0;

	while(1)
	{
		socklen_t sock_addr_size;
		struct sockaddr client_addr;
		sock_addr_size = sizeof(struct sockaddr);

		// Accept the connection
		clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &sock_addr_size);
		if(clientfd == -1)
		{
			// accept() failed
			syslog(LOG_ERR, "ERROR: accept()\n");
			printf("Accept failed\n");

			close(sockfd);
			exit(EXIT_FAILURE);
		}

		syslog(LOG_INFO, "Accepted Connection from %s\n", client_addr.sa_data);
		printf("clientfd = %d --- Accept passed\n", clientfd);
		printf("Accepted Connection from %s\n", client_addr.sa_data);

		// Create the file
		int fd = open(filepath, O_CREAT, 0644);	
		if(fd < 0)
		{
			syslog(LOG_ERR, "ERROR: open()\n");
			exit(EXIT_FAILURE);
		}
		close(fd);

		// Malloc a storage array and set it to zero (or directly use calloc())
		char *storage_array = (char *)malloc(RECV_SIZE * sizeof(char));
		memset(storage_array, 0, RECV_SIZE);

		//storage_array[0] = '\0';
		
		int packet_size = 0;
		bool enter_received = false;
		
		// if we want to use memcpy instead of strncpy
		//int memcpy_counter = 0;
		//char c;
		
		while(enter_received == false)
		{
			nread = 0;
			nread = recv(clientfd, recv_buffer, RECV_SIZE, 0);
			
			//printf("nread = %ld\n", nread);
			
			if( nread < 0)
			{
				printf("recv failed\n");
			}
			else
			{
				int i = 0;
				// if it fails iterate through only till nread
				for(i = 0; i < RECV_SIZE; i++)
				{
					// why does this work??
					// c = recv_buffer[i];
					// if (c)
					// {

					// }
					//printf("recv_buffer[%d] = %d\n", i, recv_buffer[i]);
					//usleep(10);
					if(recv_buffer[i] == '\n')// || recv_buffer[i] == EOF)
					{
						// packet complete
						printf("Received Enter at i = %d\n", i);
						enter_received = true;
						/* In first packet 
						 * recv_buffer[6] will be 'g'
						 * recv_buffer[7] will be '\n'
						 * Hence when we come into this condition i will be equal to 7
						 * But the packet size is actually 8, hence i++
						*/
						i++;	
						break;
					}
				}
				// Here i = 100 ---- when \n not found in the recv_buffer array
				packet_size += i; 

				//storage_array = (char *)realloc(storage_array, (packet_size * sizeof(char) ));

				/* packet_size+1 done in below line to make space for '\0' that is added by strncat after appending the string
				 * This \0 gets overwritten during the next strncat and hence there are no memory gaps between 2 packets
				*/
				char *newpointer = (char *)realloc(storage_array, ((packet_size+1) * sizeof(char) ) );
				//char *newpointer = (char *)realloc(storage_array, ((packet_size) * sizeof(char) ) );
				if (newpointer == NULL)
				{
					syslog(LOG_ERR, "ERROR: realloc()\n");
					printf("Realloc failed\n");
					exit(EXIT_FAILURE);
				} 
				else
				{
					// `newpointer` now points to a new memory block with the contents of oldpointer
					// `storage_array` points to an invalid address
					storage_array = newpointer;
					// storage_array points to the correct address
				}

				/* Using memcpy */
				//memcpy(storage_array+memcpy_counter, recv_buffer, i);
				//memcpy_counter += packet_size;

				/* Using strncpy */
				strncat(storage_array, recv_buffer, i);
				//storage_array[packet_size] = '\n';

				// Set the receive buffer to 0
				memset(recv_buffer, 0, RECV_SIZE);
			}

		}
		// Update total_data_size
		total_data_size += packet_size;

		/* For debugging */
		printf("Packet Size =  %d\n", packet_size);
		printf("total_data_size =  %d\n", total_data_size);
		//printf("Storage array = \n%s", storage_array);
		
		/* Write to file */
		fd = open(filepath, O_WRONLY | O_APPEND);
		write(fd, storage_array, packet_size);
		fdatasync(fd);
		close(fd);
		
		// Open the file to read
		fd = open(filepath, O_RDONLY);
	
		/* Logic to read one byte from the file and send one byte at a time to client*/
		/*
		int j = 0;
		char a;
		while(j < total_data_size)
		{
			nread = read(fd, &a, 1);
			if( nread < 0)
			{
				printf("read failed\n");
			}
			// printf("Server reply = %c\n", recv_buffer);
			//printf("%c", recv_buffer);
			// replace this with writing to file
			//strncat(output, &recv_buffer, nread);
			//write(fd, recv_buffer, nread);
			send(clientfd, &a, 1, 0);
			j++;
		}
		*/

		/* Logic to read one line from the file and send one line at a time to client*/
		char *line = NULL;
 		size_t len = 0;

 		// Convert fd to type FILE* to use the function getline
 		FILE *stream = fdopen(fd, "r");

 	    while ((nread = getline(&line, &len, stream)) != -1)
 	    {
 	    	//printf("%s", line);
 	    	send(clientfd, line, nread, 0);
        }

        free(line);
		close(fd);
		free(storage_array);
		
	}
	close(sockfd);
	close(clientfd);


	return 0;
}

