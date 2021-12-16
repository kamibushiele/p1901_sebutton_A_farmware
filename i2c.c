#include "i2c.h"

typedef enum {
    State_Start,
    State_StartWait,
    State_SendADDWait1,
    State_SendData,
    State_SendDataWait,
    State_ReceiveData,
    State_ReadDataWait,
    State_SendACK,
    State_SendACkWait,
    State_Restart,
    State_RestartWait,
    State_SendADDWait2,
    State_SReceiveContinueWait,
    State_StopWait,
    State_Unknown,
} i2cState_t;

typedef enum {
    Mode_Write,
    Mode_Read,
    Mode_WriteRead,
    Mode_SRead,
    Mode_SWriteRead,
    Mode_Unknown,
} i2cMode_t;


static uint8_t *i2cWriteBuffer;
static int i2cWriteLength;
static int i2cWriteCursor;
static uint8_t *i2cReadBuffer;
static int i2cReadLength;
static int i2cReadCursor;
static uint8_t i2cWriteAdd;
static uint8_t i2cReadAdd;
static i2cState_t i2cState = State_Unknown;
static i2cMode_t i2cMode = Mode_Unknown;
static bool i2cISRFlug = false;
static bool i2cSReadContinueFlug;
static bool i2cReadEnd;
static bool i2cReadForceEnd;

static void(*i2cSequenceEndCallBack)();
static void(*i2cSReadStopCallBack)();

/*レジスタレベル*/
inline static void i2cStartEnable() {
    SSP1CON2bits.SEN = 1;
}

inline static void i2cRestartEnable() {
    SSP1CON2bits.RSEN = 1;
}

inline static void i2cReadEnable() {
    SSP1CON2bits.RCEN = 1;
}

inline static void i2cStopEnable() {
    SSP1CON2bits.PEN = 1;
}

inline static void i2cSend(uint8_t data) {
    SSP1BUF = data; //自動で送信開始
}

inline static uint8_t i2cRead() {
    return SSP1BUF;
}

inline static bool i2cReadAck() {
    return SSP1CON2bits.ACKSTAT;
}

inline static void i2cSendACK(uint8_t ack) {
    SSP1CON2bits.ACKDT = ack;
    SSP1CON2bits.ACKEN = 1;
}

/*ここまでレジスタレベル*/

static void i2cBufferWrite() {
    i2cSend(i2cWriteBuffer[i2cWriteCursor++]);
}

static void i2cBufferRead() {
    i2cReadBuffer[i2cReadCursor++] = i2cRead();
}

static void i2cSBufferRead() {
    i2cReadBuffer[0] = i2cRead();
    i2cReadCursor++;
}

static void i2cSetAdd(uint8_t add) {
    i2cWriteAdd = add << 1;
    i2cReadAdd = i2cWriteAdd | 1;
}

static void private_i2cStart(uint8_t add, uint8_t *writeBuffer, int writeLength, uint8_t *readBuffer, int readLength) {
    i2cWriteBuffer = writeBuffer;
    i2cWriteLength = writeLength;
    i2cWriteCursor = 0;
    i2cReadBuffer = readBuffer;
    i2cReadLength = readLength;
    i2cReadCursor = 0;
    i2cSetAdd(add);
    i2cState = State_Start;
}

static void i2cSReadContinue(bool forceEnd) {
    i2cSReadContinueFlug = true;
    i2cReadForceEnd = forceEnd;
}

/*外部インターフェース*/
void (*i2c_Init(void(*sequenceEndCallBack)(), void(*sReadStopCallBack)()))(bool) {
    //    //_XTAL_FREQ = 16M
    //    SSP1ADD = 9; //400kHz
    //_XTAL_FREQ = 32M
    SSP1ADD = 19; //400kHz
    SSP1STAT = 0x00;
    SSP1CON1 = 0b0101000;
    i2cSequenceEndCallBack = sequenceEndCallBack;
    i2cSReadStopCallBack = sReadStopCallBack;
    return i2cSReadContinue;
}

void i2c_WriteStart(uint8_t add, uint8_t *writeBuffer, int writeLength) {
    i2cMode = Mode_Write;
    private_i2cStart(add, writeBuffer, writeLength, NULL, 0);
}

void i2c_ReadStart(uint8_t add, uint8_t *readBuffer, int readLength) {
    i2cMode = Mode_Read;
    private_i2cStart(add, NULL, 0, readBuffer, readLength);
}

void i2c_WriteReadStart(uint8_t add, uint8_t *writeBuffer, int writeLength, uint8_t *readBuffer, int readLength) {
    i2cMode = Mode_WriteRead;
    private_i2cStart(add, writeBuffer, writeLength, readBuffer, readLength);
}

