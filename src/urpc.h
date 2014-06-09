#ifndef _URPC_H
#define _URPC_H

#include <stdint.h>

struct urpc_handler {
    uint8_t id;
    int8_t (*handler)(uint8_t *buf, uint16_t *len);
};

extern const struct urpc_handler RPC_Handlers[];

#define URPC_BUFFER_SIZE     2048

void urpc_server(void);

#endif

