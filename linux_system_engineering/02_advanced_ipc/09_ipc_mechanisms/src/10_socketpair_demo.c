/**
 * @file 10_socketpair_demo.c
 * @brief Demonstration of Bidirectional Full-Duplex IPC using socketpair() on Linux.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define BUFFER_SIZE 256

int main(void) {
    int sv[2]; /* sv[0] and sv[1] */

    /* 1. Create a bidirectional pair of connected UNIX domain stream sockets */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
        perror("socketpair");
        exit(EXIT_FAILURE);
    }

    printf("=== Bidirectional Full-Duplex IPC using socketpair() ===\n");

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(sv[0]);
        close(sv[1]);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /* CHILD PROCESS */
        /* Close unused descriptor sv[0] */
        close(sv[0]);

        char buf[BUFFER_SIZE];
        ssize_t n = read(sv[1], buf, sizeof(buf) - 1);
        if (n == -1) {
            perror("[Child] read from sv[1]");
            close(sv[1]);
            exit(EXIT_FAILURE);
        }
        buf[n] = '\0';
        printf("[Child] Received command from Parent: \"%s\"\n", buf);

        /* Process and reply back over the SAME descriptor sv[1] (Full Duplex) */
        const char *reply = "STATUS_OK: Task executed successfully by Child PID.";
        printf("[Child] Sending response back via sv[1]...\n");
        if (write(sv[1], reply, strlen(reply)) == -1) {
            perror("[Child] write to sv[1]");
            close(sv[1]);
            exit(EXIT_FAILURE);
        }

        /* Clean shutdown */
        close(sv[1]);
        exit(EXIT_SUCCESS);
    } else {
        /* PARENT PROCESS */
        /* Close unused descriptor sv[1] */
        close(sv[1]);

        /* 1. Send command to child via sv[0] */
        const char *cmd = "EXECUTE_CALIBRATION_ROUTINE";
        printf("[Parent] Sending command to Child via sv[0]: \"%s\"\n", cmd);
        if (write(sv[0], cmd, strlen(cmd)) == -1) {
            perror("[Parent] write to sv[0]");
            close(sv[0]);
            wait(NULL);
            exit(EXIT_FAILURE);
        }

        /* 2. Read response from child over the SAME descriptor sv[0] */
        char response[BUFFER_SIZE];
        ssize_t n = read(sv[0], response, sizeof(response) - 1);
        if (n == -1) {
            perror("[Parent] read from sv[0]");
            close(sv[0]);
            wait(NULL);
            exit(EXIT_FAILURE);
        }
        response[n] = '\0';
        printf("[Parent] Received response from Child: \"%s\"\n", response);

        close(sv[0]);
        wait(NULL);
        printf("[Parent] Child exited. Demo completed successfully.\n");
    }

    return 0;
}
