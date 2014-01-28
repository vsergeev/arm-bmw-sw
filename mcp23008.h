#ifndef _MCP23008_H
#define _MCP23008_H

#include <stdint.h>
#include "i2c.h"

#define MCP_REG_IODIR   0x00
#define MCP_REG_IPOL    0x01
#define MCP_REG_GPINTEN 0x02
#define MCP_REG_DEFVAL  0x03
#define MCP_REG_INTCON  0x04
#define MCP_REG_IOCON   0x05
#define MCP_REG_GPPU    0x06
#define MCP_REG_INTF    0x07
#define MCP_REG_INTCAP  0x08
#define MCP_REG_GPIO    0x09
#define MCP_REG_OLAT    0x0A

#define MCP_IOCON_SEQOP     (1<<5)
#define MCP_IOCON_DISSLW    (1<<4)
#define MCP_IOCON_ODR       (1<<2)
#define MCP_IOCON_INTPOL    (1<<1)

struct mcp23008 {
    struct i2c_master *i2c;
    uint8_t address;
};

int mcp23008_probe(struct mcp23008 *mcp, struct i2c_master *i2c, uint8_t address);
int mcp23008_reg_write(struct mcp23008 *mcp, uint8_t reg, uint8_t data);
int mcp23008_reg_read(struct mcp23008 *mcp, uint8_t reg, uint8_t *data);

#endif

