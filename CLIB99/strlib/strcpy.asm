;* * *  Small-C/Plus  Version 1.06  * * *
;       Cain, Van Zandt, Hendrix, Yorston
;       21st April 1990
;
	module strlib\strcpy
;/*
; *	STRING FUNCTIONS FOR SMALL C
; * 	BASED ON CORRESPONDING UNIX FUNCTIONS
; */
;#include <stdio.h>
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
;//#include <string.h>
;strcpy( to, from )
	global qstrcpy
qstrcpy:
;char *to, *from ;
;{
;	char *temp ;
;	temp = to ;
	PUSH BC
	LD HL,6
	ADD HL,SP
	CALL ccgint
	POP BC
	PUSH HL
;	while( *to++ = *from++ ) ;
cc2:
	LD HL,6
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	DEC HL
	PUSH HL
	LD HL,6
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	DEC HL
	LD L,(HL)
	LD H,0
	POP DE
	LD A,L
	LD (DE),A
	LD A,L
	OR H
	JP Z,cc3
	JP cc2
cc3:
;	return temp ;
	LD HL,0
	ADD HL,SP
	CALL ccgint
	POP BC
	RET
;}
;/*
;#asm
;qstrcpy:
;	POP HL
;	POP DE		;DE is from
;	POP BC		;BC is to
;	PUSH BC
;	PUSH DE
;	PUSH HL
;	LD H,B		;return to
;	LD L,C
;ccstr2:
;	LD A,(DE)
;	LD (BC),A
;	INC DE
;	INC BC
;	OR A		;test char for zero
;	JP NZ,ccstr2
;	RET
;#endasm
;*/

; --- End of Compilation ---
