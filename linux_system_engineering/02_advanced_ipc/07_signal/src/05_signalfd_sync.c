#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>

int main(void) {
    sigset_t mask;
    int sfd;
    struct signalfd_siginfo fdsi;
    ssize_t s;

    printf("My PID is %d\n", getpid());
    printf("Demonstrating Synchronous Signal Handling with signalfd().\n");

    /* 1. Tạo mask chứa các signal muốn xử lý qua signalfd (SIGINT và SIGQUIT) */
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);

    /* 2. BẮT BUỘC phải block các signal này lại, 
       để ngăn không cho handler mặc định (hoặc async handler) được gọi.
       Thay vào đó, ta sẽ tự "đọc" chúng từ file descriptor. */
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    /* 3. Khởi tạo signalfd dựa trên mask */
    sfd = signalfd(-1, &mask, 0);
    if (sfd == -1) {
        perror("signalfd");
        exit(EXIT_FAILURE);
    }

    printf("Send SIGINT (Ctrl-C) to see the event read from fd.\n");
    printf("Send SIGQUIT (Ctrl-\\) to exit the program.\n");

    /* 4. Vòng lặp chính - đọc từ file descriptor */
    while (1) {
        /* Lệnh read sẽ block (chờ) cho đến khi có signal xuất hiện trong sfd */
        s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
        if (s != sizeof(struct signalfd_siginfo)) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        /* fdsi.ssi_signo chứa số hiệu signal */
        if (fdsi.ssi_signo == SIGINT) {
            printf("\n[SYNC] Read SIGINT from signalfd. Notice we didn't use any handler!\n");
        } else if (fdsi.ssi_signo == SIGQUIT) {
            printf("\n[SYNC] Read SIGQUIT from signalfd. Exiting cleanly.\n");
            break;
        } else {
            printf("Read unexpected signal\n");
        }
    }

    close(sfd);
    return 0;
}
