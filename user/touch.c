#include <lib.h>

int main(int argc, char **argv) {
    int r;
    if (!((r = open(argv[1], O_RDONLY)) < 0)) {
        return -1;
    }
    if ((r = create(argv[1], FTYPE_REG)) < 0) {
        fprintf(1, "touch: cannot touch '%s': No such file or directory\n", argv[1]);
        return -1;
    }
    return 0;
}