
#include "popcount.h"
#include <immintrin.h>
#include <string.h>

bitVec New(uint64_t size) {

    // (size + 63) / 64
    // + 63: aufrunden
    // / 64: anzahl an 64 bit Zahlen
    size = (size + 63) / 64;

    uint64_t *data = calloc(size, sizeof(uint64_t));

    return data;
}

void add(bitVec v, sequence *Seq, size_t length) {

    uint64_t count = 0;
    uint64_t pos = 0;

    for (size_t i = 0; i < length; ++i) {
        uint64_t r = (Seq[i].pos / 64) - pos;

        // pop_count 64 bit
        uint64_t sum = count;

        for (size_t j = 0; j < r; ++j) {
            sum += _mm_popcnt_u64(v[pos + j]);
        }


        uint64_t rem = Seq[i].pos % 64;
        uint64_t mask = 1;
        mask <<= rem;
        mask--;
        sum += _mm_popcnt_u64(v[pos + r] & mask);
        mask++;
        v[pos + r] |= mask;

        Seq[i].rank = (ssize_t) sum;
    }
}

void Destroy(bitVec b) {
    free(b);
}
