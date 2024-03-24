//
// Created by ceddy on 3/24/24.
//

#ifndef BACHELORARBEIT_DATA_H
#define BACHELORARBEIT_DATA_H

#include <stddef.h>
#include <stdint.h>

typedef struct characters {
    uint8_t *buf;
    uint8_t c;
    int16_t index;
    size_t pos;
    size_t rank;
    size_t start;
    size_t stop;
} characters;

#define charBuffer getpagesize()

characters *getCharacters(int file, size_t *length, int spaces);
int readChar(characters *c, int file, int free_spaces);

#endif //BACHELORARBEIT_DATA_H
