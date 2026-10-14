/**
 * @file 04_epoll_demo.c
 * @brief Demonstrates Linux epoll API and compares Level-Triggered (LT) vs.
 * Edge-Triggered (ET) mode.
 *
 * This demo shows:
 * 1. Creating an epoll instance with epoll_create1(EPOLL_CLOEXEC).
 * 2. Registering and modifying events with epoll_ctl().
 * 3. Waiting for events using epoll_wait().
 * 4. Level-Triggered behavior: repeated notifications when partial data remains
 * unread.
 * 5. Edge-Triggered behavior (EPOLLET): notification occurs ONLY on state
 * transitions, requiring a non-blocking loop draining data until EAGAIN.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
// #include <string.h>
#include <sys/wait.h>

#define MAX_EVENTS 5

/**
 * @brief Helper to set O_NONBLOCK on a file descriptor.
 */
static int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
    return -1;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(void) {
  int pipefd[2];
  char buf[128];
  struct epoll_event ev;
  struct epoll_event events[MAX_EVENTS];

  printf("============================================================\n");
  printf("   Demo: epoll API (Level-Triggered vs. Edge-Triggered)\n");
  printf("============================================================\n\n");

  if (pipe(pipefd) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  /* Create an epoll instance */
  int epfd = epoll_create1(EPOLL_CLOEXEC);
  if (epfd == -1) {
    perror("epoll_create1");
    exit(EXIT_FAILURE);
  }

  /* ----------------------------------------------------------------- */
  /* Part 1: Level-Triggered (LT) Mode (Default)                       */
  /* ----------------------------------------------------------------- */
  printf("--- [Part 1: Level-Triggered (LT) Mode] ---\n");
  ev.events = EPOLLIN; /* Default is Level-Triggered */
  ev.data.fd = pipefd[0];

  if (epoll_ctl(epfd, EPOLL_CTL_ADD, pipefd[0], &ev) == -1) {
    perror("epoll_ctl ADD");
    exit(EXIT_FAILURE);
  }

  printf("[Writer] Writing 10 bytes: '0123456789'...\n");
  if (write(pipefd[1], "0123456789", 10) == -1)
    perror("write");

  printf("[Reader] Calling epoll_wait()...\n");
  int nfds = epoll_wait(epfd, events, MAX_EVENTS, 1000);
  printf("[Reader] epoll_wait() returned: %d event(s)\n", nfds);

  if (nfds > 0) {
    /* Read only 4 bytes, intentionally leaving 6 bytes in the kernel buffer */
    ssize_t n = read(pipefd[0], buf, 4);
    buf[n] = '\0';
    printf("[Reader] Read only %zd bytes: '%s' (6 bytes remain in buffer)\n", n,
           buf);

    printf("[Reader] Calling epoll_wait() AGAIN with 1s timeout...\n");
    nfds = epoll_wait(epfd, events, MAX_EVENTS, 1000);
    printf("[Reader] --> Result: epoll_wait() woke up IMMEDIATELY with %d "
           "event(s)!\n",
           nfds);
    printf("[Reader] (Reason: In LT mode, epoll continues to report readiness "
           "as long as unread data exists)\n");

    /* Clean up remaining 6 bytes */
    n = read(pipefd[0], buf, sizeof(buf));
    buf[n] = '\0';
    printf("[Reader] Drained remaining %zd bytes: '%s'\n\n", n, buf);
  }

  /* ----------------------------------------------------------------- */
  /* Part 2: Edge-Triggered (ET) Mode (EPOLLET)                        */
  /* ----------------------------------------------------------------- */
  printf("--- [Part 2: Edge-Triggered (ET) Mode] ---\n");
  printf("[Setup] Setting read end to O_NONBLOCK and updating to EPOLLET...\n");
  if (set_nonblocking(pipefd[0]) == -1) {
    perror("set_nonblocking");
    exit(EXIT_FAILURE);
  }

  ev.events = EPOLLIN | EPOLLET; /* Enable Edge-Triggered */
  ev.data.fd = pipefd[0];
  if (epoll_ctl(epfd, EPOLL_CTL_MOD, pipefd[0], &ev) == -1) {
    perror("epoll_ctl MOD");
    exit(EXIT_FAILURE);
  }

  printf("[Writer] Writing 10 bytes: 'ABCDEFGHIJ'...\n");
  if (write(pipefd[1], "ABCDEFGHIJ", 10) == -1)
    perror("write");

  printf("[Reader] Calling epoll_wait()...\n");
  nfds = epoll_wait(epfd, events, MAX_EVENTS, 1000);
  printf("[Reader] epoll_wait() returned: %d event(s) (State transitioned from "
         "empty -> non-empty)\n",
         nfds);

  if (nfds > 0) {
    /* Read only 4 bytes */
    ssize_t n = read(pipefd[0], buf, 4);
    buf[n] = '\0';
    printf("[Reader] Read only %zd bytes: '%s' (6 bytes remain in buffer)\n", n,
           buf);

    printf("[Reader] Calling epoll_wait() AGAIN with 1s timeout...\n");
    nfds = epoll_wait(epfd, events, MAX_EVENTS, 1000);
    if (nfds == 0) {
      printf("[Reader] --> Result: epoll_wait() TIMED OUT!\n");
      printf("[Reader] (Reason: In ET mode, no new state transition occurred, "
             "so epoll did NOT notify again!)\n");
    }

    /*
     * Canonical Edge-Triggered drain pattern:
     * Loop over read() until it returns EAGAIN / EWOULDBLOCK.
     */
    printf("[Reader] Now draining remaining data using the canonical "
           "non-blocking loop...\n");
    while (1) {
      n = read(pipefd[0], buf, sizeof(buf) - 1);
      if (n > 0) {
        buf[n] = '\0';
        printf("[Reader] Drained %zd bytes: '%s'\n", n, buf);
      } else if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          printf("[Reader] Received EAGAIN / EWOULDBLOCK: Kernel buffer is now "
                 "completely drained!\n");
          break;
        } else {
          perror("read");
          break;
        }
      } else {
        printf("[Reader] EOF encountered.\n");
        break;
      }
    }
  }

  close(pipefd[0]);
  close(pipefd[1]);
  close(epfd);

  printf("\n[epoll demo completed successfully.]\n");
  return 0;
}
