/**
 * @file mmap_ipc_demo.c
 * @brief Demonstrates using mmap() with MAP_SHARED | MAP_ANONYMOUS to share
 *        memory between a parent and child process for IPC, and msync().
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <string.h>

#define SHARED_MEM_SIZE 4096 // 1 Page

int main(void) {
    printf("--- mmap() Shared Anonymous Mapping IPC Demo ---\n");

    // Create a shared anonymous mapping
    // MAP_SHARED ensures modifications are visible to other processes mapped to the same region.
    // MAP_ANONYMOUS means it's not backed by any file.
    char *shared_mem = mmap(NULL, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (shared_mem == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    printf("[INFO] Shared memory mapped at address: %p\n", (void *)shared_mem);

    // Initialize shared memory
    strncpy(shared_mem, "Initial message from Parent", SHARED_MEM_SIZE - 1);
    shared_mem[SHARED_MEM_SIZE - 1] = '\0';

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child Process
        printf("[CHILD] Started. Reading shared memory: '%s'\n", shared_mem);
        
        printf("[CHILD] Modifying shared memory...\n");
        strncpy(shared_mem, "Hello from Child Process!", SHARED_MEM_SIZE - 1);
        
        // Use msync to ensure changes are flushed (though it's an anonymous mapping, 
        // this is just to demonstrate the API usage).
        if (msync(shared_mem, SHARED_MEM_SIZE, MS_SYNC) == -1) {
            perror("msync failed");
        }

        // Unmap memory in child before exiting
        if (munmap(shared_mem, SHARED_MEM_SIZE) == -1) {
            perror("munmap failed in child");
        }
        printf("[CHILD] Memory unmapped and exiting.\n");
        exit(EXIT_SUCCESS);
    } else {
        // Parent Process
        // Wait for child to finish
        if (wait(NULL) == -1) {
            perror("wait failed");
            exit(EXIT_FAILURE);
        }

        printf("\n[PARENT] Child has exited.\n");
        printf("[PARENT] Reading shared memory modified by child: '%s'\n", shared_mem);

        // Unmap memory in parent
        if (munmap(shared_mem, SHARED_MEM_SIZE) == -1) {
            perror("munmap failed in parent");
            exit(EXIT_FAILURE);
        }
        printf("[PARENT] Memory unmapped and exiting.\n");
    }

    return 0;
}
