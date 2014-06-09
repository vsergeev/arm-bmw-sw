#ifndef _LOCK_H

#include <system/lpc11xx/LPC11xx.h>

/* Start with system-wide locks */
#define system_lock() __disable_irq()
#define system_unlock() __enable_irq()

#endif

