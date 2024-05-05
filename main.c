#include <stddef.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "data.h"
#include "bwt.h"


int main() {
    //    char *filename = "data/GRCh38_splitlength_3.fa";
    char *filename = "data/2048.raw";

    int f = open(filename, O_RDONLY);
    if (f == -1) {
        fprintf(stderr, "error opening file: %s %s", filename, strerror(errno));
        return -1;
    }

    printf("Opened File\n");


    size_t length;
    int levels = 2;
    clock_t start = clock(), diff;

    sequence *c = getSequences(f, &length, (levels + 1) / 2);

    diff = clock() - start;
    long msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("took %ld seconds %ld milliseconds to get Characters\n", msec / 1000, msec % 1000);

    printf("Created %zu Characters\n", length);

    start = clock();
    construct(f, levels, c, length);


    diff = clock() - start;

    free(c);

    msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("Time taken %ld seconds %ld milliseconds\n", msec / 1000, msec % 1000);

    return 0;
}