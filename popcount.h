#ifndef BACHELOR_POPCOUNT_H
#define BACHELOR_POPCOUNT_H

#include <sys/types.h>
#include "data.h"


typedef uint64_t *bitVec;

bitVec New(uint64_t length);

void add(bitVec v, sequence *Seq, size_t length);

void Destroy(bitVec b);

#endif //BACHELOR_POPCOUNT_H
