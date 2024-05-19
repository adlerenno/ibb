#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include "data.h"

#define bufferSize (1024 * 4)

typedef struct slice {
    sequence *data;
    ssize_t len;
    ssize_t cap;
} slice;

void append(slice *s, sequence datum) {
    if (s->len >= s->cap) {
        ssize_t next = 2 * s->cap;
        if (next == 0)
            next = 1;

        sequence *new = malloc(next * sizeof(sequence));
        sequence *old = s->data;
        if (old != NULL) {
            memcpy(new, s->data, sizeof(sequence) * s->len);
            free(old);
        }
        s->cap = next;
        s->data = new;
    }

    s->data[s->len] = datum;
    s->len++;
}

sequence *finalize(slice s) {
    sequence *seq = malloc(sizeof(sequence) * s.len);

    memcpy(seq, s.data, sizeof(sequence) * s.len);

    free(s.data);

    return seq;
}

enum STATE {
    UNSET,
    DATA,
    COMMENT
};

sequence *getData(int file, ssize_t *length) {
    lseek(file, 0, 0);

    uint8_t b[bufferSize];
    enum STATE state = UNSET;
    ssize_t readCount = 0;

    slice s = {NULL, 0, 0};

    while (1) {
        ssize_t n = read(file, b, bufferSize);
        if (n == -1) {
            fprintf(stderr, "Error reading file: %s\n", strerror(errno));
            break;
        }
        if (n == 0)
            break;

        for (ssize_t i = 0; i < n; ++i) {
            uint8_t v = b[i];
            switch (state) {
                case UNSET:
                    if (v == '>') {
                        state = COMMENT;
                    } else if (v == 'A' || v == 'C' || v == 'G' || v == 'T') {
                        sequence se = {
                                .range = {.start = readCount + i}
                        };
                        append(&s, se);
                        state = DATA;
                    }
                    break;
                case DATA:
                    if (!(v == 'A' || v == 'C' || v == 'G' || v == 'T')) {
                        s.data[s.len - 1].range.stop = readCount + i;
                        state = UNSET;
                    }
                    break;
                case COMMENT:
                    if (v == '\n')
                        state = UNSET;
                    break;
            }
        }
        readCount += n;
    }
    if (state == DATA) {
        s.data[s.len - 1].range.stop = readCount;
    }

    *length = s.len;
    return finalize(s);
}

void initSequences(int file, sequence *seq, ssize_t length, ssize_t free_spaces) {
    free_spaces++; // $

    for (ssize_t i = 0; i < length; ++i) {
        ssize_t d = diff(seq, i);

        if (d == 0) {
            continue;
        }

        ssize_t space = min(d + free_spaces, charBuffer);

        seq[i].c = '$';
        seq[i].pos = i;
        seq[i].buf = calloc(space, 1);

//        ssize_t toRead = min(d, charBuffer - free_spaces);
        ssize_t toRead = space - free_spaces;
        ssize_t r = lseek(file, (long) seq[i].range.stop - toRead, SEEK_SET);
        if (r != seq[i].range.stop - toRead) {
            fprintf(stderr, "error seeking init seq: %zd %s\n", r, strerror(errno));
        }

        ssize_t n = read(file, seq[i].buf, toRead);
        if (n != toRead) {
            fprintf(stderr, "short read init seq: %s\n", strerror(errno));
        }
        seq[i].buf[n] = '$';
        seq[i].index = n;
        seq[i].range.stop -= n;

        if (diff(seq, i) < 0) {
            fprintf(stderr, "stop before start\n");
        }
    }
}

static int cmp(const void *a, const void *b) {
    int x = diff((sequence *) a, 0) - diff((sequence *) b, 0) + ((sequence *) a)->index - ((sequence *) b)->index;
    if (!x)
        return (int)(((sequence *)a)->pos - ((sequence*)b)->pos);
    return x;
}

sequence *getSequences(int file, ssize_t *length, ssize_t spaces) {
    sequence *seq = getData(file, length);

    initSequences(file, seq, *length, spaces);

    qsort(seq, *length, sizeof(sequence), cmp);
    return seq;
}

void readNextSeqBuffer(sequence *c, int file, ssize_t free_spaces) {
    ssize_t d = diff(c, 0);

    if (d <= 0) {
        return;
    }

    ssize_t toRead = min(d, charBuffer - free_spaces);

    for (ssize_t i = free_spaces; i >= 0; --i) {
        c->buf[toRead + i] = c->buf[i];
    }

    long r = lseek(file, (long) c->range.stop - toRead, SEEK_SET);
    if (r != c->range.stop - toRead) {
        fprintf(stderr, "Invalid seek: %s\n", strerror(errno));
    }

    ssize_t b = read(file, c->buf, toRead);
    if (b != toRead) {
        fprintf(stderr, "short read: %s\n", strerror(errno));
    }

    c->index = b;
    c->range.stop -= b;
}