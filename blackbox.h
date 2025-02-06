//
// Created by cederic on 22.01.25.
//

#ifndef BLACKBOX_H
#define BLACKBOX_H

#include <stdint.h>

typedef struct BlackboxWriter {
    uint8_t *buffer;
    const int file;
    ssize_t pos;
    ssize_t last_count;
    uint8_t last;
} BlackboxWriter;

BlackboxWriter new_writer(const int file);

void write_to_writer(BlackboxWriter *const writer, const uint8_t c, const ssize_t count);

void free_writer(const BlackboxWriter writer);

typedef struct BlackboxReader {
    uint8_t *buffer;
    const int file;
    ssize_t pos;
    ssize_t max_pos;
} BlackboxReader;

BlackboxReader new_reader(const int file);

void free_reader(const BlackboxReader reader);

uint8_t read_next(BlackboxReader *const reader, const ssize_t amount, ssize_t *const count);

#endif // BLACKBOX_H
