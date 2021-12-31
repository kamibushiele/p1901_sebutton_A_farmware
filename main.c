#include "system.h"
#include "pwm.h"
#include "tmr0.h"
#include "uart.h"
#include "string_compare.h"
#include "eeprom.h"
#include "i2c.h"
#include "extint.h"
#include <stdio.h>

#define BLOCK_MAX_LEN_DEF PAGE_LEN_DEF
#define MAX_NUM_SOUNDS 3
static const uint8_t BUFFER_LEN = BLOCK_MAX_LEN_DEF;
static const uint8_t PCM_HEADER_LEN = 3;
static const uint8_t PCM_SND_NUM_LEN = 1;
static const char * PCM_HEADER = "PCM";
static const uint8_t PCM_SND_INFO_LEN = 3;

static const char * PCMWritePreamble = "FLASH";
static const uint8_t PCMWritePreambleLen = 5;

typedef enum {
    Dump_Start,
    Dump_Read,
    ReadHeader,//EEPROMのヘッダとSoundの数を読む
    ReadHeaderWait,//EEPROMのヘッダとSoundの数の返答待ち
    ReadSoundInfoWait,//EEPROMのSound情報返答待ち
    PlayReady,
    WaitPlayButton,
    Play,
    WaitPreamble,//UART書き込み待ち
    WaitBinSize, //UART書き込みサイズ待ち
    WaitBlock,  //UARTブロック待ち
    WaitEEPROMWriteEnd, //EEPROM書き込み完了待ち
    WaitEEPROMWriteEndLast, //最後のEEPROM書き込み完了待ち
    End,
} state_t;

