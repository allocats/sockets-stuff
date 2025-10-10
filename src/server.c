#include "connection.h"

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SERVER_ERROR(msg) \
    fprintf(stderr, "%s\n", msg); \
    return 1

int main(int argc, char* argv[]) {
    unlink(SOCKET_PATH);

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        SERVER_ERROR("socket failed");
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path));

    if (bind(server_fd, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
        SERVER_ERROR("bind failed");
    }

    if (listen(server_fd, 16) == -1) {
        SERVER_ERROR("listen failed");
    }

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) {
            fprintf(stderr, "Failed to accept client");
            continue;
        }

        uint32_t net_size = 0;
        if (read_exact(client_fd, &net_size, sizeof(net_size)) == -1) {
            perror("Failed to read payload size");
            close(client_fd);
            continue;
        }
        uint32_t payload_size = ntohl(net_size);

        printf("Server: Payload size = %d\n", payload_size);
        char buffer[payload_size + 1];

        if (read_exact(client_fd, buffer, payload_size) == -1) {
            perror("Failed to read payload");
            close(client_fd);
            continue;
        }

        buffer[payload_size] = 0;

        if (write_exact(client_fd, buffer, payload_size) == -1) {
            perror("Failed to write");
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}
