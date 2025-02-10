#include "blackbox.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_COUNT 31
#define BUFFERSIZ 4096

#define byteToAmount(x) (x >> 3)
#define byteToType(x) ((uint8_t) x & 0x7)
#define TypeAmountToByte(x, y) (x | ((uint8_t) y << 3))

/*
    TODO:
    - fix bwt.c to use blackbox correctly

    - change to multibyte RLE
 */

BlackboxWriter new_writer(const char *filename) {
    const int file = creat(filename, 0644);
    if (file == -1) {
        fprintf(stderr, "Error opening file '%s': %s\n", filename, strerror(errno));
        exit(1);
    }

    const BlackboxWriter writer = {(uint8_t *) malloc(BUFSIZ), file, 0, 0, 0};
    return writer;
}

static void write_buffer(BlackboxWriter *const writer) {
    if (writer->pos == BUFFERSIZ) {
        const ssize_t r = write(writer->file, writer->buffer, writer->pos);
        if (r != writer->pos) {
            fprintf(stderr, "Error writing to file: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        writer->pos = 0;
    }
}


void write_to_writer(BlackboxWriter *const writer, const uint8_t t, const ssize_t count) {
    if (t == writer->last) {
        writer->last_count += count;
        return;
    }
    while (writer->last_count > 0) {
        const uint8_t c = writer->last_count < MAX_COUNT ? writer->last_count : MAX_COUNT;
        writer->buffer[writer->pos++] = TypeAmountToByte(writer->last, c);
        write_buffer(writer);
        writer->last_count -= c;
    }

    writer->last_count = count;
    writer->last = t;
}

BlackboxReader new_reader(const char *filename) {
    const int file = open(filename, O_RDONLY);
    if (file == -1) {
        fprintf(stderr, "Error opening file '%s': %s\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }

    const BlackboxReader reader = {(uint8_t *) malloc(BUFFERSIZ), file, 0, 0};
    return reader;
}

void free_reader(const BlackboxReader reader) { free(reader.buffer); }

static uint8_t load_next_reader(BlackboxReader *const reader) {
    if (reader->pos == reader->max_pos) {
        reader->pos = 0;
        reader->max_pos = read(reader->file, reader->buffer, BUFFERSIZ);
        if (reader->max_pos == -1) {
            fprintf(stderr, "Error reading file: %s\n", strerror(errno));
            exit(1);
        }
        if (reader->max_pos == 0) {
            return 1;
        }
    }
    return 0;
}

// read_next returns the type byte and the count of the char in *count.
// count will never be greater than amount
uint8_t read_next(BlackboxReader *const reader, const ssize_t amount, ssize_t *const count) {
    if (load_next_reader(reader) == 1) {
        // fprintf(stderr, "Reading reader without data\n");
        // exit(1);
        *count = 0;
        return 0;
    }

    uint8_t byte = reader->buffer[reader->pos++];
    ssize_t c = byteToAmount(byte);
    byte = byteToType(byte);

    if (c > amount) {
        reader->pos--;
        reader->buffer[reader->pos] = TypeAmountToByte(byte, (c - amount));
        c = amount;
    } else {
        // pos may overrun, but cant update before, because may backtrack
        if (load_next_reader(reader) == 1) {
            goto ret;
        }
        while (byteToType(reader->buffer[reader->pos]) == byte) {

            const ssize_t c2 = byteToAmount(reader->buffer[reader->pos]);
            if (c2 + c > amount) {
                reader->buffer[reader->pos] = TypeAmountToByte(byte, (c + c2 - amount));
                c = amount;
                break;
            }

            c += c2;
            reader->pos++;
            if (load_next_reader(reader) == 1) {
                break;
            }
        }
    }

ret:
    *count = c;
    return byte;
}

void copy_close(BlackboxWriter w, BlackboxReader r) {

    if (w.last == byteToType(r.buffer[r.pos])) {
        ssize_t c = 0;
        const uint8_t type = read_next(&r, INT64_MAX, &c);
        if (c != 0)
            write_to_writer(&w, type, c);
    }
    while (w.last_count > 0) {
        const uint8_t c = w.last_count < MAX_COUNT ? w.last_count : MAX_COUNT;
        w.buffer[w.pos++] = TypeAmountToByte(w.last, c);
        write_buffer(&w);
        w.last_count -= c;
    }

    if (w.pos + (r.max_pos - r.pos) < BUFFERSIZ) {
        memcpy(w.buffer + w.pos, r.buffer + r.pos, r.max_pos - r.pos);
        write(w.file, w.buffer, w.pos + r.max_pos - r.pos);
    } else {
        write(w.file, w.buffer, w.pos);
        write(w.file, r.buffer + r.pos, r.max_pos - r.pos);
    }

    ssize_t n = read(r.file, w.buffer, BUFFERSIZ);
    while (n > 0) {
        write(w.file, w.buffer, n);
        n = read(r.file, w.buffer, BUFFERSIZ);
    }

    if (n == -1) {
        fprintf(stderr, "Error reading file: %s\n", strerror(errno));
    }

    free_reader(r);
    free(w.buffer);

    close(r.file);
    close(w.file);
}
