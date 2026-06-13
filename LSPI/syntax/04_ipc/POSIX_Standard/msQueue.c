#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#define QUEUE_NAME        "/robot_cmd_queue"
#define MAX_MSG_COUNT     10           /* Maximum number of messages in queue */
#define MSG_BUFFER_SIZE   256          /* Buffer size for each message (Crucial for mq_receive) */

/* Global Mutex and Condition Variable for synchronized logging/processing */
pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

int main() {
    mqd_t mqd;
    struct mq_attr attr;
    char buffer[MSG_BUFFER_SIZE];
    unsigned int msg_prio;
    ssize_t num_read;

    /* 1. Define strict attributes for the industrial message queue */
    attr.mq_flags = 0;                 /* Blocked mode by default */
    attr.mq_maxmsg = MAX_MSG_COUNT;    /* Bound the queue capacity to prevent memory exhaustion */
    attr.mq_msgsize = MSG_BUFFER_SIZE; /* Fixed size per message */
    attr.mq_curmsgs = 0;

    /* 2. Remove old queue instance if it exists (Ensures clean startup state) */
    mq_unlink(QUEUE_NAME);

    /* 3. Open the queue with Exclusive Creation and Read-Only permissions */
    mqd = mq_open(QUEUE_NAME, O_CREAT | O_EXCL | O_RDONLY, 0660, &attr);
    if (mqd == (mqd_t)-1) {
        perror("Industrial Error: mq_open failed to initialize");
        exit(EXIT_FAILURE);
    }

    printf("[SERVER] Command Control Center initialized. Waiting for payloads...\n");

    /* 4. Core Event Loop (Industrial Standard Server Daemons) */
    while (1) {
        /* * BẪY RUNTIME CÔNG NGHIỆP: Buffer size passed to mq_receive MUST be 
         * greater than or equal to the mq_msgsize attribute of the queue.
         * Otherwise, it will immediately crash/fail with EMSGSIZE.
         */
        num_read = mq_receive(mqd, buffer, MSG_BUFFER_SIZE, &msg_prio);
        if (num_read == -1) {
            if (errno == EINTR) continue; /* Interrupted by signal, safely restart system call */
            perror("Runtime Error: mq_receive failed");
            break;
        }

        /* Null-terminate the received byte stream securely */
        buffer[num_read] = '\0';

        /* 5. Synchronize output/resource processing using POSIX Mutex */
        pthread_mutex_lock(&data_mutex);
        
        printf("[SERVER][PRIORITY: %u] Processing command: %s (%ld bytes)\n", 
               msg_prio, buffer, (long)num_read);
        
        /* Industrial Use-case: High priority emergency stop handling */
        if (msg_prio >= 30) {
            printf("[CRITICAL] EMERGENCY STOP TRIGGERED! HALTING ALL MOTORS!\n");
        }

        pthread_mutex_unlock(&data_mutex);

        /* Graceful exit mechanism for demonstration */
        if (strcmp(buffer, "SHUTDOWN") == 0) {
            printf("[SERVER] Shutdown command verified. Initiating clean exit.\n");
            break;
        }
    }

    /* 6. Strict Cleanup Phase: Prevent Resource Leaks (Kernel Persistence) */
    mq_close(mqd);
    mq_unlink(QUEUE_NAME);
    pthread_mutex_destroy(&data_mutex);

    printf("[SERVER] Resources deallocated successfully. System offline.\n");
    return 0;
}