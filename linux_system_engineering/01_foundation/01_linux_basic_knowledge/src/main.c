/**
 * @file main.c
 * @brief Demonstration of a direct System Call in Linux (write).
 * 
 * This minimal example demonstrates how a User Mode program requests the 
 * Kernel to print text to the console (Standard Output) using the write() 
 * system call, bypassing higher-level C library wrappers like printf().
 */

#include <unistd.h>
#include <string.h>

int main() {
    const char *message = "Hello from User Mode via write() System Call!\n";
    size_t length = strlen(message);
    
    // The write() function is a POSIX wrapper around the sys_write system call.
    // STDOUT_FILENO (1): File descriptor for Standard Output.
    // message: The buffer containing the string to print.
    // length: The number of bytes to write.
    // 
    // This call triggers a context switch to Kernel Mode to output data.
    ssize_t bytes_written = write(STDOUT_FILENO, message, length);
    
    // Error Handling: Always check the return value of system calls.
    // If the return value is -1, an error occurred and 'errno' is set.
    if (bytes_written == -1) {
        return 1;
    }
    
    return 0;
}
