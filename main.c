#include "system.h"
#include "pwm.h"
#include "tmr0.h"
#include "uart.h"
#include "string_compare.h"
#include "eeprom.h"
#include "i2c.h"
#include "extint.h"
#include <stdio.h>

#define CHUNK_MAX_LEN_DEF PAGE_LEN_DEF
static const uint8_t BufferLen = CHUNK_MAX_LEN_DEF;
static const uint8_t PCMHeaderLen = 3;
static const uint8_t PCMBlockHeaderLen = 5;

typedef enum {
    Dump_Start,
    Dump_Wait,
    Dump_Read,
    Dump_Dump,
    ReadHeader,//EEPROMのヘッダを読む
    ReadHeaderWait,//EEPROMのヘッダ返答待ち
    ReadBlockInfo,
    ReadBlockInfoWait,
    WaitHeader,//UART書き込み待ち
    ReadChunk,
    WaitEEPROMWriteEnd,
    WaitEEPROMWriteEndLast,
    WriteFooter,
    End,
    PlayReady,
    WaitPlayButton,
    Play,
} state_t;

static state_t state;
static uint8_t pcmValue;
static bool eepromSequencteEndFlug;
static bool eepromSReadStopFlug;
static bool intFlug;//ボタンが押されたらtrue
static bool playSoundEnFlug;
static uint16_t tmr0cnt;
static void(*eepromSReadContinue)(bool);

static void sleep();//スリープにする
static void wakeup();//スリープから解除されたら呼び出す
static void privateINTISR();//外部割り込みハンドラ関数
static void privateTMR0ISR();//TMR0割り込みハンドラ関数
static void swapBuffer(uint8_t **buffer, uint8_t *buffer1, uint8_t *buffer2);
static void eepromSequencteEndCallBack();
static void eepromSReadStopCallBack();

