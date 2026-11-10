/**
 * @file 03_mutex_sync.c
 * @brief Demonstrates fixing a data race using pthread_mutex_t.
 *
 * This example is identical to 02_data_race.c, but introduces a Mutex
 * to protect the critical section (the increment of the shared counter).
 * With successful mutex operations and completed workers, mutual exclusion
 * ensures the expected sum regardless of timing and context switches.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#define NUM_LOOPS 10000000

// Shared global variable
static int shared_counter = 0;

// Statically initialize the mutex
static pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Thread function that safely increments the global counter in a loop.
 * 
 * @param arg Unused.
 * @return void* Always NULL.
 */
void *increment_counter_safe(void *arg) {
    (void)arg; // Ignore unused parameter warning
    int ret;
    
    for (int i = 0; i < NUM_LOOPS; ++i) {
        // 1. Acquire the lock before entering the critical section
        ret = pthread_mutex_lock(&counter_mutex);
        if (ret != 0) {
            errno = ret;
            perror("pthread_mutex_lock failed");
            // Unexpected with this valid default mutex: terminate the process.
            exit(EXIT_FAILURE);
        }
        
        // --- CRITICAL SECTION START ---
        // Now it is guaranteed that no other thread is executing this block
        shared_counter++;
        // --- CRITICAL SECTION END ---
        
        // 2. Release the lock to allow other threads to enter
        ret = pthread_mutex_unlock(&counter_mutex);
        if (ret != 0) {
            errno = ret;
            perror("pthread_mutex_unlock failed");
            // Unexpected with this valid default mutex: terminate the process.
            exit(EXIT_FAILURE);
        }
    }
    
    return NULL;
}

int main(void) {
    pthread_t t1, t2;
    int ret;
    
    printf("Starting mutex synchronization example...\n");
    printf("Expected final counter value: %d\n", NUM_LOOPS * 2);
    
    // Note: If we needed to initialize the mutex dynamically with specific attributes,
    // we would use pthread_mutex_init(&counter_mutex, NULL) here.
    
    // Create first thread
    ret = pthread_create(&t1, NULL, increment_counter_safe, NULL);
    if (ret != 0) {
        errno = ret;
        perror("pthread_create t1 failed");
        exit(EXIT_FAILURE);
    }
    
    // Create second thread
    ret = pthread_create(&t2, NULL, increment_counter_safe, NULL);
    if (ret != 0) {
        errno = ret;
        perror("pthread_create t2 failed");
        exit(EXIT_FAILURE);
    }
    
    // Wait for both threads to finish
    ret = pthread_join(t1, NULL);
    if (ret != 0) {
        errno = ret;
        perror("pthread_join t1 failed");
        // Completion is unknown; exit the process without destroying shared objects.
        exit(EXIT_FAILURE);
    }
    ret = pthread_join(t2, NULL);
    if (ret != 0) {
        errno = ret;
        perror("pthread_join t2 failed");
        // Completion is unknown; exit the process without destroying shared objects.
        exit(EXIT_FAILURE);
    }
    
    printf("Actual final counter value:   %d\n", shared_counter);
    
    int failed = shared_counter != (NUM_LOOPS * 2);
    if (!failed) {
        printf("RESULT: Success! The Mutex prevented the data race.\n");
    } else {
        printf("RESULT: Error! Something went wrong.\n");
    }
    
    // Destroy the mutex since we're done with it
    pthread_mutex_destroy(&counter_mutex);
    
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
