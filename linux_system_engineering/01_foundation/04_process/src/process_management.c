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
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

/**
 * @brief Main function demonstrating fork, execve, and waitpid.
 * 
 * @return EXIT_SUCCESS on successful execution, EXIT_FAILURE on error.
 */
int main() {
    printf("Parent Process: PID = %d\n", getpid());
    printf("Forking a child process...\n");

    /* 
     * KERNEL MECHANISM: fork()
     * fork() creates a new process by duplicating the calling process. The new process
     * is referred to as the child process. The kernel uses a Copy-On-Write (COW) 
     * mechanism to optimize this: the parent and child initially share the same physical 
     * memory pages with read-only access. A page is only physically copied to a new frame
     * when either process attempts to modify it.
     */
    pid_t pid = fork();

    if (pid < 0) {
        // Fork failed. errno is set appropriately.
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child Process
        printf("Child Process: PID = %d, Parent PID = %d\n", getpid(), getppid());
        printf("Child is replacing itself with 'ls -l' using execve...\n");

        char *args[] = {"ls", "-l", NULL};
        char *env[] = {NULL}; // Passing an empty environment

        /* 
         * KERNEL MECHANISM: execve()
         * execve() completely replaces the current process's memory space (Text, Data, 
         * BSS, Heap, and Stack segments) with a new program loaded from the specified 
         * executable file. The PID remains the same, but the old program is gone.
         * The old signal handlers and mmapped memory regions are dropped.
         */
        if (execve("/bin/ls", args, env) == -1) {
            perror("execve failed");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent Process
        printf("Parent Process: Waiting for child (PID %d) to complete...\n", pid);

        int status;
        
        /*
         * KERNEL MECHANISM: waitpid()
         * waitpid() suspends execution of the calling process until a child specified 
         * by pid argument has changed state (e.g., terminated). This is crucial to 
         * reap the child process and prevent it from remaining a "zombie" process 
         * in the kernel's process table, which would otherwise hold onto PID structures.
         */
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid failed");
            exit(EXIT_FAILURE);
        }

        // Check how the child terminated using standard POSIX macros
        if (WIFEXITED(status)) {
            printf("Parent Process: Child exited normally with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Parent Process: Child was terminated by signal %d\n", WTERMSIG(status));
        }

        printf("Parent Process: Finished execution.\n");
    }

    return EXIT_SUCCESS;
}
