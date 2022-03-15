/**
 * @file    :   systemcalls.c
 * @brief   :   This source file provides functions which is used to write specified data into a 
 *				file who's path is given 
 *              
 *
 * @author  :   Sanish Kharade, Daniel Walkes
 * @date    :   January 27, 2022
 * 
 * 
 * @link    :   Linux System Programming book - Chapter 5
*/

#include "systemcalls.h"
#include <stdlib.h>		// for system()
#include <unistd.h> 	// for execl()
#include <sys/wait.h>	// for wait functions and macros
#include <fcntl.h>

#define MODE 0644

/**
 * @param cmd the command to execute with system()
 * @return true if the commands in ... with arguments @param arguments were executed 
 *   successfully using the system() call, false if an error occurred, 
 *   either in invocation of the system() command, or if a non-zero return 
 *   value was returned by the command issued in @param.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success 
 *   or false() if it returned a failure
*/

	int ret;
	ret = system(cmd);
	
	// function failed
	if(ret != 0)
	{
		return false;
	}
	
	// function passed
    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the 
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];
/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *   
*/

	int status;
	int ret;
	pid_t pid;
	
	// Fork a new process
	pid = fork();
	
	if (pid == -1)
	{
		// If fork() failed
		perror("ERROR: fork"); 
		exit(-1);
	}
	else if (pid == 0)
	{
		// If this is a child process 
		ret = execv(command[0], command);

		if (ret == -1)
		{
			perror("ERROR : execv");
			exit(-1);
		}
	}
	else
	{
		// If this is a parent process
		if (waitpid (pid, &status, 0) == -1)
		{	
			perror("ERROR : wait");
			return false;
		}
		if (! WIFEXITED(status) || WEXITSTATUS(status))
		{
			perror("Waitstatus");
			return false;
		}
	}
	
    va_end(args);
    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.  
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *   
*/
	
	int status;
	int ret;
	pid_t pid;
	
	// Open the file we want to redirect standard out to
	int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, MODE);
	if (fd < 0)
	{ 
		perror("open"); 
		return false;
	}
	
	// Fork a new process
	pid = fork ();
	if (pid == -1)
	{
		// If fork() failed
		perror("ERROR: fork"); 
		return false;
	}
	else if (pid == 0)
	{
		// If this is a child process 
		if (dup2(fd, 1) < 0)
		{
			perror("dup2"); 
			return false;
		}
		close(fd);
		
		ret = execv(command[0], command);
		if (ret == -1)
		{
			perror("ERROR : execv");
			exit(-1);
		}
	}
	else
	{
		// If this is a parent process
		if (waitpid (pid, &status, 0) == -1)
		{	
			perror("wait");
			close(fd);
			return false;
		}
		if (! WIFEXITED(status) || WEXITSTATUS(status))
		{
			perror("Waitstatus");
			close(fd);
			return false ;
		}
	}
	
	close(fd);
	va_end(args);
    return true;
}





