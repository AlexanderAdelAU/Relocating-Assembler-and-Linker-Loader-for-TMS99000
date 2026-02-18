;* * *  Small-C/Plus  Version 1.06  * * *
;       Cain, Van Zandt, Hendrix, Yorston
;       21st April 1990
;
	module printf\itod
;/*
; * itod -- convert nbr to signed decimal string of width sz
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
;itod(nbr, str, sz)
	global qitod
qitod:
;int nbr ;
;char str[] ;
;int sz ;
;{
;	char sgn ;
;	if ( nbr < 0 ) {
	DEC SP
	LD HL,7
	ADD HL,SP
	CALL ccgint
	XOR A
	OR H
	JP P,cc2
;		nbr = -nbr ;
	LD HL,7
	ADD HL,SP
	PUSH HL
	LD HL,9
	ADD HL,SP
	CALL ccgint
	CALL ccneg
	CALL ccpint
;		sgn = '-' ;
	LD HL,0
	ADD HL,SP
	PUSH HL
	LD HL,45
	POP DE
	LD A,L
	LD (DE),A
;	}
;	else
	JP cc3
cc2:
;		sgn = ' ' ;
	LD HL,0
	ADD HL,SP
	PUSH HL
	LD HL,32
	POP DE
	LD A,L
	LD (DE),A
cc3:
;	if ( sz > 0 )
	LD HL,3
	ADD HL,SP
	CALL ccgint
	XOR A
	OR H
	JP M,cc4
	OR L
	JP Z,cc4
;		str[--sz] = NULL ;
	LD HL,5
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,5
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
	JP cc5
cc4:
	LD HL,3
	ADD HL,SP
	CALL ccgint
	XOR A
	OR H
	JP P,cc6
;			sz = -sz ;
	LD HL,3
	ADD HL,SP
	PUSH HL
	LD HL,5
	ADD HL,SP
	CALL ccgint
	CALL ccneg
	CALL ccpint
;		else
	JP cc7
cc6:
;			while ( str[sz] != NULL )
cc8:
	LD HL,5
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,5
	ADD HL,SP
	CALL ccgint
	POP DE
	ADD HL,DE
	LD L,(HL)
	LD H,0
	LD A,L
	OR H
	JP Z,cc9
;				++sz ;
	LD HL,3
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	JP cc8
cc9:
cc7:
cc5:
;	while ( sz ) {
cc10:
	LD HL,3
	ADD HL,SP
	CALL ccgint
	LD A,L
	OR H
	JP Z,cc11
;		str[--sz] = nbr % 10 + '0' ;
	LD HL,5
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,5
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	CALL ccpint
	POP DE
	ADD HL,DE
	PUSH HL
	LD HL,9
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,10
	POP DE
	CALL ccdiv
	EX DE,HL
	LD DE,48
	ADD HL,DE
	POP DE
	LD A,L
	LD (DE),A
;		if ( (nbr/=10) == 0 )
	LD HL,7
	ADD HL,SP
	PUSH HL
	CALL ccgint
	PUSH HL
	LD HL,10
	POP DE
	CALL ccdiv
	CALL ccpint
	LD A,L
	OR H
	JP NZ,cc12
;			break ;
	JP cc11
;	}
cc12:
	JP cc10
cc11:
;	if ( sz )
	LD HL,3
	ADD HL,SP
	CALL ccgint
	LD A,L
	OR H
	JP Z,cc13
;		str[--sz] = sgn ;
	LD HL,5
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,5
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	CALL ccpint
	POP DE
	ADD HL,DE
	PUSH HL
	LD HL,2
	ADD HL,SP
	LD L,(HL)
	LD H,0
	POP DE
	LD A,L
	LD (DE),A
;	while ( sz > 0 )
cc13:
cc14:
	LD HL,3
	ADD HL,SP
	CALL ccgint
	XOR A
	OR H
	JP M,cc15
	OR L
	JP Z,cc15
;		str[--sz] = ' ' ;
	LD HL,5
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,5
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
	JP cc14
cc15:
;	return str ;
	LD HL,5
	ADD HL,SP
	CALL ccgint
	INC SP
	RET
;}

; --- End of Compilation ---
