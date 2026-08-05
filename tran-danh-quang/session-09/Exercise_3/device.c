#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#include "device_state.h"

#define SHM_NAME "/device_shm"
#define SLEEP_INTERVAL_SEC 1

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

    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        fprintf(stderr, "Controller may not be running.\n");
        return EXIT_FAILURE;
    }

    device_state_t *state = mmap(NULL, sizeof(device_state_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (state == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }
    close(fd);

    printf("[Device] Attached to %s\n", SHM_NAME);

    while (keep_running) {
        if (pthread_mutex_lock(&state->mutex) != 0) {
            perror("pthread_mutex_lock");
            break;
        }
        
        int current_status = state->status;
        
        if (pthread_mutex_unlock(&state->mutex) != 0) {
            perror("pthread_mutex_unlock");
            break;
        }

        if (current_status == 1) {
            printf("[Device] Status: ON  — Running...\n");
        } else {
            printf("[Device] Status: OFF — Idle.\n");
        }

        sleep(SLEEP_INTERVAL_SEC);
    }

    if (munmap(state, sizeof(device_state_t)) == -1) {
        perror("munmap");
    }

    return EXIT_SUCCESS;
}
