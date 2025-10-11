#define _GNU_SOURCE
#include "connection.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SERVER_ERROR(msg) \
    fprintf(stderr, "%s\n", msg); \
    return 1

enum conn_state {
    STATE_READ_LENGTH,
    STATE_READ_PAYLOAD,
    STATE_WRITE_REPONSE
};

typedef struct {
    int fd;
    enum conn_state state;
    uint32_t net_len;
    uint32_t payload_size;
    char* buffer;
} connection;

static int handle_read(connection* conn) {
    if (conn -> state == STATE_READ_LENGTH) {
        if (read_exact(conn -> fd, &conn -> net_len, sizeof(conn -> net_len)) == -1) {
            fprintf(stderr, "Failed to read payload header for fd: %d\n", conn -> fd);
            return -1;
        }

        conn -> payload_size = ntohl(conn -> net_len);
        conn -> buffer = calloc(1, conn -> payload_size + 1);
        conn -> state = STATE_READ_PAYLOAD;
    }

    if (conn -> state == STATE_READ_PAYLOAD) {
        if (read_exact(conn -> fd, conn -> buffer, conn -> payload_size) == -1) {
            fprintf(stderr, "Failed to read payload for fd: %d\n", conn -> fd);
            return -1;
        }

        conn -> state = STATE_WRITE_REPONSE;
    }

    return 0;
}

static int handle_write(connection* conn) {
    if (conn -> state == STATE_WRITE_REPONSE) {
        if (write_exact(conn -> fd, conn -> buffer, conn -> payload_size) == -1) {
            fprintf(stderr, "Failed to send payload for fd: %d\n", conn -> fd);
            return -1;
        }

        free(conn -> buffer);
        conn -> buffer = NULL;
        conn -> state = STATE_READ_LENGTH;
        conn -> payload_size = 0;
        conn -> net_len = 0;
    }

    return 0;
}

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
        close(server_fd);
        SERVER_ERROR("bind failed");
    }

    if (set_non_blocking(server_fd) == -1) {
        close(server_fd);
        SERVER_ERROR("failed to set nonblocking server");
    }

    if (listen(server_fd, 16) == -1) {
        close(server_fd);
        SERVER_ERROR("listen failed");
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        close(server_fd);
        SERVER_ERROR("epoll_create1 failed");
    }

    if (epoll_add(epoll_fd, server_fd, EPOLLIN) == -1) {
        close(server_fd);
        SERVER_ERROR("epoll_add failed");
    }

    struct epoll_event events[MAX_EVENTS] = {0};
    connection conns[MAX_EVENTS] = {0};
    int active_connections = 0;
    int fds = 0;

    while (1) {
        fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (fds == -1) {
            if (errno == EINTR) continue;
            close(server_fd);
            SERVER_ERROR("epoll_wait failed");
        }

        for (int i = 0; i < fds; i++) {
            int fd = events[i].data.fd;

            if (fd == server_fd) {
                int client_fd = accept4(server_fd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
                if (client_fd == -1) {
                    fprintf(stderr, "Failed to accept client");
                    continue;
                }

                conns[client_fd].fd = client_fd;
                conns[client_fd].state = STATE_READ_LENGTH;

                if (epoll_add(epoll_fd, client_fd, EPOLLIN | EPOLLOUT | EPOLLRDHUP) == -1) {
                    fprintf(stderr, "failed to add client to epoll");
                    close(client_fd);
                    continue;
                }
            } else if (events[i].events & EPOLLIN) {
                if (handle_read(&conns[fd]) == -1) {
                    free(conns[fd].buffer);
                    close(conns[fd].fd);
                    memset(&conns[fd], 0, sizeof(conns[fd]));
                    continue;
                }
            } 

            if (events[i].events & EPOLLOUT) {
                if (handle_write(&conns[fd]) == -1) {
                    free(conns[fd].buffer);
                    close(conns[fd].fd);
                    memset(&conns[fd], 0, sizeof(conns[fd]));
                    continue;
                }
            }

            if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                free(conns[fd].buffer);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
                memset(&conns[fd], 0, sizeof(conns[fd]));
            }
        }
    }

    close(server_fd);
    return 0;
}
