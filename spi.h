#ifndef _SPI_H
#define _SPI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <lpc11xx/LPC11xx.h>

struct spi_slave {
    struct spi_master *master;
    unsigned int speed;
    uint8_t mode;
    uint8_t cs_pin;
    bool cs_active_high;
};

extern struct spi_master SPI0;

void spi_init(void);
void spi_setup(struct spi_slave *slave);
void spi_select(struct spi_slave *slave);
void spi_transfer(struct spi_slave *slave, uint8_t *txbuf, uint8_t *rxbuf, size_t len);
void spi_deselect(struct spi_slave *slave);

#endif

