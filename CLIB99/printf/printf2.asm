;* * *  Small-C/Plus  Version 1.06  * * *
;       Cain, Van Zandt, Hendrix, Yorston
;       21st April 1990
;
	module printf\printf2
;/*
; *	printf - core routines for printf, sprintf, fprintf
; *	        this is the floating point version
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
;#include "float.h"
;/*
; * Floating Point declarations to ensure the Small C compiler generates the correct
; * handling for the return variables
; *
; */
;extern double floor();
	global qfloor
;extern double float();
	global qfloat
;extern int ifix();
	global qifix
;extern double sin();
	global qsin
;extern double cos();
	global qcos
;extern double tan();
	global qtan
;extern double pow();
	global qpow
;extern double fabs();
	global qfabs
;extern double log();
	global qlog
;extern double log10();
	global qlog10
;extern double exp();
	global qexp
;extern double fmod();
	global qfmod
;extern double sqrt();
	global qsqrt
;extern double atan();
	global qatan
;int _printf(), _outc(), ftoa(), ftoe();
	global q_printf
	global q_outc
	global qftoa
	global qftoe
;/* int _printf(), ftoa(),ftoe(), _outc(); */
;/*
; * printf(controlstring, arg, arg, ...)  or
; * sprintf(string, controlstring, arg, arg, ...) or
; * fprintf(file, controlstring, arg, arg, ...) -- formatted print
; *        operates as described by Kernighan & Ritchie
; *        only f,d, x, c, s, and u specs are supported.
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
;	return (_printf(stdout, _argcnt() + &args-1));
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
;int fd ;
;int *nxtarg ;
;{
;	double *pd ;
;	int i, width, prec, preclen, len ;
;	char c, right, str[128], pad;
;	char *sptr, *ctl, *cx ;
;	_Count = 0 ;
	LD HL,-149
	ADD HL,SP
	LD SP,HL
	LD HL,0
	LD (q_Count),HL
;	ctl = *nxtarg ;
	LD HL,151
	ADD HL,SP
	CALL ccgint
	CALL ccgint
	POP DE
	POP BC
	PUSH HL
	PUSH DE
;	while ( c = *ctl++ ) {
cc5:
	LD HL,136
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
;		if (c != '%' ) {
	LD HL,136
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
;			_outc(c, fd) ;
	LD HL,136
	ADD HL,SP
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,155
	ADD HL,SP
	CALL ccgint
	PUSH HL
	CALL q_outc
	POP BC
	POP BC
;			continue ;
	JP cc5
;		}
;		if ( *ctl == '%' ) {
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
;			_outc(*ctl++, fd) ;
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
	LD HL,155
	ADD HL,SP
	CALL ccgint
	PUSH HL
	CALL q_outc
	POP BC
	POP BC
;			continue ;
	JP cc5
;		}
;		cx = ctl ;
cc8:
	LD HL,2
	ADD HL,SP
	CALL ccgint
	POP BC
	PUSH HL
;		if ( *cx == '-' ) {
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
;			right = 0 ;
	LD HL,135
	ADD HL,SP
	PUSH HL
	LD HL,0
	POP DE
	LD A,L
	LD (DE),A
;			++cx ;
	LD HL,0
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
;		}
;		else
	JP cc10
cc9:
;			right = 1 ;
	LD HL,135
	ADD HL,SP
	PUSH HL
	LD HL,1
	POP DE
	LD A,L
	LD (DE),A
cc10:
;		if ( *cx == '0' ) {
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
;			pad = '0' ;
	LD HL,6
	ADD HL,SP
	PUSH HL
	LD HL,48
	POP DE
	LD A,L
	LD (DE),A
;			++cx ;
	LD HL,0
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
;		}
;		else
	JP cc12
cc11:
;			pad = ' ' ;
	LD HL,6
	ADD HL,SP
	PUSH HL
	LD HL,32
	POP DE
	LD A,L
	LD (DE),A
cc12:
;		if ( (i=utoi(cx, &width)) >= 0 )
	LD HL,145
	ADD HL,SP
	PUSH HL
	global qutoi
	LD HL,2
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,147
	ADD HL,SP
	PUSH HL
	CALL qutoi
	POP BC
	POP BC
	CALL ccpint
	XOR A
	OR H
	JP M,cc13
;			cx += i ;
	LD HL,0
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,147
	ADD HL,SP
	CALL ccgint
	POP DE
	ADD HL,DE
	POP BC
	PUSH HL
;		else
	JP cc14
cc13:
;			continue  ;
	JP cc5
cc14:
;		/* preclen is precision length */
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
;			if ( (preclen=utoi(++cx, &prec)) >= 0 )
	LD HL,139
	ADD HL,SP
	PUSH HL
	LD HL,2
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	PUSH HL
	LD HL,145
	ADD HL,SP
	PUSH HL
	CALL qutoi
	POP BC
	POP BC
	CALL ccpint
	XOR A
	OR H
	JP M,cc16
