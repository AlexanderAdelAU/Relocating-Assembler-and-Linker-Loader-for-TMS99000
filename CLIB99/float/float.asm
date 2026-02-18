;* * *  Small-C/Plus  Version 1.06  * * *
;       Cain, Van Zandt, Hendrix, Yorston
;       21st April 1990
;
	module float\float
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
;extern double _ccfloor();
	global q_ccfloor
;extern double _ccfloat();
	global q_ccfloat
;extern double _ccpow();
	global q_ccpow
;extern double _ccsin();
	global q_ccsin
;extern double _cctan();
	global q_cctan
;extern double _ccatan();
	global q_ccatan
;extern double _cccos();
	global q_cccos
;extern double _ccifix();
	global q_ccifix
;extern double _ccfabs();
	global q_ccfabs
;extern double _cclog();
	global q_cclog
;extern double _ccexp();
	global q_ccexp
;extern double _ccsqrt();
	global q_ccsqrt
;extern double _ccmod();
	global q_ccmod
;extern double _cclog10();
	global q_cclog10
;/*
; *	These are the Floating Point prototype functions declarations that call the
; *	floating point libary entry points.
; */
;double floor (x)
qfloor:
;double x;
;{
;	return _ccfloor(x);
	LD HL,2
	ADD HL,SP
	CALL dload
	CALL dpush
	CALL q_ccfloor
	POP BC
	POP BC
	POP BC
	RET
;}
;double float (x)
qfloat:
;int x;
;{
;	return _ccfloat(x);
	LD HL,2
	ADD HL,SP
	CALL ccgint
	PUSH HL
	CALL q_ccfloat
	POP BC
	RET
;}
;int ifix (x)
qifix:
;double x;
;{
;	return _ccifix(x);
	LD HL,2
	ADD HL,SP
	CALL dload
	CALL dpush
	CALL q_ccifix
	POP BC
	POP BC
	POP BC
	CALL qifix
	RET
;}
;double sin(x)
qsin:
;double x;
;{
;	return _ccsin(x);
	LD HL,2
	ADD HL,SP
	CALL dload
	CALL dpush
	CALL q_ccsin
	POP BC
	POP BC
	POP BC
	RET
;}
;double cos(x)
qcos:
;double x;
;{
;	return _cccos(x);
	LD HL,2
	ADD HL,SP
	CALL dload
	CALL dpush
	CALL q_cccos
	POP BC
	POP BC
	POP BC
	RET
;}
;double tan(x)
qtan:
;double x;
;{
;	return _cctan(x);
	LD HL,2
	ADD HL,SP
	CALL dload
	CALL dpush
	CALL q_cctan
	POP BC
	POP BC
	POP BC
	RET
;}
;double atan(x)
qatan:
;double x;
;{
;	return _ccatan(x);
	LD HL,2
	ADD HL,SP
	CALL dload
	CALL dpush
	CALL q_ccatan
	POP BC
	POP BC
	POP BC
	RET
;}
;double fabs(x)
qfabs:
;double x;
;{
;	return _ccfabs(x);
	LD HL,2
	ADD HL,SP
	CALL dload
	CALL dpush
	CALL q_ccfabs
	POP BC
	POP BC
	POP BC
	RET
;}
;double log(x)
qlog:
;double x;
;{
;	return _cclog(x);
	LD HL,2
	ADD HL,SP
	CALL dload
	CALL dpush
	CALL q_cclog
	POP BC
	POP BC
	POP BC
	RET
;}
;double log10(x)
qlog10:
;double x;
;{
;	return _cclog10(x);
	LD HL,2
	ADD HL,SP
	CALL dload
	CALL dpush
	CALL q_cclog10
	POP BC
	POP BC
	POP BC
	RET
;}
;double exp(x)
qexp:
;double x;
;{
;	return _ccexp(x);
	LD HL,2
	ADD HL,SP
	CALL dload
	CALL dpush
	CALL q_ccexp
	POP BC
	POP BC
	POP BC
	RET
;}
;double sqrt(x)
qsqrt:
;double x;
;{
;	return _ccsqrt(x);
	LD HL,2
	ADD HL,SP
	CALL dload
	CALL dpush
	CALL q_ccsqrt
	POP BC
	POP BC
	POP BC
	RET
;}
;double pow(x,y)
qpow:
;double x;double y;
;{
;	return _ccpow(x,y);
	LD HL,8
	ADD HL,SP
	CALL dload
	CALL dpush
	LD HL,8
	ADD HL,SP
	CALL dload
	CALL dpush
	CALL q_ccpow
	EX DE,HL
	LD HL,12
	ADD HL,SP
	LD SP,HL
	EX DE,HL
	RET
;}
;double fmod(x,y)
qfmod:
;double x;double y;
;{
;	return _ccmod(x,y);
	LD HL,8
	ADD HL,SP
	CALL dload
	CALL dpush
	LD HL,8
	ADD HL,SP
	CALL dload
	CALL dpush
	CALL q_ccmod
	EX DE,HL
	LD HL,12
	ADD HL,SP
	LD SP,HL
	EX DE,HL
	RET
;}

; --- End of Compilation ---
