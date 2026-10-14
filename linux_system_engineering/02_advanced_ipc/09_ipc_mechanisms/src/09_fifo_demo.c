/**
 * @file 09_fifo_demo.c
 * @brief Demonstration of Named Pipe (FIFO) IPC using mkfifo() on Linux.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
// #include <errno.h>

#define FIFO_PATH "/tmp/demo_ipc_fifo"
#define BUFFER_SIZE 256

int main(void) {
  /* 1. Remove old FIFO if it already exists from a previous abnormal run */
  unlink(FIFO_PATH);

  /* 2. Create the FIFO special file with read/write permissions for owner */
  if (mkfifo(FIFO_PATH, 0660) == -1) {
    perror("mkfifo");
    exit(EXIT_FAILURE);
  }

  printf("=== FIFO (Named Pipe) IPC Demo ===\n");
  printf("[Main] Created FIFO at: %s\n", FIFO_PATH);

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    unlink(FIFO_PATH);
    exit(EXIT_FAILURE);
  }

  if (pid == 0) {
    /* CHILD PROCESS: WRITER */
    printf(
        "[Writer] Opening FIFO for writing (blocks until reader opens)...\n");
    int write_fd = open(FIFO_PATH, O_WRONLY);
    if (write_fd == -1) {
      perror("open write_fd");
      exit(EXIT_FAILURE);
    }
    printf("[Writer] FIFO opened successfully.\n");

    const char *messages[] = {"Packet 1: System initialization",
                              "Packet 2: Telemetry reading: Temp=24.5C",
                              "Packet 3: Heartbeat ACK", NULL};

    for (int i = 0; messages[i] != NULL; ++i) {
      ssize_t bytes_written =
          write(write_fd, messages[i], strlen(messages[i]) + 1);
      if (bytes_written == -1) {
        perror("write to FIFO");
        close(write_fd);
        exit(EXIT_FAILURE);
      }
      printf("[Writer] Sent (%zd bytes): '%s'\n", bytes_written, messages[i]);
      usleep(100000); /* 100ms pause between writes */
    }

    /* Closing the write end signals EOF to the reader */
    printf("[Writer] Closing write descriptor.\n");
    close(write_fd);
    exit(EXIT_SUCCESS);
  } else {
    /* PARENT PROCESS: READER */
    printf("[Reader] Opening FIFO for reading...\n");
    int read_fd = open(FIFO_PATH, O_RDONLY);
    if (read_fd == -1) {
      perror("open read_fd");
      unlink(FIFO_PATH);
      exit(EXIT_FAILURE);
    }
    printf("[Reader] FIFO opened successfully.\n");

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    /* Read loop: read returns 0 when writer closes its end (EOF) */
    while ((bytes_read = read(read_fd, buffer, sizeof(buffer))) > 0) {
      printf("[Reader] Received (%zd bytes): '%s'\n", bytes_read, buffer);
    }

    if (bytes_read == -1) {
      perror("read from FIFO");
    } else {
      printf("[Reader] Reached EOF (writer closed pipe).\n");
    }

    close(read_fd);
    wait(NULL);

    /* Remove the FIFO special file from filesystem */
    if (unlink(FIFO_PATH) == -1) {
      perror("unlink FIFO");
    } else {
      printf("[Main] Successfully unlinked FIFO from filesystem.\n");
    }
  }

  return 0;
}