;				cx += preclen ;
	LD HL,0
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,141
	ADD HL,SP
	CALL ccgint
	POP DE
	ADD HL,DE
	POP BC
	PUSH HL
;			else
	JP cc17
cc16:
;				continue ;
	JP cc5
cc17:
;		}
;		else
	JP cc18
cc15:
;			preclen = 0 ;
	LD HL,139
	ADD HL,SP
	PUSH HL
	LD HL,0
	CALL ccpint
cc18:
;		sptr = str ;
	LD HL,4
	ADD HL,SP
	PUSH HL
	LD HL,9
	ADD HL,SP
	CALL ccpint
;		c = *cx++ ;
	LD HL,136
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
;		i = *(--nxtarg) ;
	LD HL,145
	ADD HL,SP
	PUSH HL
	LD HL,153
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	DEC HL
	CALL ccpint
	CALL ccgint
	CALL ccpint
;		switch(c) {
	LD HL,136
	ADD HL,SP
	LD L,(HL)
	LD H,0
	JP cc21
;		putchar(c);
	global qputchar
	LD HL,136
	ADD HL,SP
	LD L,(HL)
	LD H,0
	PUSH HL
	CALL qputchar
	POP BC
;			case 'd' :
cc22:
;			case 'i' :
cc23:
;				itod(i, str, 7) ;
	global qitod
	LD HL,145
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,9
	ADD HL,SP
	PUSH HL
	LD HL,7
	PUSH HL
	CALL qitod
	POP BC
	POP BC
	POP BC
;				break ;
	JP cc20
;			case 'x' :
cc24:
;				itox(i, str, 7) ;
	global qitox
	LD HL,145
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,9
	ADD HL,SP
	PUSH HL
	LD HL,7
	PUSH HL
	CALL qitox
	POP BC
	POP BC
	POP BC
;				break ;
	JP cc20
;			case 'c' :
cc25:
;				str[0] = i ;
	LD HL,7
	ADD HL,SP
	PUSH HL
	LD HL,147
	ADD HL,SP
	CALL ccgint
	POP DE
	LD A,L
	LD (DE),A
;				str[1] = NULL ;
	LD HL,8
	ADD HL,SP
	PUSH HL
	LD HL,0
	POP DE
	LD A,L
	LD (DE),A
;				break ;
	JP cc20
;			case 's' :
cc26:
;				sptr = i ;
	LD HL,4
	ADD HL,SP
	PUSH HL
	LD HL,147
	ADD HL,SP
	CALL ccgint
	CALL ccpint
;				break ;
	JP cc20
;			case 'u' :
cc27:
;				itou(i, str, 7) ;
	global qitou
	LD HL,145
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,9
	ADD HL,SP
	PUSH HL
	LD HL,7
	PUSH HL
	CALL qitou
	POP BC
	POP BC
	POP BC
;				break ;
	JP cc20
;			default:
cc28:
;				if ( preclen == 0 )
	LD HL,139
	ADD HL,SP
	CALL ccgint
	LD A,L
	OR H
	JP NZ,cc29
;					prec = 6 ;
	LD HL,141
	ADD HL,SP
	PUSH HL
	LD HL,6
	CALL ccpint
;				if ( c == 'f' ) {
cc29:
	LD HL,136
	ADD HL,SP
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,102
	POP DE
	CALL cceq
	LD A,L
	OR H
	JP Z,cc30
;					nxtarg -= 2 ;
	LD HL,151
	ADD HL,SP
	PUSH HL
	CALL ccgint
	LD DE,-4
	ADD HL,DE
	CALL ccpint
;					pd = nxtarg ;
	LD HL,147
	ADD HL,SP
	PUSH HL
	LD HL,153
	ADD HL,SP
	CALL ccgint
	CALL ccpint
;					ftoa( *pd, prec, str ) ;
	LD HL,147
	ADD HL,SP
	CALL ccgint
	CALL dload
	CALL dpush
	LD HL,147
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,15
	ADD HL,SP
	PUSH HL
	CALL qftoa
	EX DE,HL
	LD HL,10
	ADD HL,SP
	LD SP,HL
	EX DE,HL
;				}
;				else if ( c == 'e' ) {
	JP cc31
cc30:
	LD HL,136
	ADD HL,SP
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,101
	POP DE
	CALL cceq
	LD A,L
	OR H
	JP Z,cc32
;					nxtarg -= 2 ;
	LD HL,151
	ADD HL,SP
	PUSH HL
	CALL ccgint
	LD DE,-4
	ADD HL,DE
	CALL ccpint
;					pd = nxtarg ;
	LD HL,147
	ADD HL,SP
	PUSH HL
	LD HL,153
	ADD HL,SP
	CALL ccgint
	CALL ccpint
;					ftoe( *pd, prec, str ) ;
	LD HL,147
	ADD HL,SP
	CALL ccgint
	CALL dload
	CALL dpush
	LD HL,147
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,15
	ADD HL,SP
	PUSH HL
	CALL qftoe
	EX DE,HL
	LD HL,10
	ADD HL,SP
	LD SP,HL
	EX DE,HL
;				}
;				else
	JP cc33
cc32:
;					continue ;
	JP cc5
cc33:
cc31:
;		}
	JP cc20
