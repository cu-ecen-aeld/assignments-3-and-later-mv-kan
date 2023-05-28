#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...) printf(msg)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    printf("sleeping for %dms\n", thread_func_args->wait_to_obtain_ms);
    usleep(thread_func_args->wait_to_obtain_ms*1000);
    printf("slept\n");

    int rc = pthread_mutex_lock(thread_func_args->mutex);
    if (rc != 0)
        return thread_param;
    printf("rc pthread_mutex_lock = %d\n", rc);
    usleep(thread_func_args->wait_to_release_ms*1000);
    rc = pthread_mutex_unlock(thread_func_args->mutex);
    if (rc != 0)
        return thread_param;

    thread_func_args->thread_complete_success = true;
    // pthread_exit(thread_param);
    return thread_param;
}

thread_data_t *allocate_thread_data(pthread_mutex_t*mutex, int obtain_wait, int release_wait)
{
    thread_data_t *data = malloc(sizeof(thread_data_t));
    data->wait_to_obtain_ms = obtain_wait;
    data->wait_to_release_ms = obtain_wait;
    data->mutex = mutex;
    return data; 
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
    
    thread_data_t*args = allocate_thread_data(mutex, wait_to_obtain_ms, wait_to_release_ms);
    int rc = pthread_create(&args->thread_id, NULL, &threadfunc, args);
    
    printf("rc = %d\n", rc);
    if (rc != 0)
        return false;
    *thread = args->thread_id;
    printf("thread = %ld, args->thread = %ld\n", *thread, args->thread_id);
    if (thread == NULL) return false;
    return true;
}

