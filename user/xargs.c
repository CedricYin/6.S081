#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

// run command for each line
void run(char *func, char **argv) {
    if (fork() == 0) {
        exec(func, argv);
    } else {
        int status;
        wait(&status);
        if (status != 0) {
            exit(status);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(2, "Usage: xargs <command> ...\n");
        exit(1);
    }
    char *args[MAXARG];
    memset(args, 0, MAXARG * sizeof(char *));
    uint args_size = 0;

    char c, *buf;
    buf = malloc(64);
    char *p = buf;

    for (int i = 1; i < argc; i++) {
        args[args_size++] = argv[i];
    }

    while (read(0, &c, 1) > 0) {
        if (c == '\n') {
            *p = 0;
            args[args_size++] = buf;
            run(argv[1], args);

            memset(args, 0, MAXARG * sizeof(char *));
            args_size = 0;
            for (int i = 1; i < argc; i++) {
                args[args_size++] = argv[i];
            }

            buf = malloc(64);
            p = buf;
            continue;
        }

        if (c == ' ') {
            *p = 0;
            args[args_size++] = buf;

            buf = malloc(64);
            p = buf;
            continue;
        }

        *p++ = c;
    }

    exit(0);
}