#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int pfd1[2], pfd2[2];

    if (argc != 1) {
        fprintf(2, "Usage: pingpong\n");
        exit(1);
    }

    pipe(pfd1);
    pipe(pfd2);

    if (fork() == 0) {
        char buf[1];
        read(pfd1[0], buf, 1);
        printf("%d: received ping\n", getpid());
        write(pfd2[1], buf, 1);
    } else {
        char buf[1];
        write(pfd1[1], "a", 1);
        read(pfd2[0], buf, 1);
        printf("%d: received pong\n", getpid());
    }

    exit(0);
}