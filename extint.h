/* 
 * File:   extint.h
 * Author: fues
 *
 * Created on 2020/01/13, 16:42
 */

#ifndef EXTINT_H
#define	EXTINT_H

void extint_Init(void(*isrCallback)(void));
void extint_Interrupt();

#endif	/* EXTINT_H */

