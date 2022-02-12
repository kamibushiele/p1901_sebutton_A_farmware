#include "system.h"
#include "pwm.h"
#include "tmr0.h"
#include "uart.h"
#include "string_compare.h"
#include "eeprom.h"
#include "i2c.h"
#include "extint.h"
#include "adc.h"
#include <stdio.h>

#define DUMP_MODE 0

#define BLOCK_MAX_LEN_DEF PAGE_LEN_DEF
#define MAX_NUM_SOUNDS 8
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
    EnableUart,//UART開始準備
    WaitPreamble,//UART書き込み待ち
    WaitBinSize, //UART書き込みサイズ待ち
    WaitBlock,  //UARTブロック待ち
    WaitEEPROMWriteEnd, //EEPROM書き込み完了待ち
    WaitEEPROMWriteEndLast, //最後のEEPROM書き込み完了待ち
    End,
} state_t;

static state_t state;
static uint8_t pcmValue;//読み込んだ1サンプルの振幅
static bool eepromSequencteEndFlag;
static bool eepromSReadStopFlag;
static bool intFlag;//ボタンが押されたらtrue
static bool playSoundEnFlag;
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
        uint16_t bingo;
    } pcmBlock_t;
    uint8_t buffer[BLOCK_MAX_LEN_DEF];//多目的バッファ
    uint8_t bufferCursor = 0;//多目的バッファのカーソル
    int eepromCursor = 0;//EEPROM書き込み/読み込みのアドレスカーソル
    uint32_t binSize = 0;//バイナリ全体のサイズ

    pcmBlock_t pcmSounds[MAX_NUM_SOUNDS];
    uint8_t pcmNofSounds = 0;   //Soundの数
    uint8_t pcmSound = 0;       //再生するSoundのIndex
    int weightSum = 0;          //Soundsのweightの和

    bool random_mode = false;//trueならランダムfalseならシーケンシャル
    int i;
    uint16_t random;

    __delay_ms(500);
    systemInit();
    tmr0_Init(privateTMR0ISR);
    tmr0cnt = 0;
    tmr0_Start();
    pwm_Init();
    extint_Init(privateINTISR);
    eepromSReadContinue = eeprom_Init(0x50, eepromSequencteEndCallBack, eepromSReadStopCallBack);
    uartInit();
    adc_Init();
    srand(adc_Get());//空ピンからAD読んでRandのSeedに設定
#if DUMP_MODE
    uartPinEnable(true);
    state = Dump_Start;
#else
    state = ReadHeader;
