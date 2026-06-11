#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_LINE 1024     /* Maximum length of a command line */
#define MAX_ARGS 64       /* Maximum number of arguments for a command */
#define WHITESPACE " \t\r\n\a" /* Characters defined as token delimiters */

int main(int argc, char *argv[]) 
{
    char line[MAX_LINE];
    char *args[MAX_ARGS];
    pid_t childPid;
    int status;

    /* Disable buffering of stdout to ensure prompt prints immediately */
    setbuf(stdout, NULL);

    /* Main Command Loop */
    while (1) {
        /* 1. Display the shell prompt */
        printf("minishell_lv1> ");

        /* 2. Read the command line input from the user */
        if (fgets(line, sizeof(line), stdin) == NULL) {
            /* Handle End-Of-File (EOF), usually triggered by Ctrl+D */
            printf("\nExiting minishell...\n");
            break;
        }

        /* 3. Parse the input line into an array of arguments (tokens) */
        int i = 0;
        args[i] = strtok(line, WHITESPACE);
        
        while (args[i] != NULL) {
            i++;
            if (i >= MAX_ARGS - 1) {
                fprintf(stderr, "minishell: too many arguments\n");
                break;
            }
            args[i] = strtok(NULL, WHITESPACE);
        }
        /* CRITICAL: The argument vector must always be NULL-terminated */
        args[i] = NULL; 

        /* If the user just pressed Enter (empty command), skip execution */
        if (args[0] == NULL) {
            continue;
        }

        /* 4. Handle built-in commands that must run in the parent process */
        if (strcmp(args[0], "exit") == 0) {
            printf("Goodbye!\n");
            break;
        }

        /* 5. Create a child process to execute external commands */
        switch (childPid = fork()) {
        case -1:
            /* Error: fork() failed */
            perror("fork failed");
            break;

        case 0:
            /* CHILD PROCESS: Execute the program */
            /* execvp uses PATH environment variable to find the binary */
            if (execvp(args[0], args) == -1) {
                perror("minishell execution error");
            }
            /* If execvp returns, an error definitely occurred */
            _exit(EXIT_FAILURE); 

        default:
            /* PARENT PROCESS: Wait synchronously for the child to terminate */
            if (wait(&status) == -1) {
                perror("wait failed");
            }
            break;
        }
    }

    return EXIT_SUCCESS;
}