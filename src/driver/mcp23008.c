#include <stdint.h>

#include <io/i2c.h>
#include <io/i2c_reg.h>

#include "mcp23008.h"

int mcp23008_probe(struct mcp23008 *mcp, struct i2c_master *i2c, uint8_t address) {
    int ret;

    if ((ret = i2c_detect(i2c, address)) < 0) {
        mcp->i2c = NULL;
        mcp->address = 0;
        return ret;
    }

    mcp->i2c = i2c;
    mcp->address = address;

    return 0;
}

int mcp23008_reg_write(struct mcp23008 *mcp, uint8_t reg, uint8_t data) {
    return i2c_reg_write(mcp->i2c, mcp->address, reg, data);
}

int mcp23008_reg_read(struct mcp23008 *mcp, uint8_t reg, uint8_t *data) {
    return i2c_reg_read(mcp->i2c, mcp->address, reg, data);
}

