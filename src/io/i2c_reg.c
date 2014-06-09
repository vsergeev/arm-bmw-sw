#include <stdint.h>

#include "i2c.h"

int i2c_detect(struct i2c_master *i2c, uint8_t address) {
    struct i2c_msg msg = { .buf = NULL, .len = 0, . read = false };
    struct i2c_transaction transaction = { .master = i2c, .msgs = &msg, .address = address, .count = 1 };

    /* Simply look for an ACK to this address, no data */
    /* [S] [Addr W|A] [P] */
    msg.buf = NULL;
    msg.len = 0;
    msg.read = false;
    transaction.master = i2c;
    transaction.address = address;
    transaction.msgs = &msg;
    transaction.count = 1;

    return i2c_transfer(&transaction);
}

int i2c_reg_read(struct i2c_master *i2c, uint8_t address, uint8_t reg, uint8_t *data) {
    struct i2c_transaction transaction;
    struct i2c_msg msgs[2];

    /* [S] [Addr W|A] [Reg|A] [S] [Addr|R] [Data|N] [P] */
    msgs[0].buf = &reg;
    msgs[0].len = 1;
    msgs[0].read = false;
    msgs[1].buf = data;
    msgs[1].len = 1;
    msgs[1].read = true;
    transaction.master = i2c;
    transaction.address = address;
    transaction.msgs = msgs;
    transaction.count = 2;

    return i2c_transfer(&transaction);
}

int i2c_reg_write(struct i2c_master *i2c, uint8_t address, uint8_t reg, uint8_t data) {
    struct i2c_transaction transaction;
    struct i2c_msg msgs[1];
    uint8_t wbuf[2];

    /* [S] [Addr W|A] [Reg|A] [Data|A] [P] */
    wbuf[0] = reg;
    wbuf[1] = data;
    msgs[0].buf = wbuf;
    msgs[0].len = 2;
    msgs[0].read = false;
    transaction.master = i2c;
    transaction.address = address;
    transaction.msgs = msgs;
    transaction.count = 1;

    return i2c_transfer(&transaction);
}

int i2c_reg_read_multiple(struct i2c_master *i2c, uint8_t address, uint8_t reg, uint8_t *data, size_t len) {
    struct i2c_transaction transaction;
    struct i2c_msg msgs[2];

    /* [S] [Addr W|A] [Reg|A] [S] [Addr|R] [Data|A] [Data|A] ... [Data|N] [P] */
    msgs[0].buf = &reg;
    msgs[0].len = 1;
    msgs[0].read = false;
    msgs[1].buf = data;
    msgs[1].len = len;
    msgs[1].read = true;
    transaction.master = i2c;
    transaction.address = address;
    transaction.msgs = msgs;
    transaction.count = 2;

    return i2c_transfer(&transaction);
}