#endif
    while (1) {
        eeprom_InLoop();
        switch (state) {
            ///////////////////再生////////////////////////
            case Play://動作軽減のためなるべくmain loopでは処理しない
                break;

            case ReadHeader:
                eepromCursor = 0;
                eeprom_Read(&eepromCursor, buffer, PCM_HEADER_LEN + PCM_SND_NUM_LEN);
                eepromSequencteEndFlag = false;
                state = ReadHeaderWait;
                //breakなし
            case ReadHeaderWait:
                if (eepromSequencteEndFlag) {
                    eepromSequencteEndFlag = false;
                    if (!strBufComp(buffer, PCM_HEADER)) {
                        state = EnableUart;//不正なヘッダ
                        break;
                    }
                    pcmNofSounds = buffer[PCM_HEADER_LEN + 0];//soundの数
                    if(pcmNofSounds > MAX_NUM_SOUNDS){
                        state = EnableUart;//不正な数のサウンド
                        break;
                    }
                    eeprom_Read(&eepromCursor, buffer, PCM_SND_INFO_LEN*pcmNofSounds);
                    state = ReadSoundInfoWait;
                }
                break;
            case ReadSoundInfoWait:
                if (eepromSequencteEndFlag) {
                    eepromSequencteEndFlag = false;
                    random_mode = true;
                    int DataOffset = PCM_HEADER_LEN + PCM_SND_NUM_LEN + PCM_SND_INFO_LEN*pcmNofSounds;
                    weightSum = 0;
                    for(i = 0;i<pcmNofSounds;i++){
                        int infoOffset = PCM_SND_INFO_LEN*i;
                        pcmSounds[i].startAdd = DataOffset;
                        pcmSounds[i].length = buffer[infoOffset] | (buffer[infoOffset+1] << 8);
                        pcmSounds[i].weight = buffer[infoOffset+2];
                        DataOffset += pcmSounds[i].length;
                        weightSum += pcmSounds[i].weight;
                        if(pcmSounds[i].weight == 0){//weightが0のが1つでもあれば
                            random_mode = false;
                        }
                    }
                    if(random_mode){
                        uint16_t prevBingo = 0;
                        for(i = 0;i<pcmNofSounds;i++){
                            pcmSounds[i].bingo = prevBingo + pcmSounds[i].weight;
                            prevBingo = pcmSounds[i].bingo;
                        }
                    }else{
                        pcmSound = pcmNofSounds;
                    }
                    intFlag = false;
                    state = PlayReady;
                }
                break;
            case PlayReady:
                if(random_mode){
                    random = ((uint16_t)rand())%weightSum;
                    for(i = 0;i<pcmNofSounds;i++){
                        if(random < pcmSounds[i].bingo || i+1 == pcmNofSounds){
                            pcmSound = i;
                            break;
                        }
                    }
                }else{
                    pcmSound ++;
                    if(pcmSound >= pcmNofSounds){
                        pcmSound = 0;
                    }
                }
                playSoundEnFlag = false;
                pwm_On(false);
                state = WaitPlayButton;
                if(!intFlag){
                    sleep();
                }
                break;
            case WaitPlayButton:
                if (intFlag) {
                    if(PORTCbits.RC2 == 1){
                        state = EnableUart;
                        break;
                    }
                    intFlag = false;
                    eepromCursor = pcmSounds[pcmSound].startAdd;
                    eeprom_SRead(&eepromCursor, &pcmValue, pcmSounds[pcmSound].length);
                    eepromSequencteEndFlag = false;
                    playSoundEnFlag = true;
                    pwm_On(true);
                    state = Play;
                }
                break;
            ///////////////////書き込み////////////////////////
            case EnableUart:
                uartPinEnable(true);
                state = WaitPreamble;
                // breakなし
            case WaitPreamble:
                if (uartRXIntFlag) {
                    uartRXIntFlag = false;
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
                if (uartRXIntFlag) {
                    uartRXIntFlag = false;
                    buffer[bufferCursor] = uartRXData;
                    bufferCursor++;
                    if (bufferCursor >= 4) {
                        binSize = buffer[0] & 0xFF;
                        binSize |= ((uint32_t)buffer[1] & 0xFF) << 8;
                        binSize |= ((uint32_t)buffer[2] & 0xFF) << 16;
                        binSize |= ((uint32_t)buffer[3] & 0xFF) << 24;
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
                        eepromSequencteEndFlag = true;//書き込んでいないので次のWriteは無条件で開始
                    }
                }
                break;
            case WaitBlock:
            if (uartRXIntFlag) {
                uartRXIntFlag = false;
                buffer[bufferCursor] = uartRXData;
                bufferCursor++;
                eepromCursor++;
                if (eepromCursor >= binSize || bufferCursor >= eeprom_PageLen){//バイナリ終了 or ブロック終了
                    state = WaitEEPROMWriteEnd;
                }
            }
            break;
            case WaitEEPROMWriteEnd:
                if (eepromSequencteEndFlag) {
                    eeprom_Write(buffer, bufferCursor, true);
                    eepromSequencteEndFlag = false;
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
                if (eepromSequencteEndFlag) {
                    uartWrite('K');
                    __delay_ms(500);//送信直後にTXがLに落ちると書き込み側が0x00を解釈して誤認識する
                    uartPinEnable(false);
                    state = ReadHeader;
                }
                break;
            case End:
                break;
            //////////////////////////ダンプ//////////////////////////////////////////
            case Dump_Start:
                //                eepromRead(0, buffer, 64);
                if (intFlag) {
                    eeprom_SRead(0, buffer, 1000);
                    state = Dump_Read;
                }
            case Dump_Read:
                if (eepromSReadStopFlag) {
                    eepromSReadStopFlag = false;
                    eepromSReadContinue(false);
                    uartWrite(buffer[0]);
                } else if (eepromSequencteEndFlag) {
                    intFlag = false;
                    state = Dump_Start;
                }
                break;
        }
    }
}

static void sleep() {
    TMR0IE = 0;
    tmr0cnt = 0xffff;//sleep復帰後はチャタリングタイマーを無視してintFlagを立てる
    SLEEP();
}
static void wakeup() {
    TMR0IE = 1;
}
static void privateTMR0ISR() {
    if (playSoundEnFlag) {
        if(intFlag){
            eepromSReadContinue(true);//強制終了
        }else{
            eepromSReadContinue(false);
            pwm_SetDuty(pcmValue);
        }
    }
    if (tmr0cnt < 0xffff) {
        tmr0cnt++;
    }
}

static void privateINTISR() {
    wakeup();
   if (tmr0cnt >= 3200) {
        tmr0cnt = 0;
        intFlag = true;
        //        if (state == Play) {
        //            state = PlayReady;
        //        }
   }
}

static void eepromSequencteEndCallBack() {
    eepromSequencteEndFlag = true;
    if (state == Play) {
        state = PlayReady;
    }
}

static void eepromSReadStopCallBack() {
    eepromSReadStopFlag = true;
}