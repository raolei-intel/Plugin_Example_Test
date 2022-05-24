#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "vfio-migration-plugin.h"
#include "plugin.h"
#include "common.h"

#define PORT 3333

struct device {
    char *name;
    uint64_t pending_bytes;
    uint64_t state;
    char *device_states;
    uint64_t offset;
};

struct device dev = {
    .state = 0,
    .pending_bytes = 1025,
    .offset = 0,
    .device_states = "This is a test for plugin in QEMU",
//    .device_states = "FOsEQuyudpBn5GjhRPBI8xieXhKWPHEgjSx1Z9Ih70JSjYAitu7odLiyZcpi6DySZLBQRNA1ACdVgQpJaQeDnFdvkI2yArEUcJwwyrnARRyIhvVp5je5pW551jIrauzQn16svpuIyP46QJzci7ihiktRHmHj2bIwKGeDiwS9MZ6jaOrMNIzRPCT6P3NsWMZ8cyFalaTsEsgbK4ZqKoVgjFzgkpEs932An058htJYP1EdR7gaWClNnzIZqjA6P72bgpltY6PovQ71kG7xEC5uVRWWORpiT5cBZBDnBFUDZ0QNZNBqW7LvneF3BIKp8lDl4zkSVTBZs0RXJbr1K8WFDudDTqIGHh7d0UnfcvOiymRtQPqD1ofCsuLqFzRfIsLwmLWPHikIftgEWQSYSJ8OtNK8NIXNtN14U8pqmdy65pbzNqgr0Eqa4G0WJt6FLFKBGH8TuM8gcdGx8254wYQABeWvDVZ2WjGXuFfqlArggE0iyHinTCAZ81u3l07lx0Jbs4g3LGUDe2AgzDQjSg201cUBuySWYzatApArh5JjGxyGzAoxsdKSoeGVXUuIncv7jilrzjYado8KLdteDDjb0ZPX2nXNjSEr5e3ARDZtFolOhbtOc51WJOEXD8CzK2eXknZEVNvlaRlRUjw78RutNxQA6ackJ6lqVWkAhSS7SfDYZrnZ0hYacWOrKIDu8BGmjRWSHDUS8Qx5PXsLCtWoTLvvWRwON5JTnnyu7eF0ZjCj0R6MXPeTeDnxdDm9gp4x4woDYY79aAkgHlA6TbwcZgxVD2Cdg97uOz36AfObqGOqKazFpZko2HAFHnnUlOplB30N1nx9IEIwIdXWfwYE3UKkioROdhBTCc8ALXoxuawHc8d3t28zDcJ8ZBLVIn1FGpyK7THbjE61doz4AoDMXPOwhGL3ez8AVn8nmx4DRPr24shNuClsTExlUSdS6KpaCPuTeMgAG1KFOyK1HZSPWd6zqgEXlVPgMv4v2iITclf5rfeff6GqHUQuBQu0U6E4X"
};

static int init_server()
{
    struct sockaddr_in server_addr;
    int sockfd =0;
    int ret = 0;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("create socket error");
        exit(1);
    }

    bzero(&server_addr,sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    ret = bind(sockfd,(struct sockaddr*)&server_addr,sizeof(struct sockaddr));
    if (ret < 0) {
        perror("bind error");
        exit(1);
    }
    listen(sockfd,1);

    return sockfd;
}

static int device_get_state(int fd, Plugin_Message *msg)
{
    Plugin_Message msg_replay = { 0 };
    
    msg_replay.request = VFIO_MIG_MESSAGE_REPLY;
    msg_replay.size = sizeof(uint64_t);
    msg_replay.u64 = dev.state;

    if (!send_all(fd, (void *)&msg_replay, sizeof(msg_replay))) {
        perror("send error");
    }

    return 0;
}

static int device_set_state(int fd, Plugin_Message *msg)
{
    dev.state = msg->u64;

    return 0;
}

static int device_get_pending_bytes(int fd, Plugin_Message *msg)
{
    Plugin_Message msg_replay = { 0 };

    msg_replay.request = VFIO_MIG_MESSAGE_REPLY;
    msg_replay.size = sizeof(uint64_t);
    msg_replay.u64 = dev.pending_bytes;

    if (!send_all(fd, (void *)&msg_replay, sizeof(msg_replay))) {
        perror("send error");
        return -1;
    }

    return 0;
}

static int device_load_buffer(int fd, Plugin_Message *msg)
{
    char *buffer = malloc(msg->size);

    if (!receive_all(fd, buffer, msg->size)) {
        perror("receive data error");
        return -1;
    }
    printf("Buffer is:%s\n", buffer);
    free(buffer);
    return 0;
}

static int device_save_buffer(int fd, Plugin_Message *msg)
{
/*    Plugin_Message msg_replay = { 0 };

    msg_replay.request = VFIO_MIG_MESSAGE_REPLY;
    msg_replay.size = strlen(dev.device_states);

    if (!send_all(fd, (void *)&msg_replay, sizeof(msg_replay))) {
        perror("send error");
    }
 */   
    if (!send_all(fd, dev.device_states + dev.offset, msg->size)) {
        perror("send error");
    }

    dev.offset += msg->size;

    if (dev.pending_bytes >= msg->size) {
        dev.pending_bytes -= strlen(dev.device_states);
    } else{
        dev.pending_bytes = 0;
    }
    printf("pending_bytes is:%ld\n", dev.pending_bytes);

    return 0;
}

static int handle_message(int fd, Plugin_Message *msg)
{
    switch (msg->request) {
        case VFIO_MIG_GET_STATE:
            return device_get_state(fd, msg);
        case VFIO_MIG_SET_STATE:
            return device_set_state(fd, msg);
        case VFIO_MIG_UPDATE_PENDING:
            return device_get_pending_bytes(fd, msg);
        case VFIO_MIG_LOAD_BUFFER:
            return device_load_buffer(fd, msg);
        case VFIO_MIG_SAVE_BUFFER:
            return device_save_buffer(fd, msg);
        default:
            perror("unknow message");
    }
    return 0;
}

int main(void)
{
    int ret = 0;
    struct sockaddr_in client_addr;
    int client_fd = 0;
    int sockfd = init_server();
    socklen_t sin_size = sizeof(struct sockaddr);

    Plugin_Message msg = { 0 };

    for(; ;) {
        client_fd = accept(sockfd,(struct sockaddr*)&client_addr,&sin_size);
        for(; ;) {
            if (!receive_all(client_fd, (void *)&msg, sizeof(msg))) {
                perror("receive msg error");
            }

            if (msg.request == VFIO_MIG_FINISH) {
                break;
            }

            if (msg.request < VFIO_MIG_MAX) {
                handle_message(client_fd, &msg);
                msg.request = 0;
            }
        }
        close(client_fd);
    }
    close(sockfd);
    return 0;
}

