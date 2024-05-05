#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include "string.h"
#include "data.h"
#include "errno.h"

typedef struct llist {
    void *data;
    struct llist *next;
} llist_t;

//typedef struct sequence {
//    size_t start;
//    size_t stop;
//} sequence;

#define buffersize (1024 * 16)

enum state {
    comment,
    data,
    unset
};

characters *createData(int f, size_t *length) {
    uint8_t *b = malloc(buffersize);

    llist_t head = {.next=NULL};

    size_t seq_count = 0;
    size_t n, total = 0;
    enum state s = unset;
    do {
        n = read(f, b, buffersize);
        if (n == -1) {
            fprintf(stderr, "error reading file: %s\n", strerror(errno));
            break;
        }

        for (size_t i = 0; i < n; ++i) {
            switch (s) {
                case comment:
                    if (b[i] == '\n')
                        s = unset;
                    break;
                case data:
                    if (b[i] == '>' || b[i] == '\n') {
                        ((sequence *) head.next->data)->stop = total + i;
//                        printf("ending sequence at %zu\n", total+i);
                        s = unset;
                    }
                    break;
                case unset:
                    if (b[i] == '>')
                        s = comment;
                    if (b[i] == 'A' || b[i] == 'C' || b[i] == 'G' || b[i] == 'T') {
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

    characters *chars = malloc(sizeof(characters) * seq_count);
    *length = seq_count;

    while (next != NULL) {
        llist_t *l = next;

//        printf("got sequence: [%zu:%zu], length: %zu\n", ((sequence*)l->data)->start, ((sequence*)l->data)->stop, ((sequence*)l->data)->stop - ((sequence*)l->data)->start);

        chars[--seq_count].sequence = *(sequence *) (l->data);

        next = l->next;
        free(l->data);
        free(l);
    }

    free(b);
    return chars;
}


#define min(a, b) (a < b ? a : b)

void initCharacters(int file, characters *c, size_t length, int free_spaces) {
//    characters *c = malloc(sizeof(characters) * length);


//#pragma omp parallel for
    for (size_t i = 0; i < length; ++i) {
        size_t diff = c[i].sequence.stop - c[i].sequence.start;
        size_t a = min(diff + free_spaces, charBuffer);

//        characters j = {
//                .c = '$',
//                .pos = i,
//                .buf = calloc(a, 1),
//        };
        c[i].c = '$';
        c[i].pos = i;
        c[i].buf = calloc(a, 1);


        size_t toRead = min(diff, charBuffer - free_spaces);

        lseek(file, (long) (c[i].sequence.stop - toRead), SEEK_SET);

        size_t n = read(file, c[i].buf, toRead);

        c[i].buf[n] = '$';
        c[i].index = (int16_t) n;
        c[i].sequence.stop -= n;

    }
}


int readChar(characters *c, int file, int free_spaces) {
    size_t diff = c->sequence.stop - c->sequence.start;

    // case all read
    if (diff == 0) {
        return -1;
    }

    // copies first values to free spaces
//#pragma parallel
    for (int i = 0; i < free_spaces; ++i) {
        c->buf[buffersize - free_spaces + i] = c->buf[i];
    }

    size_t toRead = min(diff, charBuffer - free_spaces);

    lseek(file, (long) (c->sequence.stop - toRead), SEEK_SET);

    size_t n = read(file, c->buf, toRead);

    c->index = (int16_t) n;
    c->sequence.stop -= n;

    return 0;
}

int cmp_chars(const void *a, const void *b) {
    const characters *ac = a, *bc = b;
    return (int) (ac->sequence.stop - ac->sequence.start - bc->sequence.stop + bc->sequence.start);
}

characters *getCharacters(int file, size_t *length, int spaces) {
    characters *c = createData(file, length);
    initCharacters(file, c, *length, spaces);

    qsort(c, *length, sizeof(characters), cmp_chars);

    return c;
}
