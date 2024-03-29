#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <errno.h>
#include "string.h"
#include "data.h"

typedef struct llist {
    void *data;
    struct llist *next;
} llist_t;

typedef struct sequence {
    uint64_t start;
    uint64_t stop;
} sequence;

#define buffersize (getpagesize() * 16)

enum state {
    comment,
    data,
    unset
};

sequence *createData(int f, size_t *length) {
    uint8_t *b = malloc(buffersize);

    llist_t head = {.next=NULL};

    int seq_count = 0;
    size_t n, total = 0;
    enum state s = unset;
    do {
        n = read(f, b, buffersize);
        if (n == -1) {
            fprintf(stderr, "error reading file: %s", strerror(errno));
            break;
        }

        for (int i = 0; i < n; ++i) {
            switch (s) {
                case comment:
                    if (b[i] == '\n')
                        s = unset;
                    break;
                case data:
                    if (b[i] == '>') {
                        ((sequence *) head.next->data)->stop = total + i;
//                        printf("ending sequence at %zu\n", total+i);
                        s = comment;
                    }
                    break;
                case unset:
                    if (b[i] == '>')
                        s = comment;
                    else {
                        seq_count++;
                        llist_t *l = malloc(sizeof(llist_t));
                        sequence *se = malloc(sizeof(sequence));

                        l->data = se;
                        l->next = head.next;
                        head.next = l;

                        se->start = total + i;

//                        printf("starting sequence at %zu\n", total+i);
                        s = data;
                    }
            }
        }

        total += n;
//        printf("read %zu data, a total of %zu MiB\n", n, total >> 20);
    } while (n != 0);

    switch (s) {
        case comment:
            printf("ended with comment\n");
            break;
        case data:
            ((sequence *) head.next->data)->stop = total;
//            printf("ended with data at %zu\n", total);
            break;
        case unset:
            printf("ended perfectly\n");
            break;
    }

    llist_t *next = head.next;

    sequence *sequences = malloc(sizeof(sequence) * seq_count);
    *length = seq_count;

    while (next != NULL) {
        llist_t *l = next;

//        printf("got sequence: [%zu:%zu], length: %zu\n", ((sequence*)l->data)->start, ((sequence*)l->data)->stop, ((sequence*)l->data)->stop - ((sequence*)l->data)->start);

        sequences[--seq_count] = *(sequence *) (l->data);

        next = l->next;
        free(l->data);
        free(l);
    }

    free(b);
    return sequences;
}



#define min(a, b) (a < b ? a : b)

characters *initCharacters(int file, sequence *seq, size_t length, int free_spaces) {
    characters *c = malloc(sizeof(characters) * length);

    for (int i = 0; i < length; ++i) {
        size_t diff = seq[i].stop - seq[i].start;
        size_t a = min(diff + free_spaces, charBuffer);

        characters j = {
                .c = '$',
                .pos = i,
                .start = seq[i].start,
                .buf = calloc(a, 1),
        };

        size_t toRead = min(diff, charBuffer- free_spaces);

        lseek(file, (__off_t) (seq[i].stop - toRead), SEEK_SET);

        size_t n = read(file, j.buf, toRead);

        j.buf[n] = '$';
        j.index = (int16_t) n;
        j.stop = seq[i].stop - n;

        c[i] = j;
    }

    return c;
}


int readChar(characters *c, int file, int free_spaces) {
    size_t diff = c->stop - c->start;

    // case all read
    if (diff == 0) {
        return -1;
    }

    // copies first values to free spaces
//#pragma parallel
    for (int i = 0; i < free_spaces; ++i) {
        c->buf[buffersize - free_spaces + i] = c->buf[i];
    }

    size_t toRead = min(diff, charBuffer- free_spaces);

    lseek(file, (__off_t)(c->stop - toRead), SEEK_SET);

    size_t n = read(file, c->buf, toRead);

    c->index = (int16_t)n;
    c->stop -= n;

    return 0;
}

characters *getCharacters(int file, size_t *length, int spaces) {
    sequence *seq = createData(file, length);
    characters *c = initCharacters(file, seq, *length, spaces);

    free(seq);

    return c;
}

//#define max(a, b) (a > b ? a : b)
//
//int main() {
//    size_t l;
//
//    int f = open("GRCh38_splitlength_3.fa", O_RDONLY);
//    if (f == -1) {
//        fprintf(stderr, "error opening file: %s", strerror(errno));
//        return -1;
//    }
//
//    sequence *seq = createData(f, &l);
//
//    size_t sum = 0;
//    size_t m = 0;
//    for (int i = 0; i < l; ++i) {
////        printf("got sequence: [%zu:%zu], length: %zu\n", seq[i].start, seq[i].stop, seq[i].stop - seq[i].start);
//        sum += seq[i].stop - seq[i].start;
//
//        m = max(m, seq[i].stop - seq[i].start);
//    }
//
//    printf("got %zu chars in %zu words\n", sum, l);
//    printf("Max value: %zu\n", m);
//
//    characters *c = initCharacters(f, seq, l, 2);
//    free(seq);
//
//    printf("Created sequence");
//
//    for (int i = 0; i < l; ++i) {
//
////        printf("%hd, %zu, %c\n", c[i].index, c[i].pos, c[i].buf[c[i].index]);
//
//        free(c[i].buf);
//    }
//    free(c);
//}