#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "values.h"
#include "data.h"

typedef ssize_t Node[5];
typedef bool Leaf;

typedef struct bwt {
    Values Value;
    Node *Nodes;
    Leaf *Leaves;
    int File;
    ssize_t Layers;
} bwt;

void createDirs() {
#if defined(_WIN32) || defined(WIN32)
    mkdir("tmp");
#else
    mkdir("tmp", 0744);
#endif
}

const char *format = "tmp/%d.%d.tmp";

ssize_t insertRoot(bwt bwt1, sequence *pSequence, ssize_t length, ssize_t count, ssize_t rounds_left);

void insert(bwt bwt, sequence *seq, ssize_t length, int index, int layer, ssize_t rounds_left);

void insertLeaf(bwt bwt, sequence *seq, ssize_t length, int index);

uint8_t CmpChr(int layer, int index);

//void save_round(bwt bwt, sequence *seq, ssize_t length, ssize_t round_count) {
//    char c[50];
//
//    snprintf(c, 50, "s.c/%zd.c.txt", round_count);
//
//    FILE *f = fopen(c, "wb");
//    if (f == NULL) {
//        fprintf(stderr, "error opening file: %s %s\n", c, strerror(errno));
//        return;
//    }
//
//    fprintf(f, "BWT.Nodes:\n");
//
//    for (int i = 0; i < 1 << bwt.Layers; ++i) {
//        fprintf(f, "[%zd,%zd,%zd,%zd,%zd]\n", bwt.Nodes[i][0], bwt.Nodes[i][1], bwt.Nodes[i][2], bwt.Nodes[i][3],
//                bwt.Nodes[i][4]);
//    }
//
//    fprintf(f, "\nBWT.Leaf:\n[");
//
//    for (int i = 0; i < 1 << bwt.Layers; ++i) {
//        fprintf(f, "%d", bwt.Leaves[i]);
//    }
//
//    fprintf(f, "]\n\nSeq:\n");
//
//    for (int i = 0; i < length; ++i) {
//        fprintf(f, "{rank:%zd, c:%c, intVal:%d, pos:%zd, index:%zd, range:[%zd:%zd]}\n", seq[i].rank, seq[i].c,
//                seq[i].intVal,
//                seq[i].pos,
//                seq[i].index,
//                seq[i].range.start, seq[i].range.stop);
//    }
//
//    fclose(f);
//
//
////    if (round_count == 996) {
////        int i = system("pwsh -c cp -r tmp tmp-c-996");
////        printf("Copy: %d\n", i);
////    }
//}

void construct(int file, int layers, sequence *sequences, ssize_t length) {
    createDirs();

//    for (size_t size = 0; size < length; ++size) {
//        printf("[%zu:%zu]\n", sequences[size].range.start, sequences[size].range.stop);
//    }
//    fflush(stdout);

    bwt bwt = {
            .Value = New(),
            .Nodes = calloc(1 << layers, sizeof(Node)),
            .Leaves = calloc(1 << layers, sizeof(Leaf)),
            .File = file,
            .Layers = layers,
    };

    if (bwt.Nodes == NULL || bwt.Leaves == NULL) {
        fprintf(stderr, "Nodes or leaves are null\n");
        return;
    }

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
            sequences[length - 1].range.stop - sequences[length - 1].range.start + 1 + sequences[length - 1].index;

    ssize_t count = 0;

    printf("Round Count: %zd, index: %zd\n", totalRounds, sequences[length - 1].index);

    for (ssize_t i = 0; i < totalRounds; ++i) {
        count = insertRoot(bwt, sequences, length, count, totalRounds - i);

//        save_round(bwt, sequences, length, i);
//        printf("%02.02f%%\n", (double )i * 100 / (double )totalRounds);
    }

    for (int i = 0; i < length; ++i) {
        free(sequences[i].buf);
    }

    Destroy(bwt.Value);
    free(bwt.Nodes);
    free(bwt.Leaves);
}

ssize_t updateCount(bwt bwt1, sequence *pSequence, ssize_t length, ssize_t count, ssize_t left) {
    size_t c = 0;

    for (ssize_t i = length - count - 1; i >= 0; --i) {
        if (diff(pSequence, i) + pSequence[i].index == left - 1) {
            c++;
            count++;
            continue;
        }
        break;
    }

    add(bwt1.Value, pSequence + length - count, c);

    return count;
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
            return 5;
    }
}

