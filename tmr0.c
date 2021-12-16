#include "tmr0.h"

static void(*tmr0ISRCallBack)(void);

void tmr0_Init(void(*isrCallback)(void)) {
    tmr0ISRCallBack = isrCallback;
    //8kHz
    T0CON0 = 0b00000000; //8bit mode, post 1:1
//    //_XTAL_FREQ = 16M
//    T0CON1 = 0b01010001; //pre 1:2
    //_XTAL_FREQ = 32M
    T0CON1 = 0b01010010; //pre 1:4
    TMR0H = 250;
    TMR0L = 0;
}
void tmr0_Start(){
    T0CON0bits.T0EN = 1;
    TMR0L = 0;
}
void tmr0_Stop(){
    T0CON0bits.T0EN = 0;
}

void tmr0_Interrupt() {
    tmr0ISRCallBack();
}