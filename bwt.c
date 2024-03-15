#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdlib.h>


typedef int Node[5];
typedef int Leaf[2];

typedef struct buffer {
    uint8_t *b;
    size_t size;
} buffer_t;

typedef struct bwt_t {
    int k;
    Node *Nodes;
    Leaf *Leafs;
    buffer_t b;
} bwt_t;

typedef struct characters {
    uint8_t *buf;
    uint16_t index;
    size_t pos;
    size_t rank;
} characters;

int cmp(const void *a, const void *b) {
    characters *aa = a, *bb = b;
    return (int) (aa->pos - bb->pos);
}

static inline int acgtToInt(const uint8_t c) {
    switch (c) {
        case '$':
            return 0;
        case 'A':
            return 1;
        case 'C':
            return 2;
        case 'G':
            return 3;
        case 'T':
            return 4;
        default:
            return 0;
    }
}

void addNodes(Node *a, const Node b) {
    for (int i = 0; i < 5; ++i) {
        *a[i] += b[i];
    }
}

void insertLeaf(bwt_t, characters *chars, size_t length, int i) {

}

#define cmpChr(i) (i % 2 == 0 ? 'C' : (j & 1) == 0 ? 'A' : 'G')

void insert(const bwt_t bwt, characters *chars, size_t length, int i, int k) {
    if (k > bwt.k+1)
        return insertLeaf(bwt, chars, length, i ^ (1 << (bwt.k-1)));


    int j = 0;
    // left subtree
    for (; j < length; ++j) {
        if (chars[j].buf[chars[j].index + k / 2] >= cmpChr(i))
            break;

        // update Count Array
        uint8_t c = acgtToInt(chars[j].buf[chars[j].index]);
        bwt.Nodes[i][c]++;
    }
    // right subtree
    for (int l = j; l < length; ++l) {
        // Update rank
        uint8_t c = acgtToInt(chars[j].buf[chars[j].index]);
        chars[l].rank += bwt.Nodes[i][c];
    }

    // left
    insert(bwt, chars, j, i << 1, k+1);

    // right
    insert(bwt, chars+j, length-j, (i << 1) + 1, k+1);
}


size_t insertRoot(bwt_t bwt, characters *chars, size_t length) {
    qsort(chars, length, sizeof(*chars), cmp);

    Node t = {};
    for (int i = 0; i < length; ++i) {
        // TODO position berechnen, rank zurücksetzten, daten laden, index setzten

        if (chars[i].index == 0) {
//             TODO neue daten laden
            // TODO daten entfernen
        }
        chars[i].index--;


        uint8_t c = acgtToInt(chars[i].buf[chars[i].index]);

        chars[i].pos = chars[i].rank + bwt.Nodes[0][c];
        chars[i].rank = 0;

        if (c < 1)
            t[1]++;
        if (c < 2)
            t[2]++;
        if (c < 3)
            t[3]++;
        if (c < 4)
            t[4]++;
    }

    addNodes(bwt.Nodes, t); // frist node

    insert(bwt, chars, length, 1, 2);

    return length;
}

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

    size_t page = sysconf(_SC_PAGESIZE);
    uint8_t *buffer = mmap(NULL, page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, (off_t) 0);

    if (buffer == MAP_FAILED) {
        fprintf(stderr, "Cannot map %ld pages (%ld bytes): %s.\n", (long) 1, (long) (1 * page), strerror(errno));
        return;
    }

    root.b.b = buffer;
    root.b.size = page;

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
        printf("%d\n", 1 << (i - 1));
    }
}