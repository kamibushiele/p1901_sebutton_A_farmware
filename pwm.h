#ifndef PWM_H
#define	PWM_H

#include "system.h"
#define PWMMODE 1
#define CLC_INVERT 1

void pwm_Init();
void pwm_SetDuty(uint16_t duty);
void pwm_On(bool on);

#endif
