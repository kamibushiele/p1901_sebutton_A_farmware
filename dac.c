#include "dac.h"

void dacInit(){
    DACCON0 = 0b10100000;
}
void dacSet(uint8_t level){
    DACCON1 = level;
}
