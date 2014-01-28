#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <lpc11xx/LPC11xx.h>

#include "i2c.h"

#include "queue.h"

//#define DEBUG
#include "debug.h"

#define I2C_PCLK        48000000
#define I2C_QUEUE_SIZE  5
#define I2C_NUM_RETRIES 3

/**********************************************************************/

struct i2c_master {
    LPC_I2C_TypeDef *i2c;
    struct {
        struct i2c_transaction *active_transaction;
        int retries;
        unsigned int i2c_msg_index;
        unsigned int i2c_msg_byte_index;
    };
    QUEUE(5) transaction_queue;
};

struct i2c_master I2C0;

#define I2C_CON_AA      (1<<2)  /* Assert acknowledge flag */
#define I2C_CON_SI      (1<<3)  /* Interrupt flag */
#define I2C_CON_STO     (1<<4)  /* STOP flag */
#define I2C_CON_STA     (1<<5)  /* START flag */
#define I2C_CON_I2EN    (1<<6)  /* I2C enable flag */

/**********************************************************************/

static void i2c_master_dequeue(struct i2c_master *master) {
    /* Dequeue the next transaction */
    master->active_transaction = queue_dequeue((struct queue *)&master->transaction_queue);
    if (master->active_transaction == NULL)
        return;

    dbg("%s: dequeued transaction address=0x%02x, msgs=%p, num=%d\n", __func__, master->active_transaction->address, master->active_transaction->msgs, master->active_transaction->count);

    /* Reset our retry and message indices */
    master->retries = I2C_NUM_RETRIES;
    master->i2c_msg_index = 0;
    master->i2c_msg_byte_index = 0;

    /* Transmit the start condition to start this transaction */
    master->i2c->CONSET = I2C_CON_STA;
}

