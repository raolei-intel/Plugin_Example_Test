#ifndef PLUGIN_H
#define PLUGIN_H

#include <stdint.h>

typedef enum VFIOMigrationRequest {
    VFIO_MIG_NONE = 0,
    VFIO_MIG_UPDATE_PENDING = 1,
    VFIO_MIG_SET_STATE = 2,
    VFIO_MIG_GET_STATE = 3,
    VFIO_MIG_SAVE_BUFFER = 4,
    VFIO_MIG_LOAD_BUFFER = 5,
    VFIO_MIG_MESSAGE_REPLY = 6,
    VFIO_MIG_FINISH = 7,
    VFIO_MIG_MAX
} VFIOMigrationRequest;


typedef struct Plugin_Message {
    VFIOMigrationRequest request;
    uint32_t size;
    uint64_t u64;
} Plugin_Message;

#endif
