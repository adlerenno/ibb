#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

void openf();
void pure_open();


// return for
// $: 0
// A: 1
// C: 2
// G: 3
// T: 4
static inline char ACGT_to_int(char c) {
    return (c & 16) >> 2 | c & 2 |
            (!(((c & 2) >> 1) ^ ((c & 4) >> 2))) & 1;
}


void test() {
    char t[] = {'$', 'A', 'C', 'G', 'T'};

    for (int i = 0; i < 5; ++i) {
        printf("%c: %d\n", t[i], ACGT_to_int(t[i]));
    }
}


//int main() {
//
//    test();
//
////   openf();
////   pure_open();
//
//    return 0;
//}


void pure_open() {

    int fd = open("test", O_RDONLY);

    if (fd < 0) {
        printf("%d error occurred\n", errno);
        return;
    }

    char buf[1024];
    int read_count = 0, r;

    do {
        r = read(fd, &buf, 1024);
        read_count += r;
        printf("%.*s\n", r, buf);

    } while (r == 1024);

    printf("read %d bytes\n", read_count);


    int err = close(fd);
    if (err != 0)
        printf("An error occurred closing the file: %d\n", err);

}

void openf() {
    FILE *f = fopen("test", "r");

    if (f == NULL) {
        printf("Error opening file\n");
        printf("%d\n", errno);
        exit(-1);
    }

    unsigned long long read = 0;
    char read_buf[1024];
    unsigned long long r;

    do {
        r = fread((void *) &read_buf, sizeof(char), 1024, f);
        read += r;

        printf("%.*s\n", r, read_buf);

    } while (r == 1024);

    fclose(f);

    printf("Read %llu bytes\n", read);
}