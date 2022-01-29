#include "system.h"
#include "tmr0.h"
#include "uart.h"
#include "i2c.h"
#include "extint.h"

// CONFIG1
#pragma config FEXTOSC = OFF    // FEXTOSC External Oscillator mode Selection bits (Oscillator not enabled)
#pragma config RSTOSC = HFINT32 // Power-up default value for COSC bits (HFINTOSC with 2x PLL (32MHz))
#pragma config CLKOUTEN = OFF    // Clock Out Enable bit (CLKOUT function is enabled; FOSC/4 clock appears at OSC2)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config FCMEN = OFF       // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config MCLRE = ON      // Master Clear Enable bit (MCLR/VPP pin function is digital input; MCLR internally disabled; Weak pull-up under control of port pin's WPU control bit.)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config WDTE = OFF       // Watchdog Timer Enable bits (WDT disabled; SWDTEN is ignored)
#pragma config LPBOREN = OFF    // Low-power BOR enable bit (ULPBOR disabled)
#pragma config BOREN = OFF       // Brown-out Reset Enable bits (Brown-out Reset enabled, SBOREN bit ignored)
#pragma config BORV = LOW       // Brown-out Reset Voltage selection bit (Brown-out voltage (Vbor) set to 2.45V)
#pragma config PPS1WAY = OFF     // PPSLOCK bit One-Way Set Enable bit (The PPSLOCK bit can be cleared and set only once; PPS registers remain locked after one clear/set cycle)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable bit (Stack Overflow or Underflow will cause a Reset)
#pragma config DEBUG = OFF      // Debugger enable bit (Background debugger disabled)

// CONFIG3
#pragma config WRT = OFF        // User NVM self-write protection bits (Write protection off)
#pragma config LVP = OFF         // Low Voltage Programming Enable bit (Low Voltage programming enabled. MCLR/VPP pin function is MCLR. MCLRE configuration bit is ignored.)

// CONFIG4
#pragma config CP = OFF         // User NVM Program Memory Code Protection bit (User NVM code protection disabled)
#pragma config CPD = OFF        // Data NVM Memory Code Protection bit (Data NVM code protection disabled)