static void i2c_master_sm(struct i2c_master *master) {
    struct i2c_transaction *transaction = master->active_transaction;
    uint8_t status = master->i2c->STAT;
    bool dequeue = false;

    /* See UM10398, pg. 270 */

    /* START transmitted */
    if (status == 0x08) {
        dbg("%s: status=0x%02x, START transmitted\n", __func__, status);

        /* Load slave address + R/W bit for this message in data register */
        master->i2c->DAT = (transaction->address << 1) | (transaction->msgs[master->i2c_msg_index].read & 0x1);
        /* Set assert acknowledge flag */
        master->i2c->CONSET = I2C_CON_AA;
        /* Clear start condition */
        master->i2c->CONCLR = I2C_CON_STA;

    /* Repeated START transmitted */
    } else if (status == 0x10) {
        dbg("%s: status=0x%02x, Repeated START transmitted\n", __func__, status);

        /* Load slave address + R/W bit for this message in data register */
        master->i2c->DAT = (transaction->address << 1) | (transaction->msgs[master->i2c_msg_index].read & 0x1);
        /* Set assert acknowledge flag */
        master->i2c->CONSET = I2C_CON_AA;
        /* Clear start condition */
        master->i2c->CONCLR = I2C_CON_STA;

    /*** Master Transmit States ***/

    /* ACK to [START, Slave Address, Write] received */
    /* ACK to [DATA] received received */
    } else if (status == 0x18 || status == 0x28) {
        if (status == 0x18) {
            dbg("%s: status=0x%02x, (mtx) ACK to [START, Slave Address, Write] received\n", __func__, status);
        } else if (status == 0x28) {
            dbg("%s: status=0x%02x, (mtx) ACK to [DATA] received\n", __func__, status);
        }

        /* If we have data bytes for this message left to transmit */
        if (master->i2c_msg_byte_index < transaction->msgs[master->i2c_msg_index].len) {
            dbg("%s: transmitting data byte msg_index=%d, byte_index=%d, data=0x%02x\n", __func__, master->i2c_msg_index, master->i2c_msg_byte_index, transaction->msgs[master->i2c_msg_index].buf[master->i2c_msg_byte_index]);

            /* Load the next transmit data byte */
            master->i2c->DAT = transaction->msgs[master->i2c_msg_index].buf[master->i2c_msg_byte_index];
            /* Set assert acknowledge flag */
            master->i2c->CONSET = I2C_CON_AA;

            master->i2c_msg_byte_index++;

        /* If we have finished this message, but have messages left to transmit/receive */
        } else if ((master->i2c_msg_index+1) < transaction->count) {
            master->i2c_msg_byte_index = 0;
            master->i2c_msg_index++;

            dbg("%s: transmitting repeated start for new msg_index=%d\n", __func__, master->i2c_msg_index);

            /* Transmit a repeated start condition for our next message */
            master->i2c->CONSET = I2C_CON_STA | I2C_CON_AA;

        /* Otherwise, we're finished with this transaction */
        } else {
            dbg("%s: done with transaction, transmitting stop condition\n", __func__);
            /* Transmit a STOP condition */
            master->i2c->CONSET = I2C_CON_STO | I2C_CON_AA;
            /* Mark the transfer complete and successful */
            master->active_transaction->complete = true;
            master->active_transaction->error = 0;
            /* Dequeue another transaction */
            dequeue = true;
        }

    /* NACK to [START, Slave Address, Write] received (Error) */
    /* NACK to [DATA] received (Error) */
    } else if (status == 0x20 || status == 0x30) {
        if (status == 0x20) {
            dbg("%s: status=0x%02x, (mtx) NACK to [START, Slave Address, Write] received (error)\n", __func__, status);
        } else if (status == 0x30) {
            dbg("%s: status=0x%02x, (mtx) NACK to [DATA] received (error)\n", __func__, status);
        }

        /* If this is our first message and first byte, re-attempt the transfer
         * up to I2C_NUM_RETRIES times with a STOP followed by another START */
        if (master->i2c_msg_index == 0 && master->i2c_msg_byte_index == 0 && master->retries > 0) {
            dbg("%s: initial NACK, attempting retry number %d\n", __func__, I2C_NUM_RETRIES-(master->retries-1));
            /* Transmit STOP + START condition */
            master->i2c->CONSET = I2C_CON_STO | I2C_CON_STA | I2C_CON_AA;
            /* Decrement retries */
            master->retries--;

        /* If this is not our first message or not our first byte, e.g. a NACK
         * response to a repeated start condition, or a NACK to a transmitted
         * byte, or all retries have been exhausted, then resort to failing the
         * transaction. */
        } else {
            if (master->i2c_msg_index > 0 || master->i2c_msg_byte_index > 0) {
                dbg("%s: intermediate NACK, failing transaction\n", __func__);
            } else {
                dbg("%s: retries exhausted, failing transaction\n", __func__);
            }
            /* Transmit STOP condition */
            master->i2c->CONSET = I2C_CON_STO | I2C_CON_AA;
            /* Mark the transfer complete and failed */
            master->active_transaction->complete = true;
            master->active_transaction->error = (master->i2c_msg_index > 0 || master->i2c_msg_byte_index > 0) ? I2C_ERROR_NACK : I2C_ERROR_TIMEOUT;
            /* Dequeue another transaction */
            dequeue = true;
        }

    /* Arbitration Lost (Error) */
    } else if (status == 0x38) {
        dbg("%s: status=0x%02x, (mtx/mrx) arbitration lost (error)\n", __func__, status);

        /* Arbitration lost handling currently not really supported by this state machine. */

        /* Mark the transfer complete and failed */
        master->active_transaction->complete = true;
        master->active_transaction->error = I2C_ERROR_ARBITRATION;
        /* Dequeue another transaction */
        dequeue = true;

    /*** Master Receive States ***/

    /* ACK to [START, Slave Address, Read] received */
    /* ACK sent to received [DATA] */
    } else if (status == 0x40 || status == 0x50) {
        if (status == 0x40) {
            dbg("%s: status=0x%02x, (mrx) ACK to [START, Slave Address, Read] received\n", __func__, status);
        } else if (status == 0x50) {
            dbg("%s: status=0x%02x, (mrx) ACK sent to received [DATA]\n", __func__, status);
        }

        /* Receive the data byte */
        if (status == 0x50 && master->i2c_msg_byte_index < transaction->msgs[master->i2c_msg_index].len) {
            transaction->msgs[master->i2c_msg_index].buf[master->i2c_msg_byte_index] = master->i2c->DAT;
            dbg("%s: received data byte 0x%02x\n", __func__, transaction->msgs[master->i2c_msg_index].buf[master->i2c_msg_byte_index]);
            master->i2c_msg_byte_index++;
        }

        /* If we have one last data byte left to receive */
        if ((master->i2c_msg_byte_index+1) == transaction->msgs[master->i2c_msg_index].len) {
            /* Transmit a NACK to the last data byte */
            master->i2c->CONCLR = I2C_CON_AA;

            dbg("%s: transmitting NACK for last data byte\n", __func__);

        /* If we have more than one data byte left to receive */
        } else if ((master->i2c_msg_byte_index+1) < transaction->msgs[master->i2c_msg_index].len) {
            /* Transmit an ACK for the next data byte */
            master->i2c->CONSET = I2C_CON_AA;

            dbg("%s: transmitting ACK for next data byte\n", __func__);

        /* If we have finished this message, but have messages left to transmit/receive */
        } else if ((master->i2c_msg_index+1) < transaction->count) {
            master->i2c_msg_byte_index = 0;
            master->i2c_msg_index++;

            dbg("%s: transmitting repeated start for new msg_index=%d\n", __func__, master->i2c_msg_index);

            /* Transmit a repeated start condition for our next message */
            master->i2c->CONSET = I2C_CON_STA | I2C_CON_AA;

        /* Otherwise, we're finished with this transaction */
        } else {
            dbg("%s: done with transaction, transmitting stop condition\n", __func__);
            /* Transmit a STOP condition */
            master->i2c->CONSET = I2C_CON_STO | I2C_CON_AA;
            /* Mark the transfer complete and successful */
            master->active_transaction->complete = true;
            master->active_transaction->error = 0;
            /* Dequeue another transaction */
            dequeue = true;
        }

    /* NACK sent to received [DATA] (Last data byte) */
    } else if (status == 0x58) {
        dbg("%s: status=0x%02x: (mrx) NACK sent to received [DATA] (last data byte)\n", __func__, status);

        /* Receive the last data byte */
        if (master->i2c_msg_byte_index < transaction->msgs[master->i2c_msg_index].len) {
            transaction->msgs[master->i2c_msg_index].buf[master->i2c_msg_byte_index] = master->i2c->DAT;
            dbg("%s: received data byte 0x%02x\n", __func__, transaction->msgs[master->i2c_msg_index].buf[master->i2c_msg_byte_index]);
            master->i2c_msg_byte_index++;
        }

        /* If we have another message to transmit */
        if ((master->i2c_msg_index+1) < transaction->count) {
            master->i2c_msg_byte_index = 0;
            master->i2c_msg_index++;

            dbg("%s: transmitting repeated start for new msg_index=%d\n", __func__, master->i2c_msg_index);

            /* Transmit a repeated start condition for our next message */
            master->i2c->CONSET = I2C_CON_STA | I2C_CON_AA;

        /* Otherwise, we're finished with this transaction */
        } else {
            dbg("%s: done with transaction, transmitting stop condition\n", __func__);
            /* Transmit a STOP condition */
            master->i2c->CONSET = I2C_CON_STO | I2C_CON_AA;
            /* Mark the transfer complete and successful */
            master->active_transaction->complete = true;
            master->active_transaction->error = 0;
            /* Dequeue another transaction */
            dequeue = true;
        }

    /* NACK to [START, Slave Address, Read] received (Error) */
    } else if (status == 0x48) {
        dbg("%s: status=0x%02x: (mrx) NACK to [START, Slave Address, Read] received (error)\n", __func__, status);

        /* If this is our first message and first byte, re-attempt the transfer
         * up to I2C_NUM_RETRIES times with a STOP followed by another START */
        if (master->i2c_msg_index == 0 && master->i2c_msg_byte_index == 0 && master->retries > 0) {
            dbg("%s: initial NACK, attempting retry number %d\n", __func__, I2C_NUM_RETRIES-(master->retries-1));
            /* Transmit STOP + START condition */
            master->i2c->CONSET = I2C_CON_STO | I2C_CON_STA | I2C_CON_AA;
            /* Decrement retries */
            master->retries--;

        /* If this is not our first message or not our first byte, e.g. a NACK
         * response to a repeated start condition, or retries have been
         * exhausted, then resort to failing the transaction. */
        } else {
            if (master->i2c_msg_index > 0 || master->i2c_msg_byte_index > 0) {
                dbg("%s: intermediate NACK, failing transaction\n", __func__);
            } else {
                dbg("%s: retries exhausted, failing transaction\n", __func__);
            }
            /* Transmit STOP condition */
            master->i2c->CONSET = I2C_CON_STO | I2C_CON_AA;
            /* Mark the transfer complete and failed */
            master->active_transaction->complete = true;
            master->active_transaction->error = (master->i2c_msg_index > 0 || master->i2c_msg_byte_index > 0) ? I2C_ERROR_NACK : I2C_ERROR_TIMEOUT;
            /* Dequeue another transaction */
            dequeue = true;
        }

    } else {
        dbg("%s: status=0x%02x: unknown status! aborting transaction\n", __func__, status);

        /* Transmit STOP condition */
        master->i2c->CONSET = I2C_CON_STO | I2C_CON_AA;
        /* Mark the transfer complete and failed */
        master->active_transaction->complete = true;
        master->active_transaction->error = I2C_ERROR_UNKNOWN;
        /* Dequeue another transaction */
        dequeue = true;
    }

    /* If we're moving onto the next transaction */
    if (dequeue)
        i2c_master_dequeue(master);

    /* Clear interrupt flag SI */
    master->i2c->CONCLR = I2C_CON_SI;

    dbg("%s: con i2en=%d sta=%d sto=%d si=%d aa=%d\n\n", __func__, !!(master->i2c->CONSET & I2C_CON_I2EN), !!(master->i2c->CONSET & I2C_CON_STA), !!(master->i2c->CONSET & I2C_CON_STO), !!(master->i2c->CONSET & I2C_CON_SI), !!(master->i2c->CONSET & I2C_CON_AA));
}

