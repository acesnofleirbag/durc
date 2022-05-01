#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "helpers.h"

int
main() {
    static char* exec_path = "./../build/durc";
    Buffer* buffer = (Buffer*) malloc(sizeof(Buffer));

    strcpy(buffer->input, "insert 1 bart bart@mail.com");

    // TODO: need a bidirectional pipe

    int fds[2] = { 0 }; // NOTE: 0 = read | 1 = write

    pipe(fds);

    switch(fork()) {
        case -1:
            perror("error to fork the process\n");
            exit(EXIT_FAILURE);
        case 0:
            dup2(fds[1], STDOUT_FILENO); // NOTE: closes automatically

            close(fds[1]);

            execl(exec_path, "durc", NULL);

            read(fds[0], buffer->output, sizeof(buffer->output));

            close(fds[0]);
            break;
        default:
            write(fds[1], buffer->input, sizeof(buffer->input));

            close(fds[0]);
            close(fds[1]);
    }

    printf("%s\n", buffer->output);

    return EXIT_SUCCESS;
}
