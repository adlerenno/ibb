#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "popcount.h"
#include "data.h"
#include "tpool.h"

typedef ssize_t Node[5];
typedef bool Leaf;

__attribute__((aligned(64)))
typedef struct bwt {
    bitVec bitVec;
    Node *Nodes;
    Leaf *Leaves;
    sequence *swap;
    int File;
    ssize_t Layers;
    tpool_t *pool;
    size_t dollarCount;
} bwt;

typedef struct worker_args {
    bwt bwt;
    sequence *seq;
    ssize_t length;
    int i, k;
    ssize_t sum_acc;
    Node node_acc;
} worker_args;

typedef struct root_split {
    bwt *bwt;
    sequence *seq;
    ssize_t length;
    int i;
    Node acc;
} root_split;

void createDirs() {
#if defined(_WIN32) || defined(WIN32)
    mkdir("tmp");
#else
    mkdir("tmp", 0744);
#endif
}

const char *format = "tmp/%d.%d.tmp";

ssize_t insertRoot(bwt *bwt1, sequence **pSequence, ssize_t length, ssize_t count, ssize_t rounds_left);

void insert(bwt bwt, sequence *seq, ssize_t length, int index, int layer, ssize_t sum_acc, Node node_acc);

void insertLeaf(bwt bwt, sequence *seq, ssize_t length, int index, ssize_t sum_acc, Node node_acc);

uint8_t CmpChr(int layer, int index);

uint8_t acgt(uint8_t c);

void insertRootSplit(bwt *, sequence *, size_t, int, Node);

void insertRootSplitDA(bwt *, sequence *, size_t, size_t);

void worker_RootSplit(void *args) {
    root_split *a = args;
    insertRootSplit(a->bwt, a->seq, a->length, a->i, a->acc);
    free(args);
}

void worker_insert(void *args) {
    worker_args *a = args;
    insert(a->bwt, a->seq, a->length, a->i, a->k, a->sum_acc, a->node_acc);
    free(args);
}

void sort(sequence **swap, ssize_t skip, sequence **seq, ssize_t length, Node *nSmallerCopy) {
    Node n = {};

    for (ssize_t i = skip; i < length; ++i) {
        n[(*seq)[i].intVal]++;
    }

    Node nSmaller = {skip, 0, 0, 0, 0};

    for (int i = 0; i < 4; ++i) {
        (*nSmallerCopy)[i] = nSmaller[i];
        nSmaller[i + 1] = nSmaller[i] + n[i];
    }
    (*nSmallerCopy)[4] = nSmaller[4];

    for (ssize_t i = skip; i < length; ++i) {
        ssize_t p = nSmaller[(*seq)[i].intVal];
        nSmaller[(*seq)[i].intVal]++;

        (*swap)[p] = (*seq)[i];
    }
    sequence *s = *swap;
    *swap = *seq;
    *seq = s;
}

void construct(int file, int layers, int procs, sequence *sequences, ssize_t length) {
    createDirs();

    bwt bwt = {
            .bitVec = New((uint64_t) length),
            .Nodes = calloc(1 << layers, sizeof(Node)),
            .Leaves = calloc(1 << layers, sizeof(Leaf)),
            .File = file,
            .Layers = layers,
            .pool = tpool_create(min(procs, length)),
            .swap = malloc(length * sizeof(sequence)),
    };

    memcpy(bwt.swap, sequences, length * sizeof(sequence));

    if (bwt.Nodes == NULL || bwt.Leaves == NULL) {
        fprintf(stderr, "Nodes or leaves are null\n");
        return;
    }

    // init Leaves
    char name[25];

    for (int i = 0; i < 1 << layers; ++i) {
        for (int j = 0; j < 2; ++j) {
            snprintf(name, 25, format, i, j);
            int f = open(name, O_CREAT | O_TRUNC | O_RDONLY, 0644);
            if (f == -1) {
                fprintf(stderr, "Error creating file %s, err: %s\n", name, strerror(errno));
                return;
            }
            close(f);
        }
    }

    printf("Created with layers = %d\n", layers);

    ssize_t totalRounds =
            diff(sequences, length - 1) + 1 + sequences[length - 1].index;

    ssize_t count = 0;

//    uint64_t totalSumOfChars = length;
//    for (ssize_t i = 0; i < length; ++i) {
//        totalSumOfChars += sequences[i].range.stop - sequences[i].range.start + sequences[i].index;
//    }
//    size_t sumOfInsertedChars = 0;

    printf("Round Count: %zd, index: %zd\n", totalRounds, sequences[length - 1].index);

//    clock_t start = clock(), d;

    for (ssize_t i = 0; i < totalRounds; ++i) {
        count = insertRoot(&bwt, &sequences, length, count, totalRounds - i);

//        sumOfInsertedChars += count;
//        d = clock() - start;
//        if (d * 1000 / CLOCKS_PER_SEC > 5000) {
//            printf("%02.02f%% %ld\n",
//                   (double) (sumOfInsertedChars) * 100 / (double) (totalSumOfChars),
//                   d * 1000 / CLOCKS_PER_SEC);
//            start = clock();
//        }
    }

    for (int i = 0; i < length; ++i) {
        free(sequences[i].buf);
    }

    tpool_destroy(bwt.pool);
    Destroy(bwt.bitVec);
    free(bwt.Nodes);
    free(bwt.Leaves);
}

