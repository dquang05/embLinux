/**
 * @file 03_poll_demo.c
 * @brief Demonstrates I/O Multiplexing using poll().
 *
 * This demo shows:
 * 1. How to monitor descriptors using an array of struct pollfd.
 * 2. Separation of requested events (events) and returned events (revents).
 * 3. Handling millisecond-precision timeouts.
 * 4. Detecting hangup/closure events with POLLHUP.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define NUM_PIPES 3

int main(void) {
    int pipes[NUM_PIPES][2];
    struct pollfd fds[NUM_PIPES];
    char buf[128];

    printf("============================================================\n");
    printf("   Demo: I/O Multiplexing with poll()\n");
    printf("============================================================\n\n");

    /* Create 3 pipes */
    for (int i = 0; i < NUM_PIPES; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    /* Spawn child workers to write at different delays */
    for (int i = 0; i < NUM_PIPES; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            /* Child process i: write after (i + 1) seconds */
            for (int j = 0; j < NUM_PIPES; j++) {
                close(pipes[j][0]);
                if (j != i) close(pipes[j][1]);
            }

            sleep(i + 1);
            char msg[64];
            snprintf(msg, sizeof(msg), "Data from Worker %d (Pipe %d)", i + 1, i + 1);
            if (write(pipes[i][1], msg, strlen(msg) + 1) == -1) {
                perror("[Child] write");
            }

            close(pipes[i][1]); /* Triggers POLLHUP on parent */
            _exit(EXIT_SUCCESS);
        }
    }

    /* Parent Process: Configure pollfd array */
    for (int i = 0; i < NUM_PIPES; i++) {
        close(pipes[i][1]); /* Close unused write ends */

        fds[i].fd = pipes[i][0];
        fds[i].events = POLLIN; /* We are interested in incoming data */
        fds[i].revents = 0;     /* Cleared by caller */
    }

    int active_fds = NUM_PIPES;
    printf("[Parent] Monitoring %d pipes using poll()...\n\n", NUM_PIPES);

    while (active_fds > 0) {
        /* Timeout of 1200 milliseconds (1.2 seconds) */
        printf("[Parent] Calling poll() with 1200ms timeout...\n");
        int ready = poll(fds, NUM_PIPES, 1200);

        if (ready == -1) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        } else if (ready == 0) {
            printf("[Parent] --> poll() TIMEOUT! (No descriptors ready in this interval)\n\n");
            continue;
        }

        printf("[Parent] --> poll() woke up! %d descriptor(s) reported events.\n", ready);

        for (int i = 0; i < NUM_PIPES; i++) {
            if (fds[i].fd == -1) continue; /* Already closed */

            if (fds[i].revents & POLLIN) {
                ssize_t n = read(fds[i].fd, buf, sizeof(buf));
                if (n > 0) {
                    printf("[Parent] [Pipe %d] Received: '%s'\n", i + 1, buf);
                } else if (n == 0) {
                    printf("[Parent] [Pipe %d] EOF reached.\n", i + 1);
                    close(fds[i].fd);
                    fds[i].fd = -1; /* Setting fd to -1 tells poll() to ignore this slot */
                    active_fds--;
                }
            }

            if (fds[i].revents & POLLHUP) {
                if (fds[i].fd != -1) {
                    printf("[Parent] [Pipe %d] POLLHUP: Writer closed end.\n", i + 1);
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    active_fds--;
                }
            }
        }
        printf("\n");
    }

    for (int i = 0; i < NUM_PIPES; i++) {
        wait(NULL);
    }

    printf("[Parent] poll() demo completed successfully.\n");
    return 0;
}
