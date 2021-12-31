
#include "eeprom.h"
#include "i2c.h"

static uint8_t eepromi2cadd;
static uint8_t buffer1[PAGE_LEN_DEF + EEPROM_ADDLEN_DEF];
static uint8_t buffer2[PAGE_LEN_DEF + EEPROM_ADDLEN_DEF]; //(64+2)*2配列にすると容量オーバーでビルドエラー
static uint8_t * buffer;
static int eepromCursor;
static int bufferCursor;

void (* eeprom_InLoop)();
const uint8_t eeprom_PageLen = PAGE_LEN_DEF;
const uint8_t eeprom_AddLen = EEPROM_ADDLEN_DEF;

static void swapBuffer(uint8_t **buffer, uint8_t *buffer1, uint8_t * buffer2) {
    static int nowBuffer = 0;
    if (nowBuffer == 0) {
        *buffer = buffer1 + EEPROM_ADDLEN_DEF;
        nowBuffer = 1;
    } else {
        *buffer = buffer2 + EEPROM_ADDLEN_DEF;
        nowBuffer = 0;
    }
}

void (*eeprom_Init(uint8_t chipAdd, void (*sequencteEndFlug)(), void(*sReadStopCallBack)()))(bool) {
    eeprom_InLoop = i2c_InLoop;
    eepromi2cadd = chipAdd;
    eepromCursor = 0;
    bufferCursor = 0;
    swapBuffer(&buffer, buffer1, buffer2);
    return i2c_Init(sequencteEndFlug, sReadStopCallBack);
}

void eeprom_SetCursor(int address){
    eepromCursor = address;
}

//void eeprom_InLoop() {
//    i2c_InLoop();
//}

bool eeprom_Write(uint8_t data[], int dataLen, bool forceWrite) {
    int i;
    bool write = false;
    int startAdd = eepromCursor - bufferCursor;
    int writeLen;
    uint8_t* writeBuffer = false;
    for (i = 0; i < dataLen; i++) {
        buffer[bufferCursor] = data[i];
        eepromCursor++;
        bufferCursor++;
        if (eepromCursor % eeprom_PageLen == 0) {//ページ境界に到達
            writeLen = bufferCursor + eeprom_AddLen;
            writeBuffer = buffer - eeprom_AddLen;
            swapBuffer(&buffer, buffer1, buffer2);
            write = true;
            bufferCursor = 0;
        }
    }
    if (!write && forceWrite) {//ページ境界に到達してないけど書き込む
        writeLen = bufferCursor + eeprom_AddLen;
        writeBuffer = buffer - eeprom_AddLen;
        write = true;
        bufferCursor = 0;
    }
    if (write) {
        writeBuffer[0] = (startAdd >> 8)&0xFF;
        writeBuffer[1] = startAdd & 0xFF;
        i2c_WriteStart(eepromi2cadd, writeBuffer, writeLen);
    }
    return write;
}

void eeprom_Read(int *address, uint8_t *data, int dataLen) {
    uint8_t addressBlock[2];
    addressBlock[0] = (*address >> 8)&0xFF;
    addressBlock[1] = *address & 0xFF;
    i2c_WriteReadStart(eepromi2cadd, addressBlock, 2, data, dataLen);
    *address += dataLen;
}

void eeprom_SRead(int *address, uint8_t *data, int dataLen) {
    uint8_t addressBlock[2];
    addressBlock[0] = (*address >> 8)&0xFF;
    addressBlock[1] = *address & 0xFF;
    i2c_SWriteReadStart(eepromi2cadd, addressBlock, 2, data, dataLen);
    *address += dataLen;
}