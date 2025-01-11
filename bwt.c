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
#include "constants.h"

typedef ssize_t Node[5];
typedef bool Leaf;

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

char *format;

ssize_t insertRoot(bwt *bwt1, sequence **pSequence, ssize_t length, ssize_t count, ssize_t rounds_left);

void insert(bwt bwt, sequence *seq, ssize_t length, int index, int layer, ssize_t sum_acc, Node node_acc);

void insertLeaf(bwt bwt, sequence *seq, ssize_t length, int index, ssize_t sum_acc, Node node_acc);

uint8_t CmpChr(int layer, int index);

uint8_t acgt(uint8_t c);

void insertRootSplit(bwt, sequence *, size_t, int, Node);

void insertRootSplitDA(bwt *, sequence *, size_t, size_t);

void combineBWT(const char *outFile, Leaf *leaves, ssize_t layers);

uint8_t toACGT(uint8_t c);

void worker_RootSplit(void *args) {
    root_split *a = args;
    insertRootSplit(*a->bwt, a->seq, a->length, a->i, a->acc);
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

void construct(int file, const char *temp_dir, ssize_t layers, int procs, sequence *sequences, ssize_t length,
               const char *output_filename) {
    format = calloc(LEAVE_PATH_LENGTH + 1, sizeof(char));
    if (format == NULL)
        return;
    snprintf(format, LEAVE_PATH_LENGTH, "%s/%%d.%%d.tmp", temp_dir);

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
    char name[LEAVE_PATH_LENGTH];

    for (int i = 0; i < 1 << layers; ++i) {
        for (int j = 0; j < 2; ++j) {
            snprintf(name, LEAVE_PATH_LENGTH, format, i, j);
            int f = open(name, O_CREAT | O_TRUNC | O_RDONLY, 0644);
            if (f == -1) {
                fprintf(stderr, "Error creating file %s, err: %s\n", name, strerror(errno));
                return;
            }
            close(f);
        }
    }
#ifdef VERBOSE
    printf("Created with layers = %zd\n", layers);
#endif

    ssize_t totalRounds =
            diff(sequences, length - 1) + 1 + sequences[length - 1].index;

    ssize_t count = 0;

#ifdef VERBOSE
    printf("Round Count: %zd, index: %zd\n", totalRounds, sequences[length - 1].index);
#endif

    for (ssize_t i = 0; i < totalRounds; ++i) {
        count = insertRoot(&bwt, &sequences, length, count, totalRounds - i);
    }

    for (int i = 0; i < length; ++i) {
        free(sequences[i].buf);
    }

    tpool_destroy(bwt.pool);
    Destroy(bwt.bitVec);
    free(bwt.Nodes);

    combineBWT(output_filename, bwt.Leaves, layers);

    free(bwt.Leaves);
}

ssize_t updateCount(bitVec bit, sequence *pSequence, ssize_t length, ssize_t count, ssize_t left) {
    size_t c = 0;

    for (ssize_t i = length - count - 1; i >= 0; --i) {
        if (diff(pSequence, i) + pSequence[i].index >= left - 1) {
            c++;
            count++;
            continue;
        }
        break;
    }

    add(bit, pSequence + length - count, c);

    return count;
}

//int cmp(const void *a, const void *b) {
//    return (int) ((*(sequence *) a).pos - (*(sequence *) b).pos);
//}

ssize_t insertRoot(bwt *bwt, sequence **pSequence, ssize_t length, ssize_t count, ssize_t rounds_left) {
    count = updateCount(bwt->bitVec, *pSequence, length, count, rounds_left);

    Node start;

    sort(&bwt->swap, length - count, pSequence, length, &start);

    Node stop = {
        start[1],
        start[2],
        start[3],
        start[4],
        length
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
            args->seq = *pSequence + s1;
            args->i = i - 1;
            args->length = s2 - s1;

            for (int j = 0; j < 5; ++j) {
                args->acc[j] = N2[j];
            }

            tpool_add_work(bwt->pool, worker_RootSplit, args);
        }
    }

    if (start[0] < stop[1]) {
        insertRootSplitDA(bwt, seq, stop[0] - start[0], stop[1] - start[1]);
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
            readNextSeqBufferParallel(&seq[j], bwt->File, bwt->Layers / 2 + 1);
        }

        seq[j].index--;
        if (seq[j].index == -1) {
            seq[j].c = '$';
        } else {
            seq[j].c = seq[j].buf[seq[j].index];
        }

        seq[j].pos = seq[j].rank + bwt->dollarCount + j - lengthD;
        seq[j].intVal = acgt(seq[j].c);
        seq[j].rank = 0;

        bwt->Nodes[0][seq[j].intVal]++;
    }

    Node zero = {0};

    insert(*bwt, seq, lengthD + lengthA, 4, 2, 0, zero);
}

