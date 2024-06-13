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

    run(filename, layers);

    return 0;
}

void run(const char *filename, int layers) {
    int flag = 0;
#if defined(_WIN32) || defined(WIN32)
    flag = O_BINARY;
#endif

    int f = open(filename, O_RDONLY | flag);
    if (f == -1) {
        fprintf(stderr, "error opening file: %s %s", filename, strerror(errno));
        return;
    }

    printf("Opened File\n");


    ssize_t length;
    struct timespec start, diff;
    clock_gettime(CLOCK_MONOTONIC, &start);


    sequence *c = getSequences(f, &length, layers / 2 + 1);

    clock_gettime(CLOCK_MONOTONIC, &diff);
    long sec = (diff.tv_sec - start.tv_sec);
    long msec = (diff.tv_nsec - start.tv_nsec) / 1000000;
    printf("took %ld seconds %ld milliseconds to get Characters\n", sec, msec);

    printf("Created %zu Characters\n", length);

    clock_gettime(CLOCK_MONOTONIC, &start);
    construct(f, layers, c, length);


    clock_gettime(CLOCK_MONOTONIC, &diff);

    free(c);
    close(f);

    sec = (diff.tv_sec - start.tv_sec);
    long min = sec / 60;
    sec %= 60;

    printf("Time taken %ldmin%lds\n", min, sec);
}