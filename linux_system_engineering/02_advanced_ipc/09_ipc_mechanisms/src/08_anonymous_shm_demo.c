/**
 * @file 08_anonymous_shm_demo.c
 * @brief Demonstration of Anonymous Shared Memory, POSIX Process-Shared Semaphores,
 *        and Copy-On-Write (COW) using mmap().
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>

#define BUFFER_SIZE 256

/**
 * @struct shared_data_t
 * @brief Structure residing inside shared anonymous memory.
 */
typedef struct {
    sem_t sem_reader;       /**< Signaled when child writes data */
    sem_t sem_writer;       /**< Signaled when parent reads data */
    char message[BUFFER_SIZE]; /**< Data payload */
} shared_data_t;

int main(void) {
    /* 1. Allocate Anonymous SHARED memory (MAP_SHARED | MAP_ANONYMOUS) */
    shared_data_t *shm = mmap(NULL, sizeof(shared_data_t),
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED | MAP_ANONYMOUS,
                              -1, 0);
    if (shm == MAP_FAILED) {
        perror("mmap shared");
        exit(EXIT_FAILURE);
    }

    /* 2. Allocate Anonymous PRIVATE memory to demonstrate Copy-On-Write (COW) */
    char *cow_mem = mmap(NULL, BUFFER_SIZE,
                         PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS,
                         -1, 0);
    if (cow_mem == MAP_FAILED) {
        perror("mmap private");
        munmap(shm, sizeof(shared_data_t));
        exit(EXIT_FAILURE);
    }

    /* Initialize COW page with initial string */
    snprintf(cow_mem, BUFFER_SIZE, "Original Unmodified COW Data");

    /* 3. Initialize POSIX Unnamed Semaphores inside shared memory with pshared = 1 */
    if (sem_init(&shm->sem_reader, 1, 0) == -1) {
        perror("sem_init sem_reader");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&shm->sem_writer, 1, 1) == -1) {
        perror("sem_init sem_writer");
        exit(EXIT_FAILURE);
    }

    printf("=== Anonymous Shared Memory & Copy-On-Write Demo ===\n");
    printf("[Main] Pre-fork COW Memory: '%s'\n\n", cow_mem);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /* CHILD PROCESS */
        /* Wait for permission to write to shared memory */
        if (sem_wait(&shm->sem_writer) == -1) {
            perror("sem_wait");
            exit(EXIT_FAILURE);
        }

        printf("[Child] Writing to Shared Anonymous Memory...\n");
        snprintf(shm->message, BUFFER_SIZE, "Greetings from Child via Anonymous SHM!");

        printf("[Child] Modifying Private Anonymous (COW) Memory...\n");
        snprintf(cow_mem, BUFFER_SIZE, "Child has privately overwritten this page!");
        printf("[Child] Child's COW view: '%s'\n", cow_mem);

        /* Signal reader that data is ready */
        if (sem_post(&shm->sem_reader) == -1) {
            perror("sem_post");
            exit(EXIT_FAILURE);
        }

        /* Unmap child's copies and exit */
        munmap(cow_mem, BUFFER_SIZE);
        munmap(shm, sizeof(shared_data_t));
        exit(EXIT_SUCCESS);
    } else {
        /* PARENT PROCESS */
        /* Wait for child to produce data in shared memory */
        if (sem_wait(&shm->sem_reader) == -1) {
            perror("sem_wait");
            exit(EXIT_FAILURE);
        }

        printf("\n[Parent] Reading from Shared Anonymous Memory: '%s'\n", shm->message);

        /* Check the private memory page - it must remain unchanged due to COW! */
        printf("[Parent] Checking Private Anonymous (COW) Memory: '%s'\n", cow_mem);
        if (strcmp(cow_mem, "Original Unmodified COW Data") == 0) {
            printf("[Parent] -> SUCCESS: COW isolated the child's write from the parent!\n");
        } else {
            printf("[Parent] -> UNEXPECTED: Private memory was altered!\n");
        }

        /* Allow writer turn */
        sem_post(&shm->sem_writer);

        /* Wait for child process to terminate */
        wait(NULL);

        /* Clean up resources */
        sem_destroy(&shm->sem_reader);
        sem_destroy(&shm->sem_writer);
        munmap(cow_mem, BUFFER_SIZE);
        munmap(shm, sizeof(shared_data_t));
        printf("\n[Parent] Cleaned up semaphores and unmapped all memory.\n");
    }

    return 0;
}
