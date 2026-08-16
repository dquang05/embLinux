/**
 * @file links_demo.c
 * @brief Demonstrates creating and removing hard and soft links.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define TARGET_FILE "build/target.txt"
#define HARD_LINK "build/hard_link.txt"
#define SOFT_LINK "build/soft_link.txt"

/**
 * @brief Helper function to print inode number.
 */
void print_inode(const char *filename) {
    struct stat file_stat;
    // Use lstat to get info about the link itself, not the target
    if (lstat(filename, &file_stat) == -1) {
        perror("lstat failed");
    } else {
        printf("File: %-20s | Inode: %lu | Hard Links: %lu\n", 
               filename, (unsigned long)file_stat.st_ino, (unsigned long)file_stat.st_nlink);
    }
}

int main(void) {
    printf("--- Links Demo ---\n");

    /* 1. Create a target file */
    int fd = open(TARGET_FILE, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("open target failed");
        return EXIT_FAILURE;
    }
    if (write(fd, "Data", 4) == -1) {
        perror("write target failed");
    }
    close(fd);

    /* 2. Create a Hard Link */
    if (link(TARGET_FILE, HARD_LINK) == -1) {
        perror("link failed");
        return EXIT_FAILURE;
    }
    printf("Hard link created: %s -> %s\n", HARD_LINK, TARGET_FILE);

    /* 3. Create a Soft (Symbolic) Link */
    // Note: symlink target path is relative to the symlink's location if it's a relative path.
    // Since both the symlink and target are inside build/, we just link to "target.txt"
    if (symlink("target.txt", SOFT_LINK) == -1) {
        perror("symlink failed");
        return EXIT_FAILURE;
    }
    printf("Soft link created: %s -> target.txt\n", SOFT_LINK);

    /* 4. Compare Inodes */
    printf("\n--- Inode Information ---\n");
    print_inode(TARGET_FILE);
    print_inode(HARD_LINK);
    print_inode(SOFT_LINK);

    /* 5. Clean up */
    unlink(TARGET_FILE);
    unlink(HARD_LINK);
    unlink(SOFT_LINK);
    printf("\nCleaned up files.\n");

    return EXIT_SUCCESS;
}