uint8_t acgt_s(uint8_t c) {
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

int cmp(const void *a, const void *b) {
    return (int) ((*(sequence *) a).pos - (*(sequence *) b).pos);
}

ssize_t insertRoot(bwt bwt, sequence *pSequence, ssize_t length, ssize_t count, ssize_t rounds_left) {

    count = updateCount(bwt, pSequence, length, count, rounds_left);

    Node N = {0};

    sequence *seq = pSequence + length - count;

    for (ssize_t i = 0; i < count; ++i) {
        if (seq[i].index == 0) {
//            raise(SIGINT);
            readNextSeqBuffer(seq + i, bwt.File, bwt.Layers / 2 + 1);
        }
        uint8_t intVal = seq[i].intVal;
        seq[i].index--;
        if (seq[i].index == -1) {
            seq[i].c = '$';
        } else {
            seq[i].c = seq[i].buf[seq[i].index];
        }

        seq[i].pos = seq[i].rank + bwt.Nodes[0][intVal];
        seq[i].rank = 0;
        seq[i].intVal = acgt(seq[i].c);

        if (seq[i].intVal > 4) {
            fprintf(stderr, "intValue unexpected: %x, %x\n", seq[i].c, seq[i].intVal);
        }

        if (intVal < 1)
            N[1]++;
        if (intVal < 2)
            N[2]++;
        if (intVal < 3)
            N[3]++;
        if (intVal < 4)
            N[4]++;
    }

    for (ssize_t i = 0; i < count; ++i) {
        seq[i].pos += N[acgt_s(seq[i].buf[seq[i].index + 1])];
    }


    qsort(seq, count, sizeof(sequence), cmp);

    for (int i = 0; i < 5; ++i) {
        bwt.Nodes[0][i] += N[i];
    }

//    save_round(bwt, pSequence, length, rounds_left);

    insert(bwt, seq, count, 1, 0, rounds_left);

    return count;
}

void insert(bwt bwt, sequence *seq, ssize_t length, int index, int layer, ssize_t rounds_left) {
    if (layer >= bwt.Layers) {
        insertLeaf(bwt, seq, length, index ^ (1 << bwt.Layers));
        return;
    }

    ssize_t j = 0;

    ssize_t sum =
            bwt.Nodes[index][0] + bwt.Nodes[index][1] + bwt.Nodes[index][2] + bwt.Nodes[index][3] + bwt.Nodes[index][4];

    uint8_t chr = CmpChr(layer, index);
    for (; j < length; ++j) {
        if (seq[j].buf[seq[j].index + (ssize_t) layer / 2 + 1] > chr)
            break;

        bwt.Nodes[index][seq[j].intVal]++;
    }

    sum += j;

    for (ssize_t k = j; k < length; ++k) {
        seq[k].rank += bwt.Nodes[index][seq[k].intVal];

        if (sum > seq[k].pos) {
//            fprintf(stderr, "Sum > Pos: %zd > %zd; R: %zd; index: %zd, length: %zd\n", sum, seq[k].pos, rounds_left,
//                    seq[k].index, length);
            seq[k].pos = sum;
        }
        seq[k].pos -= sum;
    }

    if (length - j > 0)
        insert(bwt, seq + j, length - j, (index << 1) + 1, layer + 1, rounds_left);

    if (j > 0)
        insert(bwt, seq, j, index << 1, layer + 1, rounds_left);
}

uint8_t CmpChr(int layer, int index) {
    if ((layer % 2) == 0)
        return 'C';
    if ((index & 1) == 0)
        return 'A';
    return 'G';
}


void insertLeaf(bwt bwt, sequence *seq, ssize_t length, int index) {

    char name[50];
    snprintf(name, 50, format, index, bwt.Leaves[index]);
//    printf("reader: %s\t", name);

    FILE *reader = fopen(name, "rb");

    if (reader == NULL) {
        fprintf(stderr, "error opening file: %s: %s\n", name, strerror(errno));
        exit(-1);
    }

    bwt.Leaves[index] = !bwt.Leaves[index];

    snprintf(name, 50, format, index, bwt.Leaves[index]);
//    printf("writer: %s\n", name);
    FILE *writer = fopen(name, "wb");
    if (writer == NULL) {
        fprintf(stderr, "error opening file: %s: %s\n", name, strerror(errno));
        exit(-1);
    }

    uint8_t buffer[BUFSIZ];

    Node N = {0};
    ssize_t charCount = 0;
    bool finished = false;

    for (ssize_t i = 0; i < length; ++i) {
        while (charCount < seq[i].pos && !finished) {
            ssize_t read = (ssize_t) fread(buffer, 1, min(BUFSIZ, seq[i].pos - charCount), reader);
            if (read == 0) {
                if (feof(reader)) {
//                    fprintf(stderr, "unexpected end of file\n");
                    finished = true;
                    break;
                }
                fprintf(stderr, "Error reading leaf: %s\n", strerror(errno));
                continue;
            }

            charCount += read;

            for (int j = 0; j < read; ++j) {
                N[acgt_s(buffer[j])]++;
            }

            ssize_t w = fwrite(buffer, 1, read, writer);
            if (w != read) {
                fprintf(stderr, "short write: %s\n", strerror(errno));
            }
        }

        seq[i].rank += N[seq[i].intVal];
        N[seq[i].intVal]++;

        charCount++;

        buffer[0] = seq[i].c;
        size_t wrote = fwrite(buffer, 1, 1, writer);
        if (wrote != 1) {
            fprintf(stderr, "error writing file: %s\n", strerror(errno));
        }
    }

    if (!finished) {
        ssize_t read = fread(buffer, 1, BUFSIZ, reader);

        while (read != 0) {
            ssize_t wrote = fwrite(buffer, 1, read, writer);
            if (wrote != read) {
                fprintf(stderr, "short write: %s\n", strerror(errno));
            }
            read = fread(buffer, 1, BUFSIZ, reader);
            if (read == -1) {
                fprintf(stderr, "error reading: %s\n", strerror(errno));
                break;
            }
        }
    }

    fclose(reader);
    int r = fflush(writer);
    if (r) {
        fprintf(stderr, "error flushing: %s\n", strerror(errno));
    }
    fclose(writer);
}