void I2C_Handler(void) {
    i2c_master_sm(&I2C0);
}

/**********************************************************************/

void i2c_init(void) {
    /* Configure PIO0.4 for SCL, PIO0.5 for SDA */
    LPC_IOCON->PIO0_4 = 0x1;
    LPC_IOCON->PIO0_5 = 0x1;

    /* Deassert I2C0 controller reset */
    LPC_SYSCON->PRESETCTRL |= (1<<1);

    /* Disable I2C0 */
    LPC_I2C->CONCLR = I2C_CON_I2EN;

    /* Set I2C data rate */
    /* I2C Bit Frequency = I2CPCLK / (SCLH+SCLL) */
    /* 400KHz = 48MHz / 120
     * SCLH = 60, SCLL = 60 */
    LPC_I2C->SCLL = 60;
    LPC_I2C->SCLH = 60;

    /* Disable monitor mode */
    LPC_I2C->MMCTRL = 0;

    /* Enable I2C interrupt */
    NVIC_ClearPendingIRQ(I2C_IRQn);
    NVIC_EnableIRQ(I2C_IRQn);

    /* Enable I2C0 controller */
    LPC_I2C->CONSET = I2C_CON_I2EN;

    /* Setup I2C master structure */
    I2C0.i2c = LPC_I2C;
    I2C0.active_transaction = NULL;
    queue_init((struct queue *)&I2C0.transaction_queue, I2C_QUEUE_SIZE);
}

