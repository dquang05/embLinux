/*File I/O operations (System Calls) in Linux 
Read and write in stdio lib is not present here*/

#include <fcntl.h>
#include <sys/stat.h>
#include "tlpi_hdr.h"

int main(int argc, char *argv[]) {
    int fd;
    off_t offset;
    ssize_t numRead, numWritten;
    char writeBuf[] = "Hello Linux";
    char readBuf[50];

    /* 1. Use open() to create a new file or truncate an existing one */
    /* Flags: Write-only, Create if missing, Truncate old data */
    /* Permissions: rw-r--r-- (Read/Write for owner, Read for others) */
    fd = open("test.txt", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        errExit("open");
    }

    /* 2. Use write() to output the initial string */
    numWritten = write(fd, writeBuf, strlen(writeBuf));
    if (numWritten != strlen(writeBuf)) {
        fatal("couldn't write whole buffer");
    }
    printf("1. Wrote '%s' to test.txt (Offset is now at %ld)\n", writeBuf, (long)lseek(fd, 0, SEEK_CUR));

    /* 3. Use lseek() to jump past EOF (creating a file hole) */
    /* Seek to 30 bytes from the start of the file */
    offset = lseek(fd, 30, SEEK_SET);
    if (offset == -1) {
        errExit("lseek past EOF");
    }

    /* Write some more data after the hole */
    if (write(fd, "End", 3) != 3) {
        fatal("couldn't write after hole");
    }
    printf("2. Jumped to offset 30 and wrote 'End'. File size is now %ld bytes.\n", (long)lseek(fd, 0, SEEK_CUR));

    /* 4. Use lseek() to reset the pointer to the beginning for reading */
    if (lseek(fd, 0, SEEK_SET) == -1) {
        errExit("lseek reset");
    }

    /* 5. Use read() to pull 40 bytes (this will include the file hole) */
    numRead = read(fd, readBuf, 40);
    if (numRead == -1) {
        errExit("read");
    }

    /* 6. Display the bytes read in Hexadecimal to observe the null bytes in the hole */
    printf("3. Read %ld bytes from file. Hex dump of data:\n   ", (long)numRead);
    for (int i = 0; i < numRead; i++) {
        printf("%02x ", (unsigned char)readBuf[i]);
    }
    printf("\n");

    /* 7. Use close() to clean up the file descriptor */
    if (close(fd) == -1) {
        errExit("close");
    }

    exit(EXIT_SUCCESS);
}
