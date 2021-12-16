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
        CLC1POL = 0x80; //!OUT,G1,G2,G3,G4
        CLC1CON = 0b10000000; //AND-OR notInterrupt
    
        CLC2SEL0 = 16; //G1 input1=PWM5
        CLC2SEL1 = 0; //Don't Care
        CLC2SEL2 = 0; //Don't Care
        CLC2SEL3 = 0; //Don't Care
        CLC2GLS0 = 0x02; //G1input1 = true
        CLC2GLS1 = 0x00; //G2input 0000
        CLC2GLS2 = 0x00; //G3input 0000
        CLC2GLS3 = 0x00; //G4input 0000
        CLC2POL = 0x80; //!OUT,G1,G2,G3,G4
        CLC2CON = 0b10000000; //AND-OR notInterrupt
        CLC1POLbits.G2POL = ;
        CLC2POLbits.G2POL = 1;
#else
        //Hブリッジ駆動(ブレーキなし) CLCXPOLbits.G2POLが1だとPWMが出力、0だと0が出力 0にして出力POLを1にすると1が出力
        CLC1SEL0 = 16; //G1 input1=PWM5
        CLC1SEL1 = 0; //Don't Care
        CLC1SEL2 = 0; //Don't Care
        CLC1SEL3 = 0; //Don't Care
        CLC1GLS0 = 0x02; //G1input1 = true
        CLC1GLS1 = 0x00; //G2input 0000
        CLC1GLS2 = 0x00; //G3input 0000
        CLC1GLS3 = 0x00; //G4input 0000
        CLC1POL = 0x00; //OUT,G1,G2,G3,G4
        CLC1CON = 0b10000000; //AND-OR notInterrupt
    
        CLC2SEL0 = 16; //G1 input1=PWM5
        CLC2SEL1 = 0; //Don't Care
        CLC2SEL2 = 0; //Don't Care
        CLC2SEL3 = 0; //Don't Care
        CLC2GLS0 = 0x02; //G1input1 = true
        CLC2GLS1 = 0x00; //G2input 0000
        CLC2GLS2 = 0x00; //G3input 0000
        CLC2GLS3 = 0x00; //G4input 0000
        CLC2POL = 0x00; //OUT,G1,G2,G3,G4
        CLC2CON = 0b10000000; //AND-OR notInterrupt
#endif
}

static void private_pwm_SetDuty(uint16_t duty) {
    PWM5DCH = duty >> 2;
    PWM5DCL = duty << 8;
}

void pwm_SetDuty(uint16_t duty) {
        if (duty >= 0x80) {
            CLC1POLbits.G2POL = 1;
            CLC2POLbits.G2POL = 0;
            private_pwm_SetDuty(duty-0x80);
        } else {
            CLC1POLbits.G2POL = 0;
            CLC2POLbits.G2POL = 1;
            private_pwm_SetDuty(0x80-duty);
        }
}

void pwm_On(bool on) {
//    CLC1POLbits.G2POL = on;
#if PWMMODE == 0
    if(on){
        CLC1POL = 0x80; //!OUT,G1,G2,G3,G4
        CLC2POL = 0x80; //!OUT,G1,G2,G3,G4
    }else{
        CLC1POL = 0x00; //OUT,G1,G2,G3,G4→出力0
        CLC2POL = 0x00; //OUT,G1,G2,G3,G4→出力0
//        pwm_SetDuty(0x80);
    }
#else
    if(on){
        CLC1POL = 0x00; //OUT,G1,G2,G3,G4
        CLC2POL = 0x00; //OUT,G1,G2,G3,G4
    }else{
//        CLC1POL = 0x80; //!OUT,G1,G2,G3,G4→出力1
//        CLC2POL = 0x80; //!OUT,G1,G2,G3,G4→出力1
    }
#endif
}