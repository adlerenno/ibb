//
// Created by cederic on 22.01.25.
//

#ifndef BLACKBOX_H
#define BLACKBOX_H

#include <stdint.h>
#include <stdio.h>

typedef struct BlackboxWriter {
    uint8_t *buffer;
    const int file;
    ssize_t pos;
    ssize_t last_count;
    uint8_t last;
} BlackboxWriter;

BlackboxWriter new_writer(const char *filename);

void write_to_writer(BlackboxWriter *writer, uint8_t t, ssize_t count);

typedef struct BlackboxReader {
    uint8_t *buffer;
    const int file;
    ssize_t pos;
    ssize_t max_pos;
} BlackboxReader;

BlackboxReader new_reader(const char *filename);

uint8_t read_next(BlackboxReader *reader, ssize_t amount, ssize_t *count);

void copy_close(BlackboxWriter w, BlackboxReader r);

#endif // BLACKBOX_H
