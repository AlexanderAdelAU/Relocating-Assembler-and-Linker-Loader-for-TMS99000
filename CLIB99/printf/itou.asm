;* * *  Small-C/Plus  Version 1.06  * * *
;       Cain, Van Zandt, Hendrix, Yorston
;       21st April 1990
;
	module printf\itou
;/*
; * itou -- convert nbr to unsigned decimal string of width sz
; *	       right adjusted, blank filled ; returns str
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
;itou(nbr, str, sz)
	global qitou
qitou:
;int nbr ;
;char str[] ;
;int sz ;
;{
;	int lowbit ;
;	if ( sz > 0 )
	PUSH BC
	LD HL,4
	ADD HL,SP
	CALL ccgint
	XOR A
	OR H
	JP M,cc2
	OR L
	JP Z,cc2
;		str[--sz] = NULL ;
	LD HL,6
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,6
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
	LD HL,4
	ADD HL,SP
	CALL ccgint
	XOR A
	OR H
	JP P,cc4
;			sz = -sz ;
	LD HL,4
	ADD HL,SP
	PUSH HL
	LD HL,6
	ADD HL,SP
	CALL ccgint
	CALL ccneg
	CALL ccpint
;		else
	JP cc5
cc4:
;			while ( str[sz] != NULL )
cc6:
	LD HL,6
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,6
	ADD HL,SP
	CALL ccgint
	POP DE
	ADD HL,DE
	LD L,(HL)
	LD H,0
	LD A,L
	OR H
	JP Z,cc7
;				++sz ;
	LD HL,4
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
	LD HL,4
	ADD HL,SP
	CALL ccgint
	LD A,L
	OR H
	JP Z,cc9
;		lowbit = nbr & 1 ;
	LD HL,8
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,1
	POP DE
	CALL ccand
	POP BC
	PUSH HL
;		nbr = (nbr >> 1) & 0x7fff ;  /* divide by 2 */
	LD HL,8
	ADD HL,SP
	PUSH HL
	LD HL,10
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,1
	POP DE
	CALL ccasr
	PUSH HL
	LD HL,32767
	POP DE
	CALL ccand
	CALL ccpint
;		str[--sz] = ( (nbr%5) << 1 ) + lowbit + '0' ;
	LD HL,6
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,6
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	CALL ccpint
	POP DE
	ADD HL,DE
	PUSH HL
	LD HL,10
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,5
	POP DE
	CALL ccdiv
	EX DE,HL
	PUSH HL
	LD HL,1
	POP DE
	CALL ccasl
	PUSH HL
	LD HL,4
	ADD HL,SP
	CALL ccgint
	POP DE
	ADD HL,DE
	LD DE,48
	ADD HL,DE
	POP DE
	LD A,L
	LD (DE),A
;		if ( (nbr/=5) == 0 )
	LD HL,8
	ADD HL,SP
	PUSH HL
	CALL ccgint
	PUSH HL
	LD HL,5
	POP DE
	CALL ccdiv
	CALL ccpint
	LD A,L
	OR H
	JP NZ,cc10
;			break ;
	JP cc9
;	}
cc10:
	JP cc8
cc9:
;	while ( sz )
cc11:
	LD HL,4
	ADD HL,SP
	CALL ccgint
	LD A,L
	OR H
	JP Z,cc12
;		str[--sz] = ' ' ;
	LD HL,6
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,6
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
	JP cc11
cc12:
;	return str ;
	LD HL,6
	ADD HL,SP
	CALL ccgint
	POP BC
	RET
;}

; --- End of Compilation ---
