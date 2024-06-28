#include <lib.h>

int main(int argc, char **argv) {
    int r;
    // printf("%s man! What can I say !!!", argv[1]);
    if (strcmp(argv[1], "-p") == 0) {
        if (!((r = open(argv[2], O_RDONLY)) < 0)) {
            return -1;
        }
        if ((r = create(argv[2], FTYPE_DIR)) < 0) {
            char *path = argv[2];
            char p[MAXPATHLEN];
            int len = strlen(path);
            for (int i = 0; i < len; i++) {
                if (path[i] == '/') {
                    create(p, FTYPE_DIR);
                }
                p[i] = path[i];
            }
            create(p, FTYPE_DIR);
        }
    } else {
        if (!((r = open(argv[1], O_RDONLY)) < 0)) {
            fprintf(1, "mkdir: cannot create directory '%s': File exists\n", argv[1]);
            return -1;
        }
        if ((r = create(argv[1], FTYPE_DIR)) < 0) {
            fprintf(1, "mkdir: cannot create directory '$s': No such file or directory\n", argv[1]);
            return -1;
        }
    }
    return 0;
}