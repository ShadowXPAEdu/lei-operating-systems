#include <stdio.h>

int IsServerRunning(const char *path) {
    FILE *fptr = fopen(path, "r+");

    if (fptr == NULL)
        return 0;

    fclose(fptr);
    return -1;
}