cc21:
	CALL ccswitch
	DEFW cc22,100
	DEFW cc23,105
	DEFW cc24,120
	DEFW cc25,99
	DEFW cc26,115
	DEFW cc27,117
	DEFW 0
	JP cc28
cc20:
;		ctl = cx ; /* accept conversion spec */
	LD HL,0
	ADD HL,SP
	CALL ccgint
	POP DE
	POP BC
	PUSH HL
	PUSH DE
;		if ( c != 's' )
	LD HL,136
	ADD HL,SP
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,115
	POP DE
	CALL ccne
	LD A,L
	OR H
	JP Z,cc34
;			while ( *sptr == ' ' )
cc35:
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
	JP Z,cc36
;				++sptr ;
	LD HL,4
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	JP cc35
cc36:
;		len = -1 ;
cc34:
	LD HL,137
	ADD HL,SP
	PUSH HL
	LD HL,-1
	CALL ccpint
;		while ( sptr[++len] )
cc37:
	LD HL,4
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,139
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
	JP Z,cc38
;			; /* get length */
	JP cc37
cc38:
;		if ( c == 's' && len>prec && preclen>0 )
	LD HL,136
	ADD HL,SP
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,115
	POP DE
	CALL cceq
	LD A,L
	OR H
	JP Z,cc40
	LD HL,137
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,143
	ADD HL,SP
	CALL ccgint
	POP DE
	CALL ccgt
	LD A,L
	OR H
	JP Z,cc40
	LD HL,139
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,0
	POP DE
	CALL ccgt
	LD A,L
	OR H
	JP Z,cc40
	LD HL,1
	JP cc41
