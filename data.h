#ifndef BACHELORARBEIT_DATA_H
#define BACHELORARBEIT_DATA_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct Range {
    ssize_t start;
    ssize_t stop;
} Range;

typedef struct sequence {
    uint8_t *buf;
//    uint8_t c;
    uint8_t intVal;
    ssize_t index;
    size_t pos;
    size_t rank;
    Range range;
} sequence;

#define charBuffer (128)

sequence *getSequences(int file, size_t *length, size_t spaces);

void readNextSeqBuffer(sequence *c, int file, size_t free_spaces);

#endif //BACHELORARBEIT_DATA_H
