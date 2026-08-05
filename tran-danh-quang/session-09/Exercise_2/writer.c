#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "device_cfg.h"

#define CFG_FILE_PATH "/tmp/device.cfg"

void clear_stdin(void) {
    int c;
    while (1) {
        c = getchar();
        if (c == '\n' || c == EOF) {
            break;
        }
    }
}

int get_user_input(char *buffer, size_t size) {
    while (fgets(buffer, size, stdin) == NULL) {
        if (errno == EINTR) {
            /* Retry on EINTR */
            continue;
        }
        return -1;
    }
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    } else {
        clear_stdin(); /* Handle buffer overflow */
    }
    return 0;
}

int main(void) {
    int fd = open(CFG_FILE_PATH, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    if (ftruncate(fd, sizeof(device_cfg_t)) == -1) {
        perror("ftruncate");
        close(fd);
        return EXIT_FAILURE;
    }

    device_cfg_t *cfg = mmap(NULL, sizeof(device_cfg_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (cfg == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }
    
    /* Initialize default values if uninitialized */
    if (cfg->baud_rate == 0) {
        cfg->baud_rate = 9600;
        cfg->sampling_rate_hz = 100;
        cfg->log_level = 2;  /* INFO */
        if (msync(cfg, sizeof(device_cfg_t), MS_SYNC) == -1) {
            perror("msync");
        }
    }
    
    if (close(fd) == -1) {
        perror("close");
        /* Continue execution, not fatal */
    }

    printf("[Config Writer] Loaded %s\n", CFG_FILE_PATH);

    while (1) {
        printf("Current: baud_rate=%d sampling_rate=%d log_level=%d\n\n", 
               cfg->baud_rate, cfg->sampling_rate_hz, cfg->log_level);

        printf("Select field to update [baud/rate/log/quit]: ");
        char field[INPUT_BUFFER_SIZE];
        if (get_user_input(field, sizeof(field)) != 0) {
            break; /* EOF or error */
        }

        if (strcmp(field, "quit") == 0) {
            break;
        } else if (strcmp(field, "baud") == 0) {
            printf("Select baud rate [9600/115200/460800]: ");
            char val_str[INPUT_BUFFER_SIZE];
            if (get_user_input(val_str, sizeof(val_str)) == 0) {
                int val = atoi(val_str);
                if (val == 9600 || val == 115200 || val == 460800) {
                    cfg->baud_rate = val;
                    if (msync(cfg, sizeof(device_cfg_t), MS_SYNC) == -1) {
                        perror("msync");
                    } else {
                        printf("[Updated] baud_rate = %d\n", val);
                    }
                } else {
                    printf("Invalid baud rate.\n");
                }
            } else {
                break;
            }
        } else if (strcmp(field, "rate") == 0) {
            printf("Enter sampling rate (1-1000): ");
            char val_str[INPUT_BUFFER_SIZE];
            if (get_user_input(val_str, sizeof(val_str)) == 0) {
                int val = atoi(val_str);
                if (val >= 1 && val <= 1000) {
                    cfg->sampling_rate_hz = val;
                    if (msync(cfg, sizeof(device_cfg_t), MS_SYNC) == -1) {
                        perror("msync");
                    } else {
                        printf("[Updated] sampling_rate_hz = %d\n", val);
                    }
                } else {
                    printf("Invalid sampling rate.\n");
                }
            } else {
                break;
            }
        } else if (strcmp(field, "log") == 0) {
            printf("Select log level [0=OFF, 1=ERROR, 2=INFO, 3=DEBUG]: ");
            char val_str[INPUT_BUFFER_SIZE];
            if (get_user_input(val_str, sizeof(val_str)) == 0) {
                int val = atoi(val_str);
                if (val >= 0 && val <= 3) {
                    cfg->log_level = val;
                    if (msync(cfg, sizeof(device_cfg_t), MS_SYNC) == -1) {
                        perror("msync");
                    } else {
                        printf("[Updated] log_level = %d\n", val);
                    }
                } else {
                    printf("Invalid log level.\n");
                }
            } else {
                break;
            }
        } else {
            printf("Invalid field.\n");
        }
    }

    if (munmap(cfg, sizeof(device_cfg_t)) == -1) {
        perror("munmap");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
