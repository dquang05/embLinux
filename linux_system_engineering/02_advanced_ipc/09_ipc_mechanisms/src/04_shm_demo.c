/**
 * @file 04_shm_demo.c
 * @brief Demo program for System V Shared Memory and Semaphores in Linux System Engineering.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
#if defined(__linux__)
    struct seminfo *__buf;
#endif
};

#define SHM_SIZE 1024

/* Semaphore operations wrapper */
int reserve_sem(int semid, int semnum) {
    struct sembuf sops;
    sops.sem_num = semnum;
    sops.sem_op = -1;
    sops.sem_flg = 0;
    return semop(semid, &sops, 1);
}

int release_sem(int semid, int semnum) {
    struct sembuf sops;
    sops.sem_num = semnum;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    return semop(semid, &sops, 1);
}

int main(void) {
    int shmid, semid;
    pid_t pid;
    char *shmp;
    union semun arg;

    /* 1. Create a Shared Memory Segment */
    shmid = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0600);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    /* 2. Create Semaphore Set with 2 semaphores 
     * sem 0: Writer's turn
     * sem 1: Reader's turn
     */
    semid = semget(IPC_PRIVATE, 2, IPC_CREAT | 0600);
    if (semid == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    /* Initialize Writer's semaphore to 1 (Available), Reader's to 0 (In-use) */
    arg.val = 1;
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl writer");
        exit(EXIT_FAILURE);
    }
    
    arg.val = 0;
    if (semctl(semid, 1, SETVAL, arg) == -1) {
        perror("semctl reader");
        exit(EXIT_FAILURE);
    }

    /* 3. Attach the Shared Memory Segment */
    shmp = shmat(shmid, NULL, 0);
    if (shmp == (void *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    /* 4. Fork */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        /* PARENT: WRITER */
        /* Wait for Writer's turn */
        reserve_sem(semid, 0);

        printf("[Writer] Writing to shared memory...\n");
        strncpy(shmp, "Hello from Writer Process via Shared Memory!", SHM_SIZE);

        /* Give Reader a turn */
        release_sem(semid, 1);
        
        wait(NULL); /* Wait for child */

        /* 5. Clean up */
        shmdt(shmp);
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);
        printf("[Writer] Cleaned up IPC resources.\n");
    } else {
        /* CHILD: READER */
        /* Wait for Reader's turn */
        reserve_sem(semid, 1);

        printf("[Reader] Read from shared memory: %s\n", shmp);

        /* Release Writer's turn */
        release_sem(semid, 0);

        shmdt(shmp);
        exit(EXIT_SUCCESS);
    }

    return 0;
}
