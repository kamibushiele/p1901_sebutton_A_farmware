#ifndef I2C_H
#define	I2C_H

#include "system.h"

void (*i2c_Init(void(*sequenceEndCallBack)(), void(*sReadStopCallBack)()))(bool);
void i2c_WriteStart(uint8_t add, uint8_t *writeBuffer, int writeLength);
void i2c_ReadStart(uint8_t add, uint8_t *readBuffer, int readLength);
void i2c_WriteReadStart(uint8_t add, uint8_t *writeBuffer, int writeLength, uint8_t *readBuffer, int readLength);
void i2c_SReadStart(uint8_t add, uint8_t *readBuffer, int readLength);
void i2c_SWriteReadStart(uint8_t add, uint8_t *writeBuffer, int writeLength, uint8_t *readBuffer, int readLength);
void i2c_InLoop();
void i2c_Interrupt();

#endif	/* I2C_H */

