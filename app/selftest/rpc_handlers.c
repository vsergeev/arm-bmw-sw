#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <test.h>
#include <urpc.h>

static int8_t version(uint8_t *buf, uint16_t *len) {
    memcpy(buf, STR_VERSION, strlen(STR_VERSION));
    *len = strlen(STR_VERSION);
    return 0;
}

static int8_t sum32(uint8_t *buf, uint16_t *len) {
    uint32_t sum = 0;
    uint32_t *p = (uint32_t *)buf;
    unsigned int i;

    if ((*len % 4) != 0)
        return -1;

    for (i = 0; i < *len; i += 4)
        sum += *p++;

    memcpy(buf, &sum, sizeof(sum));
    *len = sizeof(sum);

    return 0;
}

const struct urpc_handler RPC_Handlers[] = {
    { .id = 0x10, .handler = version },
    { .id = 0x20, .handler = sum32 },
    { .handler = NULL },
};

