#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int tick;

    if (argc != 2) {
        fprintf(2, "Usage: sleep <n>\n");
        exit(1);
    }

    tick = atoi(argv[1]);

    sleep(tick);

    exit(0);
}