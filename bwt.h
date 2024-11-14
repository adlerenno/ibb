//
// Created by Cederic on 05.05.2024.
//

#ifndef BA_BWT_H
#define BA_BWT_H

#include "data.h"

void construct(int file, const char *temp_dir, ssize_t layers, int processors, sequence *sequences, ssize_t length, const char *output_filename);

#endif //BA_BWT_H
