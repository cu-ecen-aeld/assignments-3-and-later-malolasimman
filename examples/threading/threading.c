/* Author name: Malola Simman Srinivasan Kannan 
 * file name : threading.c
 * Mail id: masr4788@colorado.edu
 * Date : 8 Feb 2023
 */

//headers
#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

//threadfunc is a thread in this waiting for obtain mutex time locking and waiting for release mutex time unlock the mutex
void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    // typecasting (void *)thread param to struct thread_data*
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    int error_handler1 = usleep((thread_func_args->wait_to_obtain) * 1000); //waits for a specified time before obtaining the mutex 	
    if(error_handler1 !=0) //checks whether usleep returns nonzero or checks for usleep failure error case
    {

     	    ERROR_LOG("failed to wait to obtain\n");
	    return thread_param; //returning argument back to called function
    }
    int error_handler2 = pthread_mutex_lock(thread_func_args->lock); //obtaining mutex lock
    if(error_handler2 !=0) // if mutex lock returns non zero then it prints error message
    {

            ERROR_LOG("failed to lock mutex\n");
            return thread_param; //returning argument back to called function
    }
    int  error_handler3 = usleep((thread_func_args->wait_to_release) * 1000); //waits for a specified time before obtaining the mutex
    if(error_handler3 !=0)  //checks whether usleep returns nonzero
    {
            ERROR_LOG("failed to wait to release\n");
            return thread_param; //returning argument back to called function
     }
    int error_handler4 = pthread_mutex_unlock(thread_func_args->lock); 
    if(error_handler4 !=0)// if mutex unlock returns non zero then it prints error message
    {

            ERROR_LOG("failed to unlock mutex\n");
            return thread_param; //returning argument back to called function
    }
    thread_func_args->thread_complete_success = true; // indicates that the thread has completed its execution successfully
    return thread_param; //returning argument back to called function
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
    
	// allocates dynamic memory using malloc function 
     struct thread_data *thread_args = (struct thread_data *) malloc (sizeof(struct thread_data));
     
     // Initializes the thread_data structure param
     thread_args->thread_complete_success = false;
     thread_args->lock= mutex;
     thread_args->wait_to_obtain=wait_to_obtain_ms;
     thread_args->wait_to_release=wait_to_release_ms;
     
     // creating new pthread
     int ret_1 = pthread_create(thread, NULL, threadfunc, thread_args);
     if (ret_1 != 0)//if it returns a non-zero value, then the thread could not be created, an error message is logged
     {
	     free(thread_args); // freeing the dynamic memory
             ERROR_LOG("Unable to create Thread\n");
     }
     else
     {	
             return true;//thread was created successfully,
     }   
    return false; //hread was failed to created  

}

