/**
 * @file 04_producer_consumer.c
 * @brief Demonstrates the classic Producer-Consumer problem using Mutexes and Condition Variables.
 *
 * A Producer thread generates data items and places them into a shared buffer.
 * A Consumer thread takes data items out of the buffer and processes them.
 * Condition variables are used so the Consumer blocks when the buffer is empty,
 * and the Producer blocks when the buffer is full.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
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
        usleep(100000); // 100ms
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
        usleep(150000); // 150ms (slower than producer to demonstrate buffer filling up)
    }
    
    return NULL;
}

int main(void) {
    pthread_t prod_tid, cons_tid;
    
    printf("Starting Producer-Consumer demonstration...\n");
    
    // Create threads
    if (pthread_create(&prod_tid, NULL, producer, NULL) != 0) {
        perror("Failed to create producer thread");
        exit(EXIT_FAILURE);
    }
    
    if (pthread_create(&cons_tid, NULL, consumer, NULL) != 0) {
        perror("Failed to create consumer thread");
        exit(EXIT_FAILURE);
    }
    
    // Wait for threads to finish
    pthread_join(prod_tid, NULL);
    pthread_join(cons_tid, NULL);
    
    printf("Demonstration finished successfully.\n");
    
    // Clean up
    pthread_mutex_destroy(&mtx);
    pthread_cond_destroy(&cond_not_full);
    pthread_cond_destroy(&cond_not_empty);
    
    return EXIT_SUCCESS;
}
