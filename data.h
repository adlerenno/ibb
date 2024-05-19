#ifndef BACHELORARBEIT_DATA_H
#define BACHELORARBEIT_DATA_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define diff(a, b) ((a)[b].range.stop - (a)[b].range.start)
#define min(a,b) (a < b ? a : b)

typedef struct Range {
    ssize_t start;
    ssize_t stop;
} Range;

typedef struct sequence {
    uint8_t *buf;
    uint8_t c;
    uint8_t intVal;
    ssize_t index;
    ssize_t pos;
    ssize_t rank;
    Range range;
} sequence;

#define charBuffer (1024)

sequence *getSequences(int file, ssize_t *length, ssize_t spaces);

void readNextSeqBuffer(sequence *c, int file, ssize_t free_spaces);

#endif //BACHELORARBEIT_DATA_H
