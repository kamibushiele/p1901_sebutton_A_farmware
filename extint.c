#include "extint.h"
static void(*extint_ISRCallBack)(void);

void extint_Init(void(*isrCallback)(void)) {
    extint_ISRCallBack = isrCallback;
}

void extint_Interrupt() {
    extint_ISRCallBack();
}