#include <stdint.h>
#include <stddef.h>

#include "sf.h"
#include "spi.h"
#include "tick.h"

#include "debug.h"

/**********************************************************************/

static const struct spi_flash_params spi_flash_supported[] = {
    { 0x014014, "S25FLK208K", 4096, 1048576, SF_FLAG_CHIP_ERASE },
};

/**********************************************************************/

/* Common SPI Flash Commands */
#define SF_CMD_RDID     0x9F        /* Read JEDEC ID */
#define SF_CMD_WREN     0x06        /* Write Enable */
#define SF_CMD_WRDI     0x04        /* Write Disable */
#define SF_CMD_RDSR     0x05        /* Read Status Register */
#define SF_CMD_SE       0x20        /* Sector Erase (e.g. 4K) */
#define SF_CMD_BE       0xD8        /* Block Erase (e.g. 64K) */
#define SF_CMD_CE       0xC7        /* Chip Erase */
#define SF_CMD_READ     0x03        /* Read */
#define SF_CMD_PP       0x02        /* Page Program */

/**********************************************************************/

static void sf_cmd_rdid(struct spi_slave *slave, uint32_t *jedec_id) {
    uint8_t cmd[4] = {SF_CMD_RDID, 0xff, 0xff, 0xff};
    spi_select(slave);
    spi_transfer(slave, cmd, cmd, sizeof(cmd));
    spi_deselect(slave);

    *jedec_id = cmd[1] << 16;
    *jedec_id |= cmd[2] << 8;
    *jedec_id |= cmd[3];
}

static void sf_cmd_wren(struct spi_slave *slave) {
    uint8_t cmd[1] = {SF_CMD_WREN};
    spi_select(slave);
    spi_transfer(slave, cmd, NULL, sizeof(cmd));
    spi_deselect(slave);
}


static void sf_cmd_rdsr(struct spi_slave *slave, uint8_t *sr) {
    uint8_t cmd[2] = {SF_CMD_RDSR, 0xff};
    spi_select(slave);
    spi_transfer(slave, cmd, cmd, sizeof(cmd));
    spi_deselect(slave);

    *sr = cmd[1];
}

static void sf_cmd_sector_erase(struct spi_slave *slave, uint32_t address) {
    uint8_t cmd[4] = {SF_CMD_SE, 0xff, 0xff, 0xff};
    cmd[1] = (address >> 16) & 0xff;
    cmd[2] = (address >> 8) & 0xff;
    cmd[3] = (address) & 0xff;
    spi_select(slave);
    spi_transfer(slave, cmd, NULL, sizeof(cmd));
    spi_deselect(slave);
}

static void sf_cmd_chip_erase(struct spi_slave *slave) {
    uint8_t cmd[1] = {SF_CMD_CE};
    spi_select(slave);
    spi_transfer(slave, cmd, NULL, sizeof(cmd));
    spi_deselect(slave);
}

static void sf_cmd_read(struct spi_slave *slave, uint32_t address, uint8_t *buf, size_t len) {
    uint8_t cmd[4] = {SF_CMD_READ, 0xff, 0xff, 0xff};
    cmd[1] = (address >> 16) & 0xff;
    cmd[2] = (address >> 8) & 0xff;
    cmd[3] = (address) & 0xff;
    spi_select(slave);
    spi_transfer(slave, cmd, NULL, sizeof(cmd));
    spi_transfer(slave, NULL, buf, len);
    spi_deselect(slave);
}

static void sf_cmd_page_program(struct spi_slave *slave, uint32_t address, const uint8_t *buf, size_t len) {
    uint8_t cmd[4] = {SF_CMD_PP, 0xff, 0xff, 0xff};
    cmd[1] = (address >> 16) & 0xff;
    cmd[2] = (address >> 8) & 0xff;
    cmd[3] = (address) & 0xff;
    spi_select(slave);
    spi_transfer(slave, cmd, NULL, sizeof(cmd));
    spi_transfer(slave, buf, NULL, len);
    spi_deselect(slave);
}

/* Unused commands */
#if 0
static void sf_cmd_wrdi(struct spi_slave *slave) {
    uint8_t cmd[1] = {SF_CMD_WRDI};
    spi_select(slave);
    spi_transfer(slave, cmd, NULL, sizeof(cmd));
    spi_deselect(slave);
}

static void sf_cmd_block_erase(struct spi_slave *slave, uint32_t address) {
    uint8_t cmd[4] = {SF_CMD_BE, 0xff, 0xff, 0xff};
    cmd[1] = (address >> 16) & 0xff;
    cmd[2] = (address >> 8) & 0xff;
    cmd[3] = (address) & 0xff;
    spi_select(slave);
    spi_transfer(slave, cmd, NULL, sizeof(cmd));
    spi_deselect(slave);
}
#endif

/**********************************************************************/

static int sf_poll_wip(struct spi_slave *slave, uint32_t timeout) {
    uint32_t tic;
    uint8_t sr;

    /* Poll status register for WIP clear */

    tic = time();
    while (1) {
        sf_cmd_rdsr(slave, &sr);
        dbg("%s: read sr: 0x%02x\n", __func__, sr);
        if (!(sr & (1<<0))) {
            dbg("%s: wip cleared!\n", __func__);
            break;
        }

        if ((time() - tic) > timeout) {
            dbg("%s: wip clear polling timed out!\n", __func__);
            return SPI_FLASH_ERROR_TIMEOUT;
        }
        delay_ms(10);
    }
    dbg("%s: operation took ~%d ms\n", __func__, time()-tic);

    return 0;
}

