#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>

#include "tpool.h"
#include "values.h"
#include "bwt.h"

#define BUF_SIZE (1024 * 16)


typedef int Node[5];
typedef bool Leaf;

typedef struct bwt_t {
    Values values;
    int k;
    Node *Nodes;
    Leaf *Leafs;
    int file;
    tpool_t *pool;
} bwt_t;

int cmp(const void *a, const void *b) {
    return (int) (((sequence *) a)->pos - ((sequence *) b)->pos);
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

void addNodes(Node a, const Node b) {
    a[0] += b[0];
    a[1] += b[1];
    a[2] += b[2];
    a[3] += b[3];
    a[4] += b[4];

}

#define c(i) sequences[i].c // buf[chars[i].index]
#define min(a, b) (a < b ? a : b)

void insertLeaf(bwt_t bwt, sequence *sequences, size_t length, int i, size_t sum_acc, Node t) {

//    FILE *reader = bwt.Leafs[i][0], *writer = bwt.Leafs[i][1];

    char s[100];

    snprintf(s, 100, "tmp/%u.%u.tmp", i, bwt.Leafs[i]);

    FILE *reader = fopen(s, "rb");
    if (reader == NULL) {
        fprintf(stderr, "error opening tmp file: %s %s\n", s, strerror(errno));
        exit(-1);
    }

    bwt.Leafs[i] ^= 1;

    snprintf(s, 100, "tmp/%u.%u.tmp", i, bwt.Leafs[i]);

    FILE *writer = fopen(s, "wb");
    if (writer == NULL) {
        fprintf(stderr, "error opening tmp file: %s %s\n", s, strerror(errno));
        exit(-1);
    }


    uint8_t buffer[BUF_SIZE];

    size_t pos = sum_acc;
    bool finished = false;

    for (int j = 0; j < length; ++j) {
        while (pos < sequences[j].pos && !finished) {
            size_t current = fread(buffer, 1, min(BUF_SIZE, sequences[j].pos - pos), reader);
            if (!current || current == -1) {
                finished = true;
                continue;
            } else
                pos += current;

            // update count array
            for (int k = 0; k < current; ++k) {
                t[acgtToInt(buffer[k])]++;
            }

            size_t n = fwrite(buffer, 1, current, writer);
            if (n != current)
                fprintf(stderr, "error writing buffer to stream %zu, %s\n", n, strerror(errno));
        }

        sequences[j].rank = t[acgtToInt(c(j))]++;
        pos++;
        buffer[0] = c(j);
        size_t n = fwrite(buffer, 1, 1, writer);
        if (n != 1)
            fprintf(stderr, "error writing buffer to stream %zu, %s\n", n, strerror(errno));
    }

    // copy till the end
    if (!finished) {
        size_t current = fread(buffer, 1, BUF_SIZE, reader);

        while (current && current != -1) {
            size_t n = fwrite(buffer, 1, current, writer);
            if (n != current)
                fprintf(stderr, "error writing buffer to stream %zu, %s\n", n, strerror(errno));
            current = fread(buffer, 1, BUF_SIZE, reader);
        }
    }


    int n = fflush(writer);
    if (n != 0)
        fprintf(stderr, "error flushing buffer to stream %d, %s\n", n, strerror(errno));

    n = fclose(reader);
    if (n != 0)
        fprintf(stderr, "error closing stream %d, %s\n", n, strerror(errno));

    n = fclose(writer);
    if (n != 0)
        fprintf(stderr, "error closing stream %d, %s\n", n, strerror(errno));
}

void worker_insert(void *args);

typedef struct worker_args {
    bwt_t bwt;
    sequence *seq;
    size_t length;
    int i, k;
    size_t sum_acc;
    Node node_acc;
} worker_args;

#define cmpChr(k, i) (k % 2 == 0 ? 'C' : ((i & 1) == 0 ? 'A' : 'G'))

void NodesAdd(Node res, const Node a, const Node b) {
    res[0] = a[0] + b[0];
    res[1] = a[1] + b[1];
    res[2] = a[2] + b[2];
    res[3] = a[3] + b[3];
    res[4] = a[4] + b[4];
}

void insert(bwt_t bwt, sequence *sequences, size_t length, int i, int k, size_t sum_acc, Node node_acc) {
    if (k > bwt.k)
        return insertLeaf(bwt, sequences, length, i ^ (1 << (bwt.k - 1)), sum_acc, node_acc);


    int j = 0;
    // left subtree
    for (; j < length; ++j) {
        if (sequences[j].buf[sequences[j].index + (k / 2)] > cmpChr(k, i))
            break;

        // update Count Array
        uint8_t c = acgtToInt(c(j));
        bwt.Nodes[i][c]++;
    }

    // right
    if (length - j) {
        size_t sum =
                bwt.Nodes[i][0] + bwt.Nodes[i][1] + bwt.Nodes[i][2] + bwt.Nodes[i][3] + bwt.Nodes[i][4];
        worker_args *a = malloc(sizeof(worker_args));
        a->bwt = bwt;
        a->seq = sequences + j;
        a->length = length - j;
        a->i = (i << 1) + 1;
        a->k = k + 1;
        a->sum_acc = sum + sum_acc;
        NodesAdd(a->node_acc, bwt.Nodes[i], node_acc);
        tpool_add_work(bwt.pool, worker_insert, a);
//        Node n = {0};
//        NodesAdd(n, bwt.Nodes[i], node_acc);
//        insert(bwt, sequences + j, length - j, (i << 1) + 1, k + 1, sum+sum_acc, n);
    }

    // left
    if (j)
        insert(bwt, sequences, j, i << 1, k + 1, sum_acc, node_acc);
}


void worker_insert(void *args) {
    worker_args *a = args;
    insert(a->bwt, a->seq, a->length, a->i, a->k, a->sum_acc, a->node_acc);
    free(args);
}

size_t insertRoot(bwt_t bwt, sequence *seq, size_t length, size_t count, size_t roundsLeft) {

    {
        // get strings with length = rounds left-1
        size_t c = 0;

        // OPT: binary search
        for (ssize_t i = (ssize_t) (length - count - 1); i >= 0; --i) {
            // rounds-1 because of added $
            if (seq[i].range.stop - seq[i].range.start + seq[i].index >= roundsLeft - 1) {
                c++;
                count++;
            } else {
                break;
            }
        }

        add(bwt.values, seq - count + length, c);
    }

    sequence *sequences = seq + (length - count);

    Node t = {};
    for (int i = 0; i < count; ++i) {

        if (sequences[i].index == 0) {

            readNextSeqBuffer(&sequences[i], bwt.file, (bwt.k + 1) / 2);

        }


        uint8_t c = acgtToInt(c(i));
        sequences[i].index--;

        if (sequences[i].index == -1) {
            sequences[i].c = '$';
        } else {
            sequences[i].c = sequences[i].buf[sequences[i].index];
        }

        sequences[i].pos = sequences[i].rank + bwt.Nodes[0][c]; // rank + count_smaller vorgänger
        sequences[i].rank = 0;


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

    for (int i = 0; i < count; ++i) {
        sequences[i].pos += t[acgtToInt(sequences[i].buf[sequences[i].index + 1])];
    }

    if (count == 0)
        return 0;

    qsort(sequences, count, sizeof(*sequences), cmp);

    addNodes(bwt.Nodes[0], t); // first node

    Node n = {0};
    insert(bwt, sequences, count, 1, 2, 0, n);

    tpool_wait(bwt.pool);

    return count;
}

void ensureDir() {
    DIR *dir = opendir("tmp");
    if (dir) {
        closedir(dir);
    }
    if (errno == ENOENT) {
        // dir doesn't exist
#if defined(_WIN32) || defined(WIN32) // windows
        int n = mkdir("tmp");
#else // linux
        int n = mkdir("tmp", 0744);
#endif
        if (n != 0) {
            fprintf(stderr, "error creating tmp directory: %s\n", strerror(errno));
            exit(-1);
        }
    }
}


#define min(a, b) (a < b ? a : b)

void construct(int file, int layers, sequence *sequences, size_t length) {

    ensureDir();

    bwt_t bwt = {.k = layers, .file = file, .values = New(), .pool = tpool_create(min(length, 16))};

    // enter
    {
        bwt.Nodes = calloc((1 << (layers - 1)), sizeof(Node));
        bwt.Leafs = calloc((1 << (layers - 1)), sizeof(bool));

        char s[100];

        for (unsigned int i = 0; i < 1 << (layers - 1); ++i) {
            for (unsigned int j = 0; j < 2; ++j) {
                snprintf(s, 100, "tmp/%u.%u.tmp", i, j);
                FILE *f1 = fopen(s, "wb+");
                if (f1 == NULL) {
                    fprintf(stderr, "error creating tmp file: k=%d %s %s\n", layers, s, strerror(errno));
                    goto close;
                }
                fclose(f1);

            }
        }

    }

    printf("Created with layers = %d\n", layers);

    uint64_t totalSumOfChars = length;
    for (size_t i = 0; i < length; ++i) {
        totalSumOfChars += sequences[i].range.stop - sequences[i].range.start + sequences[i].index;
    }

    printf("Total inserting %zu characters\n", totalSumOfChars);

    size_t totalRounds =
            sequences[length - 1].range.stop - sequences[length - 1].range.start + 1 +
            (size_t) sequences[length - 1].index;

    size_t count = 0;

    size_t sumOfInsertedChars = 0, last = 0, diff = totalSumOfChars / 500;

    for (int i = 0; i < totalRounds; ++i) {
        count = insertRoot(bwt, sequences, length, count, totalRounds - i);

        sumOfInsertedChars += count;

        if (sumOfInsertedChars - last > diff) {
            printf("%02.02f%%\n",
                   (double) (sumOfInsertedChars) * 100 / (double) (totalSumOfChars));
            last = sumOfInsertedChars;
        }
    }

    close:

    for (size_t i = 0; i < length; ++i) {
        free(sequences[i].buf);
    }

    tpool_destroy(bwt.pool);
    Destroy(bwt.values);
    free(bwt.Nodes);
    free(bwt.Leafs);
}
