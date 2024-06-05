#ifndef BACHELOR_POPCOUNT_H
#define BACHELOR_POPCOUNT_H

#include <sys/types.h>
#include "data.h"

typedef struct bitVec bitVec;

bitVec New(u_int64_t length);

void add(bitVec b, sequence *Seq, size_t length);

#endif //BACHELOR_POPCOUNT_H
