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

#define bufferSize (512)

enum state {
    comment,
    data,
    unset
};

sequence *createData(int f, size_t *length) {
    uint8_t *b = malloc(bufferSize);

    llist_t head = {.next=NULL};

    size_t seq_count = 0;
    int n;
    size_t total = 0;
    enum state state = unset;
    do {
        n = read(f, b, bufferSize);
        if (n == -1) {
            fprintf(stderr, "error reading file: %s\n", strerror(errno));
            break;
        }

        for (int i = 0; i < n; ++i) {
            switch (state) {
                case comment:
                    if (b[i] == '\n' || b[i] == '\r')
                        state = unset;
                    break;
                case data:
                    if (b[i] == '>' || b[i] == '\n' || b[i] == '\r') {
                        ((Range *) head.next->data)->stop = total + i;
//                        printf("ending sequence at %zu\n", total+i);
                        state = unset;
                    }
                    break;
                case unset:
                    if (b[i] == '>')
                        state = comment;
                    else if (b[i] == 'A' || b[i] == 'C' || b[i] == 'G' || b[i] == 'T') {
                        seq_count++;
                        llist_t *l = malloc(sizeof(llist_t));
                        Range *range = malloc(sizeof(Range));

                        l->data = range;
                        l->next = head.next;
                        head.next = l;

                        range->start = total + i;

//                        printf("starting sequence at %zu\n", total+i);
                        state = data;
                    }
                    break;
            }
        }

        total += n;
//        printf("read %zu data, a total of %zu MiB\n", n, total >> 20);
    } while (n != 0);

    switch (state) {
        case comment:
            printf("ended with comment\n");
            break;
        case data:
            ((Range *) head.next->data)->stop = total;
//            printf("ended with data at %zu\n", total);
            break;
        case unset:
            printf("ended perfectly\n");
            break;
    }

    llist_t *next = head.next;

    sequence *chars = malloc(sizeof(sequence) * seq_count);
    *length = seq_count;

    while (next != NULL) {
        llist_t *l = next;

//        printf("got sequence: [%zu:%zu], length: %zu\n", ((sequence*)l->data)->start, ((sequence*)l->data)->stop, ((sequence*)l->data)->stop - ((sequence*)l->data)->start);

        chars[--seq_count].range = *(Range *) (l->data);

        next = l->next;
        free(l->data);
        free(l);
    }

    free(b);
    return chars;
}


#define min(a, b) (a < b ? a : b)

void initCharacters(int file, sequence *c, size_t length, size_t free_spaces) {
    free_spaces++; // $

    for (size_t i = 0; i < length; ++i) {
        size_t diff = c[i].range.stop - c[i].range.start;
        size_t a = min(diff + free_spaces, charBuffer);

        c[i].c = '$';
        c[i].intVal = 0;
        c[i].pos = i;
        c[i].buf = calloc(a, 1);


        size_t toRead = min(diff, charBuffer - free_spaces);

        lseek(file, (long) (c[i].range.stop - toRead), SEEK_SET);

        size_t n = read(file, c[i].buf, toRead);

        c[i].buf[n] = '$';
        c[i].index = (int16_t) n;
        c[i].range.stop -= n;

    }
}


void readNextSeqBuffer(sequence *c, int file, size_t free_spaces) {
    size_t diff = c->range.stop - c->range.start;

    // case all read
    if (diff <= 0) {
        return;
    }

    size_t toRead = min(diff, charBuffer - free_spaces);


    // copies first values to free spaces
    for (size_t i = 0; i < free_spaces; ++i) {
        c->buf[toRead + i] = c->buf[i];
    }


    lseek(file, (long) (c->range.stop - toRead), SEEK_SET);

    size_t n = read(file, c->buf, toRead);

    c->index = (ssize_t) n;
    c->range.stop -= n;
}

static int cmp(const void *a, const void *b) {
    const sequence *ac = a, *bc = b;
    return (int) (ac->range.stop - ac->range.start + ac->index - bc->range.stop + bc->range.start - bc->index);
}

sequence *getSequences(int file, size_t *length, size_t spaces) {
    sequence *c = createData(file, length);
    initCharacters(file, c, *length, spaces);

    qsort(c, *length, sizeof(sequence), cmp);

    return c;
}
