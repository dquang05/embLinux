/*
 * ============================================================================
 * Project: Advanced POSIX Threads Demonstration
 * ============================================================================
 *
 * Purpose
 * -------
 * This program demonstrates several important POSIX Thread (pthread)
 * concepts working together in a single application:
 *
 * 1. Thread Creation and Joining
 *    - Creating worker threads and producer threads.
 *    - Waiting for thread termination using pthread_join().
 *
 * 2. Mutex Synchronization
 *    - Protecting shared resources from race conditions.
 *
 * 3. Condition Variables
 *    - Implementing a Producer-Consumer model efficiently.
 *    - Avoiding busy waiting by allowing threads to sleep until work arrives.
 *
 * 4. Thread Local Storage (TLS)
 *    - Giving each thread its own private copy of a variable.
 *    - Tracking how many tasks each worker processed independently.
 *
 * 5. Detached Threads
 *    - Running a background monitoring thread that does not need joining.
 *
 * 6. Thread Cancellation
 *    - Demonstrating forceful thread termination using pthread_cancel().
 *
 * 7. Cleanup Handlers
 *    - Preventing deadlocks by automatically releasing mutexes when a
 *      thread is canceled while holding a lock.
 *
 * Architecture
 * ------------
 *
 *                 +------------------+
 *                 |     Producer     |
 *                 +------------------+
 *                          |
 *                          v
 *                    task_queue
 *                          |
 *                          v
 *     +----------+  +----------+  +----------+
 *     | Worker 1 |  | Worker 2 |  | Worker 3 |
 *     +----------+  +----------+  +----------+
 *                          ^
 *                          |
 *                  Condition Variable
 *
 *     +--------------------+
 *     | Detached Monitor   |
 *     +--------------------+
 *
 * Design Thinking
 * ---------------
 *
 * - Shared data (task_queue, producers_done) must be protected by a mutex.
 *
 * - Workers should not continuously poll for work because that wastes CPU.
 *   Instead, they sleep using pthread_cond_wait() until the producer
 *   notifies them.
 *
 * - Worker processing occurs outside the critical section to maximize
 *   concurrency and reduce lock contention.
 *
 * - Cleanup handlers are attached immediately after acquiring the mutex.
 *   This guarantees the mutex will be unlocked even if a thread is canceled
 *   at a cancellation point.
 *
 * - TLS is used so each worker can maintain private statistics without
 *   requiring additional synchronization.
 *
 * ============================================================================
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_WORKERS 3
#define NUM_TASKS 5

// -----------------------------------------------------------------------------
// GLOBAL SHARED RESOURCES
// -----------------------------------------------------------------------------

// Single-slot task queue.
// 0 means empty, positive values represent task IDs.
int task_queue = 0;

// Indicates that the producer has finished generating tasks.
int producers_done = 0;

// Mutex protecting all shared resources.
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

// Condition variable used to wake sleeping workers.
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// -----------------------------------------------------------------------------
// THREAD LOCAL STORAGE (TLS)
// -----------------------------------------------------------------------------

// Each worker owns an independent copy of this variable.
// No synchronization is needed because it is private per thread.
__thread int tasks_completed = 0;

// -----------------------------------------------------------------------------
// CLEANUP HANDLER
// -----------------------------------------------------------------------------

// Automatically unlock the mutex if the thread is canceled.
// Prevents deadlocks caused by abandoned locks.
void cleanup_mutex(void *arg)
{
    printf("[Cleanup] Thread canceled. Unlocking mutex to prevent deadlock.\n");
    pthread_mutex_unlock((pthread_mutex_t *)arg);
}

// -----------------------------------------------------------------------------
// WORKER THREAD (CONSUMER)
// -----------------------------------------------------------------------------

void *worker_func(void *arg) {
    int id = *(int *)arg;
    free(arg); 

    printf("[Worker %d] Started.\n", id);

    while (1) {
        /* Declare variables outside the push/pop macro scope */
        int my_task = 0;
        int should_exit = 0;

        pthread_mutex_lock(&mtx);
        
        /* Push cleanup handler. Intrinsically opens a '{' block */
        pthread_cleanup_push(cleanup_mutex, &mtx);

        /* Wait for a task using Condition Variable */
        while (task_queue == 0 && !producers_done) {
            pthread_cond_wait(&cond, &mtx);
        }

        /* Check conditions and update flags/variables */
        if (producers_done && task_queue == 0) {
            should_exit = 1;
        } else {
            my_task = task_queue;
            task_queue = 0; /* Mark queue as empty */
        }

        /* Pop cleanup handler. Intrinsically closes the '}' block.
         * Executes the handler (unlocks the mutex). */
        pthread_cleanup_pop(1);

        /* Variables should_exit and my_task are safely accessible here */
        if (should_exit) {
            break;
        }

        /* Process the task outside the critical section */
        printf("[Worker %d] Processing task %d...\n", id, my_task);
        usleep(500000); /* Simulate work */
        
        /* Update the Thread-Local variable safely */
        tasks_completed++;
    }

    printf("[Worker %d] Exiting naturally. Processed %d tasks.\n", id, tasks_completed);
    return NULL;
}

