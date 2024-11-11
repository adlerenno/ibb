#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include "data.h"
#include "bwt.h"


void run(const char *filename, int layers);

int main(int argc, char *argv[]) {
    int opt;
    int layers = 5;
    char *filename = "";
    char *output_filename = "";
    char *temp_dir = "";
    int processors = 1;

    while ((opt = getopt(argc, argv, "hi:o:t:k:p:")) != -1) {
        switch (opt) {
            case 'h':
            default:
                printf("Usage: \n\t./IBB [-i <input_file>] [-o <output_file>] [-t <temp_dir>] [-k <layers>] [-p <processor_count>] \nor\n\t./exc -h\n\n");
                printf("Default parameters: \nlayers: %d\nprocessor_count: %d\n", layers, processors);
                return 0;
            case 'i':
                filename = optarg;
                if (access(filename, F_OK) != 0) {
                    printf("Invalid input file.");
                    return -1;
                }
                break;
            case 'o':
                output_filename = optarg;
                /*if (access(output_filename, F_OK) == 0)
                {
                    printf("Output file exists. Choose other.");
                    return -1;
                }*/
                break;
            case 't':
                temp_dir = optarg;
                /*if (access(output_filename, F_OK) == 0)
                {
                    printf("Temp dir does not exist. Choose other.");
                    return -1;
                }*/
                break;
            case 'k':
                layers = atoi(optarg);
                if (layers < 3) {
                    printf("Invalid layers.");
                    return -1;
                }
                break;
            case 'p':
                processors = atoi(optarg);
                if (processors < 1) {
                    printf("Invalid processor count.");
                    return -1;
                }
                break;
        }
    }

    printf("input  file:\t%s,\n"
           "output file:\t%s,\n"
           "temp dir:\t%s,\n"
           "layers: %d,\n"
           "processors:\t%d\n", filename, output_filename, temp_dir, layers, processors);

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