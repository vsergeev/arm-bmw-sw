#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <lpc11xx/LPC11xx.h>

#include "spi.h"

#include "debug.h"

#define SPI_PCLK            48000000

/**********************************************************************/

struct spi_master {
    LPC_SSP_TypeDef *ssp;
};

struct spi_master SPI0;

/**********************************************************************/

void spi_init(void) {
    /* Configure P0.8 for MISO, P0.9 for MOSI, P0.6 for SCK function */
    LPC_IOCON->PIO0_8 = 0x1;
    LPC_IOCON->PIO0_9 = 0x1;
    LPC_IOCON->PIO0_6 = 0x2;
    LPC_IOCON->SCK_LOC = 0x2;

    /* Deassert SPI0 controller reset */
    LPC_SYSCON->PRESETCTRL |= (1<<0);

    /* Disable SPI0 */
    LPC_SSP0->CR1 = 0;

    /* SPI Frequency = PCLK / (CPSDVSR * SCR+1)
     *  CPSDVSR = 2-254, even only
     *  SCR = 0-255 */

    /* Set 8-bit transfers, SPI frame format, mode 0, SCR = 0 */
    LPC_SSP0->CR0 = 0x7;
    /* SPI Frequency = PCLK / (2*1) = PCLK/2 */
    LPC_SSP0->CPSR = 2;
    /* Disable interrupts */
    LPC_SSP0->IMSC = 0;

    /* Setup SPI0 spi master singleton */
    SPI0.ssp = LPC_SSP0;

    /* Enable SPI0 */
    LPC_SSP0->CR1 |= (1<<1);
}

void spi_setup(struct spi_slave *slave) {
    struct spi_master *master = slave->master;
    uint32_t cpsr;

    /* Disable SPI0 */
    master->ssp->CR1 = 0;

    dbg("%s: set up slave cs=%d\n", __func__, slave->cs_pin);

    /* Set SPI mode. */
    /* CPHA and CPOL bits are flipped in register from mode value. */
    master->ssp->CR0 = (master->ssp->CR0 & ~0xc0) | ((slave->mode & 0x1) << 7) | ((slave->mode & 0x2) << 5);

    dbg("%s: set spi mode=%d\n", __func__, slave->mode);

    /* Set SPI speed. */
    /* Use SCR=0, and find the largest even CPSR. */
    cpsr = SPI_PCLK / slave->speed;
    /* Snap to next even value */
    cpsr += (cpsr % 2);
    /* Saturate low at 2, high at 254 */
    cpsr = (cpsr < 2) ? 2 : cpsr;
    cpsr = (cpsr > 254) ? 254 : cpsr;
    master->ssp->CPSR = cpsr;

    dbg("%s: set spi speed=%d hz (cpsr=%d)\n", __func__, SPI_PCLK/cpsr, cpsr);

    /* Re-enable SPI0 */
    LPC_SSP0->CR1 |= (1<<1);
}

void spi_select(struct spi_slave *slave) {
    uint32_t value = (slave->cs_active_high) ? 0xffff : 0x0;

    dbg("%s: select slave cs=%d, active high=%s\n", __func__, slave->cs_pin, (slave->cs_active_high) ? "true" : "false");

    if (slave->cs_pin < 12)
        LPC_GPIO0->MASKED_ACCESS[(1 << slave->cs_pin)] = value;
    else if (slave->cs_pin >= 12 && slave->cs_pin < 24)
        LPC_GPIO1->MASKED_ACCESS[(1 << (slave->cs_pin-12))] = value;
}

void spi_deselect(struct spi_slave *slave) {
    uint32_t value = (slave->cs_active_high) ? 0x0 : 0xffff;

    dbg("%s: deselect slave cs=%d, active high=%s\n", __func__, slave->cs_pin, (slave->cs_active_high) ? "true" : "false");

    if (slave->cs_pin < 12)
        LPC_GPIO0->MASKED_ACCESS[(1 << slave->cs_pin)] = value;
    else if (slave->cs_pin >= 12 && slave->cs_pin < 24)
        LPC_GPIO1->MASKED_ACCESS[(1 << (slave->cs_pin-12))] = value;
}

void spi_transfer(struct spi_slave *slave, const uint8_t *txbuf, uint8_t *rxbuf, size_t len) {
    unsigned int txi, rxi;
    volatile uint8_t dummy;

    dbg("%s: transfer start txbuf=%p, rxbuf=%p, len=%d\n", __func__, txbuf, rxbuf, len);

    /* We use a polling transfer, because interrupts are inefficient in terms
     * of data transfer time for SPI frequencies that are close to our CPU
     * frequency. */
    for (txi = 0, rxi = 0; rxi < len; ) {
        /* While TX FIFO is not full, txbuf has bytes left, and the TX/RX gap is less than FIFO size */
        while ((slave->master->ssp->SR & (1<<1)) && (txi < len) && ((txi - rxi) < 8)) {
            if (txbuf)
                slave->master->ssp->DR = txbuf[txi];
            else
                slave->master->ssp->DR = 0xff;
            txi++;
        }
        /* While the Receive FIFO is not empty, and rxbuf has bytes left */
        while ((slave->master->ssp->SR & (1<<2)) && (rxi < len)) {
            if (rxbuf)
                rxbuf[rxi] = slave->master->ssp->DR;
            else {
                dummy = slave->master->ssp->DR;
                dummy;
            }
            rxi++;
        }
    }

    #if 0
    unsigned int i;
    for (i = 0; i < len; i++) {
        /* While TX FIFO is not empty */
        while (!(slave->master->ssp->SR & (1<<0)))
            ;
        if (txbuf)
            slave->master->ssp->DR = txbuf[i];
        else
            slave->master->ssp->DR = 0xff;

        /* While RX FIFO is empty */
        while (!(slave->master->ssp->SR & (1<<2)))
            ;
        if (rxbuf)
            rxbuf[i] = slave->master->ssp->DR;
        else
            dummy = slave->master->ssp->DR;
    }
    #endif
}

