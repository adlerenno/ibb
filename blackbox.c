//
// Created by cederic on 22.01.25.
//

#include "blackbox.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


#define MAX_COUNT 32
#define BUFSIZ 1024



BlackboxWriter NewWriter(const int file) {
    BlackboxWriter writer = {(uint8_t *) malloc(BUFSIZ), file, 0, 0, 0};
    return writer;
}

void FreeWriter(const BlackboxWriter writer) {
    if (writer.pos != 0) {
        write(writer.file, writer.buffer,  writer.pos);
    }
    free(writer.buffer);
}

void WriteToWriter(BlackboxWriter *const writer, const uint8_t c, const uint8_t count) {
    if (c == writer->last) {
        writer->last_count += count;
        if (writer->last_count >= MAX_COUNT) {
            writer->buffer[writer->pos++] = MAX_COUNT << 3 | c;
            if (writer->pos == BUFSIZ) {
                write(writer->file, writer->buffer, writer->pos);
                writer->pos = 0;
            }
        }
        return;
    }
    writer->buffer[writer->pos++] = writer->last_count << 3 | writer->last;
    if (writer->pos == BUFSIZ) {
        write(writer->file, writer->buffer, writer->pos);
        writer->pos = 0;
    }

    writer->last_count = count;
    writer->last = c;
}

BlackboxReader NewReader(const int file) {
  BlackboxReader reader = {(uint8_t *) malloc(BUFSIZ), file, 0, 0, 0};
  return reader;
}

void FreeReader(const BlackboxReader reader) {
  free(reader.buffer);
}