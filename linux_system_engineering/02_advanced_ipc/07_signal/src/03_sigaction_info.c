#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

/**
 * @brief Handler với 3 tham số để lấy thông tin chi tiết (SA_SIGINFO)
 */
void siginfo_handler(int sig, siginfo_t *info, void *ucontext) {
    (void)ucontext; /* Không dùng ucontext */
    
    printf("\n[HANDLER] Received signal %d (%s)\n", sig, strsignal(sig));
    
    /* Kiểm tra xem signal được gửi từ đâu */
    if (info->si_code == SI_USER) {
        printf("[HANDLER] Sent by user process:\n");
        printf("  - Sender PID: %d\n", info->si_pid);
        printf("  - Sender UID: %d\n", info->si_uid);
    } else if (info->si_code == SI_KERNEL) {
        printf("[HANDLER] Sent by kernel.\n");
    } else {
        printf("[HANDLER] si_code = %d\n", info->si_code);
    }
}

int main(void) {
    struct sigaction sa;

    printf("My PID is %d\n", getpid());
    
    /* Thiết lập handler với SA_SIGINFO */
    sa.sa_sigaction = siginfo_handler; /* Lưu ý dùng sa_sigaction thay vì sa_handler */
    sa.sa_flags = SA_SIGINFO;          /* Bắt buộc phải có cờ này */
    sigemptyset(&sa.sa_mask);

    /* Đăng ký bắt SIGUSR1 và SIGUSR2 */
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction SIGUSR2");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for signals... (e.g., run `kill -SIGUSR1 %d` in another terminal)\n", getpid());
    printf("Press Ctrl-C to exit.\n");

    /* Chờ tín hiệu */
    while (1) {
        pause(); /* Tạm dừng process cho đến khi có signal */
    }

    return 0;
}
