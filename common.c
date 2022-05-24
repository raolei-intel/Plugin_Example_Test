#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <fcntl.h>

bool send_all(int fd, char *buffer, int size)
{
    while (size > 0) {
        int send_size = send(fd, buffer, size, 0);
        if (send_size == -1) {
            if (errno == EINTR) {
                continue;
            } else if (errno == EAGAIN) {
                usleep(100);
                continue;
            } else {
                return false;
            }
        }
        size = size - send_size;
        buffer += send_size;

    }
    return true;
}

bool receive_all(int fd, char *buffer, int size)
{
    while (size > 0) {
        int recv_size = recv(fd, buffer, size, 0);
        if (recv_size == -1) {
            if (errno == EINTR) {
                continue;
            } else if (errno == EAGAIN) {
                usleep(100);
                continue;
            } else {
                return false;
            }
        }                                                                                                                                                                                                     size = size - recv_size;
        buffer += recv_size;
    }

    return true;
}
