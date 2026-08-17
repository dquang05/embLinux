#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main() {
    printf("Parent Process: PID = %d\n", getpid());
    printf("Forking a child process...\n");

    pid_t pid = fork();

    if (pid < 0) {
        // Fork failed
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child Process
        printf("Child Process: PID = %d, Parent PID = %d\n", getpid(), getppid());
        printf("Child is replacing itself with 'ls -l' using execve...\n");

        // Prepare arguments for execve
        char *args[] = {"ls", "-l", NULL};
        char *env[] = {NULL}; // Passing an empty environment

        // Execute 'ls'. We use the full path /bin/ls because execve doesn't search PATH.
        if (execve("/bin/ls", args, env) == -1) {
            perror("execve failed");
            // Only reached if execve fails
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent Process
        printf("Parent Process: Waiting for child (PID %d) to complete...\n", pid);

        int status;
        // waitpid ensures we wait for the specific child process
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid failed");
            exit(EXIT_FAILURE);
        }

        // Check how the child terminated
        if (WIFEXITED(status)) {
            printf("Parent Process: Child exited normally with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Parent Process: Child was terminated by signal %d\n", WTERMSIG(status));
        }

        printf("Parent Process: Finished execution.\n");
    }

    return EXIT_SUCCESS;
}
