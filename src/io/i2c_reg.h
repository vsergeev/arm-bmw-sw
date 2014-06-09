#ifndef _I2C_REG_H
#define _I2C_REG_H

#include <io/i2c.h>

int i2c_detect(struct i2c_master *i2c, uint8_t address);
int i2c_reg_read(struct i2c_master *i2c, uint8_t address, uint8_t reg, uint8_t *data);
int i2c_reg_write(struct i2c_master *i2c, uint8_t address, uint8_t reg, uint8_t data);

int i2c_reg_read_multiple(struct i2c_master *i2c, uint8_t address, uint8_t reg, uint8_t *data, size_t len);

#endif

