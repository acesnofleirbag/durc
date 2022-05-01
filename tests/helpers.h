#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    char input[28];
    char output[14];
} Buffer;

void write_cmd(FILE* process, Buffer* buffer) {
    if (fwrite(&buffer->input, sizeof(char), sizeof(buffer->input), process) != sizeof(buffer->input)) {
        perror("error to write on process");
        exit(EXIT_FAILURE);
    }

    pclose(process);
}

void read_cmd(FILE* process, Buffer* buffer) {
    while (!feof(process)) {
        // NOTE: 'executed\ndb: ' === 14 bits
        if (fread(&buffer->output, sizeof(char), sizeof(buffer->output), process) != 0) {
            perror("error to read on process");
        }

        pclose(process);
    }
}

#endif
