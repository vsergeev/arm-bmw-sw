#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <uart.h>
#include <test.h>
#include <i2c.h>

void test_i2c(void) {
    uint8_t buf1[2], buf2[2], buf3[2], buf4[2];
    struct i2c_msg msgs[4];
    struct i2c_transaction transaction, transaction2, transaction3;

    ptest();

    /* Test transfer to non-existent slave address, should timeout. */
    msgs[0].buf = NULL;
    msgs[0].len = 0;
    msgs[0].read = false;
    transaction.master = &I2C0;
    transaction.address = 0x70;
    transaction.msgs = msgs;
    transaction.count = 1;
    debug_printf(STR_TAB "Testing transfer to non-existent slave address\n");
    debug_printf(STR_TAB "Press enter to start transfer...");
    uart_getc(); uart_putc('\n');
    do {
        passert(i2c_transfer(&transaction) == I2C_ERROR_TIMEOUT);
        pinteract("I2C transaction start occurred?");
    } while(1);

    /* Test transfer to existent slave address, should succeed. */
    /* Don't write any data, though. */
    msgs[0].buf = NULL;
    msgs[0].len = 0;
    msgs[0].read = false;
    transaction.master = &I2C0;
    transaction.address = 0x20;
    transaction.msgs = msgs;
    transaction.count = 1;
    debug_printf(STR_TAB "Testing transfer to existent slave address, 0 length\n");
    debug_printf(STR_TAB "Press enter to start transfer...");
    uart_getc(); uart_putc('\n');
    do {
        passert(i2c_transfer(&transaction) == 0);
        pinteract("I2C transaction occured?");
    } while(1);

    /* Test three queued transactions */
    msgs[0].buf = NULL;
    msgs[0].len = 0;
    msgs[0].read = false;
    transaction.master = &I2C0;
    transaction.address = 0x70;
    transaction.msgs = msgs;
    transaction.count = 1;
    transaction2.master = &I2C0;
    transaction2.address = 0x20;
    transaction2.msgs = msgs;
    transaction2.count = 1;
    transaction3.master = &I2C0;
    transaction3.address = 0x70;
    transaction3.msgs = msgs;
    transaction3.count = 1;
    debug_printf(STR_TAB "Testing queued transfers to non-existent slave\n");
    debug_printf(STR_TAB "Press enter to start transfer...");
    uart_getc(); uart_putc('\n');
    do {
        i2c_start_transaction(&transaction);
        i2c_start_transaction(&transaction2);
        i2c_start_transaction(&transaction3);
        /* All three transactions should be done by transaction 3 done */
        i2c_wait_transaction(&transaction3);
        passert(transaction.complete == true);
        passert(transaction2.complete == true);
        passert(transaction3.complete == true);
        passert(transaction.error == I2C_ERROR_TIMEOUT);
        passert(transaction2.error == 0);
        passert(transaction3.error == I2C_ERROR_TIMEOUT);
        pinteract("Three I2C transaction starts occurred?");
    } while(1);

    /* Setup transaction for rest of tests */
    transaction.master = &I2C0;
    transaction.address = 0x20;
    transaction.msgs = msgs;

    debug_printf(STR_TAB "Testing single write to I/O expander (i2c address 0x20)\n");
    debug_printf(STR_TAB "Press enter to start transfer...");
    uart_getc(); uart_putc('\n');
    /* Test single write: 0x02 = 0xaa */
    buf1[0] = 0x02;
    buf1[1] = 0xaa;
    msgs[0].buf = buf1;
    msgs[0].len = 2;
    msgs[0].read = false;
    transaction.count = 1;
    passert(i2c_transfer(&transaction) == 0);

    debug_printf(STR_TAB "Testing single read to I/O expander (i2c address 0x20)\n");
    debug_printf(STR_TAB "Press enter to start transfer...");
    uart_getc(); uart_putc('\n');
    /* Test single read: 0x02 */
    buf1[0] = 0x02;
    buf2[0] = 0xff;
    msgs[0].buf = buf1;
    msgs[0].len = 1;
    msgs[0].read = false;
    msgs[1].buf = buf2;
    msgs[1].len = 1;
    msgs[1].read = true;
    transaction.count = 2;
    passert(i2c_transfer(&transaction) == 0);
    passert(buf2[0] == 0xaa);

    debug_printf(STR_TAB "Testing double write to I/O expander (i2c address 0x20)\n");
    debug_printf(STR_TAB "Press enter to start transfer...");
    uart_getc(); uart_putc('\n');
    /* Test double write */
    buf1[0] = 0x02;
    buf1[1] = 0xbb;
    buf2[0] = 0x03;
    buf2[1] = 0xcc;
    msgs[0].buf = buf1;
    msgs[0].len = 2;
    msgs[0].read = false;
    msgs[1].buf = buf2;
    msgs[1].len = 2;
    msgs[1].read = false;
    transaction.count = 2;
    passert(i2c_transfer(&transaction) == 0);

    debug_printf(STR_TAB "Testing double read to I/O expander (i2c address 0x20)\n");
    debug_printf(STR_TAB "Press enter to start transfer...");
    uart_getc(); uart_putc('\n');
    /* Test double read */
    buf1[0] = 0x02;
    buf2[0] = 0xff;
    buf3[0] = 0x03;
    buf4[0] = 0xff;
    msgs[0].buf = buf1;
    msgs[0].len = 1;
    msgs[0].read = false;
    msgs[1].buf = buf2;
    msgs[1].len = 1;
    msgs[1].read = true;
    msgs[2].buf = buf3;
    msgs[2].len = 1;
    msgs[2].read = false;
    msgs[3].buf = buf4;
    msgs[3].len = 1;
    msgs[3].read = true;
    transaction.count = 4;
    passert(i2c_transfer(&transaction) == 0);
    passert(buf2[0] = 0xbb);
    passert(buf4[0] = 0xcc);

    debug_printf(STR_TAB "Testing single read to I/O expander (i2c address 0x20)\n");
    debug_printf(STR_TAB "Press enter to start transfer...");
    uart_getc(); uart_putc('\n');
    /* Test single read */
    buf1[0] = 0x02;
    buf2[0] = 0xff;
    msgs[0].buf = buf1;
    msgs[0].len = 1;
    msgs[0].read = false;
    msgs[1].buf = buf2;
    msgs[1].len = 1;
    msgs[1].read = true;
    transaction.count = 2;
    passert(i2c_transfer(&transaction) == 0);
    passert(buf2[0] == 0xbb);

    pokay("All tests passed!");
}

