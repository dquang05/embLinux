/**
 * @file error_handling_demo.c
 * @brief Demonstrates error handling in Linux using errno, perror(), and strerror().
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Main execution function.
 * 
 * Attempts to open a non-existent file to intentionally trigger an error,
 * then demonstrates how to properly capture and log that error.
 * 
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on expected failure.
 */
int main(void) {
    printf("Attempting to open a non-existent file...\n");

    // Attempt to open a file that does not exist in read-only mode
    int fd = open("/tmp/this_file_does_not_exist.txt", O_RDONLY);

    // Error Handling Standard: Always check the return value
    if (fd == -1) {
        // Method 1: Using perror()
        // perror() automatically appends the string representation of errno
        perror("Error (via perror) opening file");

        // Method 2: Using strerror()
        // strerror() returns a pointer to the error message string
        fprintf(stderr, "Error (via strerror) opening file: %s (errno: %d)\n", strerror(errno), errno);
        
        return EXIT_FAILURE; // Return failure as expected
    }

    // Resource Management Standard: Close what you open
    // (This block will never be reached in this demo, but it's good practice)
    if (close(fd) == -1) {
        perror("Error closing file");
        return EXIT_FAILURE;
    }

    printf("File opened successfully.\n");
    return EXIT_SUCCESS;
}
