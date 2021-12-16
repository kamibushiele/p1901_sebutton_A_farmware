#include "i2c_no_isr.h"

//SSP1STATbits.BF       //送信中は1
//SSP1CON1bits.WCOL     //送信中にBUFに書き込むと1、手動でクリアする。
//SSPCON2bits.ACKSTAT   //ACK=0が返ったら0

//SSP1CON2bits.ACKDT    //返すACK設定
//SSP1CON2bits.ACKEN    //ACKを送る
void i2cInit() {
    SSP1ADD = 19; //400kHz
    SSP1STAT = 0x00;
    SSP1CON1 = 0b0101000;
}

void i2cStartSequence() {
    SSP1IF = 0;
    SSP1CON2bits.SEN = 1;
    while (!SSP1IF);
}

void i2cRestartSequence() {
    SSP1IF = 0;
    SSP1CON2bits.RSEN = 1;
    while (!SSP1IF);
}

void i2cStartSequenceWaitACK(bool restart, uint8_t add, bool read) {
    bool ack;
    uint8_t addBlock = add << 1;
    if (read) {
        addBlock |= 1;
    }
    do {
        if (restart) {
            i2cRestartSequence();
        } else {
            i2cStartSequence();
        }
        ack = i2cSend(addBlock);
    } while (ack);
}

void i2cStopSequence() {
    SSP1IF = 0;
    SSP1CON2bits.PEN = 1;
    while (!SSP1IF);
}

void i2cReadSequence() {
    SSP1IF = 0;
    SSP1CON2bits.RCEN = 1;
    while (!SSP1IF);
}

bool i2cSend(uint8_t data) {
    SSP1IF = 0;
    SSP1BUF = data; //自動で送信開始
    while (!SSP1IF); //送信終了待ち
    return SSP1CON2bits.ACKSTAT; //ACK確認
}

void i2cSendACK(uint8_t ack) {
    SSP1IF = 0;
    SSP1CON2bits.ACKDT = ack;
    SSP1CON2bits.ACKEN = 1;
    while (!SSP1IF); //ACK送信確認
}

void i2cDataWrite(uint8_t add, uint8_t *writeData, int writeDataLen) {
    int i;
    i2cStartSequenceWaitACK(false, add, false);
    for (i = 0; i < writeDataLen; i++) {
        i2cSend(writeData[i]);
    }
    i2cStopSequence();
}

void i2cDataRead(uint8_t add, uint8_t *readData, int readDataLen) {
    int i;
    i2cStartSequenceWaitACK(false, add, true);
    for (i = 0; i < readDataLen; i++) {
        i2cReadSequence();
        readData[i] = SSP1BUF;
        if (i < readDataLen - 1) {
            i2cSendACK(0);
        } else {
            i2cSendACK(1); //最後はAKC=1を送る
        }
    }
    i2cStopSequence();
}

void i2cDataWriteRead(uint8_t add, uint8_t *writeData, int writeDataLen, uint8_t *readData, int readDataLen) {
    int i;
    i2cStartSequenceWaitACK(false, add, false);
    for (i = 0; i < writeDataLen; i++) {
        i2cSend(writeData[i]);
    }
    i2cStartSequenceWaitACK(true, add, true);
    for (i = 0; i < readDataLen; i++) {
        i2cReadSequence();
        readData[i] = SSP1BUF;
        if (i < readDataLen - 1) {
            i2cSendACK(0);
        } else {
            i2cSendACK(1); //最後はAKC=1を送る
        }
    }
    i2cStopSequence();
}

void i2cACKPool(uint8_t add) {
    i2cStartSequenceWaitACK(false, add, false);
    i2cStopSequence();
}