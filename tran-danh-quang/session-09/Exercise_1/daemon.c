#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#include "sensor_shm.h"

#define PROC_LOADAVG_PATH "/proc/loadavg"
#define PROC_MEMINFO_PATH "/proc/meminfo"
#define SLEEP_INTERVAL_SEC 2
#define PROC_LINE_BUFSIZE 256

volatile sig_atomic_t keep_running = 1;

void sigint_handler(int sig) {
    (void)sig; /* Unused */
    keep_running = 0;
}

int read_loadavg(double *load1) {
    FILE *fp = fopen(PROC_LOADAVG_PATH, "r");
    if (fp == NULL) {
        perror("fopen /proc/loadavg");
        return -1;
    }
    if (fscanf(fp, "%lf", load1) != 1) {
        fprintf(stderr, "Failed to read loadavg\n");
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}

int read_meminfo(double *ram_used_pct) {
    FILE *fp = fopen(PROC_MEMINFO_PATH, "r");
    if (fp == NULL) {
        perror("fopen /proc/meminfo");
        return -1;
    }
    
    char line[PROC_LINE_BUFSIZE];
    long mem_total = 0, mem_free = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line + 9, "%ld", &mem_total);
        } else if (strncmp(line, "MemFree:", 8) == 0) {
            sscanf(line + 8, "%ld", &mem_free);
        }
    }
    fclose(fp);

    if (mem_total == 0) {
        fprintf(stderr, "MemTotal is 0\n");
        return -1;
    }

    *ram_used_pct = ((double)(mem_total - mem_free) / mem_total) * 100.0;
    return 0;
}

sensor_data_t *init_daemon(int *shmid_out) {
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return NULL;
    }

    int shmid = shmget(SHM_KEY, sizeof(sensor_data_t), IPC_CREAT | IPC_EXCL | 0666);
    if (shmid == -1) {
        if (errno == EEXIST) {
            fprintf(stderr, "shmget: Shared memory already exists. Is another daemon running?\n");
        } else {
            perror("shmget");
        }
        return NULL;
    }

    sensor_data_t *shm_ptr = (sensor_data_t *)shmat(shmid, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("shmat");
        if (shmctl(shmid, IPC_RMID, NULL) == -1) {
            perror("shmctl IPC_RMID");
        }
        return NULL;
    }

    printf("[Daemon] Shared memory created. Key=0x%x\n", SHM_KEY);
    *shmid_out = shmid;
    return shm_ptr;
}

int main(void) {
    int shmid = -1;
    sensor_data_t *shm_ptr = init_daemon(&shmid);
    if (shm_ptr == NULL) {
        return EXIT_FAILURE;
    }

    while (keep_running) {
        double load1 = 0.0;
        double ram_used_pct = 0.0;
        if (read_loadavg(&load1) != 0 || read_meminfo(&ram_used_pct) != 0) {
            sleep(SLEEP_INTERVAL_SEC);
            continue;
        }

        shm_ptr->cpu_temp = 40.0 + load1 * 10.0;
        shm_ptr->ram_used_pct = ram_used_pct;

        shm_ptr->timestamp = time(NULL);

        printf("[Daemon] Written: temp=%.2f ram=%.2f%%\n", shm_ptr->cpu_temp, shm_ptr->ram_used_pct);

        sleep(SLEEP_INTERVAL_SEC);
    }

    printf("\n[Daemon] Cleaning up shared memory. Goodbye.\n");

    if (shmdt(shm_ptr) == -1) {
        perror("shmdt");
    }

    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
