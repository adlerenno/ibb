#include <stdio.h>
#include <malloc.h>

#define left(i) (i == 0 ? 1 : i << 1)
#define right(i) ((i << 1) + 1)
#define k 10000000

void heapify(int *arr, size_t length);

void heapifyDown(int *arr, size_t length, size_t i);

void print(int *a, size_t length) {
    for (int i = 0; i < length; ++i) {
        printf("%d ", a[i]);
    }
    printf("\n");
}


void heapifyDown(int *arr, size_t length, size_t i) {
    while (left(i) <= length) {
        size_t m;
        if (right(i) > length) {
            m = left(i);
        } else {
            if (arr[left(i) - 1] < arr[right(i) - 1]) {
                m = left(i);
            } else {
                m = right(i);
            }
        }

        if (arr[i - 1] < arr[m - 1]) {
            return;
        }
        int t = arr[i - 1];
        arr[i - 1] = arr[m - 1];
        arr[m - 1] = t;
        i = m;
    }
}

void heapify(int *arr, size_t length) {
    for (size_t i = length / 2; i >= 1; --i) {
        heapifyDown(arr, length, i);

    }
}

void deleteMin(int *arr, size_t length) {
    int e = arr[0];
    arr[0] = arr[length - 1];
    arr[length - 1] = e;
    heapifyDown(arr, length - 1, 1);
}


//int main() {
//    int *a = malloc(sizeof(int) * k);
//
//    for (int i = k; i > 0; --i) {
//        a[k - i] = i;
//    }
//
//    heapify(a, k);
//
//    size_t l = k;
//    for (int i = 0; i < k; ++i) {
//        deleteMin(a, l);
////        print(a, l);
//        l--;
//        if (a[l] != i+1)
//            printf("%d\n", a[l]);
//    }
//
//
//    free(a);
//}