void insertRootSplit(bwt bwt, sequence *seq, size_t length, int i, Node N) {
    for (ssize_t j = 0; j < length; ++j) {
        if (seq[j].index == 0) {
            readNextSeqBufferParallel(&seq[j], bwt.File, bwt.Layers / 2 + 1);
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

        bwt.Nodes[i][seq[j].intVal]++;
    }

    Node zero = {0};
    insert(bwt, seq, length, i + 4, 2, 0, zero);
}

void insert(bwt bwt, sequence *seq, ssize_t length, int i, int layer, ssize_t sum_acc, Node node_acc) {
    if (layer >= bwt.Layers) {
        insertLeaf(bwt, seq, length, i ^ (1 << bwt.Layers), sum_acc, node_acc);
        return;
    }

    ssize_t j = 0;

    Node N2 = {
        node_acc[0] + bwt.Nodes[i][0],
        node_acc[1] + bwt.Nodes[i][1],
        node_acc[2] + bwt.Nodes[i][2],
        node_acc[3] + bwt.Nodes[i][3],
        node_acc[4] + bwt.Nodes[i][4],
    };

    uint8_t chr = CmpChr(layer, i);
    for (; j < length; ++j) {
        if (seq[j].buf[seq[j].index + (ssize_t) layer / 2 + 1] > chr)
            break;

        bwt.Nodes[i][seq[j].intVal]++;
    }

    ssize_t sum =
            bwt.Nodes[i][0] + bwt.Nodes[i][1] + bwt.Nodes[i][2] + bwt.Nodes[i][3] + bwt.Nodes[i][4];

    // right subtree
    if (length - j > 0) {
        worker_args *a = malloc(sizeof(worker_args));
        a->bwt = bwt;
        a->length = length - j;
        a->seq = seq + j;
        a->k = layer + 1;
        a->i = (i << 1) + 1;
        a->sum_acc = sum_acc + sum;

        for (int i = 0; i < 5; ++i) {
            a->node_acc[i] = N2[i];
        }

        tpool_add_work(bwt.pool, worker_insert, a);
    }

    // left subtree
    if (j > 0)
        insert(bwt, seq, j, i << 1, layer + 1, sum_acc, node_acc);
}

uint8_t CmpChr(int layer, int index) {
    if ((layer % 2) == 0)
        return 'C';
    if ((index & 1) == 0)
        return 'A';
    return 'G';
}

const uint8_t MAX_COUNT = 0x1f;

void insertLeaf(bwt bwt, sequence *seq, ssize_t length, int index, ssize_t charCount, Node N) {
    char name[LEAVE_PATH_LENGTH];
    snprintf(name, LEAVE_PATH_LENGTH, format, index, bwt.Leaves[index]);

    int reader = open(name, O_RDONLY);

    if (!reader) {
        fprintf(stderr, "error opening file: %s: %s\n", name, strerror(errno));
        exit(-1);
    }

    bwt.Leaves[index] = !bwt.Leaves[index];

    snprintf(name, LEAVE_PATH_LENGTH, format, index, bwt.Leaves[index]);
    FILE *writer = fopen(name, "wb");
    if (writer == NULL) {
        fprintf(stderr, "error opening file: %s: %s\n", name, strerror(errno));
        exit(-1);
    }

    uint8_t buffer[BUFSIZ];
    uint8_t charBuf[1];
    uint8_t last = 0xff;

    bool finished = false;

    ssize_t max = 0, current = 0;

    for (ssize_t i = 0; i < length; ++i) {
        if (seq[i].pos < charCount) {
            fprintf(stderr, "pos < charCount: %zd < %zd\n", seq[i].pos, charCount);
            exit(-1);
        }

        while (charCount < seq[i].pos && !finished) {
            if (current >= max) {
                const size_t re = read(reader, buffer, BUFSIZ);

                if (re == -1) {
                    fprintf(stderr, "Error reading leaf: %s\n", strerror(errno));
                    continue;
                } else if (re == 0) {
                    fprintf(stderr, "unexpected end of file\n");
                    finished = true;
                    break;
                }
                current = 0;
                max = re;
            }

            size_t work = 0, offset = 0;

//
//            if ((buffer[current] & 0x07) == (last & 0x07)) {
//                uint8_t count = (buffer[current] >> 3) + (last >> 3);
//                if (count > MAX_COUNT) {
//                    N[last & 0x07] += MAX_COUNT - (last >> 3) ;
//                    charCount += MAX_COUNT - (last >> 3);
//
//                    last = MAX_COUNT << 3 | (last & 0x07);
//
//                    buffer[current] = (last & 0x07) | (count - MAX_COUNT) << 3;
//                } else {
//                    last = (count << 3) | (last & 0x07);
//
//                    ssize_t cc = buffer[current] >> 3;
//
//                    N[last & 0x07] += cc;
//                    current++;
//                    charCount += cc;
//                }
//                size_t wrote = fwrite(charBuf, 1, 1, writer);
//                if (wrote != 1) {
//                    fprintf(stderr, "error writing file: %s\n", strerror(errno));
//                }
//                work++;
//            } else
            if (last != 0xff) {
                size_t wrote = fwrite(charBuf, 1, 1, writer);
                if (wrote != 1) {
                    fprintf(stderr, "error writing file: %s\n", strerror(errno));
                }
            }

            last = 0xff;
            for (size_t j = current + work; j < max && charCount < seq[i].pos; ++j) {
                uint8_t c = buffer[j];
                uint8_t count = (c >> 3);
                charCount += count;
                N[c & 0x07] += count;
                ++work;
            }
            // Backtrack if too far
            if (charCount > seq[i].pos) {
                uint8_t diff = charCount - seq[i].pos;
                uint8_t c = buffer[current + work - 1];
                N[c & 0x07] -= diff;

                last = (((c >> 3) - diff) << 3) | c & 0x07;
                buffer[current + work - 1] = (diff << 3) | (last & 0x07);
                work--;
                charCount -= diff;
            } else if (charCount == seq[i].pos) {
                last = buffer[current + work - 1];
                work--;
                offset = 1;
            }

            if (work > 0) {
                size_t w = fwrite(buffer + current, 1, work, writer);
                if (w != work) {
                    fprintf(stderr, "short write: %s\n", strerror(errno));
                }
            }
            current += work + offset;
            offset = 0;
        }

        seq[i].rank += N[seq[i].intVal];

        charCount++;

        if (seq[i].intVal == (last & 0x07)) {
            uint8_t count = (last >> 3) + 1;
            if (count > MAX_COUNT) {
                charBuf[0] = last;
                size_t wrote = fwrite(charBuf, 1, 1, writer);
                if (wrote != 1) {
                    fprintf(stderr, "error writing file: %s\n", strerror(errno));
                }

                last = 1 << 3 | seq[i].intVal;
            } else {
                last = count << 3 | seq[i].intVal;
            }
        } else if (last != 0xff) {
            charBuf[0] = last;
            size_t wrote = fwrite(charBuf, 1, 1, writer);
            if (wrote != 1) {
                fprintf(stderr, "error writing file: %s\n", strerror(errno));
            }
            last = 1 << 3 | seq[i].intVal;
        } else {
            last = 1 << 3 | seq[i].intVal;
        }
    }

    if (!finished) {
        // check for remaining data in buf
        if (last != 0xff) {
            charBuf[0] = last;
            fwrite(charBuf, 1, 1, writer);
        }
        if (current != max) {
            fwrite(buffer + current, 1, max - current, writer);
        }

        size_t re = read(reader, buffer, BUFSIZ);

        while (re != 0 && re != -1) {
            size_t wrote = fwrite(buffer, 1, re, writer);
            if (wrote != re) {
                fprintf(stderr, "short write: %s\n", strerror(errno));
            }
            re = read(reader, buffer, BUFSIZ);
        }
        if (re == -1) {
            fprintf(stderr, "error reading: %s\n", strerror(errno));
        }
    }

    close(reader);
    int r = fclose(writer); // fclose flushes
    if (r) {
        fprintf(stderr, "error flushing: %s\n", strerror(errno));
    }
}

uint8_t acgt(const uint8_t c) {
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

void writeOutput(int out, const char *outFile, const uint8_t *writebuf, int *writesize) {
    const ssize_t w = write(out, writebuf, *writesize);
    if (w != *writesize) {
        fprintf(stderr, "error writing to outfile %s: %s\n", outFile, strerror(errno));
        return;
    }
    *writesize = 0;
}


void combineBWTExtend(const char *outFile, Leaf *leaves, ssize_t layers) {
    if (leaves == NULL) {
        fprintf(stderr, "combineBWT failed because of previous error\n");
        return;
    }


    int out = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (!out) {
        fprintf(stderr, "error opening out file %s: %s\n", outFile, strerror(errno));
        return;
    }

    char filename[100];
    uint8_t readbuf[BUFSIZ];
    uint8_t writebuf[BUFSIZ];

    int writesize = 0;

    for (int i = 0; i < (1 << layers); ++i) {
        snprintf(filename, 100, format, i, leaves[i]);
        int f = open(filename, O_RDONLY);
        if (!f) {
            fprintf(stderr, "error opening leaf-file %s: %s\n", filename, strerror(errno));
            return;
        }

        while (1) {
            const ssize_t n = read(f, readbuf, BUFSIZ);
            if (n == 0) {
                break;
            }
            if (n == -1) {
                fprintf(stderr, "error reading file %s: %s\n", filename, strerror(errno));
                return;
            }
            for (ssize_t j = 0; j < n; ++j) {
                const size_t count = readbuf[j] >> 3;
                const uint8_t c = toACGT(readbuf[j] & 0x7);
                int k = 0;
                for (; k < count && writesize < BUFSIZ; ++k) {
                    writebuf[writesize++] = c;
                }
                if (writesize + count >= BUFSIZ) {
                    writeOutput(out, outFile, writebuf, &writesize);
                }
                // assumes BUFSIZ > 32
                for (; k < count; ++k) {
                    writebuf[writesize++] = c;
                }
            }
        }
    }

    if (writesize != 0) {
        const ssize_t w = write(out, writebuf, writesize);
        if (w != writesize) {
            fprintf(stderr, "error writing to outfile %s: %s\n", outFile, strerror(errno));
        }
    }

    close(out);
}

uint8_t toACGT(uint8_t c) {
    switch (c) {
        case 1:
            return 'A';
        case 2:
            return 'C';
        case 3:
            return 'G';
        case 4:
            return 'T';
        default:
            return '$';
    }
}