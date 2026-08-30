/**
 * @file 05_posix_mq_demo.c
 * @brief Demo program for POSIX Message Queues in Linux System Engineering.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <sys/wait.h>

#define MQ_NAME "/demo_posix_mq"
#define MAX_SIZE 1024

int main(void) {
    mqd_t mqd;
    pid_t pid;
    struct mq_attr attr;
    char buffer[MAX_SIZE];

    /* Set queue attributes */
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_SIZE;
    attr.mq_curmsgs = 0;

    /* Create the message queue */
    mqd = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0644, &attr);
    if (mqd == (mqd_t) -1) {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        /* PARENT: WRITER */
        const char *msg1 = "Low priority message";
        const char *msg2 = "High priority message";

        printf("[Writer] Sending low priority message...\n");
        mq_send(mqd, msg1, strlen(msg1) + 1, 1);

        printf("[Writer] Sending high priority message...\n");
        mq_send(mqd, msg2, strlen(msg2) + 1, 10);

        wait(NULL);

        /* Clean up */
        mq_close(mqd);
        mq_unlink(MQ_NAME);
        printf("[Writer] Cleaned up POSIX MQ.\n");
    } else {
        /* CHILD: READER */
        unsigned int prio;
        ssize_t bytes_read;

        /* Sleep a bit to allow writer to queue both messages */
        sleep(1);

        /* Read first message (should be the high priority one due to strictly priority ordering) */
        bytes_read = mq_receive(mqd, buffer, MAX_SIZE, &prio);
        if (bytes_read >= 0) {
            printf("[Reader] Read: '%s' (Priority: %u)\n", buffer, prio);
        }

        /* Read second message (should be the low priority one) */
        bytes_read = mq_receive(mqd, buffer, MAX_SIZE, &prio);
        if (bytes_read >= 0) {
            printf("[Reader] Read: '%s' (Priority: %u)\n", buffer, prio);
        }

        mq_close(mqd);
        exit(EXIT_SUCCESS);
    }

    return 0;
}
