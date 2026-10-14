/**
 * @file 04_timerfd_epoll_loop.c
 * @brief Demonstration of integrating Linux timerfd into an epoll event loop.
 *
 * This program demonstrates:
 * 1. Creating a monotonic timer as a file descriptor via timerfd_create().
 * 2. Configuring non-blocking periodic expirations via timerfd_settime().
 * 3. Registering the timerfd into an epoll instance (epoll_ctl) and handling
 *    ticks synchronously inside epoll_wait() without any signal handlers or worker threads.
 * 4. Reading the 8-byte uint64_t expiration counter to detect accumulated ticks.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <time.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#define MAX_EVENTS 8
#define TOTAL_TICKS 5

int main(void) {
    printf("====================================================\n");
    printf(" Demo 04: Linux timerfd Multiplexed in epoll Event Loop\n");
    printf("====================================================\n\n");

    /* ---------------------------------------------------------
     * 1. Create timerfd on CLOCK_MONOTONIC with TFD_NONBLOCK & TFD_CLOEXEC
     * --------------------------------------------------------- */
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (tfd == -1) {
        perror("timerfd_create");
        return EXIT_FAILURE;
    }
    printf("[1] Created timerfd (fd = %d)\n", tfd);

    /* ---------------------------------------------------------
     * 2. Arm timerfd: Initial delay 200ms, recurring every 200ms
     * --------------------------------------------------------- */
    struct itimerspec its;
    /* Initial expiration */
    its.it_value.tv_sec     = 0;
    its.it_value.tv_nsec    = 200000000L; /* 200 ms */
    /* Periodic interval */
    its.it_interval.tv_sec  = 0;
    its.it_interval.tv_nsec = 200000000L; /* 200 ms */

    if (timerfd_settime(tfd, 0, &its, NULL) == -1) {
        perror("timerfd_settime");
        close(tfd);
        return EXIT_FAILURE;
    }
    printf("[2] Armed timerfd: fire in 200ms, period = 200ms\n");

    /* ---------------------------------------------------------
     * 3. Create epoll instance and register timerfd
     * --------------------------------------------------------- */
    int epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd == -1) {
        perror("epoll_create1");
        close(tfd);
        return EXIT_FAILURE;
    }

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events   = EPOLLIN;
    ev.data.fd  = tfd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &ev) == -1) {
        perror("epoll_ctl(EPOLL_CTL_ADD timerfd)");
        close(tfd);
        close(epfd);
        return EXIT_FAILURE;
    }
    printf("[3] Added timerfd to epoll instance (epfd = %d)\n\n", epfd);

    /* ---------------------------------------------------------
     * 4. epoll Event Loop
     * --------------------------------------------------------- */
    struct timespec loop_start, now;
    if (clock_gettime(CLOCK_MONOTONIC, &loop_start) == -1) {
        perror("clock_gettime");
    }

    struct epoll_event events[MAX_EVENTS];
    int ticks_received = 0;

    printf("[4] Starting epoll_wait() loop (waiting for %d ticks)...\n", TOTAL_TICKS);

    while (ticks_received < TOTAL_TICKS) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR) {
                continue; /* Interrupted by signal, retry */
            }
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == tfd && (events[i].events & EPOLLIN)) {
                uint64_t expirations = 0;
                /* Mandatory: read 8 bytes to consume the timer event */
                ssize_t s = read(tfd, &expirations, sizeof(uint64_t));
                if (s == sizeof(uint64_t)) {
                    ticks_received += (int)expirations;

                    clock_gettime(CLOCK_MONOTONIC, &now);
                    double elapsed = (double)(now.tv_sec - loop_start.tv_sec) +
                                     (double)(now.tv_nsec - loop_start.tv_nsec) / 1e9;

                    printf(" -> [TICK #%d] Elapsed: %6.3fs | Expirations read: %llu (Total: %d)\n",
                           ticks_received, elapsed, (unsigned long long)expirations, ticks_received);
                } else if (s == -1 && errno != EAGAIN) {
                    perror("read(timerfd)");
                }
            }
        }
    }

    /* ---------------------------------------------------------
     * 5. Clean Resource Release
     * --------------------------------------------------------- */
    close(tfd);
    close(epfd);

    printf("\nAll %d ticks processed. Cleaned up descriptors successfully.\n", TOTAL_TICKS);
    return EXIT_SUCCESS;
}
