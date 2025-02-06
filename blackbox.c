#include "blackbox.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_COUNT 31
#define BUFFERSIZ 16

#define byteToAmount(x) (x >> 3)
#define byteToType(x) ((uint8_t) x & 0x7)
#define TypeAmountToByte(x, y) (x | (y << 3))

/*
    TODO:
    - Copy at the end
    - take const char * instead of file, move file completly to reader/writer
    - fix bwt.c to use blackbox correctly

    - change to multibyte RLE
 */

BlackboxWriter new_writer(const int file) {
    const BlackboxWriter writer = {(uint8_t *) malloc(BUFSIZ), file, 0, 0, 0xff};
    return writer;
}

void write_buffer(BlackboxWriter *const writer) {
    if (writer->pos == BUFSIZ) {
        ssize_t r = write(writer->file, writer->buffer, writer->pos);
        if (r != writer->pos) {
            fprintf(stderr, "Error writing to file: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        writer->pos = 0;
    }
}

void free_writer(BlackboxWriter writer) {
    if (writer.last != 0xff) {
        while (writer.last_count >= MAX_COUNT) {
            writer.buffer[writer.pos++] = TypeAmountToByte(writer.last, MAX_COUNT);
            write_buffer(&writer);
            writer.last_count -= MAX_COUNT;
        }
        writer.buffer[writer.pos++] = writer.last_count << 3 | writer.last;
        write_buffer(&writer);
    }

    if (writer.pos != 0) {
        write(writer.file, writer.buffer, writer.pos);
    }
    free(writer.buffer);
}

void write_to_writer(BlackboxWriter *const writer, const uint8_t c, const ssize_t count) {
    if (c == writer->last) {
        writer->last_count += count;
        return;
    }
    if (writer->last != 0xff) {
        while (writer->last_count >= MAX_COUNT) {
            writer->buffer[writer->pos++] = TypeAmountToByte(writer->last, MAX_COUNT);
            write_buffer(writer);
            writer->last_count -= MAX_COUNT;
        }
        writer->buffer[writer->pos++] = writer->last_count << 3 | writer->last;
        write_buffer(writer);
    }

    writer->last_count = count;
    writer->last = c;
}

BlackboxReader new_reader(const int file) {
    const BlackboxReader reader = {(uint8_t *) malloc(BUFFERSIZ), file, 0, 0};
    return reader;
}

void free_reader(const BlackboxReader reader) { free(reader.buffer); }

void load_next_reader(BlackboxReader *const reader) {
    if (reader->pos == reader->max_pos) {
        reader->max_pos = read(reader->file, reader->buffer, BUFFERSIZ);
        if (reader->max_pos == -1) {
            fprintf(stderr, "Error reading file: %s\n", strerror(errno));
            exit(1);
        }
        reader->pos = 0;
    }
}

// read_next returns the type byte and the count of the char in *count.
// count will never be greater than amount
uint8_t read_next(BlackboxReader *const reader, const ssize_t amount, ssize_t *const count) {
    load_next_reader(reader);

    uint8_t byte = reader->buffer[reader->pos++];
    ssize_t c = byteToAmount(byte);
    byte = byteToType(byte);

    if (c > amount) {
        reader->pos--;
        reader->buffer[reader->pos] = TypeAmountToByte(byte, c - amount);
        c = amount;
    } else {
        // pos may overrun, but cant update before, because may backtrack
        load_next_reader(reader);

        while (byteToType(reader->buffer[reader->pos]) == byte) {
            const ssize_t c2 = byteToAmount(reader->buffer[reader->pos]);
            if (c2 + c > amount) {
                reader->buffer[reader->pos++] = TypeAmountToByte(byte, c + c2 - amount);
                break;
            }

            c += c2;
            reader->pos++;
            load_next_reader(reader);
        }
    }

    *count = c;
    return byte;
}
