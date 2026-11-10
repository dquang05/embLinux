/**
 * @file 04_producer_consumer.c
 * @brief Demonstrates the classic Producer-Consumer problem using Mutexes and Condition Variables.
 *
 * A Producer thread generates data items and places them into a shared buffer.
 * A Consumer thread takes data items out of the buffer and processes them.
 * Condition variables are used so the Consumer blocks when the buffer is empty,
 * and the Producer blocks when the buffer is full.
 * Finite, non-canceling teaching example: one producer and one consumer,
 * each handling the same fixed number of items.
 */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define BUFFER_SIZE 5
#define ITEMS_TO_PRODUCE 15

// Shared Buffer
static int buffer[BUFFER_SIZE];
static int count = 0;  // Number of items currently in the buffer
static int in = 0;     // Index where producer will insert the next item
static int out = 0;    // Index where consumer will extract the next item

// Synchronization primitives
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_not_full = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_not_empty = PTHREAD_COND_INITIALIZER;

/* Complete each simulated delay after EINTR; scheduling can extend elapsed time. */
static void simulate_delay(long nanoseconds) {
    struct timespec remaining = {.tv_sec = 0, .tv_nsec = nanoseconds};
    while (nanosleep(&remaining, &remaining) == -1) {
        if (errno != EINTR) {
            perror("nanosleep failed");
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * @brief Producer thread function.
 */
void *producer(void *arg) {
    (void)arg;
    
    for (int i = 1; i <= ITEMS_TO_PRODUCE; ++i) {
        // Lock the mutex before checking the condition
        pthread_mutex_lock(&mtx);
        
        // Wait while the buffer is full
        // MUST use a while-loop to handle spurious wakeups
        while (count == BUFFER_SIZE) {
            printf("[Producer] Buffer FULL. Waiting...\n");
            pthread_cond_wait(&cond_not_full, &mtx);
        }
        
        // --- CRITICAL SECTION START ---
        buffer[in] = i; // Produce item
        printf("[Producer] Produced item: %d at index %d\n", i, in);
        in = (in + 1) % BUFFER_SIZE;
        count++;
        // --- CRITICAL SECTION END ---
        
        // Signal the consumer that the buffer is no longer empty
        pthread_cond_signal(&cond_not_empty);
        
        // Release the mutex
        pthread_mutex_unlock(&mtx);
        
        // Simulate production time
        simulate_delay(100000000L); // 100ms
    }
    
    return NULL;
}

/**
 * @brief Consumer thread function.
 */
void *consumer(void *arg) {
    (void)arg;
    
    for (int i = 1; i <= ITEMS_TO_PRODUCE; ++i) {
        // Lock the mutex before checking the condition
        pthread_mutex_lock(&mtx);
        
        // Wait while the buffer is empty
        while (count == 0) {
            printf("[Consumer] Buffer EMPTY. Waiting...\n");
            pthread_cond_wait(&cond_not_empty, &mtx);
        }
        
        // --- CRITICAL SECTION START ---
        int item = buffer[out]; // Consume item
        printf("[Consumer] Consumed item: %d from index %d\n", item, out);
        out = (out + 1) % BUFFER_SIZE;
        count--;
        // --- CRITICAL SECTION END ---
        
        // Signal the producer that the buffer is no longer full
        pthread_cond_signal(&cond_not_full);
        
        // Release the mutex
        pthread_mutex_unlock(&mtx);
        
        // Simulate processing time
        simulate_delay(150000000L); // 150ms (slower than producer to demonstrate buffer filling up)
    }
    
    return NULL;
}

int main(void) {
    pthread_t prod_tid, cons_tid;
    
    printf("Starting Producer-Consumer demonstration...\n");
    
    // On startup failure, terminate the process; a lone producer may block.
    int ret = pthread_create(&prod_tid, NULL, producer, NULL);
    if (ret != 0) {
        fprintf(stderr, "Failed to create producer thread: %s\n", strerror(ret));
        exit(EXIT_FAILURE);
    }
    
    ret = pthread_create(&cons_tid, NULL, consumer, NULL);
    if (ret != 0) {
        fprintf(stderr, "Failed to create consumer thread: %s\n", strerror(ret));
        exit(EXIT_FAILURE);
    }
    
    // Wait for threads to finish
    ret = pthread_join(prod_tid, NULL);
    if (ret != 0) {
        errno = ret;
        perror("pthread_join prod_tid failed");
        // Completion is unknown; exit the process without destroying shared objects.
        exit(EXIT_FAILURE);
    }
    ret = pthread_join(cons_tid, NULL);
    if (ret != 0) {
        errno = ret;
        perror("pthread_join cons_tid failed");
        // Completion is unknown; exit the process without destroying shared objects.
        exit(EXIT_FAILURE);
    }
    
    printf("Demonstration finished successfully.\n");
    
    // Clean up
    pthread_mutex_destroy(&mtx);
    pthread_cond_destroy(&cond_not_full);
    pthread_cond_destroy(&cond_not_empty);
    
    return EXIT_SUCCESS;
}
