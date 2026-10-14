/**
 * @file 02_select_demo.c
 * @brief Demonstrates I/O Multiplexing using select().
 *
 * This demo shows:
 * 1. How to monitor multiple descriptors simultaneously using fd_set.
 * 2. How to use FD_ZERO, FD_SET, and FD_ISSET macros.
 * 3. Handling timeouts with struct timeval.
 * 4. The value-result nature of select() requiring re-initialization on each loop iteration.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int main(void) {
    int pipe1[2];
    int pipe2[2];
    char buf[128];

    printf("============================================================\n");
    printf("   Demo: I/O Multiplexing with select()\n");
    printf("============================================================\n\n");

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork 1");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) {
        /* Child 1: Writes to pipe1 after 1 second */
        close(pipe1[0]);
        close(pipe2[0]);
        close(pipe2[1]);

        sleep(1);
        const char *msg = "Message from Producer 1 (Pipe 1)";
        if (write(pipe1[1], msg, strlen(msg) + 1) == -1) {
            perror("[Child 1] write");
        }
        close(pipe1[1]);
        _exit(EXIT_SUCCESS);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork 2");
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) {
        /* Child 2: Writes to pipe2 after 3 seconds */
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);

        sleep(3);
        const char *msg = "Message from Producer 2 (Pipe 2)";
        if (write(pipe2[1], msg, strlen(msg) + 1) == -1) {
            perror("[Child 2] write");
        }
        close(pipe2[1]);
        _exit(EXIT_SUCCESS);
    }

    /* Parent Process: Monitors both pipe1[0] and pipe2[0] */
    close(pipe1[1]);
    close(pipe2[1]);

    int p1_open = 1;
    int p2_open = 1;

    printf("[Parent] Monitoring Pipe 1 (fd %d) and Pipe 2 (fd %d)...\n\n",
           pipe1[0], pipe2[0]);

    while (p1_open || p2_open) {
        fd_set read_fds;
        FD_ZERO(&read_fds);

        int max_fd = -1;
        if (p1_open) {
            FD_SET(pipe1[0], &read_fds);
            max_fd = MAX(max_fd, pipe1[0]);
        }
        if (p2_open) {
            FD_SET(pipe2[0], &read_fds);
            max_fd = MAX(max_fd, pipe2[0]);
        }

        /* 
         * Set timeout to 1.5 seconds.
         * NOTE: select() modifies timeval on Linux, so we must reset it each loop!
         */
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 500000; /* 1.5s */

        printf("[Parent] Calling select() with 1.5s timeout...\n");
        int ready = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);

        if (ready == -1) {
            if (errno == EINTR) continue; /* Interrupted by signal */
            perror("select");
            break;
        } else if (ready == 0) {
            printf("[Parent] --> select() TIMEOUT! (No descriptors ready in this interval)\n\n");
            continue;
        }

        printf("[Parent] --> select() woke up! %d descriptor(s) ready.\n", ready);

        /* Inspect Pipe 1 */
        if (p1_open && FD_ISSET(pipe1[0], &read_fds)) {
            ssize_t n = read(pipe1[0], buf, sizeof(buf));
            if (n > 0) {
                printf("[Parent] Read from Pipe 1: '%s'\n", buf);
            } else if (n == 0) {
                printf("[Parent] Pipe 1 closed (EOF).\n");
                close(pipe1[0]);
                p1_open = 0;
            }
        }

        /* Inspect Pipe 2 */
        if (p2_open && FD_ISSET(pipe2[0], &read_fds)) {
            ssize_t n = read(pipe2[0], buf, sizeof(buf));
            if (n > 0) {
                printf("[Parent] Read from Pipe 2: '%s'\n", buf);
            } else if (n == 0) {
                printf("[Parent] Pipe 2 closed (EOF).\n");
                close(pipe2[0]);
                p2_open = 0;
            }
        }
        printf("\n");
    }

    wait(NULL);
    wait(NULL);

    printf("[Parent] select() demo completed successfully.\n");
    return 0;
}
