#include "adc.h"

void adc_Init(){
    ADCON0 = 0b00001000;//channel=ANA2,Disable
    ADCON1 = 0b11100000;//right,FOSC/64
}
int adc_Get(){
    ADCON0bits.ADON = 1;
    ADIF = 0;
    ADCON0bits.ADGO = 1;
    while(!ADIF);
    ADCON0bits.ADON = 0;
    return ADRESL |ADRESH<<8;
}
