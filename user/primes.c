#include "kernel/types.h"
#include "user/user.h"

void work(int pfd[2]) {
    close(pfd[1]);
    int p;
    read(pfd[0], &p, 4);
    printf("prime %d\n", p);
    
    if (p >= 31) return ;

    int input_pipe[2];
    pipe(input_pipe);

    if (fork() == 0) {
        work(input_pipe);
    } else {
        close(input_pipe[0]);
        int n;
        while (read(pfd[0], &n, 4) > 0) {
            if (n % p != 0) {
                write(input_pipe[1], &n, 4);
            }
        }
        close(input_pipe[1]);
        close(pfd[0]);
        wait(0);
    }
}

int main(int argc, char *argv[]) {
    int input_pipe[2];
    pipe(input_pipe);

    if (fork() == 0) {
        work(input_pipe);
    } else {
        close(input_pipe[0]);
        for (int i = 2; i <= 35; i++) {
            write(input_pipe[1], &i, 4);
        }
        close(input_pipe[1]);
        wait(0);
    }

    exit(0);
}