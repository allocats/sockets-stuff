#include "connection.h"

#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define CLIENT_ERROR(msg) \
    fprintf(stderr, "%s\n", msg); \
    return 1

int main(int argc, char* argv[]) {
    if (argc != 2) {
        CLIENT_ERROR("Need input");
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        CLIENT_ERROR("socket failed");
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path));

    if (connect(fd, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
        close(fd);
        CLIENT_ERROR("failed to connect");
    }

    uint32_t size = strlen(argv[1]);
    uint32_t net_size = htonl(size);
    if (write_exact(fd, &net_size, sizeof(net_size)) == -1) {
        CLIENT_ERROR("write error!");
    }

    if (write_exact(fd, argv[1], size) == -1) {
        CLIENT_ERROR("write error");
    }

    char buffer[size + 1];
    if (read_exact(fd, buffer, size) == -1) {
        CLIENT_ERROR("read error");
    }
    buffer[size] = 0;

    printf("Sent [%s] with size of %d\n", argv[1], size);
    printf("\nReceived: %s\n", buffer);
    close(fd);
}
