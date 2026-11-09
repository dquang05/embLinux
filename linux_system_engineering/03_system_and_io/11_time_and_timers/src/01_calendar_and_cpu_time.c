/**
 * @file 01_calendar_and_cpu_time.c
 * @brief Demonstration of Calendar Time, Monotonic Benchmarking, and Process CPU Time.
 *
 * This program demonstrates:
 * 1. Retrieving wall-clock time and converting it to human-readable strings using
 *    thread-safe localtime_r() and strftime().
 * 2. High-precision elapsed time benchmarking using CLOCK_MONOTONIC.
 * 3. Retrieving User CPU time vs. System CPU time using times() and sysconf(_SC_CLK_TCK).
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#include <unistd.h>
//#include <errno.h>

/**
 * @brief Performs simulated work consuming both user-mode and system-mode CPU time.
 * @param iterations Number of iterations for the user-mode calculation loop.
 */
static void perform_workload(long iterations) {
    volatile double dummy = 0.0;
    for (long i = 0; i < iterations; ++i) {
        dummy += (double)i * 0.0001;
    }
    (void)dummy;

    /* Perform system calls to consume system CPU time */
    for (int i = 0; i < 50000; ++i) {
        (void)getpid();
    }
}

int main(void) {
    printf("====================================================\n");
    printf(" Demo 01: Calendar Time, Monotonic & CPU Benchmarking\n");
    printf("====================================================\n\n");

    /* ---------------------------------------------------------
     * 1. Calendar Time: Wall-Clock Timestamp & Formatting
     * --------------------------------------------------------- */
    struct timespec real_ts;
    if (clock_gettime(CLOCK_REALTIME, &real_ts) == -1) {
        perror("clock_gettime(CLOCK_REALTIME)");
        return EXIT_FAILURE;
    }

    struct tm broken_down;
    if (localtime_r(&real_ts.tv_sec, &broken_down) == NULL) {
        perror("localtime_r");
        return EXIT_FAILURE;
    }

    char time_str[128];
    if (strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &broken_down) == 0) {
        fprintf(stderr, "strftime failed to format time buffer\n");
        return EXIT_FAILURE;
    }

    printf("[1] Current Wall-Clock Time:\n");
    printf("    Formatted: %s.%06ld\n", time_str, real_ts.tv_nsec / 1000);
    printf("    Epoch Seconds: %ld, Nanoseconds: %ld\n\n",
           (long)real_ts.tv_sec, real_ts.tv_nsec);

    /* ---------------------------------------------------------
     * 2. Benchmark Elapsed Time (CLOCK_MONOTONIC) & CPU Usage (times)
     * --------------------------------------------------------- */
    long clock_ticks_per_sec = sysconf(_SC_CLK_TCK);
    if (clock_ticks_per_sec == -1) {
        perror("sysconf(_SC_CLK_TCK)");
        return EXIT_FAILURE;
    }

    struct timespec mono_start, mono_end;
    struct tms tms_start, tms_end;

    if (clock_gettime(CLOCK_MONOTONIC, &mono_start) == -1) {
        perror("clock_gettime(CLOCK_MONOTONIC start)");
        return EXIT_FAILURE;
    }

    if (times(&tms_start) == (clock_t)-1) {
        perror("times(start)");
        return EXIT_FAILURE;
    }

    printf("[2] Executing CPU workload (math loop + getpid syscalls)...\n");
    perform_workload(10000000L);

    if (times(&tms_end) == (clock_t)-1) {
        perror("times(end)");
        return EXIT_FAILURE;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &mono_end) == -1) {
        perror("clock_gettime(CLOCK_MONOTONIC end)");
        return EXIT_FAILURE;
    }

    /* Calculate elapsed wall-clock / monotonic duration */
    double elapsed_sec = (double)(mono_end.tv_sec - mono_start.tv_sec) +
                         (double)(mono_end.tv_nsec - mono_start.tv_nsec) / 1e9;

    /* Calculate user and system CPU times in seconds */
    clock_t user_ticks = tms_end.tms_utime - tms_start.tms_utime;
    clock_t sys_ticks  = tms_end.tms_stime - tms_start.tms_stime;

    double user_cpu_sec = (double)user_ticks / (double)clock_ticks_per_sec;
    double sys_cpu_sec  = (double)sys_ticks  / (double)clock_ticks_per_sec;

    printf("\n[3] Workload Benchmark Results:\n");
    printf("    Kernel Clock Ticks/sec (USER_HZ): %ld\n", clock_ticks_per_sec);
    printf("    Elapsed Wall-Clock Time         : %.4f seconds\n", elapsed_sec);
    printf("    User-Mode CPU Time (Compute)    : %.4f seconds (%ld ticks)\n",
           user_cpu_sec, (long)user_ticks);
    printf("    System-Mode CPU Time (Syscalls) : %.4f seconds (%ld ticks)\n",
           sys_cpu_sec, (long)sys_ticks);
    printf("    Total CPU Time (User + Sys)     : %.4f seconds\n",
           user_cpu_sec + sys_cpu_sec);

    return EXIT_SUCCESS;
}
