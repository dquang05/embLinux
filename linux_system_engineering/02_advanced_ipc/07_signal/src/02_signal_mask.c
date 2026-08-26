#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

/**
 * @brief Handler đơn giản cho SIGINT
 */
void sigint_handler(int sig) {
    (void)sig; /* Tránh warning unused parameter */
    /* KHÔNG NÊN dùng printf trong handler thực tế, 
       nhưng ở đây dùng để minh họa rõ ràng hành vi */
    const char msg[] = "\n[HANDLER] Received SIGINT! Note how this only prints once even if you pressed Ctrl-C multiple times while blocked.\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

/**
 * @brief In ra các signal đang pending
 */
void print_pending_signals(void) {
    sigset_t pending_set;
    if (sigpending(&pending_set) == -1) {
        perror("sigpending");
        return;
    }

    if (sigismember(&pending_set, SIGINT)) {
        printf("[MAIN] SIGINT is currently PENDING (waiting to be delivered).\n");
    } else {
        printf("[MAIN] No SIGINT pending.\n");
    }
}

int main(void) {
    struct sigaction sa;
    sigset_t block_set, old_set;

    /* 1. Thiết lập handler cho SIGINT */
    sa.sa_handler = sigint_handler;
    sa.sa_flags = 0;
    if (sigemptyset(&sa.sa_mask) == -1) {
        perror("sigemptyset");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* 2. Tạo một signal set chỉ chứa SIGINT */
    if (sigemptyset(&block_set) == -1 || sigaddset(&block_set, SIGINT) == -1) {
        perror("sigset ops");
        exit(EXIT_FAILURE);
    }

    printf("My PID is %d\n", getpid());
    printf("1. Normal state: press Ctrl-C to trigger handler.\n");
    sleep(3);

    /* 3. Block SIGINT */
    printf("\n2. Blocking SIGINT...\n");
    if (sigprocmask(SIG_BLOCK, &block_set, &old_set) == -1) {
        perror("sigprocmask SIG_BLOCK");
        exit(EXIT_FAILURE);
    }

    printf("SIGINT is now BLOCKED. Press Ctrl-C multiple times (e.g. 5 times).\n");
    printf("Notice that the program does NOT terminate and handler is NOT called.\n");
    
    /* Chờ 5 giây để user bấm Ctrl-C */
    for (int i = 5; i > 0; --i) {
        printf("Sleeping %d...\n", i);
        sleep(1);
        print_pending_signals();
    }

    /* 4. Unblock SIGINT */
    printf("\n3. Unblocking SIGINT...\n");
    if (sigprocmask(SIG_SETMASK, &old_set, NULL) == -1) {
        perror("sigprocmask SIG_SETMASK");
        exit(EXIT_FAILURE);
    }

    /* Ngay khi unblock, NẾU có SIGINT đang pending, handler sẽ được gọi NGAY LẬP TỨC.
       Hơn nữa, dù user bấm 5 lần Ctrl-C, handler cũng chỉ chạy 1 lần. */
    
    printf("\nSIGINT is UNBLOCKED. Program will exit now.\n");

    return 0;
}
