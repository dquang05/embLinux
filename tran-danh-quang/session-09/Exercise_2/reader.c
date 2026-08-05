#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <errno.h>

#include "device_cfg.h"

#define CFG_FILE_PATH "/tmp/device.cfg"
#define SLEEP_INTERVAL_SEC 2

volatile sig_atomic_t keep_running = 1;

void sigint_handler(int sig) {
    (void)sig; /* Unused */
    keep_running = 0;
}

int main(void) {
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    int fd = open(CFG_FILE_PATH, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    device_cfg_t *cfg = mmap(NULL, sizeof(device_cfg_t), PROT_READ, MAP_SHARED, fd, 0);
    if (cfg == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }

    if (close(fd) == -1) {
        perror("close");
        return EXIT_FAILURE;
    }

    printf("[Config Reader] Polling %s every %ds...\n", CFG_FILE_PATH, SLEEP_INTERVAL_SEC);

    while (keep_running) {
        /* Convert log_level int to string for better readability in output log */
        static const char *names[] = {"OFF", "ERROR", "INFO", "DEBUG"};
        const char *log_str = (cfg->log_level >= 0 && cfg->log_level <= 3) ? names[cfg->log_level] : "UNKNOWN";

        printf("baud_rate=%d  sampling_rate=%d Hz  log_level=%s\n",
               cfg->baud_rate, cfg->sampling_rate_hz, log_str);

        sleep(SLEEP_INTERVAL_SEC);
    }

    if (munmap(cfg, sizeof(device_cfg_t)) == -1) {
        perror("munmap");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