ssize_t updateCount(bwt bwt1, sequence *pSequence, ssize_t length, ssize_t count, ssize_t left) {
    size_t c = 0;

    for (ssize_t i = length - count - 1; i >= 0; --i) {
        if (diff(pSequence, i) + pSequence[i].index >= left - 1) {
            c++;
            count++;
            continue;
        }
        break;
    }

    add(bwt1.bitVec, pSequence + length - count, c);

    return count;
}

//int cmp(const void *a, const void *b) {
//    return (int) ((*(sequence *) a).pos - (*(sequence *) b).pos);
//}

ssize_t insertRoot(bwt *bwt, sequence **pSequence, ssize_t length, ssize_t count, ssize_t rounds_left) {

    count = updateCount(*bwt, *pSequence, length, count, rounds_left);

    Node start;

    sort(&bwt->swap, length - count, pSequence, length, &start);

    Node stop = {
            start[1],
            start[2],
            start[3],
            start[4],
            count
    };

    sequence *seq = *pSequence + length - count;

    Node N = {};
    // init N with bwt.Nodes
    memcpy(&N, bwt->Nodes, sizeof(Node));

    for (int i = 2; i < 5; ++i) {
        ssize_t s1 = start[i];
        ssize_t s2 = stop[i];

        Node N2 = {N[0], N[1], N[2], N[3], N[4]};

        // update before call, otherwise race condition
        for (int j = 0; j < 5; ++j) {
            N[j] += bwt->Nodes[i - 1][j];
        }

        if (s1 < s2) {
            root_split *args = malloc(sizeof(root_split));
            args->bwt = bwt;
            args->seq = seq + s1;
            args->i = i - 1;
            args->length = s2 - s1;

            for (int j = 0; j < 5; ++j) {
                args->acc[j] = N2[j];
            }

            tpool_add_work(bwt->pool, worker_RootSplit, args);
        }
    }

    if (start[0] < stop[1]) {
        insertRootSplitDA(bwt, seq, stop[0], stop[1] - start[1]);
    }

    tpool_wait(bwt->pool);

    return count;
}

void insertRootSplitDA(bwt *bwt, sequence *seq, size_t lengthD, size_t lengthA) {

    for (size_t j = 0; j < lengthD; ++j) {
        seq[j].index--;

        seq[j].c = seq[j].buf[seq[j].index];

        seq[j].pos = seq[j].rank;
        seq[j].intVal = acgt(seq[j].c);
        seq[j].rank = 0;

        bwt->Nodes[0][seq[j].intVal]++;
    }


    bwt->dollarCount += lengthD;

    for (size_t j = lengthD; j < lengthD + lengthA; ++j) {
        if (seq[j].index == 0) {
            readNextSeqBuffer(&seq[j], bwt->File, bwt->Layers / 2 + 1);
        }

        seq[j].index--;
        if (seq[j].index == -1) {
            seq[j].c = '$';
        } else {
            seq[j].c = seq[j].buf[seq[j].index];
        }

        seq[j].pos = seq[j].rank + bwt->dollarCount + j;
        seq[j].intVal = acgt(seq[j].c);
        seq[j].rank = 0;

        bwt->Nodes[0][seq[j].intVal]++;
    }

    Node zero = {0};

    insert(*bwt, seq, lengthD + lengthA, 4, 2, 0, zero);
}

void insertRootSplit(bwt *bwt, sequence *seq, size_t length, int index, Node N) {

    for (ssize_t j = 0; j < length; ++j) {
        if (seq[j].index == 0) {
            readNextSeqBuffer(&seq[j], bwt->File, bwt->Layers / 2 + 1);
        }

        seq[j].index--;
        if (seq[j].index == -1) {
            seq[j].c = '$';
        } else {
            seq[j].c = seq[j].buf[seq[j].index];
        }

        seq[j].pos = seq[j].rank + j;
        seq[j].intVal = acgt(seq[j].c);
        seq[j].rank = N[seq[j].intVal];

        bwt->Nodes[index][seq[j].intVal]++;
    }

    Node zero = {0};
    insert(*bwt, seq, length, index + 4, 2, 0, zero);
}

