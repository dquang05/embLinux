/**
 * @file 01_thread_creation.c
 * @brief Demonstrates basic thread creation, joining, and argument passing.
 *
 * This example shows how to spawn a new thread using pthread_create(),
 * pass a struct containing arguments to it, and retrieve its return value
 * using pthread_join().
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

/**
 * @brief Structure to pass multiple arguments to the thread.
 */
typedef struct {
    int thread_id;
    char *message;
} thread_args_t;

/**
 * @brief The function executed by the new thread.
 * 
 * @param arg Pointer to the thread arguments (expected to be thread_args_t*).
 * @return void* A pointer to an integer containing the thread's exit status.
 */
void *thread_function(void *arg) {
    thread_args_t *args = (thread_args_t *)arg;
    
    printf("[Thread %d] Started. Message: %s\n", args->thread_id, args->message);
    
    // Simulate some work
    printf("[Thread %d] Working...\n", args->thread_id);
    sleep(1);
    
    // Allocate memory for the return value
    int *return_val = malloc(sizeof(int));
    if (return_val == NULL) {
        perror("malloc failed in thread");
        pthread_exit(NULL);
    }
    
    *return_val = args->thread_id * 100; // Arbitrary return value for demonstration
    printf("[Thread %d] Finished work. Returning %d.\n", args->thread_id, *return_val);
    
    // Terminate thread and return the value
    pthread_exit((void *)return_val);
}

int main(void) {
    pthread_t my_thread;
    thread_args_t args = {
        .thread_id = 1,
        .message = "Hello from the main thread!"
    };
    
    printf("[Main] Creating thread...\n");
    
    // 1. Create the thread
    int ret = pthread_create(&my_thread, NULL, thread_function, (void *)&args);
    if (ret != 0) {
        // pthread functions do not set errno, they return the error code directly
        errno = ret;
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
    }
    
    printf("[Main] Thread created successfully (ID: %lu). Waiting for it to finish...\n", (unsigned long)my_thread);
    
    // 2. Join the thread to retrieve its return value
    void *thread_result = NULL;
    ret = pthread_join(my_thread, &thread_result);
    if (ret != 0) {
        errno = ret;
        perror("pthread_join failed");
        exit(EXIT_FAILURE);
    }
    
    // 3. Process the return value and clean up
    if (thread_result != NULL) {
        int *val = (int *)thread_result;
        printf("[Main] Thread joined successfully. Thread returned: %d\n", *val);
        free(val); // Free the memory allocated by the thread
    } else {
        printf("[Main] Thread joined but returned NULL.\n");
    }
    
    printf("[Main] Exiting.\n");
    return EXIT_SUCCESS;
}
