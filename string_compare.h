#ifndef CHAR_COMPAIR_H
#define	CHAR_COMPAIR_H

/**** strBufComp **********************************
 * バッファ最初からの文字列が比較文字列と等しいか
 * 
 * arr : バッファ(文字が代入される配列)
 * string : 比較文字列
 **************************/
bool strBufComp(const char *arr, const char *string);
/**** strLoopBufComp **********************************
 * 循環するバッファ中の最新の文字列が比較文字列と等しいか
 * 
 * arr : バッファ(文字が循環して代入される配列)
 * arrLend : バッファサイズ
 * arrCursor : 最新のデータのアドレス
 * string : 比較文字列
 * stringLen : 比較文字列の長さ(\0は含まない)
 **************************/
bool strLoopBufComp(const char *arr, int arrLen, int arrCursor, const char *string, int stringLen);

#endif	/* CHAR_COMPAIR_H */