void insert(bwt bwt, sequence *seq, ssize_t length, int index, int layer, ssize_t sum_acc, Node node_acc) {
    if (layer >= bwt.Layers) {
        insertLeaf(bwt, seq, length, index ^ (1 << bwt.Layers), sum_acc, node_acc);
        return;
    }

    ssize_t j = 0;

    uint8_t chr = CmpChr(layer, index);
    for (; j < length; ++j) {
        if (seq[j].buf[seq[j].index + (ssize_t) layer / 2 + 1] > chr)
            break;

        bwt.Nodes[index][seq[j].intVal]++;
    }

    ssize_t sum =
            bwt.Nodes[index][0] + bwt.Nodes[index][1] + bwt.Nodes[index][2] + bwt.Nodes[index][3] + bwt.Nodes[index][4];

    // right subtree
    if (length - j > 0) {
        worker_args *a = malloc(sizeof(worker_args));
        a->bwt = bwt;
        a->length = length - j;
        a->seq = seq + j;
        a->k = layer + 1;
        a->i = (index << 1) + 1;
        a->sum_acc = sum_acc + sum;

        for (int i = 0; i < 5; ++i) {
            a->node_acc[i] = node_acc[i] + bwt.Nodes[index][i];
        }

        tpool_add_work(bwt.pool, worker_insert, a);
    }

    // left subtree
    if (j > 0)
        insert(bwt, seq, j, index << 1, layer + 1, sum_acc, node_acc);
}

uint8_t CmpChr(int layer, int index) {
    if ((layer % 2) == 0)
        return 'C';
    if ((index & 1) == 0)
        return 'A';
    return 'G';
}

void insertLeaf(bwt bwt, sequence *seq, ssize_t length, int index, ssize_t charCount, Node N) {

    char name[50];
    snprintf(name, 50, format, index, bwt.Leaves[index]);

    FILE *reader = fopen(name, "rb");

    if (reader == NULL) {
        fprintf(stderr, "error opening file: %s: %s\n", name, strerror(errno));
        exit(-1);
    }

    bwt.Leaves[index] = !bwt.Leaves[index];

    snprintf(name, 50, format, index, bwt.Leaves[index]);
    FILE *writer = fopen(name, "wb");
    if (writer == NULL) {
        fprintf(stderr, "error opening file: %s: %s\n", name, strerror(errno));
        exit(-1);
    }

    uint8_t buffer[BUFSIZ];

//    ssize_t charCount = sum;
    bool finished = false;

    for (ssize_t i = 0; i < length; ++i) {
        while (charCount < seq[i].pos && !finished) {
            size_t read = fread(buffer, 1, min(BUFSIZ, seq[i].pos - charCount), reader);
            if (read == 0) {
                if (feof(reader)) {
                    fprintf(stderr, "unexpected end of file\n");
                    finished = true;
                    break;
                }
                fprintf(stderr, "Error reading leaf: %s\n", strerror(errno));
                continue;
            }

            charCount += read;

            for (size_t j = 0; j < read; ++j) {
                N[buffer[j]]++;
            }

            size_t w = fwrite(buffer, 1, read, writer);
            if (w != read) {
                fprintf(stderr, "short write: %s\n", strerror(errno));
            }
        }

        seq[i].rank += N[seq[i].intVal];
//        N[seq[i].intVal]++;

        charCount++;

        buffer[0] = seq[i].intVal;
        size_t wrote = fwrite(buffer, 1, 1, writer);
        if (wrote != 1) {
            fprintf(stderr, "error writing file: %s\n", strerror(errno));
        }
    }

    if (!finished) {
        size_t read = fread(buffer, 1, BUFSIZ, reader);

        while (read != 0) {
            size_t wrote = fwrite(buffer, 1, read, writer);
            if (wrote != read) {
                fprintf(stderr, "short write: %s\n", strerror(errno));
            }
            read = fread(buffer, 1, BUFSIZ, reader);
        }
        if (!feof(reader)) {
            fprintf(stderr, "error reading: %s\n", strerror(errno));
        }
    }

    fclose(reader);
    int r = fflush(writer);
    if (r) {
        fprintf(stderr, "error flushing: %s\n", strerror(errno));
    }
    fclose(writer);
}

uint8_t acgt(uint8_t c) {
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