cc40:
	LD HL,0
cc41:
	LD A,L
	OR H
	JP Z,cc39
;			len = prec ;
	LD HL,137
	ADD HL,SP
	PUSH HL
	LD HL,143
	ADD HL,SP
	CALL ccgint
	CALL ccpint
;		if (right)
cc39:
	LD HL,135
	ADD HL,SP
	LD L,(HL)
	LD H,0
	LD A,L
	OR H
	JP Z,cc42
;			while ( ((width--)-len) > 0 )
cc43:
	LD HL,143
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	CALL ccpint
	INC HL
	PUSH HL
	LD HL,139
	ADD HL,SP
	CALL ccgint
	POP DE
	CALL ccsub
	XOR A
	OR H
	JP M,cc44
	OR L
	JP Z,cc44
;				_outc(pad, fd) ;
	LD HL,6
	ADD HL,SP
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,155
	ADD HL,SP
	CALL ccgint
	PUSH HL
	CALL q_outc
	POP BC
	POP BC
	JP cc43
cc44:
;		while ( len ) {
cc42:
cc45:
	LD HL,137
	ADD HL,SP
	CALL ccgint
	LD A,L
	OR H
	JP Z,cc46
;			_outc(*sptr++, fd) ;
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
	LD HL,155
	ADD HL,SP
	CALL ccgint
	PUSH HL
	CALL q_outc
	POP BC
	POP BC
;			--len ;
	LD HL,137
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	CALL ccpint
;			--width ;
	LD HL,143
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	CALL ccpint
;		}
	JP cc45
cc46:
;		while ( ((width--)-len) > 0 )
cc47:
	LD HL,143
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	CALL ccpint
	INC HL
	PUSH HL
	LD HL,139
	ADD HL,SP
	CALL ccgint
	POP DE
	CALL ccsub
	XOR A
	OR H
	JP M,cc48
	OR L
	JP Z,cc48
;			_outc(pad, fd) ;
	LD HL,6
	ADD HL,SP
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,155
	ADD HL,SP
	CALL ccgint
	PUSH HL
	CALL q_outc
	POP BC
	POP BC
	JP cc47
cc48:
;	}
	JP cc5
cc6:
;	if (_String != 0) *_String = '\000' ;
	LD HL,(q_String)
	LD A,L
	OR H
	JP Z,cc49
	LD HL,(q_String)
	PUSH HL
	LD HL,0
	POP DE
	LD A,L
	LD (DE),A
;	return(_Count) ;
cc49:
	LD HL,(q_Count)
	EX DE,HL
	LD HL,149
	ADD HL,SP
	LD SP,HL
	EX DE,HL
	RET
