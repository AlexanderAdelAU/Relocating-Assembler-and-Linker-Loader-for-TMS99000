;* * *  Small-C/Plus  Version 1.06  * * *
;       Cain, Van Zandt, Hendrix, Yorston
;       21st April 1990
;
	module printf\printf
;/*
; *	printf - core routines for printf, sprintf, fprintf
; *	         used by both integer and f.p. versions
; *
; *	Compile with -m option
; *
; *	R M Yorston 1987
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
;int _printf(), _outc();
	global q_printf
	global q_outc
;/*
; #define NULL 0
; #define ERR -1
; */
;/*
; * printf(controlstring, arg, arg, ...)  or
; * sprintf(string, controlstring, arg, arg, ...) or
; * fprintf(file, controlstring, arg, arg, ...) -- formatted print
; *        operates as described by Kernighan & Ritchie
; *        only d, x, c, s, and u specs are supported.
; */
;char *_String;
q_String:	DEFS 2
	global q_String
;int _Count;
q_Count:	DEFS 2
	global q_Count
;printf(args)
	global qprintf
qprintf:
;	int args; {
;	_String = NULL;
	LD HL,0
	LD (q_String),HL
;	return (_printf(stdout, _argcnt() + &args -1));
	LD HL,1
	PUSH HL
	global q_argcnt
	CALL q_argcnt
	PUSH HL
	LD HL,6
	ADD HL,SP
	POP DE
	EX DE,HL
	ADD HL,HL
	ADD HL,DE
	DEC HL
	DEC HL
	PUSH HL
	CALL q_printf
	POP BC
	POP BC
	RET
;}
;fprintf(args)
	global qfprintf
qfprintf:
;	int args; {
;	int *nxtarg;
;	_String = NULL;
	PUSH BC
	LD HL,0
	LD (q_String),HL
;	nxtarg = _argcnt() + &args;
	CALL q_argcnt
	PUSH HL
	LD HL,6
	ADD HL,SP
	POP DE
	EX DE,HL
	ADD HL,HL
	ADD HL,DE
	POP BC
	PUSH HL
;	return (_printf(*(--nxtarg), --nxtarg));
	LD HL,0
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	DEC HL
	CALL ccpint
	CALL ccgint
	PUSH HL
	LD HL,2
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	DEC HL
	CALL ccpint
	PUSH HL
	CALL q_printf
	POP BC
	POP BC
	POP BC
	RET
;}
;sprintf(args)
	global qsprintf
