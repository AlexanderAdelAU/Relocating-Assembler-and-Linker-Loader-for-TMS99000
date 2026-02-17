;* * *  Small-C/Plus  Version 1.06  * * *
;       Cain, Van Zandt, Hendrix, Yorston
;       21st April 1990
;
	module printf\itox
;/*
; * itox -- converts nbr to hex string of length sz
; *	       right adjusted and blank filled, returns str
; *
; *	      if sz > 0 terminate with null byte
; *	      if sz  =  0 find end of string
; *	      if sz < 0 use last byte for data
; */
;#include "stdio.h"
;/*
;** STDIO.H -- Standard Small-C Definitions
;**
;** Copyright 1984  L. E. Payne and J. E. Hendrix
;*/
;#define stdin    0
;#define stdout   1
;#define stderr   2
;#define ERR   (-2)
;#define EOF   (-1)
;#define YES      1
;#define NO       0
;#define NULL     0
;#define CR      0x0D /* '\r' */
;#define LF      0x0A /* '\n' */
;#define BELL     7
;#define SPACE  ' '
;#define NEWLINE LF      /*23*/ /*45*/
;#define FILE char
;itox(nbr, str, sz)
	global qitox
qitox:
;int nbr ;
;char str[] ;
;int sz ;
;{
;	int digit, offset ;
;	if ( sz > 0 )
	PUSH BC
	PUSH BC
	LD HL,6
	ADD HL,SP
	CALL ccgint
	XOR A
	OR H
	JP M,cc2
	OR L
	JP Z,cc2
;		str[--sz] = NULL ;
	LD HL,8
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,8
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	CALL ccpint
	POP DE
	ADD HL,DE
	PUSH HL
	LD HL,0
	POP DE
	LD A,L
	LD (DE),A
;	else if ( sz < 0 )
	JP cc3
cc2:
	LD HL,6
	ADD HL,SP
	CALL ccgint
	XOR A
	OR H
	JP P,cc4
;		sz = -sz ;
	LD HL,6
	ADD HL,SP
	PUSH HL
	LD HL,8
	ADD HL,SP
	CALL ccgint
	CALL ccneg
	CALL ccpint
;	else
	JP cc5
cc4:
;		while ( str[sz] != NULL )
cc6:
	LD HL,8
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,8
	ADD HL,SP
	CALL ccgint
	POP DE
	ADD HL,DE
	LD L,(HL)
	LD H,0
	LD A,L
	OR H
	JP Z,cc7
;			++sz ;
	LD HL,6
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	JP cc6
cc7:
cc5:
cc3:
;	while ( sz ) {
cc8:
	LD HL,6
	ADD HL,SP
	CALL ccgint
	LD A,L
	OR H
	JP Z,cc9
;		digit = nbr & 15 ;
	LD HL,10
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,15
	POP DE
	CALL ccand
	POP DE
	POP BC
	PUSH HL
	PUSH DE
;		nbr = ( nbr >> 4 ) & 0xfff ;
	LD HL,10
	ADD HL,SP
	PUSH HL
	LD HL,12
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,4
	POP DE
	CALL ccasr
	PUSH HL
	LD HL,4095
	POP DE
	CALL ccand
	CALL ccpint
;		if ( digit < 10 )
	LD HL,2
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,10
	POP DE
	CALL cclt
	LD A,L
	OR H
	JP Z,cc10
;			offset = 48 ;
	LD HL,48
	POP BC
	PUSH HL
;		else
	JP cc11
cc10:
;			offset = 55 ;
	LD HL,55
	POP BC
	PUSH HL
cc11:
;		str[--sz] = digit + offset ;
	LD HL,8
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,8
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	CALL ccpint
	POP DE
	ADD HL,DE
	PUSH HL
	LD HL,4
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,4
	ADD HL,SP
	CALL ccgint
	POP DE
	ADD HL,DE
	POP DE
	LD A,L
	LD (DE),A
;		if ( nbr == 0 )
	LD HL,10
	ADD HL,SP
	CALL ccgint
	LD A,L
	OR H
	JP NZ,cc12
;			break ;
	JP cc9
;	}
cc12:
	JP cc8
cc9:
;	while ( sz )
cc13:
	LD HL,6
	ADD HL,SP
	CALL ccgint
	LD A,L
	OR H
	JP Z,cc14
;		str[--sz] = ' ' ;
	LD HL,8
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,8
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	CALL ccpint
	POP DE
	ADD HL,DE
	PUSH HL
	LD HL,32
	POP DE
	LD A,L
	LD (DE),A
	JP cc13
cc14:
;	return str ;
	LD HL,8
	ADD HL,SP
	CALL ccgint
	POP BC
	POP BC
	RET
;}

; --- End of Compilation ---
