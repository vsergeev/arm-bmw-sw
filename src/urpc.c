#include <stdint.h>
#include <stddef.h>

#include <io/uart.h>

//#define DEBUG
#include <debug.h>

#include "urpc.h"

/*
    RPC Protocol

    RPC Request/Response Format

        55 ab 55 cd CCCC MM RR LLLL <data>
            CCCC    16-bit little endian checksum over method, length, return code, data
              MM    8-bit method id
              RR    8-bit signed return code (-128 for method not found, don't care for request)
            LLLL    16-bit little endian length of data
          <data>    data

    RPC Handler Prototype

        int8_t foo(uint8_t *buf, uint16_t *len);

*/

#define CRC16_POLY          0x1021  /* CRC-16-CCITT */

static void crc16(uint16_t *crc, const uint8_t *buf, size_t len) {
    unsigned int i, j;

    for (i = 0; i < len; i++) {
        *crc = *crc ^ (buf[i] << 8);
        for (j = 0; j < 8; j++) {
            if (*crc & 0x8000)
                *crc = (*crc << 1) ^ CRC16_POLY;
            else
                *crc <<= 1;
        }
    }
}

static int rpc_read_request(uint8_t *method, uint16_t *len, uint8_t *buf) {
    uint32_t header = 0x0;
    int8_t retcode;
    uint16_t rpc_checksum, calc_checksum;

    /* RPC Format: 55 ab 55 cd CCCC MM RR LLLL <data> */

    /* Find the header */
    while (1) {
        header = (header << 8) | uart_getc();
        if (header == 0x55ab55cd)
            break;
    }
    /* Read checksum */
    rpc_checksum = uart_getc();
    rpc_checksum |= uart_getc() << 8;
    /* Read method */
    *method = uart_getc();
    /* Read return code */
    retcode = uart_getc();
    /* Read length */
    *len = uart_getc();
    *len |= uart_getc() << 8;

    dbg("%s: found rpc request with checksum 0x%04x, method 0x%02x, retcode %d, length %u\n", __func__, rpc_checksum, *method, retcode, *len);

    /* Validate length */
    if (*len > URPC_BUFFER_SIZE) {
        dbg("%s: error, huge data length %u (URPC_BUFFER_SIZE is %u)\n", __func__, *len, URPC_BUFFER_SIZE);
        return -1;
    }

    /* Read data into buffer */
    uart_read(buf, *len);

    /* Verify checksum */
    calc_checksum = 0x0000;
    crc16(&calc_checksum, (uint8_t *)method, 1);
    crc16(&calc_checksum, (uint8_t *)&retcode, 1);
    crc16(&calc_checksum, (uint8_t *)len, 2);
    crc16(&calc_checksum, (uint8_t *)buf, *len);
    if (calc_checksum != rpc_checksum) {
        dbg("%s: error, invalid checksum. got 0x%04x, computed 0x%04x\n", __func__, rpc_checksum, calc_checksum);
        return -2;
    }

    return 0;
}

static void rpc_write_response(uint8_t method, int8_t retcode, uint16_t len, uint8_t *buf) {
    uint16_t checksum;

    /* RPC Format: 55 ab 55 cd CCCC MM RR LLLL <data> */

    /* Calculate rpc checksum */
    checksum = 0x0000;
    crc16(&checksum, (uint8_t *)&method, 1);
    crc16(&checksum, (uint8_t *)&retcode, 1);
    crc16(&checksum, (uint8_t *)&len, 2);
    crc16(&checksum, buf, len);

    /* Write header */
    uart_putc(0x55);
    uart_putc(0xab);
    uart_putc(0x55);
    uart_putc(0xcd);
    /* Write checksum */
    uart_putc(checksum & 0xff);
    uart_putc((checksum >> 8) & 0xff);
    /* Write method */
    uart_putc(method);
    /* Write retcode */
    uart_putc((uint8_t)retcode);
    /* Write length */
    uart_putc(len & 0xff);
    uart_putc((len >> 8) & 0xff);
    /* Write data */
    uart_write(buf, len);
}

void urpc_server(void) {
    uint8_t buf[URPC_BUFFER_SIZE] __attribute__((aligned(4)));
    uint16_t len;
    uint8_t method;
    int8_t retcode;

    unsigned int i;

    while (1) {
        /* Read an RPC request */
        if (rpc_read_request(&method, &len, buf) < 0)
            continue;

        /* Find the RPC handler for this method */
        for (i = 0; RPC_Handlers[i].handler != NULL; i++) {
            if (method == RPC_Handlers[i].id) {
                dbg("%s: found request for rpc method 0x%02x\n", __func__, method);

                /* Call RPC handler */
                retcode = RPC_Handlers[i].handler(buf, &len);

                if (retcode >= 0) {
                    dbg("%s: success for rpc method 0x%02x (retcode %d)\n", __func__, method, retcode);
                } else {
                    dbg("%s: failure for rpc method 0x%02x (retcode %d)\n", __func__, method, retcode);
                }

                break;
            }
        }

        /* If RPC handler was not found, return failure code -128 */
        if (RPC_Handlers[i].handler == NULL) {
            dbg("%s: found request for unknown rpc method 0x%02x\n", __func__, method);
            retcode = -128;
            len = 0;
        }

        /* Write the RPC response */
        rpc_write_response(method, retcode, len, buf);
    }
}

