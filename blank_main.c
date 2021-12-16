#define _XTAL_FREQ 1000000

#include <xc.h>
#include <stdio.h>
//******************* コンフィグレーション ****************************
#pragma config FEXTOSC = OFF,RSTOSC = HFINT32  // HFINTOSC (1MHz)
#pragma config CLKOUTEN = OFF,CSWEN = ON,FCMEN = OFF
#pragma config MCLRE = OFF,PWRTE = OFF,WDTE = OFF,LPBOREN = OFF
#pragma config BOREN = OFF,BORV = LOW,PPS1WAY = OFF,STVREN = ON
#pragma config DEBUG = OFF
#pragma config WRT = OFF,LVP = OFF,CP = OFF,CPD = OFF

#define CONTRAST  0x28          // for 3.3V
 // #define CONTRAST  0x18       // for 5.0V


 // プロトタイプ *******************************
 void i2c_int(void);
 void i2cByteWrite(char, char, char);
 void i2cTxData(char);
 void LCD_dat(char);
 void LCD_cmd(char);
 void LCD_clr(void);
 void LCD_posyx(char,char);
 void LCD_int(void);

 void LCD_str(char *);
 void LCD_ROMstr(const char *);
 
 
void main() {

    LATA   = 0x00;
    TRISA  = 0xFF;          // Port すべて入力
    ANSELA = 0x00;          // すべてデジタル
    WPUA   = 0xFF;          // 弱プルアップ ON
    LATC   = 0x00;
    TRISC  = 0xFF;          // Port すべて入力
    ANSELC = 0x00;          // すべてデジタル
    WPUC   = 0xFF;          // 弱プルアップ ON

    // --------------------------------------------------------
    i2c_int();
    LCD_int();                  // LCDを初期化
    printf("LCD Test");
    LCD_posyx(1,1);             // 下段にカーソル移動
    printf("%04X",0x12AB);      // 数値をHEX表示する
    while(1);
}
//******************************************************
// LCD で printf関数が使用できるようにするため putch を設定する
//******************************************************
void putch(char data) {
        LCD_dat(data);      // LCD への一文字表示関数
}

//******************************************************
// I2C 関連 RC5 SCL, RC4 SDA
//******************************************************
void i2c_int(void){
    ODCC0  = 1;             // RC4をオープンドレイン
    ODCC1  = 1;             // RC5をオープンドレイン
    TRISC0 = 1;             // RC4を入力にする
    TRISC1 = 1;             // RC5を入力にする
    RC1PPS     = 25;        // RC4をDAT出力に指定
    RC0PPS     = 24;        // RC5をCLK出力に指定
    SSP1DATPPS = 0b10001;      // RC4をDATに入力指定
    SSP1CLKPPS = 0b10000;      // RC5をCLK入力に指定

    // SSP1設定 ------------------------------------------
    SSP1STAT = 0b10000000;     // スルーレート制御はOff
    SSP1ADD  = 19;              // クロック設定 125k@1MHz
    SSP1CON1 = 0b00101000;     // I2C Master modeにする
}
//-------- ByteI2C送信
void i2cByteWrite(char addr, char cont, char data){
    SSP1CON2bits.SEN = 1;      // Start condition 開始
    while(SSP1CON2bits.SEN);   // Start condition 確認
    i2cTxData(addr);           // アドレス送信
    i2cTxData(cont);           // 制御コード送信
    i2cTxData(data);           // データ送信
    SSP1CON2bits.PEN = 1;      // Stop condition 開始
    while(SSP1CON2bits.PEN);   // Stop condition 確認
}
//-------- Data送信
void i2cTxData(char data){
    PIR1bits.SSP1IF = 0;       // 終了フラグクリア
    SSP1BUF = data;            // データセット
    while(!PIR1bits.SSP1IF);   // 送信終了待ち
}
//********************************************************
// LCD 関連
//********************************************************
//-------- １文字表示
void LCD_dat(char chr){
    i2cByteWrite(0x7C, 0x40, chr);
    __delay_us(50);            // 50μsec
}
//-------- コマンド出力
void LCD_cmd(char cmd){
    i2cByteWrite(0x7C, 0x00, cmd);
    if(cmd & 0xFC)             // 上位６ビットに１がある命令
        __delay_us(50);        // 50usec
    else
        __delay_ms(2);         // 2msec ClearおよびHomeコマンド
}
//-------- 全消去
void LCD_clr(void){
    LCD_cmd(0x01);             //Clearコマンド出力
}
//-------- カーソル位置指定
void LCD_posyx(char ypos, char xpos){
    unsigned char pcode;
    switch(ypos & 0x03){
        case 0:    pcode=0x80;break;
        case 1:    pcode=0xC0;break;
    }
    LCD_cmd(pcode += xpos);
}
//-------- 初期化
void LCD_int(void){
    __delay_ms(100);
    LCD_cmd(0x38);             // 8bit 2行 表示命令モード
    LCD_cmd(0x39);             // 8bit 2行 拡張命令モード
    LCD_cmd(0x14);             // OSC  BIAS 設定1/5
                               // コントラスト設定
    LCD_cmd(0x70 + (CONTRAST & 0x0F));
    LCD_cmd(0x5C + (CONTRAST >> 4));
    LCD_cmd(0x6B);             // Ffollwer
    __delay_ms(100);
    __delay_ms(100);
    LCD_cmd(0x38);             // 表示命令モード
    LCD_cmd(0x0C);             // Display On
    LCD_cmd(0x01);             // Clear Display
}
//-------- 文字列出力
void LCD_str(char *str){
    while(*str)                //文字列の終わり(00)まで継続
        LCD_dat(*str++);       //文字出力しポインタ＋１
}
//-------- Rom 文字列出力
void LCD_ROMstr(const char *str){
    while(*str)                //文字列の終わり(00)まで継続
        LCD_dat(*str++);       //文字出力しポインタ＋１
}