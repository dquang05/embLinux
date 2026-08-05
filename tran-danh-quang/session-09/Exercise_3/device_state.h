#ifndef DEVICE_STATE_H
#define DEVICE_STATE_H

#include <pthread.h>

typedef struct {
    pthread_mutex_t mutex;
    int             status;  /* 0 = OFF, 1 = ON */
} device_state_t;

#endif /* DEVICE_STATE_H */