qsprintf:
;	int args; {
;	int *nxtarg;
;	nxtarg = _argcnt() + &args - 1;
	PUSH BC
	CALL q_argcnt
	PUSH HL
	LD HL,6
	ADD HL,SP
	POP DE
	EX DE,HL
	ADD HL,HL
	ADD HL,DE
	DEC HL
	DEC HL
	POP BC
	PUSH HL
;	_String = *nxtarg;
	LD HL,0
	ADD HL,SP
	CALL ccgint
	CALL ccgint
	LD (q_String),HL
;	return (_printf( stdin, --nxtarg));
	LD HL,0
	PUSH HL
	LD HL,2
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	DEC HL
	CALL ccpint
	PUSH HL
	CALL q_printf
	POP BC
	POP BC
	POP BC
	RET
;}
;_printf(fd, nxtarg)
q_printf:
;	int fd;int *nxtarg; {
;	int i, prec, preclen, len;
;	char c, right, str[7], pad;
;	int width;
;	char *sptr, *ctl, *cx;
;	_Count = 0;
	LD HL,-26
	ADD HL,SP
	LD SP,HL
	LD HL,0
	LD (q_Count),HL
;	ctl = *nxtarg;
	LD HL,28
	ADD HL,SP
	CALL ccgint
	CALL ccgint
	POP DE
	POP BC
	PUSH HL
	PUSH DE
;	while (c = *ctl++) {
cc5:
	LD HL,17
	ADD HL,SP
	PUSH HL
	LD HL,4
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
	JP Z,cc6
;		if (c != '%') {
	LD HL,17
	ADD HL,SP
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,37
	POP DE
	CALL ccne
	LD A,L
	OR H
	JP Z,cc7
;			_outc(c, fd);
	LD HL,17
	ADD HL,SP
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,32
	ADD HL,SP
	CALL ccgint
	PUSH HL
	CALL q_outc
	POP BC
	POP BC
;			continue;
	JP cc5
;		}
;		if (*ctl == '%') {
cc7:
	LD HL,2
	ADD HL,SP
	CALL ccgint
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,37
	POP DE
	CALL cceq
	LD A,L
	OR H
	JP Z,cc8
;			_outc(*ctl++, fd);
	LD HL,2
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	DEC HL
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,32
	ADD HL,SP
	CALL ccgint
	PUSH HL
	CALL q_outc
	POP BC
	POP BC
;			continue;
	JP cc5
;		}
;		cx = ctl;
cc8:
	LD HL,2
	ADD HL,SP
	CALL ccgint
	POP BC
	PUSH HL
;		if (*cx == '-') {
	LD HL,0
	ADD HL,SP
	CALL ccgint
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,45
	POP DE
	CALL cceq
	LD A,L
	OR H
	JP Z,cc9
;			right = 0;
	LD HL,16
	ADD HL,SP
	PUSH HL
	LD HL,0
	POP DE
	LD A,L
	LD (DE),A
;			++cx;
	LD HL,0
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
;		} else
	JP cc10
cc9:
;			right = 1;
	LD HL,16
	ADD HL,SP
	PUSH HL
	LD HL,1
	POP DE
	LD A,L
	LD (DE),A
cc10:
;		if (*cx == '0') {
	LD HL,0
	ADD HL,SP
	CALL ccgint
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,48
	POP DE
	CALL cceq
	LD A,L
	OR H
	JP Z,cc11
;			pad = '0';
	LD HL,8
	ADD HL,SP
	PUSH HL
	LD HL,48
	POP DE
	LD A,L
	LD (DE),A
;			++cx;
	LD HL,0
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
;		} else
	JP cc12
cc11:
;			pad = ' ';
	LD HL,8
	ADD HL,SP
	PUSH HL
	LD HL,32
	POP DE
	LD A,L
	LD (DE),A
cc12:
;		if ((i = utoi(cx, &width)) >= 0)
	LD HL,24
	ADD HL,SP
	PUSH HL
	global qutoi
	LD HL,2
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,10
	ADD HL,SP
	PUSH HL
	CALL qutoi
	POP BC
	POP BC
	CALL ccpint
	XOR A
	OR H
	JP M,cc13
;			cx += i;
	LD HL,0
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,26
	ADD HL,SP
	CALL ccgint
	POP DE
	ADD HL,DE
	POP BC
	PUSH HL
;		else
	JP cc14
cc13:
;			continue;
	JP cc5
cc14:
;		if (*cx == '.') {
	LD HL,0
	ADD HL,SP
	CALL ccgint
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,46
	POP DE
	CALL cceq
	LD A,L
	OR H
	JP Z,cc15
;			if ((preclen = utoi(++cx, &prec)) >= 0)
	LD HL,20
	ADD HL,SP
	PUSH HL
	LD HL,2
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	PUSH HL
	LD HL,26
	ADD HL,SP
	PUSH HL
	CALL qutoi
	POP BC
	POP BC
	CALL ccpint
	XOR A
	OR H
	JP M,cc16
;				cx += preclen;
	LD HL,0
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,22
	ADD HL,SP
	CALL ccgint
	POP DE
	ADD HL,DE
	POP BC
	PUSH HL
;			else
	JP cc17
cc16:
;				continue;
	JP cc5
cc17:
;		} else
	JP cc18
cc15:
;			preclen = 0;
	LD HL,20
	ADD HL,SP
	PUSH HL
	LD HL,0
	CALL ccpint
cc18:
;		sptr = str;
	LD HL,4
	ADD HL,SP
	PUSH HL
	LD HL,11
	ADD HL,SP
	CALL ccpint
;		c = *cx++;
	LD HL,17
	ADD HL,SP
	PUSH HL
	LD HL,2
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
;		i = *(--nxtarg);
	LD HL,24
	ADD HL,SP
	PUSH HL
	LD HL,30
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	DEC HL
	CALL ccpint
	CALL ccgint
	CALL ccpint
;		switch (c) {
	LD HL,17
	ADD HL,SP
	LD L,(HL)
	LD H,0
	JP cc21
;		case 'd':
cc22:
;			itod(i, str, 7);
	global qitod
	LD HL,24
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,11
	ADD HL,SP
	PUSH HL
	LD HL,7
	PUSH HL
	CALL qitod
	POP BC
	POP BC
	POP BC
;			break;
	JP cc20
;		case 'x':
cc23:
;			itox(i, str, 7);
	global qitox
	LD HL,24
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,11
	ADD HL,SP
	PUSH HL
	LD HL,7
	PUSH HL
	CALL qitox
	POP BC
	POP BC
	POP BC
;			break;
	JP cc20
;		case 'c':
cc24:
;			str[0] = i;
	LD HL,9
	ADD HL,SP
	PUSH HL
	LD HL,26
	ADD HL,SP
	CALL ccgint
	POP DE
	LD A,L
	LD (DE),A
;			str[1] = NULL;
	LD HL,10
	ADD HL,SP
	PUSH HL
	LD HL,0
	POP DE
	LD A,L
	LD (DE),A
;			break;
	JP cc20
;		case 's':
cc25:
;			sptr = i;
	LD HL,4
	ADD HL,SP
	PUSH HL
	LD HL,26
	ADD HL,SP
	CALL ccgint
	CALL ccpint
;			break;
	JP cc20
;		case 'u':
cc26:
;			itou(i, str, 7);
	global qitou
	LD HL,24
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,11
	ADD HL,SP
	PUSH HL
	LD HL,7
	PUSH HL
	CALL qitou
	POP BC
	POP BC
	POP BC
;			break;
	JP cc20
;		default:
cc27:
;			continue;
	JP cc5
;		}
	JP cc20
cc21:
	CALL ccswitch
	DEFW cc22,100
	DEFW cc23,120
	DEFW cc24,99
	DEFW cc25,115
	DEFW cc26,117
	DEFW 0
	JP cc27
cc20:
;		ctl = cx; /* accept conversion spec */
	LD HL,0
	ADD HL,SP
	CALL ccgint
	POP DE
	POP BC
	PUSH HL
	PUSH DE
;		if (c != 's')
	LD HL,17
	ADD HL,SP
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,115
	POP DE
	CALL ccne
	LD A,L
	OR H
	JP Z,cc28
;			while (*sptr == ' ')
cc29:
	LD HL,4
	ADD HL,SP
	CALL ccgint
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,32
	POP DE
	CALL cceq
	LD A,L
	OR H
	JP Z,cc30
;				++sptr;
	LD HL,4
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	JP cc29
cc30:
;		len = -1;
cc28:
	LD HL,18
	ADD HL,SP
	PUSH HL
	LD HL,-1
	CALL ccpint
;		while (sptr[++len])
cc31:
	LD HL,4
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,20
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	POP DE
	ADD HL,DE
	LD L,(HL)
	LD H,0
	LD A,L
	OR H
	JP Z,cc32
;			; /* get length */
	JP cc31
cc32:
;		if (c == 's' && len > prec && preclen > 0)
	LD HL,17
	ADD HL,SP
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,115
	POP DE
	CALL cceq
	LD A,L
	OR H
	JP Z,cc34
	LD HL,18
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,24
	ADD HL,SP
	CALL ccgint
	POP DE
	CALL ccgt
	LD A,L
	OR H
	JP Z,cc34
	LD HL,20
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,0
	POP DE
	CALL ccgt
	LD A,L
	OR H
	JP Z,cc34
	LD HL,1
	JP cc35
cc34:
	LD HL,0
cc35:
	LD A,L
	OR H
	JP Z,cc33
;			len = prec;
	LD HL,18
	ADD HL,SP
	PUSH HL
	LD HL,24
	ADD HL,SP
	CALL ccgint
	CALL ccpint
;		if (right)
cc33:
	LD HL,16
	ADD HL,SP
	LD L,(HL)
	LD H,0
	LD A,L
	OR H
	JP Z,cc36
;			while (((width--) - len) > 0)
cc37:
	LD HL,6
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	CALL ccpint
	INC HL
	PUSH HL
	LD HL,20
	ADD HL,SP
	CALL ccgint
	POP DE
	CALL ccsub
	XOR A
	OR H
	JP M,cc38
	OR L
	JP Z,cc38
;				_outc(pad, fd);
	LD HL,8
	ADD HL,SP
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,32
	ADD HL,SP
	CALL ccgint
	PUSH HL
	CALL q_outc
	POP BC
	POP BC
	JP cc37
cc38:
;		while (len) {
cc36:
cc39:
	LD HL,18
	ADD HL,SP
	CALL ccgint
	LD A,L
	OR H
	JP Z,cc40
;			_outc(*sptr++, fd);
	LD HL,4
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	DEC HL
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,32
	ADD HL,SP
	CALL ccgint
	PUSH HL
	CALL q_outc
	POP BC
	POP BC
;			--len;
	LD HL,18
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	CALL ccpint
;			--width;
	LD HL,6
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	CALL ccpint
;		}
	JP cc39
cc40:
;		while (((width--) - len) > 0)
cc41:
	LD HL,6
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	CALL ccpint
	INC HL
	PUSH HL
	LD HL,20
	ADD HL,SP
	CALL ccgint
	POP DE
	CALL ccsub
	XOR A
	OR H
	JP M,cc42
	OR L
	JP Z,cc42
;			_outc(pad, fd);
	LD HL,8
	ADD HL,SP
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,32
	ADD HL,SP
	CALL ccgint
	PUSH HL
	CALL q_outc
	POP BC
	POP BC
	JP cc41
cc42:
;	}
	JP cc5
cc6:
;	if (_String != 0)
	LD HL,(q_String)
	LD A,L
	OR H
	JP Z,cc43
;		*_String = '\000';
	LD HL,(q_String)
	PUSH HL
	LD HL,0
	POP DE
	LD A,L
	LD (DE),A
;	return (_Count);
cc43:
	LD HL,(q_Count)
	EX DE,HL
	LD HL,26
	ADD HL,SP
	LD SP,HL
	EX DE,HL
	RET
;}
;/*
; * _outc - output a single character
; *         if _String is not null send output to a string instead
; */
;_outc(c, fd)
q_outc:
;	char c;int fd; {
;	if (_String == NULL)
	LD HL,(q_String)
	LD A,L
	OR H
	JP NZ,cc45
;		putc(c, fd);
	global qputc
	LD HL,4
	ADD HL,SP
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,4
	ADD HL,SP
	CALL ccgint
	PUSH HL
	CALL qputc
	POP BC
	POP BC
;	else
	JP cc46
cc45:
;		*_String++ = c;
	LD HL,(q_String)
	INC HL
	LD (q_String),HL
	DEC HL
	PUSH HL
	LD HL,6
	ADD HL,SP
	LD L,(HL)
	LD H,0
	POP DE
	LD A,L
	LD (DE),A
cc46:
;	++_Count;
	LD HL,(q_Count)
	INC HL
	LD (q_Count),HL
;}
	RET

; --- End of Compilation ---