;}
;/* convert double number to string (f format) */
;ftoa(x,f,str)
qftoa:
;double x;	/* the number to be converted */
;int f;		/* number of digits to follow decimal point */
;char *str;	/* output string */
;{
;	double scale;		/* scale factor */
;	int i,				/* copy of f, and # digits before decimal point */
;		d;				/* a digit */
;	if( x < 0.0 ) {
	LD HL,-10
	ADD HL,SP
	LD SP,HL
	LD HL,16
	ADD HL,SP
	CALL dload
	CALL dpush
	LD HL,0
	CALL qfloat
	CALL dlt
	LD A,L
	OR H
	JP Z,cc51
;		*str++ = '-' ;
	LD HL,12
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	DEC HL
	PUSH HL
	LD HL,45
	POP DE
	LD A,L
	LD (DE),A
;		x = -x ;
	LD HL,16
	ADD HL,SP
	PUSH HL
	LD HL,18
	ADD HL,SP
	CALL dload
	CALL minusfa
	POP HL
	CALL dstore
;	}
;	i = f ;
cc51:
	LD HL,14
	ADD HL,SP
	CALL ccgint
	POP DE
	POP BC
	PUSH HL
	PUSH DE
;	scale = 2.0 ;
	LD HL,4
	ADD HL,SP
	PUSH HL
	LD HL,2
	CALL qfloat
	POP HL
	CALL dstore
;	while ( i-- )
cc52:
	LD HL,2
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	CALL ccpint
	INC HL
	LD A,L
	OR H
	JP Z,cc53
;		scale *= 10.0 ;
	LD HL,4
	ADD HL,SP
	PUSH HL
	CALL dload
	CALL dpush
	LD HL,10
	CALL qfloat
	CALL dmul
	POP HL
	CALL dstore
	JP cc52
cc53:
;	x += 1.0 / scale ;
	LD HL,16
	ADD HL,SP
	PUSH HL
	CALL dload
	CALL dpush
	LD HL,12
	ADD HL,SP
	CALL dload
	LD DE,1
	CALL dpush2
	POP HL
	CALL qfloat
	CALL dswap
	CALL ddiv
	CALL dadd
	POP HL
	CALL dstore
;	/* count places before decimal & scale the number */
;	i = 0 ;
	LD HL,0
	POP BC
	PUSH HL
;	scale = 1.0 ;
	LD HL,2
	ADD HL,SP
	PUSH HL
	LD HL,1
	CALL qfloat
	POP HL
	CALL dstore
;	while ( x >= scale ) {
cc54:
	LD HL,14
	ADD HL,SP
	CALL dload
	CALL dpush
	LD HL,8
	ADD HL,SP
	CALL dload
	CALL dge
	LD A,L
	OR H
	JP Z,cc55
;		scale *= 10.0 ;
	LD HL,2
	ADD HL,SP
	PUSH HL
	CALL dload
	CALL dpush
	LD HL,10
	CALL qfloat
	CALL dmul
	POP HL
	CALL dstore
;		i++ ;
	LD HL,0
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	DEC HL
;	}
	JP cc54
cc55:
;	while ( i-- ) {
cc56:
	LD HL,0
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	CALL ccpint
	INC HL
	LD A,L
	OR H
	JP Z,cc57
;		/* output digits before decimal */
;		scale = floor(0.5 + scale * 0.1 ) ;
	LD HL,2
	ADD HL,SP
	PUSH HL
	LD HL,4
	ADD HL,SP
	CALL dload
	CALL dpush
	LD HL,0
	CALL qfloat
	CALL dmul
	LD DE,0
	ADD HL,DE
	PUSH HL
	CALL qfloor
	POP BC
	POP HL
	CALL dstore
;		d = ifix( x / scale ) ;
	LD HL,-2
	ADD HL,SP
	PUSH HL
	LD HL,16
	ADD HL,SP
	CALL dload
	CALL dpush
	LD HL,10
	ADD HL,SP
	CALL dload
	CALL ddiv
	CALL dpush
	CALL qifix
	POP BC
	POP BC
	POP BC
	POP BC
	PUSH HL
;		*str++ = d + '0' ;
	LD HL,12
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	DEC HL
	PUSH HL
	LD HL,2
	ADD HL,SP
	CALL ccgint
	LD DE,48
	ADD HL,DE
	POP DE
	LD A,L
	LD (DE),A
;		x -= float(d) * scale ;
	LD HL,16
	ADD HL,SP
	PUSH HL
	CALL dload
	CALL dpush
	LD HL,8
	ADD HL,SP
	CALL ccgint
	PUSH HL
	CALL qfloat
	POP BC
	CALL dpush
	LD HL,18
	ADD HL,SP
	CALL dload
	CALL dmul
	CALL dsub
	POP HL
	CALL dstore
;	}
	POP BC
	JP cc56
cc57:
;	if ( f <= 0 ) {
	LD HL,12
	ADD HL,SP
	CALL ccgint
	LD A,L
	OR H
	JR Z,$+7
	XOR A
	OR H
	JP P,cc58
;		*str = NULL ;
	LD HL,10
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,0
	POP DE
	LD A,L
	LD (DE),A
;		return ;
	LD HL,8
	ADD HL,SP
	LD SP,HL
	RET
;	}
;	*str++ = '.' ;
cc58:
	LD HL,10
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	DEC HL
	PUSH HL
	LD HL,46
	POP DE
	LD A,L
	LD (DE),A
;	while ( f-- ) {
cc59:
	LD HL,12
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	CALL ccpint
	INC HL
	LD A,L
	OR H
	JP Z,cc60
;		/* output digits after decimal */
;		x *= 10.0 ;
	LD HL,14
	ADD HL,SP
	PUSH HL
	CALL dload
	CALL dpush
	LD HL,10
	CALL qfloat
	CALL dmul
	POP HL
	CALL dstore
;		d = ifix(x) ;
	LD HL,-2
	ADD HL,SP
	PUSH HL
	LD HL,16
	ADD HL,SP
	CALL dload
	CALL dpush
	CALL qifix
	POP BC
	POP BC
	POP BC
	POP BC
	PUSH HL
;		*str++ = d + '0' ;
	LD HL,12
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	DEC HL
	PUSH HL
	LD HL,2
	ADD HL,SP
	CALL ccgint
	LD DE,48
	ADD HL,DE
	POP DE
	LD A,L
	LD (DE),A
;		x -= float(d) ;
	LD HL,16
	ADD HL,SP
	PUSH HL
	CALL dload
	CALL dpush
	LD HL,8
	ADD HL,SP
	CALL ccgint
	PUSH HL
	CALL qfloat
	POP BC
	CALL dsub
	POP HL
	CALL dstore
;	}
	POP BC
	JP cc59
cc60:
;	*str = NULL ;
	LD HL,10
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,0
	POP DE
	LD A,L
	LD (DE),A
;}
	LD HL,8
	ADD HL,SP
	LD SP,HL
	RET
