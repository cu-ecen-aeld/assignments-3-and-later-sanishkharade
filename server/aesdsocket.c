/**
 * @file    :   aesdsocket.c
 * @brief   :   This source file reads data from a socket on port 9000, appends it to a file and then 
 * 				sends back that data to the client when one complete packet has been read.
 *              
 * @author  :   Sanish Kharade
 * @date    :   February 18, 2022
 * 
 * 
 * @link    :   For all functions - man pages and Linux System Programming book
 * 				Realloc - https://stackoverflow.com/questions/3850749/does-realloc-overwrite-old-contents
 * 				Socket functions - https://beej.us/guide/bgnet/html/
 * 				
*/

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
#include <errno.h>		// for errno

#include <syslog.h>		// for syslog
#include <arpa/inet.h>	// for inet_ntop

#include <pthread.h>	// for pthread functions
#include <sys/queue.h>	// for Linked List
#include <poll.h>		// for poll()

#include <sys/time.h>
#include <time.h>

#define PORT "9000"
#define MAX_CONNECTIONS 10
#define POLL_TIMEOUT 1
#define NO_OF_FDS_FOR_POLL 1

bool application_running = true;
bool alarm_triggered = false;

int sockfd;
int clientfd;

#define USE_AESD_CHAR_DEVICE 1

#if (USE_AESD_CHAR_DEVICE == 1)
	char filepath[50] = "/dev/aesdchar";
#else
	char filepath[50] = "/var/tmp/aesdsocketdata";
#endif

void* socket_handler(void* thread_param);
static void time_handler(int sig_num);
void* cleanup_handler(void* arg);
void graceful_shutdown(void);

// use power of 2 -> 100, 1000 didn't work
#define RECV_SIZE 128

typedef struct
{
	bool thread_complete;
	pthread_t thread;
	int client_sock_fd;
	char *ip_v4;
}thread_params_t;

// Linked List Implementation
SLIST_HEAD(slisthead, slist_data_s) head;

typedef struct slist_data_s slist_data_t;
struct slist_data_s
{
	thread_params_t thread_param;
	SLIST_ENTRY(slist_data_s) entries;
};

// Mutex 
pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_t cleanup_thread;

/**
 * @brief   :   Signal Handler function
 *              
 * @param   :   signo - Signal received
 *
 * @return  :   void
 * 
*/
static void signal_handler (int signo)
{
	// Keeping the signal handler as small as possible 
	application_running = false;

	if (signo == SIGINT)
	{
		syslog(LOG_INFO, "Caught signal SIGINT, exiting\n");
	}
	else if (signo == SIGTERM)
	{
		syslog(LOG_INFO, "Caught signal SIGTERM, exiting\n");
	}
	// else
	// {
	// 	//  this should never happen 
	// 	syslog(LOG_ERR, "Unknown Signal received\n");
	// 	exit (EXIT_FAILURE);
	// }
}


/**
 * @brief   :   Function for graceful shutdown
 *              
 * @param   :   none
 *
 * @return  :   void
 * 
*/
void graceful_shutdown(void)
{
	syslog(LOG_INFO, "Executing Graceful Shutdown procedure\n");

	struct slisthead *head_cleanup = (struct slisthead*) (&head);
	slist_data_t *iterator_node = NULL;

	SLIST_FOREACH(iterator_node, head_cleanup, entries)
	{
		// Just in case anything was not cleaned by the cleanup thread
		if(iterator_node->thread_param.thread_complete == true)
		{
			/*
				* 	Thread has completed its execution.
				*	Kill the thread
				*	Remove the thread from the Linked List and free the node
			*/

			shutdown(iterator_node->thread_param.client_sock_fd, SHUT_RDWR);

			// Kill the thread
			int ret = -1;
			ret = pthread_kill(iterator_node->thread_param.thread, SIGKILL);
			if(ret != 0)
			{
				syslog(LOG_ERR, "ERROR: pthread_kill() : %s\n", strerror(ret));
				exit(EXIT_FAILURE);
			}

			// Remove thread from the linked list
			SLIST_REMOVE(head_cleanup, iterator_node, slist_data_s, entries);

			// Free the node
			free(iterator_node);

			break;
		}
	}

	// For errors 
	int status;

    status = pthread_kill(cleanup_thread, SIGKILL);
	if(status != 0)
	{
		syslog(LOG_ERR, "ERROR: pthread_kill() : %s\n", strerror(status));
		exit(EXIT_FAILURE);
	}

	// Shutdown the socket
	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);

	shutdown(clientfd, SHUT_RDWR);
	close(clientfd);

	// Destroy the mutex
	status = pthread_mutex_destroy(&mutex_lock);
	if(status != 0)
	{
		syslog(LOG_ERR, "ERROR: pthread_mutex_destroy() : %s\n", strerror(status));	
		exit(EXIT_FAILURE);
	}
