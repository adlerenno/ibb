#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <libgen.h>
#include "data.h"
#include "bwt.h"


void run(const char *filename, const char *temp_dir, int layers, int processors, const char *output_filename);

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
                printf("Usage: \n\t%s [-i <input_file>] [-o <output_file>] [-t <temp_dir>] [-k <layers>] [-p <processor_count>] \nor\n\t%s -h\n\n", argv[0], argv[0]);
                printf("Default parameters: \n\tlayers: %d\n\tprocessor_count: %d\n", layers, processors);
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
                if (strcmp(output_filename, "") == 0)
                {
                    printf("Output file not specified correctly.");
                    return -1;
                }
                /*if (access(output_filename, F_OK) == 0)
                {
                    printf("Output file exists. Choose other.");
                    return -1;
                }*/
                break;
            case 't':
                temp_dir = optarg;
                if (access(temp_dir, F_OK) != 0)  // Does only check existence, not if it is a directory.
                {
                    printf("Temp dir does not exist. Choose other.");
                    return -1;
                }
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
    if (strcmp(temp_dir, "") == 0)
    {
        temp_dir = dirname(output_filename);
    }

#ifdef VERBOSE
    printf("Parameters:\n"
           "\tinput  file: %s\n"
           "\toutput file: %s\n"
           "\ttemp dir:\t %s\n"
           "\tlayers:\t\t %d\n"
           "\tprocessors:\t %d\n", filename, output_filename, temp_dir, layers, processors);
#endif

    run(filename, temp_dir, layers, processors, output_filename);

    return 0;
}

void run(const char *filename, const char *temp_dir, int layers, int processors, const char *output_filename) {
    int flag = 0;
#if defined(_WIN32) || defined(WIN32)
    flag = O_BINARY;
#endif

    int f = open(filename, O_RDONLY | flag);
    if (f == -1) {
        fprintf(stderr, "error opening file: %s %s", filename, strerror(errno));
        return;
    }

#ifdef VERBOSE
    printf("Opened File\n");
#endif

    ssize_t length;
#ifdef TIME
    struct timespec start, diff;
    clock_gettime(CLOCK_MONOTONIC, &start);
#endif


    sequence *c = getSequences(f, &length, layers / 2 + 1);

#ifdef TIME
    clock_gettime(CLOCK_MONOTONIC, &diff);
    long sec = (diff.tv_sec - start.tv_sec);
    long msec = (diff.tv_nsec - start.tv_nsec) / 1000000;
    printf("took %ld seconds %ld milliseconds to get Characters\n", sec, msec);
#endif

#ifdef VERBOSE
    printf("Created %zu Characters\n", length);
#endif

#ifdef TIME
    clock_gettime(CLOCK_MONOTONIC, &start);
#endif
    construct(f, temp_dir, layers, processors, c, length, output_filename);

#ifdef TIME
    clock_gettime(CLOCK_MONOTONIC, &diff);
#endif

    free(c);
    close(f);

#ifdef TIME
    sec = (diff.tv_sec - start.tv_sec);
    long min = sec / 60;
    sec %= 60;

    printf("Time taken %ldmin%lds\n", min, sec);
#endif
}
