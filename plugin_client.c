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
#include <fcntl.h>
#include "vfio-migration-plugin.h"
#include "plugin.h"
#include "common.h"

struct plugin_handle {
    char *devid;
    char *arg;
    unsigned long pending_bytes;
    int fd;
};

struct plugin_handle handle; 

static int plugin_connect(char *ip, uint32_t port)
{
    int sockfd;
    int ret = 0;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket error");
        exit(1);
    }

    bzero(&server_addr,sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    ret = connect(sockfd,(struct sockaddr*)&server_addr,sizeof(struct sockaddr));
    if (ret < 0) {
        perror("connect failed");
        exit(1);
    }

    return sockfd;
}

static uint64_t plugin_get_u64(int fd, VFIOMigrationRequest request, uint64_t *u64)
{
    Plugin_Message message = { 0 };
    Plugin_Message msg_reply = { 0 };
    int ret = 0;

    message.request = request;

    if (!send_all(fd, (void *)&message, sizeof(Plugin_Message))) {
        perror("send error");
    }
    if (!receive_all(fd, (void *)&msg_reply, sizeof(msg_reply))) {
        perror("receive u64 error");
        return -1;
    }

    if (msg_reply.request != VFIO_MIG_MESSAGE_REPLY) {
        perror("unknow message");
        return -1;
    }
    *u64 = msg_reply.u64;
    return 0;
}

static void plugin_set_u64(int fd, VFIOMigrationRequest request, uint64_t u64)
{
    Plugin_Message message = { 0 };

    message.request = request;
    message.size = sizeof(uint64_t);
    message.u64 = u64;

    if (!send_all(fd, (char *)&message, sizeof(message))) {
        perror("send u64 error");
    }
}

static void* plugin_init(char *devid, char *arg)
{
    char addr[32] = "\0";
    char *ip = NULL;
    uint32_t port;

    strcpy(addr, arg);
    ip= strtok(addr, ":");
    port = atoi(strtok(NULL, ":"));

    handle.devid = devid;
    handle.arg = arg;
    handle.fd = plugin_connect(ip, port);

    return (void *)&handle;
}

static int plugin_save(void *arg, uint8_t *state, uint64_t len)
{
    struct plugin_handle *handle = (struct plugin_handle *)arg;
    Plugin_Message message = { 0 };
    int ret = 0;
    uint8_t *buffer = NULL;

    message.request = VFIO_MIG_SAVE_BUFFER;
    message.size = len;
    if (!send_all(handle->fd, (void *)&message, sizeof(message))) {
        perror("send error");
    }

/*   if (!receive_all(handle->fd, (void *)&message, sizeof(message))) {
        perror("receive error");
        return -1;
    }
*/
    if (!receive_all(handle->fd, (void *)state, len)) {
        perror("receive data error");
        return -1;
    }

    return 0;
}

static int plugin_load(void *arg, uint8_t *state, uint64_t len)
{
    struct plugin_handle *handle = (struct plugin_handle *)arg;
    Plugin_Message msg = { 0 };

    msg.request = VFIO_MIG_LOAD_BUFFER;
    msg.size = len;

    if (!send_all(handle->fd, (void *)&msg, sizeof(Plugin_Message))) {
        perror("send error");
    }

    if (!send_all(handle->fd, (void *)state, len)) {
        perror("send error");
    }

    return 0;
}

static int plugin_update_pending(void *arg, uint64_t *pending_bytes)
{
    struct plugin_handle *handle = (struct plugin_handle *)arg;

    if (plugin_get_u64(handle->fd, VFIO_MIG_UPDATE_PENDING, pending_bytes) < 0) {
        perror("get pending bytes error");
        return -1;
    }
    return 0;
}

static int plugin_set_state(void *arg, uint32_t value)
{
    struct plugin_handle *handle = (struct plugin_handle *)arg;

    plugin_set_u64(handle->fd, VFIO_MIG_SET_STATE, value);
    return 0;
}

static int plugin_get_state(void *arg, uint32_t *value)
{
    struct plugin_handle *handle = (struct plugin_handle *)arg;
    uint64_t u64 = 0;

    if (plugin_get_u64(handle->fd, VFIO_MIG_GET_STATE, &u64) < 0) {
        perror("plugin get state error");
        return -1;
    }
    *value = u64;
    return 0;
}

static int plugin_cleanup(void *arg)
{
    struct plugin_handle *handle = (struct plugin_handle *)arg;

    Plugin_Message msg = { 0 };
    msg.request = VFIO_MIG_FINISH;

    if(!send_all(handle->fd, (void *)&msg, sizeof(Plugin_Message))) {
        perror("send error");
    }

    close(handle->fd);
    return 0;
}

static VFIOMigrationPluginOps plugin_ops = {
    .init = plugin_init,
    .save = plugin_save,
    .load = plugin_load,
    .update_pending = plugin_update_pending,
    .set_state = plugin_set_state,
    .get_state = plugin_get_state,
    .cleanup = plugin_cleanup,
};

int vfio_lm_get_plugin_version(void)
{
    return VFIO_LM_PLUGIN_API_VERSION;
}
VFIOMigrationPluginOps* vfio_lm_get_plugin_ops(void)
{
    return &plugin_ops;
}

/*
int main(void)
{
    void *handle = NULL;
    uint8_t *buffer = NULL;
    uint64_t pending_bytes = 0;
    uint32_t state = 0;
    char *arg = "127.0.0.1:3333";

    handle = plugin_init(NULL, arg);

    plugin_set_state(handle, 3);
    plugin_get_state(handle, &state);
    printf("state is:%d\n", state);

    plugin_update_pending(handle, &pending_bytes);

    buffer = malloc(pending_bytes);

    plugin_save(handle, buffer, pending_bytes);

    printf("device states is:%s\n", buffer);
    plugin_load(handle, buffer, pending_bytes);
    plugin_cleanup(handle);
    free(buffer);
}*/
