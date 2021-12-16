#ifndef EEPROM
#define	EEPROM

#include "system.h"

#define PAGE_LEN_DEF 64
#define EEPROM_ADDLEN_DEF 2

extern void (* eeprom_InLoop)();
extern const uint8_t eeprom_PageLen;
extern const uint8_t eeprom_AddLen;
void (*eeprom_Init(uint8_t chipAdd, void (*sequencteEndFlug)(), void(*sReadStopCallBack)()))(bool);
//void eeprom_InLoop();
bool eeprom_Write(uint8_t data[], int dataLen, bool forceWrite);
void eeprom_Read(int *address, uint8_t *data, int dataLen);
void eeprom_SRead(int *address, uint8_t *data, int dataLen);
void eeprom_SetCursor(int address);

#endif	/* EEPROM */

