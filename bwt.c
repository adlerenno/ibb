#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "tpool.h"
#include "values.h"
#include "bwt.h"

#define BUF_SIZE (1024 * 16)

const char fileFormat[] = "tmp/%u.%u.tmp";

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

static int cmp(const void *a, const void *b) {
    return (int) (((sequence *) a)->pos - ((sequence *) b)->pos);
}

inline uint8_t acgtToInt(const uint8_t c) {
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

void insertLeaf(bwt_t bwt, sequence *sequences, size_t length, int i) {

    char s[100];

    snprintf(s, 100, fileFormat, i, bwt.Leafs[i]);

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

    Node t = {0};
    size_t pos = 0;
    bool finished = false;

    for (size_t j = 0; j < length; ++j) {
        while (pos < sequences[j].pos && !finished) {
            size_t current = fread(buffer, 1, min(BUF_SIZE, sequences[j].pos - pos), reader);
            if (!current) {
                finished = true;
                fprintf(stderr, "error reading buffer, code: %zu, error %s, pos: %zu, next: %zu\n", current,
                        strerror(errno), pos, sequences[j].pos);
                break;
            } else
                pos += current;

            // update count array
            for (size_t k = 0; k < current; ++k) {
                t[acgtToInt(buffer[k])]++;
            }

            size_t n = fwrite(buffer, 1, current, writer);
            if (n != current)
                fprintf(stderr, "error writing buffer to stream %zu, %s\n", n, strerror(errno));
        }

        sequences[j].rank += t[sequences[j].intVal];
        t[sequences[j].intVal]++;
        pos++;

        buffer[0] = c(j);
        size_t n = fwrite(buffer, 1, 1, writer);
        if (n != 1)
            fprintf(stderr, "error writing buffer to stream %zu, %s\n", n, strerror(errno));
    }

    // copy till the end
    if (!finished) {
        size_t current = fread(buffer, 1, BUF_SIZE, reader);

        while (current) {
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
//    size_t sum_acc;
//    Node node_acc;
} worker_args;

#define cmpChr(k, i) (k % 2 == 0 ? 'C' : ((i & 1) == 0 ? 'A' : 'G'))

void NodesAdd(Node res, const Node a, const Node b) {
    res[0] = a[0] + b[0];
    res[1] = a[1] + b[1];
    res[2] = a[2] + b[2];
    res[3] = a[3] + b[3];
    res[4] = a[4] + b[4];
}

void insert(bwt_t bwt, sequence *sequences, size_t length, int i, int layer) {
    if (layer >= bwt.k)
        return insertLeaf(bwt, sequences, length, i ^ (1 << bwt.k));

    if (i > 1 << bwt.k) {
        fprintf(stderr, "Should not happen");
        exit(-2);
    }


    size_t j = 0;
    // left subtree
    for (; j < length; ++j) {
        if (sequences[j].buf[sequences[j].index + (layer / 2) + 1] > cmpChr(layer, i))
            break;

        // update Count Array
//        uint8_t c = acgtToInt(c(j));
        bwt.Nodes[i][sequences[j].intVal]++;
    }

    size_t sum =
            bwt.Nodes[i][0] + bwt.Nodes[i][1] + bwt.Nodes[i][2] + bwt.Nodes[i][3] + bwt.Nodes[i][4];

    for (size_t ii = j; ii < length; ++ii) {
        sequences[ii].rank += bwt.Nodes[i][sequences[ii].intVal];
        if (sum > sequences[ii].pos) {
            fprintf(stderr, "sum is greater than the pos: %zu, %zu; index: %zu\n", sum, sequences[ii].pos,
                    sequences[ii].index);
            exit(-3);
        }
        sequences[ii].pos -= sum;
    }

    // right
    if (length - j) {

//        worker_args *a = malloc(sizeof(worker_args));
//        a->bwt = bwt;
//        a->seq = sequences + j;
//        a->length = length - j;
//        a->i = (i << 1) + 1;
//        a->k = k + 1;
//        a->sum_acc = sum + sum_acc;
//        NodesAdd(a->node_acc, bwt.Nodes[i], node_acc);
//        tpool_add_work(bwt.pool, worker_insert, a);
//        Node n = {0};
//        NodesAdd(n, bwt.Nodes[i], node_acc);
        insert(bwt, sequences + j, length - j, (i << 1) + 1, layer + 1);
    }

    // left
    if (j)
        insert(bwt, sequences, j, i << 1, layer + 1);
}


void worker_insert(void *args) {
    worker_args *a = args;
    insert(a->bwt, a->seq, a->length, a->i, a->k);
    free(args);
}

size_t updateCount(bwt_t bwt, sequence *seq, size_t length, size_t count, size_t roundsLeft) {
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
    return count;
}

size_t insertRoot(bwt_t bwt, sequence *seq, size_t length, size_t count, size_t roundsLeft) {

    count = updateCount(bwt, seq, length, count, roundsLeft);

    sequence *sequences = seq + (length - count);

    Node N = {};
    for (size_t i = 0; i < count; ++i) {

        if (sequences[i].index == 0) {

            readNextSeqBuffer(&sequences[i], bwt.file, (bwt.k + 1) / 2);

        }


        uint8_t intVal = sequences[i].intVal;
        sequences[i].index--;

        if (sequences[i].index == -1) {
            sequences[i].c = '$';
        } else {
            sequences[i].c = sequences[i].buf[sequences[i].index];
        }

        sequences[i].pos = sequences[i].rank + bwt.Nodes[0][intVal]; // rank + count_smaller vorgänger
        sequences[i].rank = 0;
        sequences[i].intVal = acgtToInt(c(i));


        if (intVal < 1)
            N[1]++;
        if (intVal < 2)
            N[2]++;
        if (intVal < 3)
            N[3]++;
        if (intVal < 4)
            N[4]++;
    }

    // count_smaller inkludiert die hinzuzufügenden Zeichen noch nicht
    for (size_t i = 0; i < count; ++i) {
        sequences[i].pos += N[acgtToInt(sequences[i].buf[sequences[i].index + 1])];
    }

    if (count == 0)
        return 0;

    qsort(sequences, count, sizeof(sequence), cmp);

    addNodes(bwt.Nodes[0], N); // first node

//    Node n = {0};
    insert(bwt, sequences, count, 1, 0);

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

    bwt_t bwt = {.k = layers, .file = file, .values = New(),
            .pool = tpool_create(min(length, 16))
    };

    // enter
    {
        bwt.Nodes = calloc(1 << layers, sizeof(Node));
        bwt.Leafs = calloc(1 << layers, sizeof(bool));

        char s[100];

        for (int i = 0; i < 1 << layers; ++i) {
            for (unsigned int j = 0; j < 2; ++j) {
                snprintf(s, 100, fileFormat, i, j);
                int f = open(s, O_CREAT | O_RDONLY | O_TRUNC, 644);
                if (f == 0) {
                    fprintf(stderr, "error creating tmp file: k=%d %s %s\n", layers, s, strerror(errno));
                    goto close;
                }
                close(f);

            }
        }

    }

    printf("Created with layers = %d\n", layers);

    printf("Longest Seq: %zu, shortest Seq: %zu\n",
           sequences[length - 1].range.stop - sequences[length - 1].range.start + sequences[length - 1].index,
           sequences[0].range.stop - sequences[0].range.start + sequences[0].index);

    uint64_t totalSumOfChars = length;
    for (size_t i = 0; i < length; ++i) {
        totalSumOfChars += sequences[i].range.stop - sequences[i].range.start + sequences[i].index;
    }

    printf("Total inserting %zu characters\n", totalSumOfChars);

    size_t totalRounds =
            sequences[length - 1].range.stop - sequences[length - 1].range.start + 1 + sequences[length - 1].index;

    size_t count = 0;

    size_t sumOfInsertedChars = 0, last = 0, diff = totalSumOfChars / 1000;

    for (size_t i = 0; i < totalRounds; ++i) {
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
