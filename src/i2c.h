#ifndef _I2C_H
#define _I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <lpc11xx/LPC11xx.h>

enum i2c_transfer_error {
    I2C_ERROR_TIMEOUT       = -1,
    I2C_ERROR_NACK          = -2,
    I2C_ERROR_ARBITRATION   = -3,
    I2C_ERROR_UNKNOWN       = -4,
};

struct i2c_msg {
    uint8_t *buf;
    unsigned int len;
    bool read;
};

struct i2c_transaction {
    struct i2c_master *master;
    uint8_t address;
    struct i2c_msg *msgs;
    unsigned int count;

    int error;
    volatile bool complete;
};

extern struct i2c_master I2C0;

void i2c_init(void);

/* Asynchronous transfer */
void i2c_start_transaction(struct i2c_transaction *transaction);
bool i2c_status_transaction(struct i2c_transaction *transaction);
void i2c_wait_transaction(struct i2c_transaction *transaction);

/* Synchronous transfer wrapper */
int i2c_transfer(struct i2c_transaction *transaction);

#endif

