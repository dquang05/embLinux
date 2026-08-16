/**
 * @file file_io_demo.c
 * @brief Demonstrates universal I/O model (open, read, write, lseek, close).
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define FILE_NAME "build/test_io.txt"
#define BUF_SIZE 100

int main(void) {
    int fd;
    ssize_t bytes_written, bytes_read;
    char write_buf[] = "Hello, Linux File System!";
    char read_buf[BUF_SIZE];

    printf("--- Universal I/O Demo ---\n");

    /* 1. Open / Create file */
    fd = open(FILE_NAME, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("open failed");
        return EXIT_FAILURE;
    }
    printf("File '%s' opened successfully (fd: %d).\n", FILE_NAME, fd);

    /* 2. Write to file */
    bytes_written = write(fd, write_buf, strlen(write_buf));
    if (bytes_written == -1) {
        perror("write failed");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("Wrote %zd bytes to the file.\n", bytes_written);

    /* 3. Change file offset (lseek) to read from beginning */
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek failed");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("File offset reset to beginning.\n");

    /* 4. Read from file */
    memset(read_buf, 0, BUF_SIZE); // Clear buffer
    bytes_read = read(fd, read_buf, BUF_SIZE - 1); // Leave space for null-terminator
    if (bytes_read == -1) {
        perror("read failed");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("Read %zd bytes: '%s'\n", bytes_read, read_buf);

    /* 5. Close file */
    if (close(fd) == -1) {
        perror("close failed");
        return EXIT_FAILURE;
    }
    printf("File closed successfully.\n");

    return EXIT_SUCCESS;
}
