#ifndef BA_VALUES_H
#define BA_VALUES_H

#include "stdint.h"
#include "data.h"

typedef struct value value;

typedef value *Values;

Values New(ssize_t length);
void add(Values v, sequence *data, size_t length);
void Destroy(Values v);




#endif //BA_VALUES_H
