//
// Created by cederic on 22.01.25.
//

#ifndef BLACKBOX_H
#define BLACKBOX_H

#include <stdint.h>
#include <stdlib.h>

typedef struct BlackboxWriter {
    uint8_t *buffer;
    const int file;
    ssize_t pos;
    ssize_t last_count;
    uint8_t last;
} BlackboxWriter;

BlackboxWriter NewWriter(const int file);

void WriteToWriter(BlackboxWriter *const writer, const uint8_t c, const uint8_t count);

void FreeWriter(const BlackboxWriter writer);


typedef struct BlackboxReader {
    uint8_t *buffer;
    const int file;
    ssize_t pos;
    ssize_t last_count;
    uint8_t last;
} BlackboxReader;

#endif //BLACKBOX_H