#if (USE_AESD_CHAR_DEVICE == 0)
	// Delete the file
	status = unlink(filepath);
	if(status != 0)
	{
		if(errno == ENOENT)
		{
			// File did not exist
			syslog(LOG_DEBUG, "File %s did not exist\n", filepath);
		}
		else
		{
			//Error
			syslog(LOG_ERR, "ERROR: unlink() : %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		// Delete the file before exitting the application
		syslog(LOG_DEBUG, "Deleting file %s\n", filepath);
	}
#endif
	closelog();
	//exit(EXIT_SUCCESS);

}

/**
 * @brief   :   Main entry point to the application
 *              
 * @param   :   argv[1] - -d if running as a daemon
 *
 * @return  :   0
 * 
*/
int main(int argc, char *argv[])
{	

	// Open the syslog for logging data
	// Using LOG_PERROR and LOG_CONS to print the log messages to the console
	openlog("AESD Socket Application", LOG_PID | LOG_PERROR | LOG_CONS, LOG_USER);

	// Status variable will store the return values of functions for error checking
	int status;

	// Create structures to get the value of addrinfo
	struct addrinfo hints;
	struct addrinfo *result, *rp;

	char ipv4[INET_ADDRSTRLEN]; // space to hold the IPv4 string

	// Linked List Node
	slist_data_t *datap = NULL;

	SLIST_INIT(&head);

	// Initialize the mutex
	status = pthread_mutex_init(&mutex_lock, NULL);
	if(status != 0)
	{
		syslog(LOG_ERR, "ERROR: pthread_mutex_init() : %s\n", strerror(status));	
		exit(EXIT_FAILURE);
	}
#if (USE_AESD_CHAR_DEVICE == 0)
	// Delete the file in case it exists
	status = unlink(filepath);
	if(status != 0)
	{
		if(errno == ENOENT)
		{
			// File did not exist
			syslog(LOG_DEBUG, "File %s did not exist\n", filepath);
		}
		else
		{
			// Error
			syslog(LOG_ERR, "ERROR: unlink() : %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		// If file existed, delete it before starting the application
		syslog(LOG_DEBUG, "Deleting file %s\n", filepath);
	}
#endif
	/*
	* Register signal_handler as our signal handler
	* for SIGINT and SIGTERM.
	*/
	if (signal(SIGINT, signal_handler) == SIG_ERR)
	{
		syslog(LOG_ERR, "ERROR: Could not register SIGINT handler\n");
		exit (EXIT_FAILURE);
	}

	if (signal(SIGTERM, signal_handler) == SIG_ERR)
	{
		syslog(LOG_ERR, "ERROR: Could not register SIGTERM handler\n");
		exit (EXIT_FAILURE);
	}

	// Set the values for members of the hints structure
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	status = getaddrinfo(NULL, PORT, &hints, &result);
	if(status != 0)
	{
		syslog(LOG_ERR, "ERROR: getaddrinfo() : %s\n", gai_strerror(status));
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
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd == -1)
		{
			/* socket() failed */
			syslog(LOG_ERR, "ERROR: socket() : %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    	{
			syslog(LOG_ERR, "ERROR: socketopt() : %s\n", strerror(errno));
        	exit(EXIT_FAILURE);
    	}
		   
		if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
		{
			/* Successfully bound to socket */
			syslog(LOG_DEBUG, "Successfully Bound to socket\n");
			break;                  
		}
	
		syslog(LOG_DEBUG, "FAILED: bind()\n");
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

    if ((argc >= 2) && (strcmp("-d", argv[1]) == 0))
    {
    	syslog(LOG_DEBUG, "Running aesdsocket as a Daemon1\n");
		status = daemon(0,0);
		if(status == -1)
		{
			// daemon() failed
			syslog(LOG_ERR, "ERROR: daemon() : %s\n", strerror(errno));
			close(sockfd);
			exit(EXIT_FAILURE);
		}	
    }

	// Listen for connections
	syslog(LOG_DEBUG, "Listening for connections on the socket\n");
	status = listen(sockfd, MAX_CONNECTIONS);
	if(status == -1)
	{
		// listen() failed
		syslog(LOG_ERR, "ERROR: listen() : %s\n", strerror(errno));
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	syslog(LOG_DEBUG, "Listen Passed\n");
	
	// pthread_t cleanup_thread;
    status = pthread_create(&cleanup_thread, NULL, cleanup_handler, &head);
    if (status)
    {
        syslog(LOG_ERR, "ERROR: pthread_create() : %s\n", strerror(status));
        exit(EXIT_FAILURE);
    }
#if (USE_AESD_CHAR_DEVICE == 0)

	// Register SIGALRM
	if (signal(SIGALRM, time_handler) == SIG_ERR)
	{
		syslog(LOG_ERR, "ERROR: Could not register SIGALRM handler\n");
		exit (EXIT_FAILURE);
	}

	struct itimerval timer_duration;
	// Loading initial value as 10 and the reload interval as 10
	timer_duration.it_value.tv_sec = 10;
	timer_duration.it_value.tv_usec = 0;
	timer_duration.it_interval.tv_sec = 10;
	timer_duration.it_interval.tv_usec = 0;

	// Set the timer	
	status = setitimer(ITIMER_REAL, &timer_duration, NULL);
	if(status != 0)
	{
		// Error 
		syslog(LOG_ERR, "ERROR: setitimer() : %s\n", strerror(errno));
		close(sockfd);
		exit(EXIT_FAILURE);
	}

#endif
	// Setup for poll()
	short events = POLL_IN;
	short revents = 0;
	struct pollfd pfd = {sockfd, events, revents};

	while(1)
	{

		socklen_t sock_addr_size;
		struct sockaddr client_addr;
		sock_addr_size = sizeof(struct sockaddr);

		// Poll will block for time = POLL_TIMEOUT
		status = poll(&pfd, NO_OF_FDS_FOR_POLL, POLL_TIMEOUT);
		if(status < 0)
		{
			// Can be due to a signal interrupting
			if(application_running == false || alarm_triggered == true)
			{
				// This is not an error just a signal interrupt
				alarm_triggered = false;
				status = 0;
			}
			else
			{
				syslog(LOG_ERR, "ERROR: poll() : %s\n", strerror(errno));
				graceful_shutdown();
				exit(EXIT_FAILURE);
			}
		}
		if(status == 0)
		{
			if(application_running == false)
			{
				// Caught SIGTERM or SIGINT
				graceful_shutdown();
				exit(EXIT_SUCCESS);
			}	
		}
		else
		{
			// Poll has returned a non negative value which means data is ready on sockfd

			// Accept the connection
			syslog(LOG_INFO, "Waiting for connections\n");
			clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &sock_addr_size);
			//check if application exited
			if(clientfd == -1 && application_running) 
			{
				// accept() failed
				syslog(LOG_ERR, "ERROR: accept() : %s\n", strerror(errno));
				shutdown(clientfd, SHUT_RDWR);
				exit(EXIT_FAILURE);
			}
			
			struct sockaddr_in *sa = (struct sockaddr_in *)&client_addr;

			const char *ret = inet_ntop(AF_INET, &(sa->sin_addr), ipv4, INET_ADDRSTRLEN);
			if(ret == NULL)
			{
				syslog(LOG_ERR, "ERROR: inet_ntop() : %s\n", strerror(errno));
			}
			else
			{
				syslog(LOG_INFO, "Accepted Connection from %s\n", ipv4);
			}
			

			// Allocating memory for new node
			datap = (slist_data_t*) malloc (sizeof(slist_data_t));

			// Added new node to the linked list at the head
			SLIST_INSERT_HEAD(&head, datap, entries); 

			datap->thread_param.client_sock_fd = clientfd;
			datap->thread_param.thread_complete = false;
			datap->thread_param.ip_v4 = ipv4;

			status = pthread_create(&(datap->thread_param.thread), NULL, socket_handler, &(datap->thread_param));
			if (status)
			{
				syslog(LOG_ERR, "ERROR: pthread_create() : %s\n", strerror(status));
				exit(EXIT_FAILURE);
			}
		}

	}
	graceful_shutdown();

	return 0;
}

/**
 * @brief   :   Cleanup Handler function
 *              
 * @param   :   signo - Signal received
 *
 * @return  :   void *
 * 
*/
void* cleanup_handler(void* arg)
{

	struct slisthead *head_cleanup = (struct slisthead*) arg;
	slist_data_t *iterator_node = NULL;

    while (1)
    {
        SLIST_FOREACH(iterator_node, head_cleanup, entries)
        {
            if(iterator_node->thread_param.thread_complete == true)
            {
				/*
				 * 	Thread has completed its execution.
				 *	Join the thread to release its resources
				 *	Remove the thread from the Linked List and free the node
                */

			    // Client shutdown after it exited. Hence no need to do this here
            	// shutdown(iterator_node->thread_param.client_sock_fd, SHUT_RDWR);
				// syslog(LOG_INFO, "Closed connection1 from %s\n", iterator_node->thread_param.ip_v4);

                // Join the thread to free up resources
                int ret = -1;
                ret = pthread_join(iterator_node->thread_param.thread, NULL);
                if(ret != 0)
                {
					syslog(LOG_ERR, "ERROR: pthread_join() : %s\n", strerror(ret));
					graceful_shutdown();
                    exit(EXIT_FAILURE);
                }

                // Remove thread from the linked list
				SLIST_REMOVE(head_cleanup, iterator_node, slist_data_s, entries);

				// Free the node
                free(iterator_node);

                break;
            }
        }
		// Sleep for 10 us
        usleep(10);
    }
    return NULL;

}

/**
 * @brief   :   Socket Handler function
 *              
 * @param   :   signo - Signal received
 *
 * @return  :   void
 * 
*/
void* socket_handler(void* thread_param)
{
	char recv_buffer[RECV_SIZE] = {0};
	int total_data_size = 0;
	ssize_t nread = 0;

	thread_params_t *params = (thread_params_t*)thread_param;

	// For checking return values for errors
	int status;

	// Malloc a storage array and set it to zero (or directly use calloc())
	char *storage_array = (char *)malloc(RECV_SIZE * sizeof(char));
	if (storage_array == NULL)
	{
		syslog(LOG_ERR, "ERROR: malloc()\n");
		graceful_shutdown();
		exit(EXIT_FAILURE);
	} 
	memset(storage_array, 0, RECV_SIZE);
		
	int packet_size = 0;
	bool enter_received = false;
	
	// if we want to use memcpy instead of strncpy
	//int memcpy_counter = 0;
	//char c;

	while(enter_received == false)
	{
		nread = 0;
		
		nread = recv(params->client_sock_fd, recv_buffer, RECV_SIZE, 0);
		if( nread == -1)
		{
			syslog(LOG_ERR, "ERROR: recv() %s \n", strerror(errno));
			free(storage_array);
			graceful_shutdown();
			exit(EXIT_FAILURE);
		}
		else
		{
			int i = 0;

			for(i = 0; i < RECV_SIZE; i++)
			{

				if(recv_buffer[i] == '\n')// || recv_buffer[i] == EOF)
				{
					// packet complete
					//printf("Received Enter at i = %d\n", i);
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

			/* packet_size+1 done in below line to make space for '\0' that is added by strncat after appending the string
				* This \0 gets overwritten during the next strncat and hence there are no memory gaps between 2 packets
			*/
			char *newpointer = (char *)realloc(storage_array, ((packet_size+1) * sizeof(char) ) );
			if (newpointer == NULL)
			{
				syslog(LOG_ERR, "ERROR: realloc()\n");
				graceful_shutdown();
				exit(EXIT_FAILURE);
			} 
			else
			{
				// 'newpointer' now points to a new memory block with the contents of oldpointer
				// 'storage_array' points to an invalid address
				storage_array = newpointer;
				// 'storage_array' points to the correct address
			}

			/* Using memcpy */
			//memcpy(storage_array+memcpy_counter, recv_buffer, i);
			//memcpy_counter += packet_size;
			//storage_array[packet_size] = '\n';
			
			/* Using strncpy */
			strncat(storage_array, recv_buffer, i);

			// Set the receive buffer to 0
			memset(recv_buffer, 0, RECV_SIZE);
		}

	}
	// Update total_data_size
	total_data_size += packet_size;

	/* For debugging */
	// printf("Packet Size =  %d\n", packet_size);
	// printf("total_data_size =  %d\n", total_data_size);
	// printf("Storage array = \n%s", storage_array);
			
	// Locking the mutex around the file operations
	status = pthread_mutex_lock(&mutex_lock);
	if(status != 0)
	{
		syslog(LOG_ERR, "ERROR: mutex_lock() : %s \n", strerror(status));
		graceful_shutdown();
		exit(EXIT_FAILURE);
	}

	int fd = open(filepath, O_CREAT|O_RDWR|O_APPEND, 0644);
	if(fd == -1)
	{
		syslog(LOG_ERR, "ERROR: open() : %s \n", strerror(errno));
		free(storage_array);
		graceful_shutdown();
		exit(EXIT_FAILURE);
	}
	// Write to file
	int bytes_wriiten = write(fd, storage_array, packet_size);
	if( bytes_wriiten == -1)
	{
		syslog(LOG_ERR, "ERROR: write() : %s \n", strerror(errno));
		graceful_shutdown();
		exit(EXIT_FAILURE);
	}
#if (USE_AESD_CHAR_DEVICE == 0)
	lseek(fd, 0, SEEK_SET);
#else 
	close(fd);
	fd = open(filepath, O_RDWR);
	if(fd == -1)
	{
		syslog(LOG_ERR, "ERROR: open() : %s \n", strerror(errno));
		free(storage_array);
		graceful_shutdown();
		exit(EXIT_FAILURE);
	}
#endif
	/* Logic to read one byte from the file and send one byte at a time to client*/
#if (USE_AESD_CHAR_DEVICE == 0)	
	char a;
	while((nread = read(fd, &a, 1)) != 0)
	{
		if( nread == -1)
		{
			syslog(LOG_ERR, "ERROR: read() : %s \n", strerror(errno));
			free(storage_array);
			graceful_shutdown();
			exit(EXIT_FAILURE);
		}

		status = send(params->client_sock_fd, &a, 1, 0);
		if(status == -1)
		{
			syslog(LOG_ERR, "ERROR: send() : %s \n", strerror(errno));
			graceful_shutdown();
			exit(EXIT_FAILURE);
		}
	}
#else
	int read_size = 64;
	char a[read_size];
	while((nread = read(fd, a, read_size)) != 0)
	{
		if( nread == -1)
		{
			syslog(LOG_ERR, "ERROR: read() : %s \n", strerror(errno));
			free(storage_array);
			graceful_shutdown();
			exit(EXIT_FAILURE);
		}

		status = send(params->client_sock_fd, a, nread, 0);
		if(status == -1)
		{
			syslog(LOG_ERR, "ERROR: send() : %s \n", strerror(errno));
			graceful_shutdown();
			exit(EXIT_FAILURE);
		}
	}
#endif

	close(fd);

	status = pthread_mutex_unlock(&mutex_lock);
	if(status != 0)
	{
		syslog(LOG_ERR, "ERROR: mutex_unlock() : %s \n", strerror(status));
		graceful_shutdown();
		exit(EXIT_FAILURE);
	}

	params->thread_complete = true;
	
	// Closing connection here itself. Node will be deleted by cleanup thread
	syslog(LOG_INFO, "Closed connection from %s\n", params->ip_v4);
	shutdown(params->client_sock_fd, SHUT_RDWR);

	free(storage_array);
	
	return thread_param;
}
#if (USE_AESD_CHAR_DEVICE == 0)
static void time_handler(int sig_num)
{
	alarm_triggered = true;
	char timestamp[100];
	time_t t;
	struct tm *tmp;
	time(&t);
	int length, status;

	// Get the local time in tmp structure
	tmp = localtime(&t);
	if(tmp == NULL)
	{
		syslog(LOG_ERR, "ERROR: localtime()\n");
		graceful_shutdown();
		exit(EXIT_FAILURE);
	}

	length = strftime(timestamp, sizeof(timestamp), "timestamp: %a, %d %b %Y - %T\n", tmp);
	if(length == 0)
	{
		syslog(LOG_DEBUG, "DEBUG: strftime() returned 0\n");
		exit(EXIT_FAILURE);
	}
	syslog(LOG_DEBUG, "%s", timestamp);
	
	// Lock the mutex for writing to file
	status = pthread_mutex_lock(&mutex_lock);
	if(status != 0)
	{
		syslog(LOG_ERR, "ERROR: mutex_lock() : %s \n", strerror(status));
		graceful_shutdown();
		exit(EXIT_FAILURE);
	}
	
	// Open the file
	int fd = open(filepath, O_CREAT|O_RDWR|O_APPEND, 0644);
	if( fd == -1 )
	{
		syslog(LOG_ERR, "ERROR: open() fail");
		graceful_shutdown();
		exit(EXIT_FAILURE);
	}

	// Write the timestamp
	int bytes_wriiten = write(fd, timestamp, length);
	if( bytes_wriiten == -1)
	{
		syslog(LOG_ERR, "ERROR: write() : %s \n", strerror(errno));
		graceful_shutdown();
		exit(EXIT_FAILURE);
		
	}
	close(fd);

	// Unlock the mutex 
	pthread_mutex_unlock(&mutex_lock);
	if(status != 0)
	{
		syslog(LOG_ERR, "ERROR: mutex_unlock() : %s \n", strerror(status));
		graceful_shutdown();
		exit(EXIT_FAILURE);
	}	

} 
#endif