void i2c_start_transfer(struct i2c_transaction *transaction) {
    struct i2c_master *master = transaction->master;

    dbg("%s: queueing transaction address=0x%02x, msgs=%p, num=%d\n", __func__, transaction->address, transaction->msgs, transaction->count);

    /* Reset transaction completion flags */
    transaction->complete = false;
    transaction->error = 0;

    /* Block until we can queue the transaction */
    while (queue_enqueue((struct queue *)&master->transaction_queue, transaction) < 0) {
        dbg("%s: blocking on enqueue\n", __func__);
        __WFI();
    }

    /* If the master is not currently working on a transaction, kick this one off */
    if (master->active_transaction == NULL) {
        dbg("%s: master is not busy, kicking off transaction\n", __func__);
        i2c_master_dequeue(master);
    } else {
        dbg("%s: master is busy, transaction will dequeue later\n", __func__);
    }
}

void i2c_wait_transfer(struct i2c_transaction *transaction) {
    dbg("%s: waiting on transaction address=0x%02x, msgs=%p, num=%d\n", __func__, transaction->address, transaction->msgs, transaction->count);
    while (!transaction->complete) {
        dbg("%s: waiting\n", __func__);
        __WFI();
    }
    dbg("%s: transaction complete with error %d\n", __func__, transaction->error);
}

int i2c_transfer(struct i2c_transaction *transaction) {
    i2c_start_transfer(transaction);
    i2c_wait_transfer(transaction);
    return transaction->error;
}

