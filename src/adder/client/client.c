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

    int retval = connect(fd, (struct sockaddr*) &addr, sizeof(addr)); 
    if (retval == -1) {
        perror("Server is down");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < argc; i++) {
        ssize_t bytes_written = write(fd, argv[i], strlen(argv[i]) + 1);
        if (bytes_written == -1) {
            perror("write");
            break;
        }
    }

    char buffer[BUFFER_SIZE];
    strcpy(buffer, "END");
    ssize_t bytes_written = write(fd, buffer, strlen(buffer) + 1);
    if (bytes_written == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read = read(fd, buffer, sizeof(buffer));
    if (bytes_read == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    buffer[sizeof(buffer) - 1] = 0;
    printf("Result = %s\n", buffer);
    close(fd);
    exit(EXIT_SUCCESS);
}