void i2c_SReadStart(uint8_t add, uint8_t *readBuffer, int readLength) {
    i2cMode = Mode_SRead;
    private_i2cStart(add, NULL, 0, readBuffer, readLength);
}

void i2c_SWriteReadStart(uint8_t add, uint8_t *writeBuffer, int writeLength, uint8_t *readBuffer, int readLength) {
    i2cMode = Mode_SWriteRead;
    private_i2cStart(add, writeBuffer, writeLength, readBuffer, readLength);
}

void i2c_InLoop() {
    switch (i2cState) {

        case State_ReceiveData://データ受信
            i2cReadEnable();
            i2cState = State_ReadDataWait;
            //breakなし
        case State_ReadDataWait://データ受信待ち
            if (i2cISRFlug) {
                i2cISRFlug = false;
                i2cReadEnd = i2cReadCursor >= i2cReadLength;
                if (i2cMode == Mode_SRead || i2cMode == Mode_SWriteRead) {
                    i2cSBufferRead();
                    i2cSReadContinueFlug = false;
                    i2cSReadStopCallBack();
                    i2cState = State_SReceiveContinueWait;
                    //この場合だけbreakなし
                } else {
                    i2cBufferRead();
                    i2cState = State_SendACK;
                    break;
                }
            } else {
                break;
            }
        case State_SReceiveContinueWait://受信継続フラグtrue待ち
            if (i2cSReadContinueFlug) {
                if (i2cReadForceEnd) {
                    i2cReadEnd = 1;
                }
                i2cState = State_SendACK;
            } else {
                break;
            }
        case State_SendACK:
            i2cSendACK(i2cReadEnd);
            i2cState = State_SendACkWait;
        case State_SendACkWait://ACK送信待ち
            if (i2cISRFlug) {
                i2cISRFlug = false;
                if (i2cReadEnd) {
                    i2cStopEnable();
                    i2cState = State_StopWait;
                } else {
                    i2cState = State_ReceiveData;
                }
            }
            break;


        case State_Start://開始
            i2cISRFlug = false;
            i2cStartEnable();
            i2cState = State_StartWait;
            //breakなし
        case State_StartWait://開始確認待ち
            if (i2cISRFlug) {
                i2cISRFlug = false;
                switch (i2cMode) {
                    case Mode_Read:
                    case Mode_SRead:
                        i2cSend(i2cReadAdd);
                        break;
                    default:
                        i2cSend(i2cWriteAdd);
                        break;
                }
                i2cState = State_SendADDWait1;
            }
            break;
        case State_SendADDWait1://1回目のアドレス送信待ち
            if (i2cISRFlug) {
                i2cISRFlug = false;
                if (!i2cReadAck()) {
                    switch (i2cMode) {
                        case Mode_Read:
                        case Mode_SRead:
                            i2cState = State_ReceiveData;
                            break;
                        default://Write,WriteRead,SWriteRead
                            i2cState = State_SendData;
                            break;
                    }
                } else {
                    i2cState = State_Start;
                }
            }
            break;
        case State_SendData://データ送信
            i2cBufferWrite();
            i2cState = State_SendDataWait;
            //breakなし
        case State_SendDataWait://データ送信終了待ち
            if (i2cISRFlug) {
                i2cISRFlug = false;
                if (i2cWriteCursor >= i2cWriteLength) {
                    if (i2cMode == Mode_Write) {
                        i2cStopEnable();
                        i2cState = State_StopWait;
                    } else {//WriteRead,SWriteRead
                        i2cState = State_Restart;
                    }
                } else {
                    i2cState = State_SendData;
                }
            }
            break;

        case State_Restart://再開
            i2cRestartEnable();
            i2cState = State_RestartWait;
            //breakなし
        case State_RestartWait://再開待ち
            if (i2cISRFlug) {
                i2cISRFlug = false;
                i2cSend(i2cReadAdd);
                i2cState = State_SendADDWait2;
            }
            break;
        case State_SendADDWait2://2回目のアドレス送信待ち
            if (i2cISRFlug) {
                i2cISRFlug = false;
                if (!i2cReadAck()) {
                    i2cState = State_ReceiveData;
                } else {
                    i2cState = State_Restart;
                }
            }
            break;

        case State_StopWait:
            if (i2cISRFlug) {
                i2cISRFlug = false;
                i2cState = State_Unknown;
                i2cSequenceEndCallBack();
            }
            break;
        case State_Unknown:
            break;
    }
}

void i2c_Interrupt() {
    i2cISRFlug = true;
}