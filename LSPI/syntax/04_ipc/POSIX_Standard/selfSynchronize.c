#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>

/* Define the structure mapped into shared memory */
struct shared_context {
    sem_t lock;
    int sensor_data;
};

int main() {
    int fd;
    struct shared_context *ctx;
    const char *shm_name = "/robot_sensors";

    /* Open shared memory object */
    fd = shm_open(shm_name, O_CREAT | O_RDWR, 0660);
    if (fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    /* Set the size of the shared memory object */
    if (ftruncate(fd, sizeof(struct shared_context)) == -1) {
        perror("ftruncate failed");
        exit(EXIT_FAILURE);
    }

    /* Map shared memory into process address space */
    ctx = mmap(NULL, sizeof(struct shared_context), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ctx == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    /* Initialize unnamed semaphore as process-shared (pshared = 1) */
    if (sem_init(&ctx->lock, 1, 1) == -1) {
        perror("sem_init failed");
        exit(EXIT_FAILURE);
    }

    /* Critical Section: Write data securely */
    sem_wait(&ctx->lock);
    ctx->sensor_data = 1024; 
    sem_post(&ctx->lock);

    /* Cleanup resources */
    close(fd);
    
    return 0;
}