int spi_flash_probe(struct spi_flash *flash, struct spi_slave *spi) {
    uint32_t jedec_id;
    unsigned int i;

    dbg("%s: entered\n", __func__);

    sf_cmd_rdid(spi, &jedec_id);
    dbg("%s: got jedec id 0x%06x\n", __func__, jedec_id);

    for (i = 0; i < sizeof(spi_flash_supported)/sizeof(spi_flash_supported[0]); i++) {
        if (spi_flash_supported[i].jedec_id == jedec_id) {
            dbg("%s: found jedec id match index=%d\n", __func__, i);
            flash->spi = spi;
            flash->params = &spi_flash_supported[i];
            return 0;
        }
    }

    dbg("%s: no jedec id match found\n", __func__);
    flash->spi = NULL;
    flash->params = NULL;

    return SPI_FLASH_ERROR_PROBE;
}

int spi_flash_erase(struct spi_flash *flash, uint32_t address, size_t len) {
    uint32_t address_stop;

    dbg("%s: address=0x%06x, len=%d\n", __func__, address, len);

    /* Address should be mod sector size.
     * (Let's not make any assumptions about what it was supposed to be ;) ) */
    if ((address % flash->params->sector_size) != 0) {
        dbg("%s: got invalid address 0x%06x (sector size 0x%06x)\n", __func__, address, flash->params->sector_size);
        return SPI_FLASH_ERROR_ARGS;
    }

    /* Length should be mod sector size.
     * (Let's not make any assumptions about what it was supposed to be ;) ) */
    if ((len % flash->params->sector_size) != 0) {
        dbg("%s: got invalid length %d (sector size 0x%06x)\n", __func__, len, flash->params->sector_size);
        return SPI_FLASH_ERROR_ARGS;
    }

    /* If we want to do a chip erase, and can do it, do it */
    if (address == 0 && len == flash->params->capacity &&
            (flash->params->flags & SF_FLAG_CHIP_ERASE)) {
        dbg("%s: write enable\n", __func__);
        sf_cmd_wren(flash->spi);

        dbg("%s: performing chip erase\n", __func__);
        sf_cmd_chip_erase(flash->spi);

        return sf_poll_wip(flash->spi, 20000);
    }

    /* Saturate length at the capacity of the device */
    if ((address + len) > flash->params->capacity) {
        dbg("%s: got out of bounds length 0x%06x (address 0x%06x)\n", __func__, len, address);
        len -= (address+len) - flash->params->capacity;
        dbg("%s: saturated length to 0x%06x\n", __func__, len);
    }

    /* Sector erase loop */
    address_stop = address + len;
    dbg("%s: sector erase loop start=0x%06x, stop=0x%06x, iter=0x%06x\n", __func__, address, address_stop, flash->params->sector_size);
    for (; address < address_stop; address += flash->params->sector_size) {
        dbg("%s: write enable\n", __func__);
        sf_cmd_wren(flash->spi);

        dbg("%s: erase sector 0x%06x\n", __func__, address);
        sf_cmd_sector_erase(flash->spi, address);

        if (sf_poll_wip(flash->spi, 500) < 0)
            return SPI_FLASH_ERROR_TIMEOUT;
    }

    return 0;
}

int spi_flash_read(struct spi_flash *flash, uint32_t address, uint8_t *buf, size_t len) {
    dbg("%s: address=0x%06x, buf=%p, len=%d\n", __func__, address, buf, len);
    sf_cmd_read(flash->spi, address, buf, len);
    return 0;
}

int spi_flash_write(struct spi_flash *flash, uint32_t address, const uint8_t *buf, size_t len) {
    uint32_t offset;
    uint16_t plen;
    dbg("%s: address=0x%06x, buf=%p, len=%d\n", __func__, address, buf, len);

    /* Addess + length should not exceed capacity of the device */
    if ((address + len) > flash->params->capacity) {
        dbg("%s: got out of bounds length address=0x%06x len=%d (capacity=0x%06x)\n", __func__, address, len, flash->params->capacity);
        return SPI_FLASH_ERROR_ARGS;
    }

    dbg("%s: write loop start\n", __func__);
    for (offset = 0; offset < len;) {
        /* Calculate program length align address to a page boundary */
        plen = 0x100 - (address & 0xff);
        /* Saturate program length by remaining length */
        plen = (plen > (len-offset)) ? (len-offset) : plen;

        dbg("%s: write iteration offset=0x%06x, address=0x%06x, plen=%d\n", __func__, offset, address, plen);

        dbg("%s: write enable\n", __func__);
        sf_cmd_wren(flash->spi);

        dbg("%s: write page address=0x%06x, buf=%p + %d, len=%d\n", __func__, address, buf, offset, plen);
        sf_cmd_page_program(flash->spi, address, buf + offset, plen);

        if (sf_poll_wip(flash->spi, 10) < 0)
            return SPI_FLASH_ERROR_TIMEOUT;

        address += plen;
        offset += plen;
    }

    return 0;
}

