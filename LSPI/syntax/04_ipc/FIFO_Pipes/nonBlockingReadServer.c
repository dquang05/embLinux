#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define FIFO_PATH "/tmp/app_data_bus"
#define BUFFER_SIZE 256

int main() {
    // Create FIFO with read/write permissions for user and group
    mkfifo(FIFO_PATH, 0660);

    printf("Server: Opening FIFO in Non-blocking mode...\n");
    
    // O_NONBLOCK prevents open() from freezing if no client has opened it yet
    int fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        perror("open failed");
        return 1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;

    printf("Server: Ready! Entering core event loop...\n");
    while (1) {
        bytesRead = read(fd, buffer, sizeof(buffer) - 1);

        if (bytesRead > 0) {
            /* Case 1: Data is successfully received */
            buffer[bytesRead] = '\0';
            printf("[DATA RECEIVED]: %s\n", buffer);
        } 
        else if (bytesRead == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            /* Case 2: No data available right now, Server doesn't freeze! */
            // In industry, we use epoll_wait() here. For simplicity, we simulate with sleep.
            usleep(500000); // Sleep for 500ms, then check again
            printf("Server doing other background tasks while waiting...\n");
        } 
        else if (bytesRead == 0) {
            /* Case 3: All clients closed connection (EOF) */
            // We just reset or keep waiting. No crash.
            usleep(500000);
        }
    }

    close(fd);
    unlink(FIFO_PATH); // Delete FIFO from filesystem
    return 0;
}