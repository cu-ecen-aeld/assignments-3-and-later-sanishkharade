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


int sockfd;
int clientfd;
char filepath[50] = "/var/tmp/aesdsocketdata";


#define READ_SIZE 100


static void signal_handler (int signo)
{
	if (signo == SIGINT)
	{
		printf ("Caught SIGINT!\n");
		unlink(filepath);
	}
	else if (signo == SIGTERM)
	{
		printf ("Caught SIGTERM!\n");
		unlink(filepath);
	}
	else
	{
	/* this should never happen */
		fprintf (stderr, "Unexpected signal!\n");
		exit (EXIT_FAILURE);
	}
	exit (EXIT_SUCCESS);
}
int main(int argc, char *argv[])
{	
	int status;

	struct addrinfo hints;
	struct addrinfo *result, *rp;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	status = getaddrinfo(NULL, "9000", &hints, &result);
	if(status != 0)
	{
		printf("Error in getaddrinfo");
	}

	/* getaddrinfo() returns a list of address structures.
	  Try each address until we successfully bind(2).
	  If socket(2) (or bind(2)) fails, we (close the socket
	  and) try the next address. */
	// taken from man page of getaddrinfo

	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sockfd = socket(AF_INET6, SOCK_STREAM, 0);
		if (sockfd == -1)
		{
			
			continue;
		}
		   
		if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
		{
			printf("rp->ai_addr = %p --- passed\n", rp->ai_addr);
			printf("sockfd = %d --- passed\n", sockfd);
			break;                  /* Success */
		}
		   

		printf("rp->ai_addr = %p --- failed\n", rp->ai_addr);
		printf("sockfd = %d --- failed\n", sockfd);
		close(sockfd);
	}


	freeaddrinfo(result);           /* No longer needed */

	if (rp == NULL) {               /* No address succeeded */
	   fprintf(stderr, "Could not bind\n");
	   exit(EXIT_FAILURE);
	}

	printf("argc = %d\n", argc);

	//add daemon
	if(argc > 1)
	{
		printf("Daemon\n");
		daemon(0,0);
	}

	/*
	* Register signal_handler as our signal handler
	* for SIGINT.
	*/
	if (signal (SIGINT, signal_handler) == SIG_ERR)
	{
		fprintf (stderr, "Cannot handle SIGINT!\n");
		exit (EXIT_FAILURE);
	}
	/*
	* Register signal_handler as our signal handler
	* for SIGTERM.
	*/
	if (signal (SIGTERM, signal_handler) == SIG_ERR)
	{
		fprintf (stderr, "Cannot handle SIGTERM!\n");
		exit (EXIT_FAILURE);
	}



	char server_reply[READ_SIZE] = {0};
	int total_data_size = 0;
	ssize_t nread = 0;
	char filepath[50] = "/var/tmp/aesdsocketdata";

	status = listen(sockfd, 2);
	if(status == -1)
	{
		printf("Listen failed\n");
	}
	printf("Listen passed\n");
	

	while(1)
	{

		socklen_t sock_addr_size;
		struct sockaddr client_addr;
		sock_addr_size = sizeof(struct sockaddr);

		// Accept the connection
		clientfd = accept(sockfd, &client_addr, &sock_addr_size);
		if(clientfd == -1)
		{
			printf("Accept failed\n");
		}
		printf("clientfd = %d --- Accept passed\n", clientfd);

		int fd = open(filepath, O_CREAT, 0644);	
		close(fd);

		char *storage_array = (char *)malloc(READ_SIZE * sizeof(char));
		memset(storage_array, 0, READ_SIZE);
		//storage_array[0] = '\0';
		int packet_size = 0;
		bool enter_received = false;
		//int memcpy_counter = 0;

		while(enter_received == false)
		{
			
			nread = recv(clientfd, server_reply, READ_SIZE, 0);
			if( nread < 0)
			{
				printf("recv failed\n");
			}
			else
			{
				int i = 0;
				for(i = 0; i < READ_SIZE; i++)
				{
					if(server_reply[i] == '\n' || server_reply[i] == -1)
					{
						// packet complete
						printf("Received Enter\n");
						enter_received = true;
						/* In first packet 
						 * server_reply[6] will be 'g'
						 * server_reply[7] will be '\n'
						 * Hence when we come into this condition i will be equal to 7
						 * But the packet size is actually 8, hence i++
						*/
						i++;	
						break;
					}
				}
				// Here i = 100 ---- when \n not found in the server_reply array
				packet_size += i; 
				
				//storage_array = (char *)realloc(storage_array, (packet_size * sizeof(char) ));

				/* packet_size+1 done in below line to make space for '\0' that is added by strncat after appending the string
				 * This \0 gets overwritten during the next strncat and hence there are no memory gaps between 2 packets
				*/
				char *newpointer = (char *)realloc(storage_array, ((packet_size+1) * sizeof(char) ) );
				//char *newpointer = (char *)realloc(storage_array, ((packet_size) * sizeof(char) ) );
				if (newpointer == NULL)
				{
					/* problems!!!!                                 */
					/* tell the user to stop playing DOOM and retry */
					/* or free(oldpointer) and abort, or whatever   */
					printf("Realloc failed\n");
					exit(1);
				} 
				else
				{
					/* everything ok                                                                 */
					/* `newpointer` now points to a new memory block with the contents of oldpointer */
					/* `oldpointer` points to an invalid address                                     */
					storage_array = newpointer;
					/* oldpointer points to the correct address                                */
					/* the contents at oldpointer have been copied while realloc did its thing */
					/* if the new size is smaller than the old size, some data was lost        */
				}

				/* Using memcpy */
				//memcpy(storage_array+memcpy_counter, server_reply, i);
				//memcpy_counter += packet_size;

				/* Using strncpy */
				strncat(storage_array, server_reply, i);
				//storage_array[packet_size] = '\n';

				memset(server_reply, 0, READ_SIZE);
			}

		}
		total_data_size += packet_size;

		/* For debugging */
		printf("Packet Size =  %d\n", packet_size);
		printf("total_data_size =  %d\n", total_data_size);
		printf("Storage array = \n%s", storage_array);
		
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
			// printf("Server reply = %c\n", server_reply);
			//printf("%c", server_reply);
			// replace this with writing to file
			//strncat(output, &server_reply, nread);
			//write(fd, server_reply, nread);
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

