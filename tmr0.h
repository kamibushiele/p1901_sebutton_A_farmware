#ifndef TMR0_H
#define	TMR0_H

#include "system.h"

void tmr0_Init(void(*isrCallback)(void));
void tmr0_Start();
void tmr0_Stop();
void tmr0_Interrupt();

#endif	/* TMR0_H */

