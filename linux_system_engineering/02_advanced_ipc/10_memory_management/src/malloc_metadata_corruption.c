/**
 * @file malloc_metadata_corruption.c
 * @brief Demonstrates how malloc() uses the space just before the returned pointer
 *        to store metadata, and how a buffer overflow corrupts this metadata,
 *        leading to a crash (SIGSEGV/SIGABRT) when free() is called.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(void) {
    printf("--- Malloc Metadata Corruption Demo ---\n");

    // Print program break before malloc
    void *initial_break = sbrk(0);
    printf("[INFO] Initial program break: %p\n", initial_break);

    // Allocate a small block
    size_t alloc_size = 16;
    char *ptr = (char *)malloc(alloc_size);
    if (ptr == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    printf("[INFO] Allocated %zu bytes. Pointer returned by malloc: %p\n", alloc_size, (void *)ptr);
    
    // Print program break after malloc
    void *new_break = sbrk(0);
    printf("[INFO] Program break after malloc: %p\n", new_break);

    // The metadata (like the block size) is usually stored right before the pointer
    // We will inspect the memory just before the pointer.
    // Note: This relies on implementation details of glibc's malloc, but is generally true.
    size_t *metadata_ptr = (size_t *)(ptr - sizeof(size_t));
    printf("[INFO] Metadata (Block size/Flags) before corruption: %zu (0x%zx)\n", *metadata_ptr, *metadata_ptr);

    // Now let's simulate a buffer underflow (writing before the block).
    // We intentionally write out of bounds to corrupt the metadata.
    printf("\n[DANGER] Corrupting malloc metadata (writing outside allocated bounds)...\n");
    
    // Corrupting the size field
    *metadata_ptr = 0xFFFFFFFF; // Set a garbage large size

    printf("[INFO] Metadata after corruption: %zu (0x%zx)\n", *metadata_ptr, *metadata_ptr);

    printf("\n[DANGER] Calling free() on the corrupted pointer. This will likely crash the program...\n");
    // This free() call should trigger a glibc memory corruption abort or SIGSEGV
    free(ptr);

    printf("[SUCCESS] If you see this, the program didn't crash (rare, depending on libc).\n");

    return 0;
}
