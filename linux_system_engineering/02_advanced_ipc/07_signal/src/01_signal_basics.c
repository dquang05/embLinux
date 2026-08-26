#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

/* Biến cờ toàn cục an toàn để giao tiếp giữa handler và main */
volatile sig_atomic_t keep_running = 1;

/**
 * @brief Signal handler an toàn
 * Chỉ thực hiện các thao tác async-signal-safe (ví dụ: gán biến)
 */
void handle_signal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        keep_running = 0;
    }
}

int main(void) {
    struct sigaction sa;

    /* Khởi tạo struct sigaction */
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0; /* Không dùng cờ đặc biệt nào */
    
    /* Làm sạch sigset, không block thêm signal nào khi handler đang chạy (ngoại trừ signal đang được xử lý) */
    if (sigemptyset(&sa.sa_mask) == -1) {
        perror("sigemptyset");
        exit(EXIT_FAILURE);
    }

    /* Đăng ký handler cho SIGINT (Ctrl-C) */
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(EXIT_FAILURE);
    }

    /* Đăng ký handler cho SIGTERM (kill <pid>) */
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction SIGTERM");
        exit(EXIT_FAILURE);
    }

    /* Bỏ qua SIGQUIT (Ctrl-\) */
    struct sigaction sa_ign;
    sa_ign.sa_handler = SIG_IGN;
    sa_ign.sa_flags = 0;
    if (sigemptyset(&sa_ign.sa_mask) == -1) {
        perror("sigemptyset");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGQUIT, &sa_ign, NULL) == -1) {
        perror("sigaction SIGQUIT");
        exit(EXIT_FAILURE);
    }

    printf("My PID is %d\n", getpid());
    printf("Send SIGINT (Ctrl-C) or SIGTERM (kill -15 %d) to stop gracefully.\n", getpid());
    printf("Send SIGQUIT (Ctrl-\\) to see that it is ignored.\n");
    printf("Send SIGKILL (kill -9 %d) to force kill (cannot be caught).\n", getpid());

    /* Vòng lặp chính */
    int count = 0;
    while (keep_running) {
        printf("Working... %d\n", count++);
        sleep(1); /* Sẽ bị ngắt nếu có signal đến */
    }

    /* Sau khi keep_running được set thành 0 trong handler */
    printf("\nReceived termination signal. Cleaning up resources...\n");
    printf("Done. Exiting safely.\n");

    return 0;
}
