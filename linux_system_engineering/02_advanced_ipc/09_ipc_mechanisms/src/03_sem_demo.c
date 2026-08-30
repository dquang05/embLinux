/**
 * @file 03_sem_demo.c
 * @brief Demo program for System V Semaphores as Binary Mutex in Linux System Engineering.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <errno.h>

/* Custom union semun is required by SUSv3 for semctl() */
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
#if defined(__linux__)
    struct seminfo *__buf;
#endif
};

int main(void) {
    int semid;
    pid_t pid;
    union semun arg;
    struct sembuf sops;

    /* 1. Create a Semaphore Set with 1 semaphore */
    semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (semid == -1) {
        perror("semget failed");
        exit(EXIT_FAILURE);
    }
    printf("[Main] Created semaphore set ID: %d\n", semid);

    /* 2. Initialize Binary Semaphore to 1 (Available) */
    arg.val = 1;
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl SETVAL failed");
        exit(EXIT_FAILURE);
    }
    printf("[Main] Initialized semaphore to 1 (Free).\n");

    /* 3. Fork a child process */
    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /* -------------------------------------
         * Child Process
         * ------------------------------------- */
        printf("[Child]  Attempting to reserve (lock) semaphore...\n");
        sops.sem_num = 0;
        sops.sem_op = -1; /* Decrement by 1 */
        sops.sem_flg = SEM_UNDO;

        if (semop(semid, &sops, 1) == -1) {
            perror("semop failed");
            exit(EXIT_FAILURE);
        }
        printf("[Child]  Semaphore reserved. Doing work (simulated 2s)...\n");
        sleep(2);
        
        printf("[Child]  Releasing (unlocking) semaphore...\n");
        sops.sem_op = 1; /* Increment by 1 */
        if (semop(semid, &sops, 1) == -1) {
            perror("semop failed");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    } else {
        /* -------------------------------------
         * Parent Process
         * ------------------------------------- */
        /* Let child lock it first to demonstrate blocking */
        sleep(1);
        printf("[Parent] Attempting to reserve (lock) semaphore...\n");
        
        sops.sem_num = 0;
        sops.sem_op = -1;
        sops.sem_flg = SEM_UNDO;
        
        /* This will block until child releases the semaphore */
        if (semop(semid, &sops, 1) == -1) {
            perror("semop failed");
            exit(EXIT_FAILURE);
        }
        printf("[Parent] Semaphore reserved. Work is done.\n");

        /* Parent unlocks */
        sops.sem_op = 1;
        if (semop(semid, &sops, 1) == -1) {
            perror("semop failed");
        }

        /* Wait for child */
        if (wait(NULL) == -1) {
            perror("wait failed");
        }

        /* 4. Delete semaphore */
        if (semctl(semid, 0, IPC_RMID) == -1) {
            perror("semctl IPC_RMID failed");
            exit(EXIT_FAILURE);
        }
        printf("[Parent] Deleted semaphore set.\n");
    }

    return 0;
}
