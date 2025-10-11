#ifndef CONNECTION_H
#define CONNECTION_H

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/plink.socket"
#define MAX_EVENTS 1024

static int epoll_add(int epoll_fd, int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        return -1;
    }

    return 0;
}

static int set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return -1;
    }

    return 0;
} 

static size_t read_exact(int fd, void* buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = read(fd, (char*) buf + total, len - total);

        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN) return total;
            return -1;
        }

        if (n == 0) return total;

        total += n;
    }

    return total;
}

static size_t write_exact(int fd, void* buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = write(fd, (char*) buf + total, len - total);

        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN) return total;
            return -1;
        }

        if (n == 0) return total;

        total += n;
    }

    return total;
}

#endif // !CONNECTION_H
