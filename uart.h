#ifndef UART_H
#define	UART_H

#include "system.h"

extern bool uartRXIntFlug;
extern bool uartTXIntFlug;
extern uint8_t uartRXData;

void uartInit();
void uartWrite(uint8_t data);
void uartWriteStr(const char *data);
void uartRXInterrupt();
void uartTXInterrupt();

#endif	/* UART_H */

