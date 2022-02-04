#include "threading.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>



// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

	// Sleep before acquiring the mutex lock
    int rc = usleep(thread_func_args->obtain_ms * 1000);
    if(rc != 0)
    {
    	thread_func_args->thread_complete_success = false;
        ERROR_LOG("usleep failed before obtaining mutex\n");
        return thread_param;
    }
    
    // Acquire the mutex lock
   	rc = pthread_mutex_lock(thread_func_args->mutex);
    if(rc != 0)
    {
    	thread_func_args->thread_complete_success = false;
        ERROR_LOG("Attempt to obtain mutex failed\n");
        return thread_param;
    }

	// Sleep after acquiring the mutex lock
    rc = usleep(thread_func_args->release_ms * 1000);
    if(rc != 0)
    {
    	thread_func_args->thread_complete_success = false;
        ERROR_LOG("usleep failed after obtaining mutex\n");
        return thread_param;
    }
    
    // Release the mutex
    rc = pthread_mutex_unlock(thread_func_args->mutex);
    if(rc != 0)
    {
    	thread_func_args->thread_complete_success = false;
        ERROR_LOG("Attempt to release mutex failed\n");
        return thread_param;
    }

    thread_func_args->thread_complete_success = true;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     * 
     * See implementation details in threading.h file comment block
     */

    struct thread_data *params = (struct thread_data*)malloc(sizeof(struct thread_data));

	// Set parameters for the thread
    params->mutex = mutex;
    params->obtain_ms = wait_to_obtain_ms;
    params->release_ms = wait_to_release_ms;

    // Create a thread and do not block
    int rc = pthread_create(thread,
                            NULL,
                            threadfunc,			// Entry point of thread
                            (void*)params);
    if(rc == 0)
    {

        DEBUG_LOG("Thread creation successful\n");
        return true;
    }
    else
    {
        ERROR_LOG("Thread creation unsuccessful\n");
    	return false;
    }

}