// -----------------------------------------------------------------------------
// PRODUCER THREAD
// -----------------------------------------------------------------------------

void *producer_func(void *arg)
{
    for (int i = 1; i <= NUM_TASKS; i++)
    {
        pthread_mutex_lock(&mtx);

        // Wait until queue becomes available.
        while (task_queue != 0)
        {
            pthread_mutex_unlock(&mtx);

            usleep(10000);

            pthread_mutex_lock(&mtx);
        }

        task_queue = i;

        printf("[Producer] Created task %d\n", i);

        // Wake exactly one worker.
        pthread_cond_signal(&cond);

        pthread_mutex_unlock(&mtx);

        usleep(200000);
    }

    pthread_mutex_lock(&mtx);

    // Notify workers that production has ended.
    producers_done = 1;

    // Wake all sleeping workers so they can exit.
    pthread_cond_broadcast(&cond);

    pthread_mutex_unlock(&mtx);

    printf("[Producer] Done producing.\n");

    return NULL;
}

// -----------------------------------------------------------------------------
// DETACHED MONITOR THREAD
// -----------------------------------------------------------------------------

void *monitor_func(void *arg)
{
    while (1)
    {
        sleep(1);

        printf("[Monitor] Heartbeat: System is active.\n");
    }

    return NULL;
}

// -----------------------------------------------------------------------------
// MAIN
// -----------------------------------------------------------------------------

int main(void)
{
    pthread_t workers[NUM_WORKERS];
    pthread_t producer;
    pthread_t monitor;

    // Create detached monitor thread.
    pthread_create(&monitor, NULL, monitor_func, NULL);

    pthread_detach(monitor);

    printf("[Main] Monitor thread detached.\n");

    // Create worker threads.
    for (int i = 0; i < NUM_WORKERS; i++)
    {
        int *id = malloc(sizeof(int));

        *id = i + 1;

        pthread_create(&workers[i], NULL, worker_func, id);
    }

    // Create producer thread.
    pthread_create(&producer, NULL, producer_func, NULL);

    // Wait for producer completion.
    pthread_join(producer, NULL);

    // Demonstrate thread cancellation.
    printf("[Main] Forcefully canceling Worker 3!\n");

    pthread_cancel(workers[2]);

    // Join all workers and verify termination status.
    for (int i = 0; i < NUM_WORKERS; i++)
    {
        void *res;

        pthread_join(workers[i], &res);

        if (res == PTHREAD_CANCELED)
        {
            printf("[Main] Verification: Worker %d was canceled successfully.\n",
                   i + 1);
        }
        else
        {
            printf("[Main] Verification: Worker %d joined normally.\n",
                   i + 1);
        }
    }

    printf("[Main] All resources cleaned up. Exiting program.\n");

    return 0;
}