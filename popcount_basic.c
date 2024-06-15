
#include "popcount.h"
#include <immintrin.h>
#include <string.h>

bitVec New(uint64_t size) {

    // (size + 63) / 64
    // + 63: aufrunden
    // / 64: anzahl an 64 bit Zahlen
    size = (size + 63) / 64;

    uint64_t *data = malloc(size * sizeof(uint64_t));
    memset(data, 0, size * sizeof(uint64_t));

    return data;
}

void add(bitVec v, sequence *Seq, size_t length) {

    uint64_t count = 0;
    uint64_t pos = 0;

    for (size_t i = 0; i < length; ++i) {
        uint64_t quo = (uint64_t) Seq[i].pos / 64;
        uint64_t rem = Seq[i].pos % 64;

        quo -= pos;

        // pop_count 64 bit
        for (uint64_t j = 0; j < quo; ++j) {
            count += _mm_popcnt_u64(v[j]);
        }

        pos += quo;
        uint64_t mask = 1;
        mask <<= rem;
        mask -= 1;
        uint64_t x = v[pos] & mask;
        int64_t d = _mm_popcnt_u64(x);

        mask = 1;
        mask <<= rem;
        v[pos] |= mask;

        Seq[i].rank = (ssize_t) d + (ssize_t) count;
    }
}

void Destroy(bitVec b) {
    free(b);
}
