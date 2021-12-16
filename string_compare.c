#include <stdbool.h>
#include "string_compare.h"

bool strBufComp(const char *arr, const char *string){
    int i;
    bool same = true;
    for(i=0;string[i] != '\0';i++){
        if (string[i] != arr[i]) {
            same = false;
            break;
        }
    }
    return same;
}
bool strLoopBufComp(const char *arr, int arrLen, int arrCursor, const char *string, int stringLen) {
    int i;
    int arrOffset;
    bool same = true;
    for (i = 0; string[i] != '\0'; i++) {
        arrOffset = i + arrCursor - (stringLen - 1);
        if (arrOffset < 0) {
            arrOffset += arrLen;
        }
        if (string[i] != arr[arrOffset]) {
            same = false;
            break;
        }
    }
    return same;
}