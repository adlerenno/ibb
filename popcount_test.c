#include <malloc.h>
#include <stdlib.h>
#include "popcount.h"


typedef struct slice {
    uint64_t *data;
    size_t len;
    size_t cap;
} slice;

void insert(slice *s, uint64_t datum, size_t pos) {
    if (s->len == s->cap) {
        fprintf(stderr, "len %ld == cap %ld", s->len, s->cap);
        exit(-1);
    }
    if (pos > s->len + 1) {
        fprintf(stderr, "pos %ld > len %ld", pos, s->len);
        exit(-1);
    }

    s->len++;
    for (size_t i = s->len - 1; i > pos; --i) {
        s->data[i] = s->data[i - 1];
    }
    s->data[pos] = datum;

}

void shuffle(sequence *array, size_t n) {
    size_t i;
    for (i = 0; i < n - 1; i++) {
        size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
        ssize_t t = array[j].pos;
        array[j].pos = array[i].pos;
        array[i].pos = t;
    }

}

int cmp(const void *a, const void *b) {
    return (int) (((sequence *) a)->pos - ((sequence *) b)->pos);
}

void test() {
    ssize_t length = 100000;
    slice s = {
            .data = malloc(sizeof(uint64_t) * length),
            .cap = length,
            .len = 0,
    };

    sequence *data = malloc(sizeof(sequence) * length);
    for (ssize_t i = 0; i < length; ++i) {
        data[i].pos = (ssize_t) i;
    }

    shuffle(data, length);

    bitVec b = New(length);

    ssize_t inserted = 0;
    while (inserted < length) {
        ssize_t amount = min(10, (length - inserted));

        qsort(data + inserted, amount, sizeof(sequence), cmp);

        add(b, data + inserted, amount);

        printf("Inserting ");
        for (ssize_t i = 0; i < amount; ++i) {
            printf("%ld (%ld), ", (data + inserted)[i].rank, (data + inserted)[i].pos);
            insert(&s, (data + inserted)[i].pos, (data + inserted)[i].rank);
        }
        printf("\n");

        inserted += amount;
    }


    for (size_t i = 0; i < s.len; ++i) {
        if (i != s.data[i])
            printf("%ld: %ld\n", i, s.data[i]);
    }


    Destroy(b);
    free(data);
    free(s.data);
}

int main() {
    test();
    return 0;
}