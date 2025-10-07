#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "../includes/connection.h"

int main(int argc, char* argv[]) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);

    int retval = bind(fd, (struct sockaddr*) &addr, sizeof(addr));
    if (retval == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    retval = listen(fd, 16);
    if (retval == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int down_flag = 0;
    char buffer[BUFFER_SIZE];

    while (1) {
        int client_fd = accept(fd, NULL, NULL);
        if (client_fd == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        int result = 0;
        while (1) {
            ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer));
            if (bytes_read == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }

            buffer[sizeof(buffer) - 1] = 0;

            if (!strncmp(buffer, "DOWN", sizeof(buffer))) {
                down_flag = 1;
                continue;
            }

            if (!strncmp(buffer, "END", sizeof(buffer))) {
                break;
            }

            if (down_flag) {
                continue;
            }

            result += atoi(buffer);
        }

        sprintf(buffer, "%d", result);

        ssize_t bytes_written = write(client_fd, buffer, sizeof(buffer));
        if (bytes_written == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        close(client_fd);

        if (down_flag) {
            break;
        }
    }

    close(fd);
    unlink(SOCKET_NAME);
    exit(EXIT_SUCCESS);
}
