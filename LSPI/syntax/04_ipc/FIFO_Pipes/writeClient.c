#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
// For example:
#define FIFO_PATH "/tmp/app_data_bus"

int main() {
    printf("Client: Connecting to Server FIFO...\n");
    
    // Open for writing. Will fail with ENXIO if Server is not running.
    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1) {
        perror("Connect failed. Is Server running?");
        return 1;
    }

    char *payload = "SENSOR_TEMP: 37.5 C | STATUS: OK";
    printf("Client: Connected! Sending payload...\n");
    
    write(fd, payload, strlen(payload));

    close(fd);
    printf("Client: Data sent successfully. Disconnected.\n");
    return 0;
}