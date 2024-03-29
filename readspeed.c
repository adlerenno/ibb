#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>

size_t readf(int file, void *buf, size_t size) {

    size_t read_data = 0;

    size_t current;

    do {
        current = read(file, buf, size);
        if (current == -1) {
            fprintf(stderr, "error reading file %s\n", strerror(errno));
            return 0;
        }
        read_data += current;
    } while (current != 0);

    return read_data;
}


int main() {

    size_t pagesize = getpagesize();

    int file = open("1gb", O_RDONLY);
    if (file == -1) {
        fprintf(stderr, "failed opening file 1gb: %s\n", strerror(errno));
        return -1;
    }


    clock_t start = clock(), diff;
    for (int i = 64; i > 0; --i) {
        void *buf = mmap(NULL, i * pagesize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, (off_t) 0);
        if (buf == MAP_FAILED) {
            fprintf(stderr, "failed alloc of %d * paesize: %s\n", i, strerror(errno));
            continue;
        }

//        clock_t start = clock(), diff;
        readf(file, buf, i * pagesize);
//        diff = clock() - start;
//
//        long msec = diff * 1000 / CLOCKS_PER_SEC;
//        printf("mmap: %d took %ld seconds %ld milliseconds\n", i, msec / 1000, msec % 1000);

        lseek(file, 0, SEEK_SET);

        munmap(buf, i * pagesize);
//
//        buf = malloc(sizeof(u_int8_t) * pagesize * i);
//
//        start = clock(), diff;
//        readf(file, buf, i * pagesize);
//        diff = clock() - start;
//
//        msec = diff * 1000 / CLOCKS_PER_SEC;
//        printf("malloc: %d took %ld seconds %ld milliseconds\n", i, msec / 1000, msec % 1000);
//
//        lseek(file, 0, SEEK_SET);
//
//        free(buf);
    }
    diff = clock() - start;

    long msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("mmap took %ld seconds %ld milliseconds\n", msec / 1000, msec % 1000);


    start = clock();
    for (int i = 64; i > 0; --i) {
//        void *buf = mmap(NULL, i * pagesize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, (off_t) 0);
//        if (buf == MAP_FAILED) {
//            fprintf(stderr, "failed alloc of %d * paesize: %s\n", i, strerror(errno));
//            continue;
//        }
//
//        clock_t start = clock(), diff;
//        readf(file, buf, i * pagesize);
//        diff = clock() - start;
//
//        long msec = diff * 1000 / CLOCKS_PER_SEC;
//        printf("mmap: %d took %ld seconds %ld milliseconds\n", i, msec / 1000, msec % 1000);
//
//        lseek(file, 0, SEEK_SET);
//
//        munmap(buf, i * pagesize);

        void *buf = malloc(sizeof(u_int8_t) * pagesize * i);

//        start = clock(), diff;
        readf(file, buf, i * pagesize);
//        diff = clock() - start;

//        msec = diff * 1000 / CLOCKS_PER_SEC;
//        printf("malloc: %d took %ld seconds %ld milliseconds\n", i, msec / 1000, msec % 1000);

        lseek(file, 0, SEEK_SET);

        free(buf);
    }
    diff = clock() - start;

    msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("malloc took %ld seconds %ld milliseconds\n", msec / 1000, msec % 1000);

}