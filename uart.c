
#include "uart.h"

bool uartRXIntFlag;
bool uartTXIntFlag;
uint8_t uartRXData;

void uartInit() {
    //_XTAL_FREQ = 16M
    TX1STA = 0b10100100; //master, 8bit, Asynchronous, HighSpeed
    RC1STA = 0b10010000; //8bit
    BAUD1CON = 0b00001010; //16bit-BaudRate, 
//    SP1BRGL = 68; //115200bps(32E6/(4*(SP1BRG+1)))
    SP1BRGL = 138; //57600bps(32E6/(4*(SP1BRG+1)))
//    SP1BRGL = 51; //19200bps(16E6/(16*(SP1BRG+1)))
    SP1BRGH = 0;
    uartRXIntFlag = false;
    RCIF = 0;
}

void uartWrite(uint8_t data) {
    TXIF = 0;
    TX1REG = data;
    while (!TXIF);
}

void uartWriteStr(const char *data) {
    int i;
    for(i=0;data[i]!='\0';i++){
        uartWrite(data[i]);
    }
}

void uartRXInterrupt() {
    uartRXData = RC1REG;
//    TX1REG = uartRXData;
    uartRXIntFlag = true;
}
void esuartTXInterrupt(){
//    uartTXIntFlag = true;
}