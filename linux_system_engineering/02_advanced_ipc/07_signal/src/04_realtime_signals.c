#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

/* Sử dụng SIGRTMIN làm tín hiệu kiểm thử */
#define TEST_SIG (SIGRTMIN + 1)

/**
 * @brief Handler cho Realtime signal, cần SA_SIGINFO để lấy dữ liệu đính kèm
 */
void rt_handler(int sig, siginfo_t *info, void *ucontext) {
    (void)ucontext;
    
    /* Chú ý: Dùng printf trong handler không an toàn, 
       nhưng dùng ở đây để minh họa cho sinh viên dễ thấy */
    printf("[HANDLER] Received Realtime Signal %d with payload (int): %d\n", 
           sig, info->si_value.sival_int);
}

int main(void) {
    struct sigaction sa;
    sigset_t block_set, old_set;
    pid_t child_pid;

    printf("Setting up realtime signal handler for signal %d\n", TEST_SIG);

    /* 1. Thiết lập handler */
    sa.sa_sigaction = rt_handler;
    sa.sa_flags = SA_SIGINFO; 
    sigemptyset(&sa.sa_mask);
    if (sigaction(TEST_SIG, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* 2. Block tín hiệu này trong Parent để chứng minh tính chất xếp hàng */
    sigemptyset(&block_set);
    sigaddset(&block_set, TEST_SIG);
    if (sigprocmask(SIG_BLOCK, &block_set, &old_set) == -1) {
        perror("sigprocmask block");
        exit(EXIT_FAILURE);
    }
    printf("[PARENT] Blocked signal %d. Spawning child to send signals...\n", TEST_SIG);

    /* 3. Fork tiến trình con để gửi signal */
    child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        /* ----- TIẾN TRÌNH CON ----- */
        printf("[CHILD] I will send 5 realtime signals to parent %d\n", getppid());
        
        for (int i = 1; i <= 5; ++i) {
            union sigval value;
            value.sival_int = i * 100; /* Gửi kèm một con số: 100, 200, 300... */
            
            printf("[CHILD] Sending signal %d with payload %d\n", TEST_SIG, value.sival_int);
            if (sigqueue(getppid(), TEST_SIG, value) == -1) {
                perror("sigqueue");
                exit(EXIT_FAILURE);
            }
        }
        
        printf("[CHILD] Done sending. Exiting.\n");
        exit(EXIT_SUCCESS);
    } else {
        /* ----- TIẾN TRÌNH CHA ----- */
        /* Chờ một chút để con gửi xong toàn bộ 5 signals */
        sleep(2);
        
        printf("\n[PARENT] Child should have finished sending. Unblocking signal %d...\n", TEST_SIG);
        printf("[PARENT] Notice that ALL 5 signals are queued and handler will run 5 times IN ORDER.\n");
        
        /* 4. Unblock tín hiệu. Ngay khi unblock, handler sẽ được kích hoạt 5 lần liên tiếp */
        if (sigprocmask(SIG_SETMASK, &old_set, NULL) == -1) {
            perror("sigprocmask unblock");
            exit(EXIT_FAILURE);
        }

        /* Chờ con kết thúc */
        wait(NULL);
        printf("\n[PARENT] Done.\n");
    }

    return 0;
}
