#ifndef I2C_NO_ISR_H
#define	I2C_NO_ISR_H

#include "system.h"

void i2cInit();
void i2cStartSequence();
void i2cRestartSequence();
void i2cStartSequenceWaitACK(bool restart, uint8_t add, bool read);
void i2cStopSequence();
void i2cReadSequence();
bool i2cSend(uint8_t data);
void i2cSendACK(uint8_t ack);
void i2cDataWrite(uint8_t add, uint8_t *writeData, int writeDataLen);
void i2cDataRead(uint8_t add, uint8_t *readData, int readDataLen);
void i2cDataWriteRead(uint8_t add, uint8_t *writeData, int writeDataLen, uint8_t *readData, int readDataLen);
void i2cACKPool(uint8_t add);

#endif	/* I2C_NO_ISR_H */

