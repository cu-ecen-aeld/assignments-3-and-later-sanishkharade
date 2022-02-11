//#define _GNU_SOURCE 
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h> 	// for close socket
int main()
{

	// create a socket using socket() - returns socketfd
	// domain - iPV4/ ipv6 	
	// protocol = 0
	int sockfd;
	int status;

/*	sockfd = socket(AF_INET6, SOCK_STREAM, 0);
	if(sockfd == -1)
	{
		printf("Error creating socket\n");

	}
	printf("sockfd = %d\n", sockfd);*/


	// bind()
	// socketfd
	// struct sockaddr
	// addr_len - sizeof(struct sockaddr)

	// set up hints and res (addrinfo)
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


/*
	status = bind(sockfd, res->ai_addr, sizeof(struct sockaddr));
	if(status == -1)
	{
		printf("Bind failed\n");
		perror("BIND: ");
	}*/
	// getaddrinfo - fill res structure with required value

	// res->ai_addr to sockaddr in bind





	// listen()
	// sockfd 
	// backlog - 2
/*	status = listen(sockfd, 2);
	if(status == -1)
	{
		printf("Listen failed\n");
	}
	printf("Listen passed\n");
	// accept()
	// sockfd
	// sockaddr - res
	// addr_len - sizeof(struct sockaddr)
	int clientfd;
	socklen_t sock_addr_size;
	struct sockaddr client_addr;
	sock_addr_size = sizeof(struct sockaddr);

	// 2nd argument is wrong
	//clientfd = accept4(sockfd, &client_addr, &sock_addr_size,SOCK_NONBLOCK);
	clientfd = accept(sockfd, &client_addr, &sock_addr_size);
	if(clientfd == -1)
	{
		printf("Accept failed\n");
	}
	printf("clientfd = %d --- Accept passed\n", clientfd);*/

	char server_reply[1000];
	char output[2000];
	// if( recvfrom(clientfd, server_reply, 100, 0, &client_addr, &sock_addr_size) < 0)
	// getaddrinfo man page -  Use client fd and not server fd


	
	ssize_t nread = 0;
	for(int i = 0; i < 5; i++)
	{
			status = listen(sockfd, 2);
			if(status == -1)
			{
				printf("Listen failed\n");
			}
			printf("Listen passed\n");
			// accept()
			// sockfd
			// sockaddr - res
			// addr_len - sizeof(struct sockaddr)
			int clientfd;
			socklen_t sock_addr_size;
			struct sockaddr client_addr;
			sock_addr_size = sizeof(struct sockaddr);

			// 2nd argument is wrong
			//clientfd = accept4(sockfd, &client_addr, &sock_addr_size,SOCK_NONBLOCK);
			clientfd = accept(sockfd, &client_addr, &sock_addr_size);
			if(clientfd == -1)
			{
				printf("Accept failed\n");
			}
			printf("clientfd = %d --- Accept passed\n", clientfd);


		nread = recv(clientfd, server_reply, 1000, 0);
		if( nread < 0)
		{
			printf("recv failed\n");
		}
		//printf("Reply received\n");
		printf("nread = %ld\n", nread);
		printf("Server reply = %s", server_reply);

		strncat(output, server_reply, nread);
		printf("Output = %s", output);
		send(clientfd, output, strlen(output), 0);
		memset(server_reply, 0, sizeof(server_reply));
	}
	// if( recv(clientfd, server_reply, 100, 0) < 0)
	// {
	// 	printf("recv failed\n");
	// }
	// printf("Reply received\n");
	// printf("Server reply = %s\n", server_reply);

	// send(clientfd, server_reply, strlen(server_reply), 0);



	//read line by line 
	return 0;
}

