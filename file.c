#include <stdint.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>

#define k 3

typedef struct data {
    char insert;
    size_t pos;
} data;

static inline size_t min(size_t a, size_t b) {
    return a > b ? b : a;
}

// assume data is in order
void writeData(int f1, int f2, data *data1, size_t length) {

    size_t page = sysconf(_SC_PAGESIZE);

    uint8_t *buffer = mmap(NULL, page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, (off_t) 0);

    if (buffer == MAP_FAILED) {
        fprintf(stderr, "Cannot map %ld pages (%ld bytes): %s.\n", (long) 1, (long) (1 * page), strerror(errno));
        return;
    }

    size_t a = 0;
    size_t r;
    bool finished = false;
    for (int i = 0; i < length; ++i) {
        while (a < data1[i].pos && !finished) {
//            r = fread(buffer, 1, min(page, data1[i].pos - a), f1);
            r = read(f1, buffer, min(
                    page, data1[i].pos - a
            ));
            if (!r) {
                fprintf(stderr, "read 0 bytes: %s\n", strerror(errno));
//                a = SIZE_MAX;
                finished = true;
            }

            a += r;
            // TODO smaller
//            fwrite(buffer, 1, r, f2);
            write(f2, buffer, r);
        }
        buffer[0] = data1[i].insert;
//        fwrite(buffer, 1, 1, f2);
        write(f2, buffer, 1);
    }
    if (!finished) {
        r = 1;
        while (r) {
//            r = fread(buffer, 1, page, f1);
                r = read(f1, buffer, page);
//            fwrite(buffer, 1, r, f2);
            write(f2, buffer, r);
        }
    }

    munmap(buffer, page);
}

//int main() {
//
//    int f1 = open("test1", O_RDWR);
//
//    if (!f1) {
//        printf("Error opening file 1: %s\n", strerror(errno));
//        printf("%d\n", errno);
//        exit(-1);
//    }
//    lseek(f1, 0, SEEK_SET);
//
////    FILE *f2 = fopen("test2", "rw+");
//    int f2 = open("test2", O_TRUNC | O_RDWR);
//
//
//    if (!f2) {
//        printf("Error opening file 2: %s\n", strerror(errno));
//        exit(-1);
//    }
//
//
//    data d[k] = {'0', 0, '1', 1, '2', 2};
//
//    for (int i = 0; i < 1000; ++i) {
//        writeData(f1, f2, d, k);
//
//        lseek(f1, 0, SEEK_SET);
//        lseek(f2, 0, SEEK_SET);
//
//        for (int j = 0; j < k; ++j) {
//            d[j].pos += j+1;
//        }
//
//        int t = f1;
//        f1 = f2;
//        f2 = t;
//    }
//
//
//
//    close(f1);
//    close(f2);
//}