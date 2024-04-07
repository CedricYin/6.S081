#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char* get_filename(const char *path) {
    char *p = malloc(128);
    memset(p, 0, 128);
    char *p1 = p;
    int len = strlen(path);
    const char *p2 = path + len - 1;
    while (*p2 != '/') p2--;
    p2++;

    while (*p2 != 0) {
        *p1 = *p2;
        p1++;
        p2++;
    }
    return p;
}

void find(const char *path, const char *name) {
    struct stat st;
    struct dirent de;
    int fd;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }
    
    switch (st.type) {
        case T_FILE: {
            char *p = get_filename(path);
            if (strcmp(p, name) == 0) {
                printf("%s\n", path);
            }
            free(p);
            break;
        }

        case T_DIR: {
            char buf[512], *p;
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
                printf("find: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de)) {
                if(de.inum == 0)
                    continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                
                // do not recursively to ./ and ../
                if (strcmp(buf + strlen(buf) - 2, "/.") && strcmp(buf + strlen(buf) - 3, "/..")) {
                    find(buf, name);
                }
            }
            break;
        }
        default:
            break;
    }

    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(2, "Usage: find <path> <name>\n");
        exit(1);
    }

    struct stat st;
    stat(argv[1], &st);
    if (st.type != T_DIR) {
        fprintf(2, "<path> must be a dir, not a file\n");
        exit(1);
    }

    find(argv[1], argv[2]);

    exit(0);
}