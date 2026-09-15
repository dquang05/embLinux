/**
 * @file mlock_mincore_demo.c
 * @brief Demonstrates virtual memory operations: mincore() to check RAM residency,
 *        madvise() to give Kernel hints, and mlock() to pin memory to RAM.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

void print_mincore_status(void *addr, size_t length, size_t page_size) {
    size_t num_pages = (length + page_size - 1) / page_size;
    unsigned char *vec = malloc(num_pages);
    
    if (vec == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    if (mincore(addr, length, vec) == -1) {
        perror("mincore failed");
        free(vec);
        exit(EXIT_FAILURE);
    }

    printf("  [MINCORE] Memory residency (1 = In RAM, 0 = Not in RAM or Swapped):\n  [ ");
    for (size_t i = 0; i < num_pages; i++) {
        // The least significant bit indicates residency
        printf("%d ", vec[i] & 1);
    }
    printf("]\n");

    free(vec);
}

int main(void) {
    printf("--- mlock() & mincore() Virtual Memory Demo ---\n");

    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        perror("sysconf failed");
        exit(EXIT_FAILURE);
    }
    printf("[INFO] System Page Size: %ld bytes\n", page_size);

    size_t num_pages = 10;
    size_t alloc_size = num_pages * page_size;

    // Map anonymous memory
    void *addr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (addr == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    printf("\n[1] Allocated %zu pages (%zu bytes) at %p\n", num_pages, alloc_size, addr);
    
    // Check residency immediately after mmap. 
    // Due to demand paging (lazy allocation), pages are NOT in physical RAM yet.
    printf("Checking residency before touching memory (Lazy Allocation):\n");
    print_mincore_status(addr, alloc_size, page_size);

    // Give kernel a hint that we will access this sequentially
    if (madvise(addr, alloc_size, MADV_SEQUENTIAL) == -1) {
        perror("madvise failed");
    }
    printf("\n[2] Called madvise() with MADV_SEQUENTIAL to hint Kernel.\n");

    // Touch the first 3 pages to trigger Page Faults and load them into RAM
    printf("\n[3] Accessing (writing to) the first 3 pages...\n");
    for (size_t i = 0; i < 3; i++) {
        char *page_ptr = (char *)addr + (i * page_size);
        page_ptr[0] = 'X'; // This triggers a page fault
    }

    printf("Checking residency after touching 3 pages:\n");
    print_mincore_status(addr, alloc_size, page_size);

    // Lock the entire memory region to RAM (Memory Pinning)
    printf("\n[4] Locking the entire memory region with mlock()...\n");
    if (mlock(addr, alloc_size) == -1) {
        perror("mlock failed (You might exceed RLIMIT_MEMLOCK limits for normal users)");
        // Continue execution even if mlock fails, to show the error
    } else {
        printf("mlock() successful! All pages are now pinned to physical RAM.\n");
    }

    printf("Checking residency after mlock():\n");
    print_mincore_status(addr, alloc_size, page_size);

    // Clean up
    munlock(addr, alloc_size);
    munmap(addr, alloc_size);
    
    printf("\n[INFO] Demo finished and memory unmapped.\n");
    return 0;
}
