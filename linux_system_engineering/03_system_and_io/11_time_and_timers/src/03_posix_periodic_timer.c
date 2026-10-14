/**
 * @file 03_posix_periodic_timer.c
 * @brief Demonstration of POSIX.1b Real-Time Interval Timers (timer_create).
 *
 * This program demonstrates:
 * 1. Creating a high-precision timer backed by CLOCK_MONOTONIC using timer_create().
 * 2. Configuring notification via a Real-Time Signal (SIGRTMIN) carrying a custom integer payload.
 * 3. Inspecting timer overruns using timer_getoverrun().
 * 4. Gracefully arming and disarming via timer_settime() and cleaning up with timer_delete().
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#define TIMER_SIGNAL (SIGRTMIN + 1)
#define TARGET_TICKS 5

static volatile sig_atomic_t g_tick_count = 0;
static timer_t g_timer_id;

/**
 * @brief Signal handler accepting SA_SIGINFO to inspect timer metadata.
 */
static void timer_signal_handler(int sig, siginfo_t *si, void *uc) {
    (void)sig;
    (void)uc;

    if (si->si_code == SI_TIMER) {
        g_tick_count++;
        int timer_magic = si->si_value.sival_int;
        int overruns    = timer_getoverrun(g_timer_id);

        /* Write compact diagnostic output (async-signal-safe) */
        char buf[128];
        int len = snprintf(buf, sizeof(buf),
                           " [TICK #%d] Timer ID payload: 0x%X | Overruns: %d\n",
                           (int)g_tick_count, timer_magic, overruns);
        if (len > 0) {
            (void)write(STDOUT_FILENO, buf, (size_t)len);
        }
    }
}

int main(void) {
    printf("====================================================\n");
    printf(" Demo 03: POSIX Interval Timer (timer_create)\n");
    printf("====================================================\n\n");

    /* ---------------------------------------------------------
     * 1. Register signal handler with SA_SIGINFO
     * --------------------------------------------------------- */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = timer_signal_handler;
    sa.sa_flags     = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    if (sigaction(TIMER_SIGNAL, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    /* ---------------------------------------------------------
     * 2. Create POSIX timer on CLOCK_MONOTONIC
     * --------------------------------------------------------- */
    struct sigevent sev;
    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify          = SIGEV_SIGNAL;
    sev.sigev_signo           = TIMER_SIGNAL;
    sev.sigev_value.sival_int = 0xBEEF; /* Custom tracking cookie */

    if (timer_create(CLOCK_MONOTONIC, &sev, &g_timer_id) == -1) {
        perror("timer_create");
        return EXIT_FAILURE;
    }

    printf("POSIX Timer created successfully. ID: %p\n", (void *)g_timer_id);

    /* ---------------------------------------------------------
     * 3. Arm timer: Initial delay 300ms, recurring every 150ms
     * --------------------------------------------------------- */
    struct itimerspec its;
    /* Initial expiration */
    its.it_value.tv_sec     = 0;
    its.it_value.tv_nsec    = 300000000L; /* 300 ms */
    /* Periodic interval */
    its.it_interval.tv_sec  = 0;
    its.it_interval.tv_nsec = 150000000L; /* 150 ms */

    printf("Arming timer: First fire in 300ms, period = 150ms...\n");
    if (timer_settime(g_timer_id, 0, &its, NULL) == -1) {
        perror("timer_settime");
        timer_delete(g_timer_id);
        return EXIT_FAILURE;
    }

    /* Wait for target ticks */
    while (g_tick_count < TARGET_TICKS) {
        pause(); /* Suspend until signal is received */
    }

    printf("\nTarget ticks (%d) reached. Disarming and cleaning up...\n", TARGET_TICKS);

    /* ---------------------------------------------------------
     * 4. Disarm and Delete Timer
     * --------------------------------------------------------- */
    memset(&its, 0, sizeof(its)); /* it_value = 0 disarms timer */
    if (timer_settime(g_timer_id, 0, &its, NULL) == -1) {
        perror("timer_settime(disarm)");
    }

    if (timer_delete(g_timer_id) == -1) {
        perror("timer_delete");
        return EXIT_FAILURE;
    }

    printf("POSIX Timer deleted. Demo 03 completed successfully.\n");
    return EXIT_SUCCESS;
}
