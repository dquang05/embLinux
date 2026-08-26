/**
 * @file 01_simple_daemon.c
 * @brief A simple implementation of converting a process to a Daemon (TLPI standard).
 * 
 * Note: In modern Linux, systemd usually handles these steps internally. 
 * This file is for educational purposes to understand what happens under the hood.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>

volatile sig_atomic_t running = 1;

/**
 * @brief Signal handler to stop the daemon gracefully upon receiving SIGTERM.
 * @param sig The signal number caught.
 */
void handle_sigterm(int sig) {
    (void)sig; // Explicitly mark unused to avoid compiler warnings
    running = 0;
}

/**
 * @brief Manually converts the current process into a background daemon.
 * 
 * Follows the standard steps: fork, setsid, umask, chdir, and closing FDs.
 */
void become_daemon() {
    pid_t pid;

    // 1. Fork off the parent process
    pid = fork();
    if (pid < 0) {
        perror("fork 1 failed");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        // Parent exits, returning control to the terminal
        exit(EXIT_SUCCESS); 
    }

    // 2. Create a new session and become the process group leader
    if (setsid() < 0) {
        perror("setsid failed");
        exit(EXIT_FAILURE);
    }

    // 3. Clear the file mode creation mask
    umask(0);

    // 4. Change the working directory to the root directory
    // This prevents the daemon from locking a mounted filesystem.
    if (chdir("/") < 0) {
        perror("chdir failed");
        exit(EXIT_FAILURE);
    }

    // 5. Close standard file descriptors (stdin, stdout, stderr)
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // 6. Redirect stdin, stdout, stderr to /dev/null
    int fd = open("/dev/null", O_RDWR);
    if (fd != -1) {
        if (dup2(fd, STDIN_FILENO) == -1) exit(EXIT_FAILURE);
        if (dup2(fd, STDOUT_FILENO) == -1) exit(EXIT_FAILURE);
        if (dup2(fd, STDERR_FILENO) == -1) exit(EXIT_FAILURE);
        
        if (fd > STDERR_FILENO) {
            close(fd);
        }
    }
}

int main(void) {
    // Convert this process into a daemon (detach from terminal)
    become_daemon();

    // Set up syslog for logging (since printf/stdout no longer work)
    // LOG_PID: Include the daemon's PID in each log message
    // LOG_DAEMON: The facility code indicating it's a system daemon
    openlog("demo_daemon", LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "Daemon has successfully started and detached.");

    // Set up signal handling for graceful shutdown (SIGTERM)
    struct sigaction sa;
    sa.sa_handler = handle_sigterm;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        syslog(LOG_ERR, "Failed to setup signal handler.");
        exit(EXIT_FAILURE);
    }

    // Main daemon work loop
    int heartbeat_count = 0;
    while (running) {
        syslog(LOG_INFO, "Daemon heartbeat: %d", heartbeat_count++);
        sleep(5); // Sleep for 5 seconds between heartbeats
    }

    // Cleanup before exiting
    syslog(LOG_INFO, "Daemon received SIGTERM. Shutting down gracefully.");
    closelog();
    
    return EXIT_SUCCESS;
}
