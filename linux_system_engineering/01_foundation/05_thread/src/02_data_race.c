/**
 * @file 02_data_race.c
 * @brief Demonstrates a data race when multiple threads access shared data without synchronization.
 *
 * This example spawns two threads that both repeatedly increment a global variable.
 * Due to the lack of synchronization (e.g., a mutex), the final value of the
 * global variable will likely be less than the expected total sum because of
 * race conditions during the read-modify-write CPU instructions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#define NUM_LOOPS 10000000

// Shared global variable
static int shared_counter = 0;

/**
 * @brief Thread function that increments the global counter in a loop.
 * 
 * @param arg Unused.
 * @return void* Always NULL.
 */
void *increment_counter(void *arg) {
    (void)arg; // Ignore unused parameter warning
    
    for (int i = 0; i < NUM_LOOPS; ++i) {
        // DATA RACE HERE!
        // The increment operation (shared_counter++) is not atomic.
        // It consists of three steps:
        // 1. Read shared_counter from memory to a CPU register
        // 2. Increment the register
        // 3. Write the register back to memory
        // If a context switch happens between these steps, increments can be lost.
        shared_counter++;
    }
    
    return NULL;
}

int main(void) {
    pthread_t t1, t2;
    int ret;
    
    printf("Starting data race example...\n");
    printf("Expected final counter value: %d\n", NUM_LOOPS * 2);
    
    // Create first thread
    ret = pthread_create(&t1, NULL, increment_counter, NULL);
    if (ret != 0) {
        errno = ret;
        perror("pthread_create t1 failed");
        exit(EXIT_FAILURE);
    }
    
    // Create second thread
    ret = pthread_create(&t2, NULL, increment_counter, NULL);
    if (ret != 0) {
        errno = ret;
        perror("pthread_create t2 failed");
        exit(EXIT_FAILURE);
    }
    
    // Wait for both threads to finish
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    printf("Actual final counter value:   %d\n", shared_counter);
    
    if (shared_counter != (NUM_LOOPS * 2)) {
        printf("RESULT: A Data Race occurred! Increments were lost.\n");
    } else {
        printf("RESULT: By pure luck, no data race was visible this time.\n");
    }
    
    return EXIT_SUCCESS;
}