static state_t state;
static uint8_t pcmValue;//読み込んだ1サンプルの振幅
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
    uint8_t buffer[BLOCK_MAX_LEN_DEF];//多目的バッファ
    uint8_t bufferCursor = 0;//多目的バッファのカーソル
    int eepromCursor = 0;//EEPROM書き込み/読み込みのアドレスカーソル
    uint32_t binSize = 0;//バイナリ全体のサイズ

    pcmBlock_t pcmSounds[MAX_NUM_SOUNDS];
    uint8_t pcmNofSounds = 0;
    uint8_t pcmSound = 0;   //再生するSound

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
    // state = WaitPreamble;
    // state = Dump_Start;

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
        #if _DEBUG
        LATA2 = 0;
        #endif
        eeprom_InLoop();
        #if _DEBUG
        LATA2 = 1;
        #endif
        switch (state) {
            ///////////////////再生////////////////////////
            case Play://動作軽減のためなるべくmain loopでは処理しない
                break;

            case ReadHeader:
                eepromCursor = 0;
                eeprom_Read(&eepromCursor, buffer, PCM_HEADER_LEN + PCM_SND_NUM_LEN);
                eepromSequencteEndFlug = false;
                state = ReadHeaderWait;
                //breakなし
            case ReadHeaderWait:
                if (eepromSequencteEndFlug) {
                    eepromSequencteEndFlug = false;
                    if (!strBufComp(buffer, PCM_HEADER)) {
                        state = End;//不正なヘッダ
                        break;
                    }
                    pcmNofSounds = buffer[PCM_HEADER_LEN + 0];//soundの数
                    if(pcmNofSounds > MAX_NUM_SOUNDS){
                        state = End;//不正な数のサウンド
                        break;
                    }
                    eeprom_Read(&eepromCursor, buffer, PCM_SND_INFO_LEN*pcmNofSounds);
                    state = ReadSoundInfoWait;
                }
                break;
            case ReadSoundInfoWait:
                if (eepromSequencteEndFlug) {
                    eepromSequencteEndFlug = false;
                    int DataOffset = PCM_HEADER_LEN + PCM_SND_NUM_LEN + PCM_SND_INFO_LEN*pcmNofSounds;
                    for(i = 0;i<pcmNofSounds;i++){
                        int infoOffset = PCM_SND_INFO_LEN*i;
                        pcmSounds[i].startAdd = DataOffset;
                        pcmSounds[i].length = buffer[infoOffset] | (buffer[infoOffset+1] << 8);
                        pcmSounds[i].weight = buffer[infoOffset+2];
                        DataOffset += pcmSounds[i].length;
                    }
                    pcmSound = pcmNofSounds;
                    state = PlayReady;
                    // binSize = buffer[0] | (buffer[1] << 8);
                    // if (binSize != 0) {
                    //     pcmSounds[pcmNofSounds].startAdd = eepromCursor - PCM_SND_INFO_LEN;
                    //     pcmSounds[pcmNofSounds].length = binSize;
                    //     pcmSounds[pcmNofSounds].weight = buffer[4];
                    //     pcmNofSounds++;
                    //     state = PlayReady;
                    // } else {
                    //     state = End;
                    // }
                }
                break;
            case PlayReady:
                pcmSound ++;
                if(pcmSound >= pcmNofSounds){
                    pcmSound = 0;
                }
                playSoundEnFlug = false;
                intFlug = false;
                pwm_On(false);
                state = WaitPlayButton;
                sleep();
                break;
            case WaitPlayButton:
                if (intFlug) {
                    if(PORTCbits.RC2 == 1){
                        state = WaitPreamble;
                        break;
                    }
                    eepromCursor = pcmSounds[pcmSound].startAdd;
                    eeprom_SRead(&eepromCursor, &pcmValue, pcmSounds[pcmSound].length);
                    eepromSequencteEndFlug = false;
                    playSoundEnFlug = true;
                    pwm_On(true);
                    state = Play;
                }
                break;
            ///////////////////書き込み////////////////////////
            case WaitPreamble:
                if (uartRXIntFlug) {
                    uartRXIntFlug = false;
                    buffer[bufferCursor] = uartRXData;
                    if (strLoopBufComp(buffer, BUFFER_LEN, bufferCursor, PCMWritePreamble, PCMWritePreambleLen)) {
                        uartWrite('K');
                        bufferCursor = 0;
                        state = WaitBinSize;
                    } else {
                        bufferCursor++;
                        if (bufferCursor >= BUFFER_LEN) {
                            bufferCursor = 0;
                        }
                    }
                }
                break;
            case WaitBinSize:
                if (uartRXIntFlug) {
                    uartRXIntFlug = false;
                    buffer[bufferCursor] = uartRXData;
                    bufferCursor++;
                    if (bufferCursor >= 4) {
                        binSize = buffer[0];
                        binSize |= buffer[1] << 8;
                        binSize |= buffer[2] << 16;
                        binSize |= buffer[3] << 32;
                        if(binSize > EEPROM_MAX_SIZE){
                            uartWrite('E');
                            state = WaitPreamble;
                            break;
                        }
                        uartWrite('K');
                        state = WaitBlock;
                        bufferCursor = 0;
                        eepromCursor = 0;
                        eeprom_SetCursor(0);
                        eepromSequencteEndFlug = true;//書き込んでいないので次のWriteは無条件で開始
                    }
                }
                break;
            case WaitBlock:
            if (uartRXIntFlug) {
                uartRXIntFlug = false;
                buffer[bufferCursor] = uartRXData;
                bufferCursor++;
                eepromCursor++;
                if (eepromCursor >= binSize || bufferCursor >= eeprom_PageLen){//バイナリ終了 or ブロック終了
                    state = WaitEEPROMWriteEnd;
                }
            }
            break;
            case WaitEEPROMWriteEnd:
                if (eepromSequencteEndFlug) {
                    eeprom_Write(buffer, bufferCursor, true);
                    eepromSequencteEndFlug = false;
                    bufferCursor = 0;
                    if (eepromCursor >= binSize) {//バイナリ終了
                        state = WaitEEPROMWriteEndLast;
                    } else {
                        uartWrite('K');
                        state = WaitBlock;
                    }
                }
                break;
            case WaitEEPROMWriteEndLast:
                if (eepromSequencteEndFlug) {
                    uartWrite('K');
                    state = ReadHeader;
                }
                break;
            case End:
                break;
            //////////////////////////ダンプ//////////////////////////////////////////
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