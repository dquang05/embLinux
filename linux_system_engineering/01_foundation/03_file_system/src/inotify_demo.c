/**
 * @file inotify_demo.c
 * @brief Demonstrates monitoring file events using inotify.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

#define EVENT_SIZE  (sizeof(struct inotify_event))
#define BUF_LEN     (1024 * (EVENT_SIZE + 16))
#define WATCH_DIR   "build"

int main(void) {
    int inotify_fd, watch_desc;
    char buffer[BUF_LEN];

    printf("--- Inotify Demo ---\n");

    /* 1. Initialize inotify instance */
    inotify_fd = inotify_init();
    if (inotify_fd == -1) {
        perror("inotify_init failed");
        return EXIT_FAILURE;
    }

    /* 2. Add a watch to the 'build' directory */
    // Monitor file creation, deletion, and modification
    watch_desc = inotify_add_watch(inotify_fd, WATCH_DIR, IN_CREATE | IN_DELETE | IN_MODIFY);
    if (watch_desc == -1) {
        perror("inotify_add_watch failed");
        close(inotify_fd);
        return EXIT_FAILURE;
    }
    
    printf("Monitoring directory: '%s' (Waiting for events...)\n", WATCH_DIR);

    /* 3. For demo purposes, we will spawn a child process to trigger an event 
          so the program doesn't hang forever waiting for user input. */
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        return EXIT_FAILURE;
    }
    
    if (pid == 0) {
        // Child: wait a moment, then create a file to trigger IN_CREATE
        sleep(1); 
        FILE *f = fopen(WATCH_DIR "/dummy.txt", "w");
        if (f) {
            fprintf(f, "Trigger event\n");
            fclose(f);
        }
        unlink(WATCH_DIR "/dummy.txt"); // trigger IN_DELETE
        exit(EXIT_SUCCESS);
    }

    /* 4. Read events (Blocking) */
    ssize_t num_read = read(inotify_fd, buffer, BUF_LEN);
    if (num_read == -1) {
        perror("read inotify failed");
        close(inotify_fd);
        return EXIT_FAILURE;
    }

    /* 5. Process events */
    printf("\nEvent(s) detected!\n");
    for (char *p = buffer; p < buffer + num_read; ) {
        struct inotify_event *event = (struct inotify_event *) p;
        
        if (event->len > 0) {
            printf("File: %s | Event: ", event->name);
            if (event->mask & IN_CREATE) printf("CREATED ");
            if (event->mask & IN_DELETE) printf("DELETED ");
            if (event->mask & IN_MODIFY) printf("MODIFIED ");
            printf("\n");
        }
        
        // Advance pointer to the next event
        p += EVENT_SIZE + event->len;
    }

    // Wait for child to exit
    wait(NULL);

    /* 6. Clean up */
    inotify_rm_watch(inotify_fd, watch_desc);
    close(inotify_fd);

    return EXIT_SUCCESS;
}
