//
// Created by Cederic on 24.04.2024.
//

#ifndef BA_VALUES_H
#define BA_VALUES_H

#include "stdint.h"


struct Values;
typedef struct Values Values;

int64_t *add(Values v, int64_t *data, size_t length);
void Destroy(Values v);
Values New();



#endif //BA_VALUES_H
