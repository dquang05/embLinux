/**
 * @file 06_posix_sem_demo.c
 * @brief Demo program for POSIX Named Semaphores.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/wait.h>

#define SEM_NAME "/demo_posix_sem"

int main(void) {
    sem_t *sem;
    pid_t pid;

    /* Create a named semaphore, initialize to 1 (available) */
    sem = sem_open(SEM_NAME, O_CREAT, 0644, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        /* PARENT */
        /* Wait/reserve the semaphore */
        sem_wait(sem);
        printf("[Parent] Acquired semaphore. Doing work...\n");
        sleep(1);
        printf("[Parent] Releasing semaphore.\n");
        sem_post(sem);

        wait(NULL);

        /* Clean up */
        sem_close(sem);
        sem_unlink(SEM_NAME);
        printf("[Parent] Cleaned up POSIX Semaphore.\n");
    } else {
        /* CHILD */
        /* Wait/reserve the semaphore */
        sem_wait(sem);
        printf("[Child] Acquired semaphore. Doing work...\n");
        sleep(1);
        printf("[Child] Releasing semaphore.\n");
        sem_post(sem);

        sem_close(sem);
        exit(EXIT_SUCCESS);
    }

    return 0;
}
