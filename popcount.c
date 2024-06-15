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
        uint64_t chunks = r / 8;

        // pop_count 512 bit
        if (chunks > 0) {
            __m512i acc = _mm512_setzero_si512();
            for (size_t j = 0; j < chunks; ++j, pos += 8) {
                const __m512i mVec = _mm512_loadu_si512(v + pos);
                const __m512i popCount = _mm512_popcnt_epi64(mVec);

                acc = _mm512_add_epi64(acc, popCount);
            }

            uint64_t results[8] __attribute__((aligned(64)));
            _mm512_store_epi64(results, acc);

            uint64_t sum = 0;
            for (int j = 0; j < 8; ++j) {
                sum += results[j];
            }

            count += sum;
        }

        // pop_count 64 bit
        uint64_t sum = count;

        chunks = r % 8;
        for (size_t j = 0; j < chunks; ++j) {
            sum += _mm_popcnt_u64(v[pos + j]);
        }


        uint64_t rem = Seq[i].pos % 64;
        uint64_t mask = 1;
        mask <<= rem;
        mask--;
        sum += _mm_popcnt_u64(v[pos + chunks] & mask);
        mask++;
        v[pos + chunks] |= mask;

        Seq[i].rank = (ssize_t) sum;
    }
}

void Destroy(bitVec b) {
    free(b);
}
