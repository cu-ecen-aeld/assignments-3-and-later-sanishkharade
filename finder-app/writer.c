#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <syslog.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#define ARGUMENTS 3
#define MODE 0644

int main(int argc, char *argv[])
{

    //printf("Hello world!\n");

    //printf("Number of arguments are %d\n", argc);
    openlog("AESD Assignment 2", LOG_PID, LOG_USER);
//syslog(LOG_INFO, "Start logging");


    if(argc < ARGUMENTS)
    {
        // error message
        syslog(LOG_ERR, "ERROR: Insufficient arguments");
        //syslog(LOG_DEBUG, "Less Arguments", argc);
        printf("Insufficient arguments\n");
        exit(1);
        
    }
    else if(argc > ARGUMENTS)
    {
        // error message
        syslog(LOG_ERR, "ERROR: Too many arguments");
        printf("Too many arguments\n");
        exit(1);
    }

    int fd;
    
    fd = creat(argv[1], MODE);

    if(fd == -1)
    {
    	// error message
    	syslog(LOG_ERR, "ERROR: File not created");
    	printf("Too many arguments\n");
       
    }

    int string_length = strlen(argv[2]);

    int bytes_written = 0;
    bytes_written = write(fd, argv[2], string_length);

    if(bytes_written == -1)
    {
    	syslog(LOG_ERR, "ERROR: Writing unsuccessful");
    	exit(1);
        // error message
    }
    if(bytes_written != string_length)
    {
    	// error message
    	syslog(LOG_ERR, "ERROR: String partially written");
    	exit(1); 
    }
    syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);
	//printf("Writing %s to %s\n", argv[2], argv[1]);
	
	closelog();

    return 0;
}






