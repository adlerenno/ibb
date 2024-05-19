#include <stddef.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include "data.h"
#include "bwt.h"


void run(const char *filename, int layers);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: bwt FILENAME [LAYERS]\n");
        return -1;
    }
    char *filename = argv[1];

    int layers = 5;

    if (argc == 3) {
        layers = atoi(argv[2]);
    }

    printf("filename: %s, layers: %d\n", filename, layers);
//    return 0;

    run(filename, layers);

    return 0;
}

void run(const char *filename, int layers) {
    int f = open(filename, O_RDONLY);
    if (f == -1) {
        fprintf(stderr, "error opening file: %s %s", filename, strerror(errno));
        return;
    }

    printf("Opened File\n");


    ssize_t length;
    clock_t start = clock(), diff;

    sequence *c = getSequences(f, &length, layers / 2 + 1);

    diff = clock() - start;
    long msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("took %ld seconds %ld milliseconds to get Characters\n", msec / 1000, msec % 1000);

    printf("Created %zu Characters\n", length);

    start = clock();
    construct(f, layers, c, length);


    diff = clock() - start;

    free(c);
    close(f);

    msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("Time taken %ld seconds %ld milliseconds\n", msec / 1000, msec % 1000);
}