;/*	e format conversion			*/
;ftoe(x,prec,str)
qftoe:
;double x ;		/* number to be converted */
;int prec ;		/* # digits after decimal place */
;char *str ;		/* output string */
;{
;	double scale;	/* scale factor */
;	int i,			/* counter */
;		d,			/* a digit */
;		expon;		/* exponent */
;	scale = 1.0 ;		/* scale = 10 ** prec */
	LD HL,-12
	ADD HL,SP
	LD SP,HL
	LD HL,6
	ADD HL,SP
	PUSH HL
	LD HL,1
	CALL qfloat
	POP HL
	CALL dstore
;	i = prec ;
	LD HL,4
	ADD HL,SP
	PUSH HL
	LD HL,18
	ADD HL,SP
	CALL ccgint
	CALL ccpint
;	while ( i-- )
cc62:
	LD HL,4
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	CALL ccpint
	INC HL
	LD A,L
	OR H
	JP Z,cc63
;	scale *= 10.0 ;
	LD HL,6
	ADD HL,SP
	PUSH HL
	CALL dload
	CALL dpush
	LD HL,10
	CALL qfloat
	CALL dmul
	POP HL
	CALL dstore
	JP cc62
cc63:
;	if ( x == 0.0 ) {
	LD HL,18
	ADD HL,SP
	CALL dload
	CALL dpush
	LD HL,0
	CALL qfloat
	CALL deq
	LD A,L
	OR H
	JP Z,cc64
;		expon = 0 ;
	LD HL,0
	POP BC
	PUSH HL
;		scale *= 10.0 ;
	LD HL,6
	ADD HL,SP
	PUSH HL
	CALL dload
	CALL dpush
	LD HL,10
	CALL qfloat
	CALL dmul
	POP HL
	CALL dstore
;	}
;	else {
	JP cc65
cc64:
;		expon = prec ;
	LD HL,16
	ADD HL,SP
	CALL ccgint
	POP BC
	PUSH HL
;		if ( x < 0.0 ) {
	LD HL,18
	ADD HL,SP
	CALL dload
	CALL dpush
	LD HL,0
	CALL qfloat
	CALL dlt
	LD A,L
	OR H
	JP Z,cc66
;			*str++ = '-' ;
	LD HL,14
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	DEC HL
	PUSH HL
	LD HL,45
	POP DE
	LD A,L
	LD (DE),A
;			x = -x ;
	LD HL,18
	ADD HL,SP
	PUSH HL
	LD HL,20
	ADD HL,SP
	CALL dload
	CALL minusfa
	POP HL
	CALL dstore
;		}
;		if ( x > scale ) {
cc66:
	LD HL,18
	ADD HL,SP
	CALL dload
	CALL dpush
	LD HL,12
	ADD HL,SP
	CALL dload
	CALL dgt
	LD A,L
	OR H
	JP Z,cc67
;			/* need: scale<x<scale*10 */
;			scale *= 10.0 ;
	LD HL,6
	ADD HL,SP
	PUSH HL
	CALL dload
	CALL dpush
	LD HL,10
	CALL qfloat
	CALL dmul
	POP HL
	CALL dstore
;			while ( x >= scale ) {
cc68:
	LD HL,18
	ADD HL,SP
	CALL dload
	CALL dpush
	LD HL,12
	ADD HL,SP
	CALL dload
	CALL dge
	LD A,L
	OR H
	JP Z,cc69
;				x /= 10.0 ;
	LD HL,18
	ADD HL,SP
	PUSH HL
	CALL dload
	CALL dpush
	LD HL,10
	CALL qfloat
	CALL ddiv
	POP HL
	CALL dstore
;				++expon ;
	LD HL,0
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
;			}
	JP cc68
cc69:
;		}
;		else {
	JP cc70
cc67:
;			while ( x < scale ) {
cc71:
	LD HL,18
	ADD HL,SP
	CALL dload
	CALL dpush
	LD HL,12
	ADD HL,SP
	CALL dload
	CALL dlt
	LD A,L
	OR H
	JP Z,cc72
;				x *= 10.0 ;
	LD HL,18
	ADD HL,SP
	PUSH HL
	CALL dload
	CALL dpush
	LD HL,10
	CALL qfloat
	CALL dmul
	POP HL
	CALL dstore
;				--expon ;
	LD HL,0
	ADD HL,SP
	PUSH HL
	CALL ccgint
	DEC HL
	CALL ccpint
;			}
	JP cc71
cc72:
;			scale *= 10.0 ;
	LD HL,6
	ADD HL,SP
	PUSH HL
	CALL dload
	CALL dpush
	LD HL,10
	CALL qfloat
	CALL dmul
	POP HL
	CALL dstore
;		}
cc70:
;		/* at this point, .1*scale <= x < scale */
;		x += 0.5 ;			/* round */
	LD HL,18
	ADD HL,SP
	PUSH HL
	CALL dload
	POP HL
	CALL dstore
;		if ( x >= scale ) {
	LD HL,22
	ADD HL,SP
	CALL dload
	CALL dpush
	LD HL,16
	ADD HL,SP
	CALL dload
	CALL dge
	LD A,L
	OR H
	JP Z,cc73
;			x /= 10.0 ;
	LD HL,22
	ADD HL,SP
	PUSH HL
	CALL dload
	CALL dpush
	LD HL,10
	CALL qfloat
	CALL ddiv
	POP HL
	CALL dstore
;			++expon ;
	LD HL,4
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
;		}
;	}
cc73:
	POP BC
	POP BC
cc65:
;	i = 0 ;
	LD HL,4
	ADD HL,SP
	PUSH HL
	LD HL,0
	CALL ccpint
;	while ( i <= prec ) {
cc74:
	LD HL,4
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,18
	ADD HL,SP
	CALL ccgint
	POP DE
	CALL ccle
	LD A,L
	OR H
	JP Z,cc75
;		scale = floor( 0.5 + scale * 0.1 ) ;
	LD HL,6
	ADD HL,SP
	PUSH HL
	LD HL,8
	ADD HL,SP
	CALL dload
	CALL dpush
	LD HL,0
	CALL qfloat
	CALL dmul
	LD DE,0
	ADD HL,DE
	PUSH HL
	CALL qfloor
	POP BC
	POP HL
	CALL dstore
;		/* now, scale <= x < 10*scale */
;		d = ifix( x / scale ) ;
	LD HL,18
	ADD HL,SP
	CALL dload
	CALL dpush
	LD HL,12
	ADD HL,SP
	CALL dload
	CALL ddiv
	CALL dpush
	CALL qifix
	POP BC
	POP BC
	POP BC
	POP DE
	POP BC
	PUSH HL
	PUSH DE
;		*str++ = d + '0' ;
	LD HL,14
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	DEC HL
	PUSH HL
	LD HL,4
	ADD HL,SP
	CALL ccgint
	LD DE,48
	ADD HL,DE
	POP DE
	LD A,L
	LD (DE),A
;		x -= float(d) * scale ;
	LD HL,18
	ADD HL,SP
	PUSH HL
	CALL dload
	CALL dpush
	LD HL,10
	ADD HL,SP
	CALL ccgint
	PUSH HL
	CALL qfloat
	POP BC
	CALL dpush
	LD HL,20
	ADD HL,SP
	CALL dload
	CALL dmul
	CALL dsub
	POP HL
	CALL dstore
;		if ( i++ ) continue ;
	LD HL,4
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	DEC HL
	LD A,L
	OR H
	JP Z,cc76
	JP cc74
;		*str++ = '.' ;
cc76:
	LD HL,14
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	DEC HL
	PUSH HL
	LD HL,46
	POP DE
	LD A,L
	LD (DE),A
;	}
	JP cc74
cc75:
;	*str++ = 'e' ;
	LD HL,14
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	DEC HL
	PUSH HL
	LD HL,101
	POP DE
	LD A,L
	LD (DE),A
;	if ( expon < 0 ) { *str++ = '-' ; expon = -expon ; }
	LD HL,0
	ADD HL,SP
	CALL ccgint
	XOR A
	OR H
	JP P,cc77
	LD HL,14
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	DEC HL
	PUSH HL
	LD HL,45
	POP DE
	LD A,L
	LD (DE),A
	LD HL,0
	ADD HL,SP
	CALL ccgint
	CALL ccneg
	POP BC
	PUSH HL
;	if(expon>9) *str++ = '0' + expon/10 ;
cc77:
	LD HL,0
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,9
	POP DE
	CALL ccgt
	LD A,L
	OR H
	JP Z,cc78
	LD HL,14
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	DEC HL
	PUSH HL
	LD HL,2
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,10
	POP DE
	CALL ccdiv
	LD DE,48
	ADD HL,DE
	POP DE
	LD A,L
	LD (DE),A
;	*str++ = '0' + expon % 10 ;
cc78:
	LD HL,14
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	DEC HL
	PUSH HL
	LD HL,2
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
;	*str = NULL;
	LD HL,14
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,0
	POP DE
	LD A,L
	LD (DE),A
;}
	LD HL,12
	ADD HL,SP
	LD SP,HL
	RET
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
	JP NZ,cc80
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
	JP cc81
cc80:
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
cc81:
;	++_Count;
	LD HL,(q_Count)
	INC HL
	LD (q_Count),HL
;}
	RET

; --- End of Compilation ---
