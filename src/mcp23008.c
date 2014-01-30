#include <stdint.h>

#include "i2c.h"

#include "mcp23008.h"

int mcp23008_probe(struct mcp23008 *mcp, struct i2c_master *i2c, uint8_t address) {
    struct i2c_transaction transaction;
    struct i2c_msg msg;

    /* Simply look for an ACK to this address */
    msg.buf = NULL;
    msg.len = 0;
    msg.read = false;
    transaction.master = i2c;
    transaction.address = address;
    transaction.msgs = &msg;
    transaction.count = 1;
    if (i2c_transfer(&transaction) < 0) {
        mcp->i2c = NULL;
        mcp->address = 0;
        return -1;
    }

    mcp->i2c = i2c;
    mcp->address = address;

    return 0;
}

int mcp23008_reg_write(struct mcp23008 *mcp, uint8_t reg, uint8_t data) {
    struct i2c_transaction transaction;
    struct i2c_msg msg;
    uint8_t buf[2];

    /* [S] [Addr|W] [Reg|A] [Data|A] [P] */
    buf[0] = reg;
    buf[1] = data;
    msg.buf = buf;
    msg.len = 2;
    msg.read = false;
    transaction.master = mcp->i2c;
    transaction.address = mcp->address;
    transaction.msgs = &msg;
    transaction.count = 1;
    if (i2c_transfer(&transaction) == 0)
        return 0;

    return -1;
}

int mcp23008_reg_read(struct mcp23008 *mcp, uint8_t reg, uint8_t *data) {
    struct i2c_transaction transaction;
    struct i2c_msg msgs[2];
    uint8_t wbuf[1], rbuf[1];

    /* [S] [Addr|W] [Reg|A] [S] [Addr|R] [Data|N] [P] */
    wbuf[0] = reg;
    rbuf[0] = 0xff;
    msgs[0].buf = wbuf;
    msgs[0].len = 1;
    msgs[0].read = false;
    msgs[1].buf = rbuf;
    msgs[1].len = 1;
    msgs[1].read = true;
    transaction.master = mcp->i2c;
    transaction.address = mcp->address;
    transaction.msgs = msgs;
    transaction.count = 2;
    if (i2c_transfer(&transaction) < 0)
        return -1;

    *data = rbuf[0];

    return 0;
}

