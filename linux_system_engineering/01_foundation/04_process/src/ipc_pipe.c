/**
 * @file ipc_pipe.c
 * @brief Demonstration of Inter-Process Communication (IPC) using unnamed pipes.
 * 
 * This program shows how a parent process and a child process can communicate
 * passing data through a unidirectional pipe created before fork().
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define BUFFER_SIZE 128

/**
 * @brief Main function demonstrating pipe creation and IPC between parent and child.
 * 
 * @return EXIT_SUCCESS on successful execution, EXIT_FAILURE on error.
 */
int main() {
    int pipefd[2];
    pid_t pid;
    char write_msg[] = "Hello from Parent Process! This is IPC via Pipe.";
    char read_msg[BUFFER_SIZE];

    /* 
     * KERNEL MECHANISM: pipe()
     * pipe() creates a unidirectional data channel that can be used for IPC.
     * The array pipefd is used to return two file descriptors referring to the ends
     * of the pipe. pipefd[0] is the read end, pipefd[1] is the write end.
     * Data written to the write end is buffered by the kernel until it is read 
     * from the read end.
     */
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    /*
     * fork() creates a child. Since the pipe was created before fork(), the child
     * inherits exact copies of the pipe file descriptors. Now both parent and child
     * have access to the same pipe.
     */
    pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        /* 
         * PARENT PROCESS: 
         * The parent will act as the Writer. It must close the read end of the pipe
         * that it doesn't need.
         */
        close(pipefd[0]); // Close unused read end

        printf("Parent (PID %d): Writing message to pipe...\n", getpid());
        if (write(pipefd[1], write_msg, strlen(write_msg) + 1) == -1) {
            perror("write failed");
            close(pipefd[1]);
            exit(EXIT_FAILURE);
        }

        close(pipefd[1]); // Close write end after writing (sends EOF to reader)
        
        /* Wait for child to finish to avoid zombie */
        waitpid(pid, NULL, 0);
        printf("Parent (PID %d): Child finished. Exiting.\n", getpid());
        
    } else {
        /* 
         * CHILD PROCESS: 
         * The child will act as the Reader. It must close the write end of the pipe
         * that it doesn't need.
         */
        close(pipefd[1]); // Close unused write end
        
        printf("Child (PID %d): Waiting to read from pipe...\n", getpid());
        
        /* Read from the pipe. This call will block until the parent writes something. */
        ssize_t bytes_read = read(pipefd[0], read_msg, BUFFER_SIZE);
        if (bytes_read == -1) {
            perror("read failed");
            close(pipefd[0]);
            exit(EXIT_FAILURE);
        }
        
        printf("Child (PID %d): Received message: '%s'\n", getpid(), read_msg);
        
        close(pipefd[0]); // Close read end after reading
    }

    return EXIT_SUCCESS;
}
