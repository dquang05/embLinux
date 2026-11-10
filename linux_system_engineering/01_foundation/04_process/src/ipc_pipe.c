/**
 * @file ipc_pipe.c
 * @brief Demonstration of Inter-Process Communication (IPC) using unnamed pipes.
 * 
 * This program shows how a parent process and a child process can communicate
 * passing data through a unidirectional pipe created before fork().
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
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
    if (fflush(stdout) == EOF) {
        perror("fflush failed");
        return EXIT_FAILURE;
    }
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
        close(pipefd[0]);
        close(pipefd[1]);
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        /* 
         * PARENT PROCESS: 
         * The parent will act as the Writer. It must close the read end of the pipe
         * that it doesn't need.
         */
        close(pipefd[0]); // Close unused read end

        printf("Parent (PID %jd): Writing message to pipe...\n", (intmax_t)getpid());
        int failed = 0;
        struct sigaction action = {0};
        action.sa_handler = SIG_IGN;
        sigemptyset(&action.sa_mask);
        if (sigaction(SIGPIPE, &action, NULL) == -1) {
            perror("sigaction failed");
            failed = 1;
        }
        /* A missing reader then produces EPIPE instead of terminating the parent. */
        size_t written = 0;
        while (!failed && written < sizeof(write_msg)) {
            ssize_t n = write(pipefd[1], write_msg + written, sizeof(write_msg) - written);
            if (n == -1 && errno == EINTR) {
                continue;
            }
            if (n <= 0) {
                if (n == -1) perror("write failed");
                else fprintf(stderr, "write made no progress\n");
                failed = 1;
                break;
            }
            written += (size_t)n;
        }

        if (close(pipefd[1]) == -1) {
            perror("close write end failed");
            failed = 1;
        }
        
        /* Wait for child to finish to avoid zombie */
        int status;
        pid_t waited;
        do {
            waited = waitpid(pid, &status, 0);
        } while (waited == -1 && errno == EINTR);
        if (waited == -1) {
            perror("waitpid failed");
            return EXIT_FAILURE;
        }
        if (failed || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Pipe transfer or child failed.\n");
            return EXIT_FAILURE;
        }
        printf("Parent (PID %jd): Child finished. Exiting.\n", (intmax_t)getpid());
        
    } else {
        /* 
         * CHILD PROCESS: 
         * The child will act as the Reader. It must close the write end of the pipe
         * that it doesn't need.
         */
        close(pipefd[1]); // Close unused write end
        
        printf("Child (PID %jd): Waiting to read from pipe...\n", (intmax_t)getpid());
        
        /* The fixed message includes a NUL; pipe reads need not match writes. */
        size_t received = 0;
        while (received < sizeof(write_msg)) {
            ssize_t n = read(pipefd[0], read_msg + received, sizeof(write_msg) - received);
            if (n == -1 && errno == EINTR) {
                continue;
            }
            if (n <= 0) {
                if (n == -1) perror("read failed");
                else fprintf(stderr, "Unexpected EOF before complete message.\n");
                close(pipefd[0]);
                exit(EXIT_FAILURE);
            }
            received += (size_t)n;
        }
        if (read_msg[sizeof(write_msg) - 1] != '\0') {
            fprintf(stderr, "Message has no terminating NUL.\n");
            close(pipefd[0]);
            exit(EXIT_FAILURE);
        }
        
        printf("Child (PID %jd): Received message: '%s'\n", (intmax_t)getpid(), read_msg);
        
        close(pipefd[0]); // Close read end after reading
    }

    if (fflush(stdout) == EOF) {
        perror("fflush failed");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
