/**
 * @file 01_pipe_demo.c
 * @brief Demo program for Ordinary Pipes in Linux System Engineering.
 *
 * This program demonstrates how to create an ordinary pipe and use it
 * to communicate between a parent process and its child process.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#define BUFFER_SIZE 256

int main(void) {
    int pfd[2];
    pid_t pid;
    char write_msg[] = "Hello from Parent Process!";
    char read_msg[BUFFER_SIZE];

    /* 1. Create the pipe */
    if (pipe(pfd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    /* 2. Fork a child process */
    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        /* -------------------------------------
         * Parent process: writes to pipe
         * ------------------------------------- */
        close(pfd[0]); /* Close unused read end */

        printf("[Parent] Writing message to pipe...\n");
        if (write(pfd[1], write_msg, strlen(write_msg) + 1) == -1) {
            perror("write failed");
            exit(EXIT_FAILURE);
        }

        /* Essential: close write end so reader sees EOF */
        close(pfd[1]); 
        
        /* Wait for child to finish */
        if (wait(NULL) == -1) {
            perror("wait failed");
            exit(EXIT_FAILURE);
        }
    } else {
        /* -------------------------------------
         * Child process: reads from pipe
         * ------------------------------------- */
        close(pfd[1]); /* Close unused write end */

        printf("[Child]  Reading message from pipe...\n");
        ssize_t num_read = read(pfd[0], read_msg, BUFFER_SIZE);
        
        if (num_read == -1) {
            perror("read failed");
            exit(EXIT_FAILURE);
        } else if (num_read == 0) {
            printf("[Child]  Reached End Of File (EOF).\n");
        } else {
            printf("[Child]  Message received: %s\n", read_msg);
        }

        /* Close read end */
        close(pfd[0]); 
    }

    return 0;
}
