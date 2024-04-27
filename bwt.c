#include <string.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>

#include "data.h"


typedef int Node[5];
typedef FILE *Leaf[2];

typedef struct buffer {
    uint8_t *b;
    size_t size;
} buffer_t;

typedef struct bwt_t {
    int k;
    Node *Nodes;
    Leaf *Leafs;
    buffer_t b;
    FILE *file;
} bwt_t;

int cmp(const void *a, const void *b) {
    return (int) (((characters *) a)->pos - ((characters *) b)->pos);
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
        (*a)[i] += b[i];
    }
}

#define c(i) chars[i].c // buf[chars[i].index]
#define min(a, b) (a < b ? a : b)

void insertLeaf(bwt_t bwt, characters *chars, size_t length, int i) {

    FILE *reader = bwt.Leafs[i][0], *writer = bwt.Leafs[i][1];

    uint8_t *buffer = bwt.b.b;
    size_t page = bwt.b.size;

    size_t readd = 0;
    bool finished = false;

    Node t = {};
    for (int j = 0; j < length; ++j) {
        while (readd < chars[j].pos && !finished) {
            size_t current = fread(buffer, 1, min(page, chars[j].pos - readd), reader);
            if (!current || current == -1) {
                finished = true;
                continue;
            } else
                readd += current;

            // count array
            for (int k = 0; k < current; ++k) {
                t[acgtToInt(buffer[k])]++;
            }

            fwrite(buffer, 1, current, writer);
        }

        chars[j].rank += t[acgtToInt(c(j))]++;
        readd++;
        buffer[0] = c(j);
        fwrite(buffer, 1, 1, writer);
    }
    if (!finished) {
        size_t current = fread(buffer, 1, page, reader);

        while (current && current != -1) {
            fwrite(buffer, 1, current, writer);
            current = fread(buffer, 1, page, reader);
        }
    }


    fflush(writer);
    rewind(writer);
    rewind(reader);


    bwt.Leafs[i][0] = writer;
    bwt.Leafs[i][1] = reader;
}

#define cmpChr(k, i) (k % 2 == 0 ? 'C' : ((i & 1) == 0 ? 'A' : 'G'))

void insert(const bwt_t bwt, characters *chars, size_t length, int i, int k) {
    if (k > bwt.k)
        return insertLeaf(bwt, chars, length, i ^ (1 << (bwt.k - 1)));


    size_t sum =
            bwt.Nodes[i][0] + bwt.Nodes[i][1] + bwt.Nodes[i][2] + bwt.Nodes[i][3] + bwt.Nodes[i][4];

    int j = 0;
    // left subtree
    for (; j < length; ++j) {
        if (chars[j].buf[chars[j].index + (k / 2)] > cmpChr(k, i))
            break;

        // update Count Array
        uint8_t c = acgtToInt(c(j));
        bwt.Nodes[i][c]++;
    }

    // right subtree
    for (int l = j; l < length; ++l) {
        // Update rank
        uint8_t c = acgtToInt(c(j));
        chars[l].rank += bwt.Nodes[i][c];
        chars[l].pos -= sum + j;
    }

    // left
    if (j)
        insert(bwt, chars, j, i << 1, k + 1);

    // right
    if (length - j)
        insert(bwt, chars + j, length - j, (i << 1) + 1, k + 1);
}


