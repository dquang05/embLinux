/**
 * @file process_management.c
 * @brief Basic demonstration of process creation, execution, and termination in Linux.
 * 
 * This program demonstrates the core lifecycle of a process using fork(), execve(),
 * waitpid(), and exit(). It shows how a parent process spawns a child, how the child
 * replaces its memory image with a new program, and how the parent waits for the 
 * child to finish to prevent zombie processes.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

/**
 * @brief Main function demonstrating fork, execve, and waitpid.
 * 
 * @return EXIT_SUCCESS on successful execution, EXIT_FAILURE on error.
 */
int main() {
    printf("Parent Process: PID = %jd\n", (intmax_t)getpid());
    printf("Forking a child process...\n");
    /* Avoid duplicating buffered output across fork when stdout is redirected. */
    if (fflush(stdout) == EOF) {
        perror("fflush failed");
        return EXIT_FAILURE;
    }

    /* 
     * KERNEL MECHANISM: fork()
     * fork() creates a new process by duplicating the calling process. The new process
     * is referred to as the child process. The kernel uses a Copy-On-Write (COW) 
     * mechanism for private writable mappings: parent and child can initially share
     * pages, and a write may create a private copy. Shared mappings remain shared.
     */
    pid_t pid = fork();

    if (pid < 0) {
        // Fork failed. errno is set appropriately.
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child Process
        printf("Child Process: PID = %jd, Parent PID = %jd\n", (intmax_t)getpid(), (intmax_t)getppid());
        printf("Child is replacing itself with 'ls -l' using execve...\n");
        /* execve replaces the process image without flushing stdio. */
        if (fflush(stdout) == EOF) {
            perror("child fflush failed");
            _exit(EXIT_FAILURE);
        }

        char *args[] = {"ls", "-l", NULL};
        char *env[] = {NULL}; // Passing an empty environment

        /* 
         * KERNEL MECHANISM: execve()
         * execve() completely replaces the current process's memory space (Text, Data, 
         * BSS, Heap, and Stack segments) with a new program loaded from the specified 
         * executable file. The PID remains the same, but the old program is gone.
         * Caught signal dispositions reset to default; ignored dispositions generally
         * remain ignored. Existing memory mappings are removed.
         */
        if (execve("/bin/ls", args, env) == -1) {
            perror("execve failed");
            _exit(EXIT_FAILURE);
        }
    } else {
        // Parent Process
        printf("Parent Process: Waiting for child (PID %jd) to complete...\n", (intmax_t)pid);

        int status;
        
        /*
         * KERNEL MECHANISM: waitpid()
         * waitpid() suspends execution of the calling process until a child specified 
         * by pid argument has changed state (e.g., terminated). This is crucial to 
         * reap the child process and prevent it from remaining a "zombie" process 
         * in the kernel's process table, which would otherwise hold onto PID structures.
         */
        pid_t waited;
        do {
            waited = waitpid(pid, &status, 0);
        } while (waited == -1 && errno == EINTR);
        if (waited == -1) {
            perror("waitpid failed");
            exit(EXIT_FAILURE);
        }

        // Check how the child terminated using standard POSIX macros
        if (WIFEXITED(status)) {
            printf("Parent Process: Child exited normally with status %d\n", WEXITSTATUS(status));
            if (WEXITSTATUS(status) != 0) {
                return EXIT_FAILURE;
            }
        } else if (WIFSIGNALED(status)) {
            printf("Parent Process: Child was terminated by signal %d\n", WTERMSIG(status));
            return EXIT_FAILURE;
        }

        printf("Parent Process: Finished execution.\n");
    }

    if (fflush(stdout) == EOF) {
        perror("fflush failed");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
