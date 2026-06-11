/* syscall_speed.c

   By repeatedly invoking a simple system call (getppid()), we can get some
   idea of the cost of making system calls.

   Usage: time syscall_speed numcalls
                           Def=10000000

   Compiling with -DNOSYSCALL causes a call to a simple function
   returning an integer, which can be used to compare the overhead
   of a simple function call against that of a system call.
*/
#include "tlpi_hdr.h"

#ifdef NOSYSCALL
static int myfunc() { return 1; }
#endif
int
main(int argc, char *argv[])
{
    int numCalls = (argc > 1) ? getInt(argv[1], GN_GT_0, "num-calls")
                              : 10000000;

#ifdef NOSYSCALL
        printf("Calling normal function\n");
#else
        printf("Calling getppid()\n");
#endif

    for (int j = 0; j < numCalls; j++)
#ifdef NOSYSCALL
        myfunc();
#else
        getppid();
#endif

    exit(EXIT_SUCCESS);
}