#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/timerfd.h>
#include <poll.h>
#include <time.h>

int main() {
    int tfd;
    struct itimerspec ts;
    struct pollfd fds[2];
    uint64_t expirations;
    char buffer[1024];

    /* 1. Create a modern timerfd using Monotonic Clock */
    tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (tfd == -1) {
        perror("timerfd_create");
        exit(EXIT_FAILURE);
    }

    /* 2. Configure timer: First tick in 2 seconds, then repeat every 3 seconds */
    ts.it_value.tv_sec = 2;
    ts.it_value.tv_nsec = 0;
    ts.it_interval.tv_sec = 3;
    ts.it_interval.tv_nsec = 0;

    if (timerfd_settime(tfd, 0, &ts, NULL) == -1) {
        perror("timerfd_settime");
        exit(EXIT_FAILURE);
    }

    /* 3. Setup multiplexed I/O monitoring via poll() */
    // Monitor standard input (Keyboard)
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    // Monitor the modern timer file descriptor
    fds[1].fd = tfd;
    fds[1].events = POLLIN;

    printf("Event Loop Started! Monitoring Keyboard and 3-second Timer simultaneously...\n");

    /* 4. Main Event Loop */
    for (;;) {
        // poll() blocks until either the user types something OR the timer ticks (-1 means no timeout)
        int ready = poll(fds, 2, -1);
        if (ready == -1) {
            perror("poll");
            break;
        }

        /* Check Event on Channel 0: Keyboard Input */
        if (fds[0].revents & POLLIN) {
            fgets(buffer, sizeof(buffer), stdin);
            printf("[User Input Detected]: %s", buffer);
            if (buffer[0] == 'q') {
                printf("Exiting Event Loop...\n");
                break;
            }
        }

        /* Check Event on Channel 1: Modern Timer Expiration */
        if (fds[1].revents & POLLIN) {
            if (read(tfd, &expirations, sizeof(uint64_t)) == -1) {
                perror("read timerfd");
                break;
            }
            printf("[Timer Event]: Clock ticked! Missed/Current cycles: %llu\n", 
                   (unsigned long long)expirations);
        }
    }

    /* 5. Clean up resources */
    close(tfd);
    return 0;
}