void systemInit() {
//    /*****************
//     * クロック設定 freq=16M
//     *****************/
//    OSCCON1 = 0x00; // clocksource&devider
//    //OSCCON2 is read only
//    OSCCON3 = 0x00; //
//    OSCEN = 0x00;
//    OSCFRQ = 0x04;
//    OSCTUNE = 0x00;

    /*****************
     * 入出力設定
     *****************/
    LATA = 0;
    LATC = 0;
    ANSELA = 0x04;//2
    ANSELC = 0x00;
    TRISA = 0x34;//5,4,2
    TRISC = 0x27;//5,2,1,0
    WPUA = 0x30;//5,4
    WPUC = 0x20;//5
    ODCONA = 0x00;
    ODCONC = 0x03;//1,0

    /*****************
     * 特殊ピン設定
     *****************/

    //    INTCONbits.GIE = 0; //割り込みは一時中断
    //    PPSLOCK = 0x55;
    //    PPSLOCK = 0xAA;
    //    PPSLOCKbits.PPSLOCKED = 0x00; // unlock PPS
    //入力設定
    /*
    10111 = Reserved. Do not use.
    10110 = Reserved. Do not use.
    10101 = Peripheral input is RC5 (1)
    10100 = Peripheral input is RC4 (1)
    10011 = Peripheral input is RC3 (1)
    10010 = Peripheral input is RC2 (1)
    10001 = Peripheral input is RC1 (1)
    10000 = Peripheral input is RC0 (1)
    ...
    01xxx = Reserved. Do not use.
    ...
    0011x = Reserved. Do not use.
    00101 = Peripheral input is RA5
    00100 = Peripheral input is RA4
    00011 = Peripheral input is RA3
    00010 = Peripheral input is RA2
    00001 = Peripheral input is RA1
    00000 = Peripheral input is RA0
     */
    SSP1CLKPPS = 0b10000; // RC0 → SCL
    SSP1DATPPS = 0b10001; // RC1 → SDA
    RXPPS = 0b00101; // RA5 → RX
    INTPPS = 0b10101; //RC5→INT

    //    SSP1CLKPPS = 0b00100; // RA4 → SCL
    //    SSP1DATPPS = 0b00101; // RA5 → SDA
    //    RXPPS = 0b00001; // RA1 → RX
    //出力設定
    /*
    11111 = Rxy source is DSM
    11110 = Rxy source is CLKR
    11101 = Rxy source is NCO1
    11100 = Rxy source is TMR0
    11011 = Reserved
    11010 = Reserved
    11001 = Rxy source is SDO1/SDA1
    11000 = Rxy source is SCK1/SCL1 (1)
    10111 = Rxy source is C2 (2)
    10110 = Rxy source is C1
    10101 = Rxy source is DT (1)
    10100 = Rxy source is EUSART TC/CK
    10011 = Reserved
    10010 = Reserved
    10001 = Reserved
    10000 = Reserved
    01111 = Reserved
    01110 = Reserved
    01101 = Rxy source is CCP2
    01100 = Rxy source is CCP1
    01011 = Rxy source is CWG1D (1)
    01010 = Rxy source is CWG1C (1)
    01001 = Rxy source is CWG1B (1)
    01000 = Rxy source is CWG1A (1)
    00111 = Reserved
    00110 = Reserved
    00101 = Rxy source is CLC2OUT
    00100 = Rxy source is CLC1OUT
    00011 = Rxy source is PWM6
    00010 = Rxy source is PWM5
    00001 = Reserved
    00000 = Rxy source is LATx
     */
    RC0PPS = 0b11000; // SCL→RC0
    RC1PPS = 0b11001; // SDA→RC1
    // RA4PPS = 0b10100; // TX→RA4//uartPinEnableで設定
    RC4PPS = 0b00100; //CLC1→RC4;
    RC3PPS = 0b00101; //CLC2→RC3;

    //    RA4PPS = 0b11000; // SCL→RA4
    //    RA5PPS = 0b11001; // SDA→RA5
    //    //    RA5PPS = 0b00010; // PWM5→RA5
    //    RA2PPS = 0b10100; // TX→RA2

    //    PPSLOCK = 0x55;
    //    PPSLOCK = 0xAA;
    //    PPSLOCKbits.PPSLOCKED = 0x01; // lock PPS


    /*****************
     * 割り込み設定
     *****************/
    INTCONbits.GIE = 1; //全体割り込み許可
    INTCONbits.PEIE = 1; //周辺モジュール割り込み許可
    INTE = 1; //外部割り込み許可
    INTCONbits.INTEDG = 0; //↑1=立ち上がり 0=立ち下がり
    TMR0IE = 1; //タイマ0割り込み許可
    SSP1IE = 1; //SSP割り込み許可
    ADIE = 0; //ADC割り込み許可
    RCIE = 1; //UART受信割り込み
    TXIE = 0; //UART送信割り込み
}

void uartPinEnable(bool enable){
    if(enable){
        TRISAbits.TRISA4 = 0;
        WPUAbits.WPUA4 = 0;
        RA4PPS = 0b10100; // TX→RA4
    }else{
        TRISAbits.TRISA4 = 1;
        WPUAbits.WPUA4 = 1;
        RA4PPS = 0b00000; // LATA4→RA4
    }
}

void __interrupt() isr(void) {
    if (TMR0IF) {
        TMR0IF = 0;
        tmr0_Interrupt();
    }
    if (SSP1IF) {
        SSP1IF = 0;
        i2c_Interrupt();
    }
    if (RCIF) {
        RCIF = 0;
        uartRXInterrupt();
    }
    if(INTF){
        INTF = 0;
        extint_Interrupt();
    }
    //    if (TXIE) {
    //        TXIE = 0;
    //        esuartTXInterrupt();
    //    }
}