#include <lib.h>

int main(int argc, char **argv) {
    struct Fd *fd;
	struct Filefd *ffd;
    int r;
    if (strcmp(argv[1], "-r") == 0) {
        if ((r = open(argv[2], O_RDONLY)) < 0) {
            fprintf(1, "rm: cannot remove '%s': No such file or directory\n", argv[2]);
            return -1;
        }
        if ((r = remove(argv[2])) < 0) {
            return -1;
        }
    } else if (strcmp(argv[1], "-rf") == 0) {
        if ((r = open(argv[2], O_RDONLY)) < 0) {
            return -1;
        }
        if ((r = remove(argv[2])) < 0) {
            return -1;
        }
    } else {
        if ((r = open(argv[1], O_RDONLY)) < 0) {
            fprintf(1, "rm: cannot remove '%s': No such file or directory\n", argv[1]);
            return -1;
        }
        r = open(argv[1], O_GETTYPE);
        if (r == -1147) {
            fprintf(1, "rm: cannot remove '%s': Is a directory\n", argv[1]);
            return -1;
        } else {
            remove(argv[1]);
        }
    }
    return 0;
}