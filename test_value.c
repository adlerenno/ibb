#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include "values.h"
#include "malloc.h"

void shuffle(sequence *array, size_t n) {
    if (n > 1) {
        size_t i;
        for (i = 0; i < n - 1; i++) {
            size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
            sequence t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

void copy(int *a, int *b, int size) {
    for (int i = size; i >= 0; --i) {
        a[i] = b[i];
    }
}

void insert(int *data, int size, sequence *s, int count) {
//    int pos = size + count;
//    int dataPos = size;

    for (int i = 0; i < count; ++i) {
        copy(data + s[i].rank + 1, data + s[i].rank, size - (int) s[i].rank);
        data[s[i].rank] = s[i].pos;
        size++;
    }

}

int main() {
    int size = 50000;
    sequence *s = malloc(size * sizeof(sequence));
    int *data = malloc(size * sizeof(data));

    for (int i = 0; i < size; ++i) {
        s[i].pos = i;
        s[i].rank = i;
    }

    srand(clock());
    shuffle(s, size);

//    for (int i = 0; i < size; ++i) {
//        printf("i: %d, pos: %zu\n", i, s[i].pos);
//    }

    int dataSize = 0;
    Values v = New();

    int stepsize = 5;

    for (int i = 0; i < size / stepsize; ++i) {
        add(v, s + dataSize, stepsize);
        insert(data, dataSize, s + dataSize, stepsize);
        dataSize += stepsize;
    }

    for (int i = 0; i < size; ++i) {
        if (data[i] != i) {
            printf("Unexpected Value: %d: %d\n", i, data[i]);
        }
    }

    free(data);
    free(s);
    return 0;
}