/**
 * @file 07_posix_shm_demo.c
 * @brief Demo program for POSIX Shared Memory and Synchronization.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>

#define SHM_NAME "/demo_posix_shm"
#define SEM_NAME "/demo_posix_shm_sem"
#define SHM_SIZE 1024

int main(void) {
    int fd;
    char *addr;
    sem_t *sem;
    pid_t pid;

    /* Create POSIX Semaphore for synchronization */
    sem = sem_open(SEM_NAME, O_CREAT, 0644, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    /* Create POSIX Shared Memory object */
    fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0644);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    /* Set size of the shared memory object */
    if (ftruncate(fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    /* Map shared memory object */
    addr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        /* PARENT: WRITER */
        sem_wait(sem);
        printf("[Writer] Writing to shared memory...\n");
        snprintf(addr, SHM_SIZE, "Hello from POSIX Shared Memory!");
        sem_post(sem);

        wait(NULL);

        /* Clean up */
        munmap(addr, SHM_SIZE);
        close(fd);
        shm_unlink(SHM_NAME);
        sem_close(sem);
        sem_unlink(SEM_NAME);
        printf("[Writer] Cleaned up POSIX SHM and Semaphore.\n");
    } else {
        /* CHILD: READER */
        sleep(1); /* Give writer a chance to write */
        sem_wait(sem);
        printf("[Reader] Read from shared memory: '%s'\n", addr);
        sem_post(sem);

        munmap(addr, SHM_SIZE);
        close(fd);
        sem_close(sem);
        exit(EXIT_SUCCESS);
    }

    return 0;
}
