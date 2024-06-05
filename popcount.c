#include "popcount.h"
#include <immintrin.h>
#include <stdio.h>
#include <string.h>

//void test() {
//
//    u_int64_t d[8] = {0, 1, 2, 3, 4, 5, 6, 7};
//
//
//    const __m512i v = _mm512_loadu_si512((const __m512i *) d);
//    const __m512i r = _mm512_popcnt_epi64(v);
//
//    u_int64_t results[8] __attribute__((aligned(64)));
//    _mm512_store_epi64(results, r);
//
//    u_int64_t sum = 0;
//    for (int i = 0; i < 8; ++i) {
//        sum += results[i];
//    }
//
//    printf("%lu \n", sum);
//}

typedef struct bitVec {
    u_int64_t *data;
} bitVec;

bitVec New(u_int64_t size) {

    // (size + 63) / 64 · 8
    // + 63: aufrunden
    // / 64: anzahl an 64 bit Zahlen
    // * 8: 8 Byte pro 64 bit Zahl
    size = (size + 7) / 8;

    uint64_t *data = aligned_alloc(64, size);
    memset(data, 0, size);

    bitVec v = {
            .data = data
    };


    return v;
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
                const __m512i mVec = _mm512_loadu_si512(v.data + pos);
                const __m512i popCount = _mm512_popcnt_epi64(mVec);

                acc = _mm512_add_epi64(acc, popCount);
            }

            u_int64_t results[8] __attribute__((aligned(64)));
            _mm512_store_epi64(results, acc);

            u_int64_t sum = 0;
            for (int j = 0; j < 8; ++j) {
                sum += results[j];
            }

            count += sum;
        }

        // pop_count 64 bit
        uint64_t sum = 0;

        chunks = r % 8;
        for (int j = 0; j < chunks; ++j) {
            sum += _mm_popcnt_u64(v.data[pos + i]);
        }

        uint64_t rem = Seq[i].pos % 64;
        sum += _mm_popcnt_u64(v.data[pos + chunks] << (64 - rem));
        v.data[pos + chunks] |= 1 << rem;

        Seq[i].rank = (ssize_t) sum;
    }
}

//int main() {
//    bitVec v = New(16);
//
//    printf("%lu\n", v.data[0]);
//}