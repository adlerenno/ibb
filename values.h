//
// Created by Cederic on 24.04.2024.
//

#ifndef BA_VALUES_H
#define BA_VALUES_H

#include "stdint.h"
#include "data.h"

struct value;
typedef struct value value;

typedef struct Values {
    value *data;
} Values;

void add(Values v, characters *data, size_t length);
void Destroy(Values v);
Values New();



#endif //BA_VALUES_H
