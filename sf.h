#ifndef _SF_H
#define _SF_H

#include <stdint.h>
#include <stddef.h>

#include "spi.h"

enum spi_flash_error {
    SPI_FLASH_ERROR_PROBE   = -1,
    SPI_FLASH_ERROR_ARGS    = -2,
    SPI_FLASH_ERROR_TIMEOUT = -3,
};

struct spi_flash_params {
    uint32_t jedec_id;
    const char *name;
    uint32_t sector_size;
    uint32_t capacity;
    uint16_t flags;
};

#define SF_FLAG_CHIP_ERASE   (1<<0)

struct spi_flash {
    struct spi_slave *spi;
    const struct spi_flash_params *params;
};

int spi_flash_probe(struct spi_flash *flash, struct spi_slave *spi);
int spi_flash_erase(struct spi_flash *flash, uint32_t address, size_t len);
int spi_flash_read(struct spi_flash *flash, uint32_t address, uint8_t *buf, size_t len);
int spi_flash_write(struct spi_flash *flash, uint32_t address, const uint8_t *buf, size_t len);

#endif

