#include "pwm.h"

void pwm_Init() {
    //PWM設定 7bit
    TMR2 = 0b0;
    T2CON = 0b00000101; // prescale=4
    PR2 = 31;
    PWM5CON = 0b10000000;
    PWM5DCH = 0b0;
    PWM5DCL = 0b0;

    PWMTMRS = 0b00000001;
    //バイポーラ駆動
    //逆相ピン設定
//    CLC1SEL0 = 16; //G1 input1=PWM5
//    CLC1SEL1 = 0; //Don't Care
//    CLC1SEL2 = 0; //Don't Care
//    CLC1SEL3 = 0; //Don't Care
//    CLC1GLS0 = 0x02; //G1input1 = true
//    CLC1GLS1 = 0x00; //G2input 0000
//    CLC1GLS2 = 0x00; //G3input 0000
//    CLC1GLS3 = 0x00; //G4input 0000
//    CLC1POL = 0x01; //OUT,!G1
//    CLC1CON = 0b10000000; //AND-OR notInterrupt
#if PWMMODE == 0
        //Hブリッジ駆動 CLCXPOLbits.G2POLが1だとPWMの反転が出力、0だと1が出力
        CLC1SEL0 = 16; //G1 input1=PWM5
        CLC1SEL1 = 0; //Don't Care
        CLC1SEL2 = 0; //Don't Care
        CLC1SEL3 = 0; //Don't Care
        CLC1GLS0 = 0x02; //G1input1 = true
        CLC1GLS1 = 0x00; //G2input 0000
        CLC1GLS2 = 0x00; //G3input 0000
        CLC1GLS3 = 0x00; //G4input 0000
        CLC1POL = 0x80; //!OUT,G4,G3,G2,G1
        CLC1CON = 0b10000000; //AND-OR notInterrupt
    
        CLC2SEL0 = 16; //G1 input1=PWM5
        CLC2SEL1 = 0; //Don't Care
        CLC2SEL2 = 0; //Don't Care
        CLC2SEL3 = 0; //Don't Care
        CLC2GLS0 = 0x02; //G1input1 = true
        CLC2GLS1 = 0x00; //G2input 0000
        CLC2GLS2 = 0x00; //G3input 0000
        CLC2GLS3 = 0x00; //G4input 0000
        CLC2POL = 0x80; //!OUT,G4,G3,G2,G1
        CLC2CON = 0b10000000; //AND-OR notInterrupt
        CLC1POLbits.G2POL = ;
        CLC2POLbits.G2POL = 1;
#else
        /******************************************
         * Hブリッジ駆動(ブレーキなし) 
         *  CLCXPOLbits.G2POLが
         * 1: PWMが出力
         *      CLCXPOLbits.G3POL, G4POL = 0にすること
         * 0: 固定
         *      CLCXPOLbits.G3POL, G4POL = 1→1が出力
         *      CLCXPOLbits.G3POL, G4POL = 0→0が出力
         * つまりCLCXPOLは
         * PWM          → OUT, G4, G3,!G2, G1   → 0x02
         * 0固定        → OUT, G4, G3, G2, G1   → 0x00
         * 1固定        → OUT,!G4,!G3, G2, G1   → 0x0C
         * 出力全反転   →!OUT                   → 0x8X
         ****************************************/
        CLC1SEL0 = 16; //G1 input1=PWM5
        CLC1SEL1 = 0; //Don't Care
        CLC1SEL2 = 0; //Don't Care
        CLC1SEL3 = 0; //Don't Care
        CLC1GLS0 = 0x02; //G1input1 = true
        CLC1GLS1 = 0x00; //G2input 0000
        CLC1GLS2 = 0x00; //G3input 0000
        CLC1GLS3 = 0x00; //G4input 0000
        CLC1POL = 0x00; //OUT,G4,G3,G2,G1
        CLC1CON = 0b10000000; //AND-OR notInterrupt
    
        CLC2SEL0 = 16; //G1 input1=PWM5
        CLC2SEL1 = 0; //Don't Care
        CLC2SEL2 = 0; //Don't Care
        CLC2SEL3 = 0; //Don't Care
        CLC2GLS0 = 0x02; //G1input1 = true
        CLC2GLS1 = 0x00; //G2input 0000
        CLC2GLS2 = 0x00; //G3input 0000
        CLC2GLS3 = 0x00; //G4input 0000
        CLC2POL = 0x00; //OUT,G4,G3,G2,G1
        CLC2CON = 0b10000000; //AND-OR notInterrupt
#endif
    pwm_On(false);
}

static void private_pwm_SetDuty(uint16_t duty) {
    PWM5DCH = duty >> 2;
    PWM5DCL = duty << 8;
}

void pwm_SetDuty(uint16_t duty) {
        if (duty >= 0x80) {
#if CLC_INVERT
            CLC1POL = 0x82;//CLC1 = INAがPWM
            CLC2POL = 0x80;//CLC2 = INBがL
#else
            CLC1POL = 0x02;//CLC1 = INAがPWM
            CLC2POL = 0x00;//CLC2 = INBがL
#endif
            private_pwm_SetDuty(duty-0x80);
        } else {
#if CLC_INVERT
            CLC1POL = 0x80;//CLC1 = INAがPWM
            CLC2POL = 0x82;//CLC2 = INBがL
#else
            CLC1POL = 0x00;//CLC1 = INAがPWM
            CLC2POL = 0x02;//CLC2 = INBがL
#endif
            private_pwm_SetDuty(0x80-duty);
        }
}

void pwm_On(bool on) {
//    CLC1POLbits.G2POL = on;
#if PWMMODE == 0
    if(on){
        CLC1POL = 0x80; //!OUT,G4,G3,G2,G1
        CLC2POL = 0x80; //!OUT,G4,G3,G2,G1
    }else{
        CLC1POL = 0x00; //OUT,G4,G3,G2,G1→出力0
        CLC2POL = 0x00; //OUT,G4,G3,G2,G1→出力0
//        pwm_SetDuty(0x80);
    }
#else
    if(on){
        T2CONbits.TMR2ON = 1;
        PWM5CONbits.PWM5EN = 1;
    }else{
        //消費電力低減のため
        T2CONbits.TMR2ON = 0;
        PWM5CONbits.PWM5EN = 0;
#if CLC_INVERT
        CLC1POL = 0x8C;//CLC1 = INAがH
        CLC2POL = 0x8C;//CLC2 = INBがH
#else
        CLC1POL = 0x0C;//CLC1 = INAがH
        CLC2POL = 0x0C;//CLC2 = INBがH
#endif
    }
#endif
}