/**
 * @file 01_blocking_vs_nonblocking.c
 * @brief Demonstrates the difference between Blocking and Non-blocking I/O using pipes.
 *
 * This demo shows:
 * 1. Default blocking behavior of a pipe (read() blocks until data arrives).
 * 2. Toggling the O_NONBLOCK flag on a file descriptor using fcntl().
 * 3. Non-blocking read() immediately returning -1 with errno set to EAGAIN/EWOULDBLOCK.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

/**
 * @brief Sets or clears the O_NONBLOCK flag on a file descriptor.
 * @param fd The target file descriptor.
 * @param nonblocking 1 to enable non-blocking, 0 to disable.
 * @return 0 on success, -1 on failure.
 */
static int set_nonblocking(int fd, int nonblocking) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL)");
        return -1;
    }

    if (nonblocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }

    if (fcntl(fd, F_SETFL, flags) == -1) {
        perror("fcntl(F_SETFL)");
        return -1;
    }
    return 0;
}

int main(void) {
    int pipefd[2];
    char buffer[128];

    printf("============================================================\n");
    printf("   Demo: Blocking vs. Non-blocking I/O using Pipes\n");
    printf("============================================================\n\n");

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /* Child Process (Writer) */
        close(pipefd[0]); /* Close unused read end */

        printf("[Child] Sleeping for 2 seconds before writing...\n");
        sleep(2);

        const char *msg1 = "Hello from Child (Blocked Phase)!";
        printf("[Child] Writing: '%s'\n", msg1);
        if (write(pipefd[1], msg1, strlen(msg1) + 1) == -1) {
            perror("[Child] write");
        }

        sleep(2);
        const char *msg2 = "Hello again (Non-blocking Phase)!";
        printf("[Child] Writing second message: '%s'\n", msg2);
        if (write(pipefd[1], msg2, strlen(msg2) + 1) == -1) {
            perror("[Child] write");
        }

        close(pipefd[1]);
        _exit(EXIT_SUCCESS);
    }

    /* Parent Process (Reader) */
    close(pipefd[1]); /* Close unused write end */

    /* ----------------------------------------------------------------- */
    /* Phase 1: Blocking I/O (Default)                                   */
    /* ----------------------------------------------------------------- */
    printf("[Parent] Phase 1: Reading in BLOCKING mode (default)...\n");
    printf("[Parent] Calling read(). Process will sleep until child writes...\n");

    ssize_t bytes_read = read(pipefd[0], buffer, sizeof(buffer));
    if (bytes_read == -1) {
        perror("[Parent] read failed");
    } else {
        printf("[Parent] Successfully received %zd bytes: '%s'\n\n", bytes_read, buffer);
    }

    /* ----------------------------------------------------------------- */
    /* Phase 2: Non-blocking I/O (O_NONBLOCK)                            */
    /* ----------------------------------------------------------------- */
    printf("[Parent] Phase 2: Enabling O_NONBLOCK on read descriptor...\n");
    if (set_nonblocking(pipefd[0], 1) == -1) {
        close(pipefd[0]);
        exit(EXIT_FAILURE);
    }

    printf("[Parent] Attempting to read immediately when no data is ready...\n");
    bytes_read = read(pipefd[0], buffer, sizeof(buffer));
    if (bytes_read == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("[Parent] Expected: read() returned -1 with EAGAIN / EWOULDBLOCK (Resource temporarily unavailable)!\n");
            printf("[Parent] The thread was NOT put to sleep.\n");
        } else {
            perror("[Parent] Unexpected read error");
        }
    }

    /* Polling loop demonstration */
    printf("[Parent] Polling the non-blocking pipe every 500ms until child writes...\n");
    int poll_count = 0;
    while (1) {
        poll_count++;
        bytes_read = read(pipefd[0], buffer, sizeof(buffer));
        if (bytes_read > 0) {
            printf("[Parent] Attempt #%d: Data arrived! Read %zd bytes: '%s'\n",
                   poll_count, bytes_read, buffer);
            break;
        } else if (bytes_read == 0) {
            printf("[Parent] Pipe closed by child (EOF).\n");
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("[Parent] Attempt #%d: Still no data (EAGAIN). Doing other work...\n", poll_count);
                usleep(500000); /* Sleep 500ms */
            } else {
                perror("[Parent] read error");
                break;
            }
        }
    }

    close(pipefd[0]);
    wait(NULL); /* Wait for child process termination */

    printf("\n[Parent] Blocking vs Non-blocking demo finished cleanly.\n");
    return 0;
}
