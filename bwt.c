#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <stdio.h>

typedef int Node[5];
typedef int Leaf[2];

typedef struct bwt_t {
    int k;
    Node *Nodes;
    Leaf *Leafs;
} bwt_t;

void construct(int k) {

    bwt_t root = {.k = k};
    root.Nodes = calloc((1 << (k - 1)), sizeof(Node));
    root.Leafs = malloc(sizeof(Leaf) * (1 << (k - 1)));

    char s[100];

    for (unsigned int i = 0; i < 1 << (k - 1); ++i) {
        for (unsigned int j = 0; j < 2; ++j) {
            snprintf(s, 100, "tmp/%u.%u.tmp", i, j);
            int f1 = open(s, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
            if (f1 == -1) {
                fprintf(stderr, "error creating tmp file: k=%d %s %s\n", k, s, strerror(errno));
                goto close;
            }
            root.Leafs[i][j] = f1;

//            usleep(50 * 1000);
        }
    }

    printf("Created with k=%d\n", k);

    close:
    for (size_t i = 0; i < 1 << (k - 1); ++i) {
        for (int j = 0; j < 2; ++j) {
            close(root.Leafs[i][k]);
        }
    }

    free(root.Nodes);
    free(root.Leafs);
}

int main() {
    for (int i = 1; i < 12; ++i) {
        clock_t start = clock(), diff;

        construct(i);

        diff = clock() - start;

        long msec = diff * 1000 / CLOCKS_PER_SEC;
        printf("Time taken %ld seconds %ld milliseconds\n", msec / 1000, msec % 1000);
        printf("%d\n", 1 << (i-1));
    }
}