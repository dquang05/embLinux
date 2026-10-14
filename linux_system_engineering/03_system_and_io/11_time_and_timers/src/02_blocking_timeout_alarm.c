/**
 * @file 02_blocking_timeout_alarm.c
 * @brief Demonstration of enforcing timeouts on blocking system calls using alarm().
 *
 * This program demonstrates:
 * 1. Setting up a signal handler for SIGALRM with SA_RESTART disabled so that
 *    blocking system calls are interrupted with EINTR instead of automatically restarted.
 * 2. Arming a timer before a blocking read() and catching the timeout.
 * 3. Safely disarming the timer with alarm(0) when data arrives before deadline.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

static volatile sig_atomic_t g_alarm_triggered = 0;

/**
 * @brief Signal handler for SIGALRM.
 * @param sig Signal number (SIGALRM).
 */
static void sigalrm_handler(int sig) {
    (void)sig;
    g_alarm_triggered = 1;
    /* Avoid non-async-signal-safe functions like printf inside production handlers */
    const char msg[] = " [SIGNAL HANDLER] SIGALRM caught! Interrupting system call.\n";
    (void)write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

int main(void) {
    printf("====================================================\n");
    printf(" Demo 02: Blocking I/O Timeout with alarm() & EINTR\n");
    printf("====================================================\n\n");

    /* ---------------------------------------------------------
     * 1. Configure SIGALRM handler without SA_RESTART
     * --------------------------------------------------------- */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigalrm_handler;
    sa.sa_flags   = 0; /* CRITICAL: SA_RESTART is 0 so blocking syscall returns EINTR */
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction(SIGALRM)");
        return EXIT_FAILURE;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    char buf[128];

    /* ---------------------------------------------------------
     * 2. Scenario A: Timeout Triggers on Empty Pipe Read
     * --------------------------------------------------------- */
    printf("[Case 1] Reading from an empty pipe with 1-second timeout...\n");
    g_alarm_triggered = 0;
    alarm(1); /* Arm 1-second timer */

    ssize_t bytes_read = read(pipefd[0], buf, sizeof(buf));
    int saved_errno = errno;
    alarm(0); /* Cancel pending alarm */

    if (bytes_read == -1) {
        if (saved_errno == EINTR) {
            printf(" -> RESULT: read() failed with EINTR (Timeout successfully enforced!)\n");
            printf("    Alarm triggered flag: %d\n\n", g_alarm_triggered);
        } else {
            fprintf(stderr, "read error: %s\n", strerror(saved_errno));
        }
    } else {
        printf(" -> Unexpected read completion: %zd bytes\n", bytes_read);
    }

    /* ---------------------------------------------------------
     * 3. Scenario B: Read Completes Before Timeout
     * --------------------------------------------------------- */
    printf("[Case 2] Writing data to pipe first, then reading with 2-second timeout...\n");
    const char payload[] = "Hello Embedded Linux Timer!";
    if (write(pipefd[1], payload, sizeof(payload)) == -1) {
        perror("write to pipe");
        close(pipefd[0]);
        close(pipefd[1]);
        return EXIT_FAILURE;
    }

    g_alarm_triggered = 0;
    alarm(2); /* Arm 2-second timeout */

    bytes_read = read(pipefd[0], buf, sizeof(buf));
    saved_errno = errno;
    alarm(0); /* Cancel timer immediately */

    if (bytes_read > 0) {
        printf(" -> RESULT: Read %zd bytes successfully before timeout: \"%s\"\n",
               bytes_read, buf);
        printf("    Alarm triggered flag: %d (Timer disarmed cleanly)\n",
               g_alarm_triggered);
    } else {
        fprintf(stderr, "read error: %s\n", strerror(saved_errno));
    }

    close(pipefd[0]);
    close(pipefd[1]);

    printf("\nDemo 02 completed successfully.\n");
    return EXIT_SUCCESS;
}
