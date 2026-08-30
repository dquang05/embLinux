/**
 * @file 02_msgq_demo.c
 * @brief Demo program for System V Message Queues in Linux System Engineering.
 *
 * This program demonstrates how to create a message queue, send messages
 * with different types, selectively receive them, and finally delete the queue.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_MTEXT 1024

/* Define the message structure */
struct mbuf {
    long mtype;
    char mtext[MAX_MTEXT];
};

int main(void) {
    int msqid;
    pid_t pid;
    struct mbuf msg;

    /* 1. Create a System V Message Queue
     * IPC_PRIVATE guarantees a unique key for this queue
     */
    msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    if (msqid == -1) {
        perror("msgget failed");
        exit(EXIT_FAILURE);
    }
    printf("[Main] Created message queue with ID: %d\n", msqid);

    /* 2. Fork a child process */
    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        /* -------------------------------------
         * Parent process: sends messages
         * ------------------------------------- */
        
        /* Send a message of type 10 */
        msg.mtype = 10;
        strncpy(msg.mtext, "Hello from Parent (Type 10)!", MAX_MTEXT);
        printf("[Parent] Sending message type 10...\n");
        if (msgsnd(msqid, &msg, strlen(msg.mtext) + 1, 0) == -1) {
            perror("msgsnd failed");
        }

        /* Send a message of type 20 */
        msg.mtype = 20;
        strncpy(msg.mtext, "Hello from Parent (Type 20)!", MAX_MTEXT);
        printf("[Parent] Sending message type 20...\n");
        if (msgsnd(msqid, &msg, strlen(msg.mtext) + 1, 0) == -1) {
            perror("msgsnd failed");
        }

        /* Wait for child to finish reading */
        if (wait(NULL) == -1) {
            perror("wait failed");
            exit(EXIT_FAILURE);
        }

        /* 3. Delete the message queue */
        printf("[Parent] Deleting message queue...\n");
        if (msgctl(msqid, IPC_RMID, NULL) == -1) {
            perror("msgctl IPC_RMID failed");
            exit(EXIT_FAILURE);
        }
    } else {
        /* -------------------------------------
         * Child process: receives messages
         * ------------------------------------- */
        
        /* Receive message of type 20 first (demonstrating selective read) */
        printf("[Child]  Waiting for message of type 20...\n");
        if (msgrcv(msqid, &msg, MAX_MTEXT, 20, 0) == -1) {
            perror("msgrcv failed");
            exit(EXIT_FAILURE);
        }
        printf("[Child]  Received: %s\n", msg.mtext);

        /* Receive message of type 10 */
        printf("[Child]  Waiting for message of type 10...\n");
        if (msgrcv(msqid, &msg, MAX_MTEXT, 10, 0) == -1) {
            perror("msgrcv failed");
            exit(EXIT_FAILURE);
        }
        printf("[Child]  Received: %s\n", msg.mtext);
    }

    return 0;
}
