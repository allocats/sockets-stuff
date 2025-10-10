#ifndef CONNECTION_H
#define CONNECTION_H

#include <errno.h>
#include <stddef.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/plink.socket"
#define BUFFER_SIZE 1024

static size_t read_exact(int fd, void* buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = read(fd, (char*) buf + total, len - total);

        if (n < 0) {
            if (errno == EINTR) continue;
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
            return -1;
        }

        if (n == 0) return total;

        total += n;
    }

    return total;
}

#endif // !CONNECTION_H
