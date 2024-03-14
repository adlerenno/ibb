//
// Created by Cederic on 14.03.2024.
//
#include <stdint.h>

#ifndef BA_FILE_H
#define BA_FILE_H

typedef struct data {
    char insert;
    size_t pos;
} data;

void writeData(int f1, int f2, data *data1, size_t length);

#endif //BA_FILE_H
