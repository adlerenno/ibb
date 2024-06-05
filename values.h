#ifndef BA_VALUES_H
#define BA_VALUES_H

#include "stdint.h"
#include "data.h"

struct value;
typedef struct value value;

typedef struct Values {
    value *data;
} Values;

void add(Values v, sequence *data, size_t length);
void Destroy(Values v);

Values New(ssize_t length);



#endif //BA_VALUES_H