void main(void) {

    typedef struct {
        int startAdd;
        int length;
        uint8_t weight;
    } pcmBlock_t;
    uint8_t buffer[CHUNK_MAX_LEN_DEF] = {};
    uint8_t bufferCursor = 0;
    int eepromCursor = 0;
    int pcmBlockCursor = 0;
    int pcmBlockSize = 0;

    pcmBlock_t pcmBlock[3] = {};
    uint8_t pcmNofBlock = 0;
    uint8_t pcmSound = 0;

    int i;
    char string[30];

    __delay_ms(500);
    systemInit();
    tmr0_Init(privateTMR0ISR);
    tmr0cnt = 0;
    tmr0_Start();
    pwm_Init();
    extint_Init(privateINTISR);
    eepromSReadContinue = eeprom_Init(0x50, eepromSequencteEndCallBack, eepromSReadStopCallBack);
    uartInit();
        state = ReadHeader;
    //                state = WaitHeader;
//    state = Dump_Start;

    //    pwmSetDuty(0x80);
    //    __delay_ms(5);
    //        uartWriteStr("_start\n");

    //    bool out;
    //    while (1) {
    //        if (tmr0Flag) {
    //            tmr0Flag = false;
    //            if (out) {
    //                pwmSetDuty(256);
    //            } else {
    //                pwmSetDuty(0);
    //            }
    //            out = !out;
    //        }
    //    }
    while (1) {
        LATA2 = 0;
        eeprom_InLoop();
        LATA2 = 1;
        switch (state) {
            case Play:
                break;

            case ReadHeader:
                eepromCursor = 0;
                eeprom_Read(&eepromCursor, buffer, PCMHeaderLen);
                eepromSequencteEndFlug = false;
                state = ReadHeaderWait;
                //breakなし
            case ReadHeaderWait:
                if (eepromSequencteEndFlug) {
                    if (strBufComp(buffer, "PCM")) {
                        state = ReadBlockInfo;
                    } else {
                        state = WaitHeader;
#ifdef _DEBUG
                        state = WaitHeader;
#endif
                    }
                }
                break;
            case ReadBlockInfo:
                eeprom_Read(&eepromCursor, buffer, PCMBlockHeaderLen);
                eepromSequencteEndFlug = false;
                state = ReadBlockInfoWait;
                //breakなし
            case ReadBlockInfoWait:
                if (eepromSequencteEndFlug) {
                    pcmBlockSize = buffer[0] | (buffer[1] << 8);
                    if (pcmBlockSize != 0) {
                        pcmBlock[pcmNofBlock].startAdd = eepromCursor - PCMBlockHeaderLen;
                        pcmBlock[pcmNofBlock].length = pcmBlockSize;
                        pcmBlock[pcmNofBlock].weight = buffer[4];
                        pcmNofBlock++;
                        state = PlayReady;
                    } else {
                        state = End;
                    }
                }
                break;
            case PlayReady:
                playSoundEnFlug = false;
                pwm_On(false);
                state = WaitPlayButton;
                intFlug = false;
                sleep();
                break;
            case WaitPlayButton:
                if (intFlug) {
                    eepromCursor = pcmBlock[0].startAdd + PCMBlockHeaderLen;
                    eeprom_SRead(&eepromCursor, &pcmValue, pcmBlock[0].length - PCMBlockHeaderLen);
                    eepromSequencteEndFlug = false;
                    playSoundEnFlug = true;
                    pwm_On(true);
                    state = Play;
                }
                break;

            case WaitHeader:
                if (uartRXIntFlug) {
                    uartRXIntFlug = false;
                    buffer[bufferCursor] = uartRXData;
                    if (strLoopBufComp(buffer, BufferLen, bufferCursor, "PCM", 3)) {
                        uartWrite('K');
                        buffer[0] = 'P';
                        buffer[1] = 'C';
                        buffer[2] = 'M';
                        eeprom_SetCursor(0);
                        eeprom_Write(buffer, 3, true);
                        eepromSequencteEndFlug = false;
                        bufferCursor = 0;
                        pcmBlockCursor = 0;
                        state = ReadChunk;
                    } else {
                        bufferCursor++;
                        if (bufferCursor >= BufferLen) {
                            bufferCursor = 0;
                        }
                    }
                }
                break;
            case ReadChunk:
                if (uartRXIntFlug) {
                    uartRXIntFlug = false;
                    buffer[bufferCursor] = uartRXData;
                    bufferCursor++;
                    pcmBlockCursor++;
                    if (pcmBlockCursor == 4) {
                        pcmBlockSize = buffer[0];
                        pcmBlockSize |= buffer[1] << 8;
                        pcmBlockSize |= buffer[2] << 8;
                        pcmBlockSize |= buffer[3] << 8;
                        if (pcmBlockSize == 0) {
                            state = WriteFooter;
                        }
                    } else if (pcmBlockCursor > 4) {
                        if (pcmBlockCursor >= pcmBlockSize || bufferCursor >= eeprom_PageLen) {//ブロック終了orチャンク終了
                            state = WaitEEPROMWriteEnd;
                        }
                    }
                }
                break;
            case WaitEEPROMWriteEnd:
                if (eepromSequencteEndFlug) {
                    eeprom_Write(buffer, bufferCursor, true);
                    eepromSequencteEndFlug = false;
                    bufferCursor = 0;
                    if (pcmBlockCursor >= pcmBlockSize) {//ブロック終了
                        pcmBlockCursor = 0;
                        state = WaitEEPROMWriteEndLast;
                    } else {
                        uartWrite('K');
                        state = ReadChunk;
                    }
                }
                break;
            case WaitEEPROMWriteEndLast:
                if (eepromSequencteEndFlug) {
                    uartWrite('K');
                    state = ReadChunk;
                }
                break;
            case WriteFooter:
                if (eepromSequencteEndFlug) {
                    //フッター \0\0\0\0
                    eeprom_Write(buffer, 4, true);
                    eepromSequencteEndFlug = false;
                    //                    eeprom_Write(&eepromCursor, buffer - eeprom_AddLen, 4);
                    uartWrite('K');
                    state = ReadHeader;
                }
                break;
            case End:
                break;
            case Dump_Start:
                //                eepromRead(0, buffer, 64);
                if (intFlug) {
                    eeprom_SRead(0, buffer, 1000);
                    state = Dump_Read;
                }
            case Dump_Read:
                if (eepromSReadStopFlug) {
                    eepromSReadStopFlug = false;
                    eepromSReadContinue(false);
                    uartWrite(buffer[0]);
                    //                    sprintf(string, "%02x\n", buffer[0]);
                    //                    uartWriteStr(string);
                } else if (eepromSequencteEndFlug) {
                    intFlug = false;
                    state = Dump_Start;
                }
                break;
        }
    }
}
//    while (1) {
//        if (uartRXIntFlug) {
//            read_data = uartRXData;
//            uartRXIntFlug = false;
//            if (read_data == 'R') {
//                data[0] = 0;
//                data[1] = 0;
//                i2cDataWriteRead(add, data, 2, &buf, 1);
//                uartWrite(buf);
//                for (i = 1; i < dataLen; i++) {
//                    i2cDataRead(add, &buf, 1);
//                    uartWrite(buf);
//                }
//            } else {
//                LATA2 = 1;
//                data[0] = dataLen >> 8;
//                data[1] = dataLen & 0xFF;
//                data[2] = read_data;
//                i2cDataWrite(add, data, 3);
//                dataLen++;
//                i2cACKPool(add);
//                LATA2 = 0;
//            }
//        }
//    }
//}

static void sleep() {
    TMR0IE = 0;
    SLEEP();
}
static void wakeup() {
    TMR0IE = 1;
}
static void privateTMR0ISR() {
    if (playSoundEnFlug) {
        eepromSReadContinue(false);
        pwm_SetDuty(pcmValue);
    }
    //        LATA2 = !RA2;
    if (tmr0cnt < 0xffff) {
        tmr0cnt++;
    }
}

static void privateINTISR() {
    wakeup();
//    if (tmr0cnt >= 1600) {
        tmr0cnt = 0;
        intFlug = true;
        //        if (state == Play) {
        //            state = PlayReady;
        //        }
//    }
}

static void eepromSequencteEndCallBack() {
    eepromSequencteEndFlug = true;
    if (state == Play) {
        state = PlayReady;
    }
}

static void eepromSReadStopCallBack() {
    eepromSReadStopFlug = true;
}