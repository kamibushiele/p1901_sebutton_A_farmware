#ifndef SYSTEM_H
#define	SYSTEM_H

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>

#define _XTAL_FREQ 16000000

void systemInit();
void uartPinEnable(bool enable);

#endif	/* SYSTEM_H */