size_t insertRoot(bwt_t bwt, characters *chars, size_t length) {

    Node t = {};
    for (int i = 0; i < length; ++i) {
        // TODO position berechnen, rank zurücksetzten, daten laden, index setzten

        if (chars[i].index == 0) {

            readChar(&chars[i], bwt.file, (bwt.k + 1) / 2);

        } else if (chars[i].index == -1) {
            free(chars[i].buf);
            chars[i--] = chars[--length];
            continue;
        }


        uint8_t c = acgtToInt(c(i));
        chars[i].index--;

        if (chars[i].index == -1) {
            chars[i].c = '$';
        } else {
            chars[i].c = chars[i].buf[chars[i].index];
        }

        chars[i].pos = chars[i].rank + bwt.Nodes[0][c]; // rank + count_smaller vorgänger
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

    Node a = {};
    for (int i = 1; i < 5; ++i) {
        a[i] = t[i - 1] + a[i - 1];
    }

    for (int i = 0; i < length; ++i) {
        chars[i].pos += t[acgtToInt(chars[i].buf[chars[i].index + 1])];
    }

    if (length == 0)
        return 0;

    qsort(chars, length, sizeof(*chars), cmp);

    addNodes(bwt.Nodes, t); // first node

    insert(bwt, chars, length, 1, 2);

    return length;
}

#define page (1024 * 4)

void construct(FILE *file, int k, characters *chars, size_t length) {


    bwt_t bwt = {.k = k, .b = {}, .file = file};

    // enter
    {
        bwt.Nodes = calloc((1 << (k - 1)), sizeof(Node));
        bwt.Leafs = malloc(sizeof(Leaf) * (1 << (k - 1)));

        char s[100];

        for (unsigned int i = 0; i < 1 << (k - 1); ++i) {
            for (unsigned int j = 0; j < 2; ++j) {
                snprintf(s, 100, "tmp/%u.%u.tmp", i, j);
                FILE *f1 = fopen(s, "wb+");
                if (f1 == NULL) {
                    fprintf(stderr, "error creating tmp file: k=%d %s %s\n", k, s, strerror(errno));
                    goto close;
                }
                bwt.Leafs[i][j] = f1;

            }
        }

        uint8_t *buffer = malloc(page);
        if (buffer == NULL) {
            fprintf(stderr, "Cannot malloc %ld pages (%ld bytes): %s.\n", (long) 1, (long) (1 * page), strerror(errno));
        }


        bwt.b.b = buffer;
        bwt.b.size = page;
    }

    printf("Created with k=%d\n", k);

    unsigned long long int i = 0;
    clock_t start = clock(), diff;
    while (length) {
        i++;
        length = insertRoot(bwt, chars, length);
//        printf("i: %llu, length: %zu\n", i, length);
        diff = clock() - start;
        long msec = diff * 1000 / CLOCKS_PER_SEC;
        if (msec > 1000) {

            printf("Time taken %ld seconds %ld milliseconds\n", msec / 1000, msec % 1000);
            diff = clock() - start;
            printf("Round: %llu, length: %zu\n", i, length);
            start = clock();
        }
    }

    printf("%llu\n", i);


    // write full bwt into single file for testing
    FILE *res = fopen("res.bwt", "w+");
    if (res == NULL) {
        fprintf(stderr, "error opening file: res.bwt %s", strerror(errno));
        goto close;
    }

    size_t n;
    for (int i = 0; i < 1 << (k-1); ++i) {
        do {
            n = fread(bwt.b.b, 1, bwt.b.size, bwt.Leafs[i][0]);
            if (n == -1) {
                fprintf(stderr, "error reading file: %d.?.tmp %s", i, strerror(errno));
                break;
            }
            fwrite(bwt.b.b, 1, n, res);
        } while (n != 0);
    }
    fclose(res);

    close:
    for (size_t i = 0; i < 1 << (k - 1); ++i) {
        for (int j = 0; j < 2; ++j) {
            fclose(bwt.Leafs[i][j]);
        }
    }


    // exit

    free(bwt.b.b);
    free(bwt.Nodes);
    free(bwt.Leafs);
}


int main() {

#if defined(_WIN32) || defined(WIN32)
    _setmaxstdio(1024);
#endif

    char *filename = "data/GRCh38_splitlength_3.fa";
//    char *filename = "data/2048.raw";

    FILE *f = fopen(filename, "rb");
    if (f == NULL) {
        fprintf(stderr, "error opening file: %s %s", filename, strerror(errno));
        return -1;
    }

    printf("Opened File\n");


    size_t length;
    int levels = 3;
    clock_t start = clock(), diff;

    characters *c = getCharacters(f, &length, (levels + 1) / 2);

    diff = clock() - start;
    long msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("took %ld seconds %ld milliseconds to get Characters\n", msec / 1000, msec % 1000);

    printf("Created %zu Characters\n", length);
    return 0;
    start = clock();
    construct(f, levels, c, length);


    diff = clock() - start;

    free(c);

    msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("Time taken %ld seconds %ld milliseconds\n", msec / 1000, msec % 1000);
}