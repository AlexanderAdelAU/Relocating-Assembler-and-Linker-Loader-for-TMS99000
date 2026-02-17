;* * *  Small-C/Plus  Version 1.06  * * *
;       Cain, Van Zandt, Hendrix, Yorston
;       21st April 1990
;
	module printf\utoi
;/*
;** utoi -- convert unsigned decimal string to integer nbr
;**          returns field size, else ERR on error
;*/
;utoi(decstr, nbr)
	global qutoi
qutoi:
;char *decstr;
;int *nbr;
;{
;  int d,t;
;  d=0;
	PUSH BC
	PUSH BC
	LD HL,0
	POP DE
	POP BC
	PUSH HL
	PUSH DE
;  *nbr=0;
	LD HL,6
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,0
	CALL ccpint
;  while((*decstr>='0')&(*decstr<='9')) {
cc2:
	LD HL,8
	ADD HL,SP
	CALL ccgint
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,48
	POP DE
	CALL ccge
	PUSH HL
	LD HL,10
	ADD HL,SP
	CALL ccgint
	LD L,(HL)
	LD H,0
	PUSH HL
	LD HL,57
	POP DE
	CALL ccle
	POP DE
	CALL ccand
	LD A,L
	OR H
	JP Z,cc3
;    t=*nbr;t=(10*t) + (*decstr++ - '0');
	LD HL,6
	ADD HL,SP
	CALL ccgint
	CALL ccgint
	POP BC
	PUSH HL
	LD HL,0
	ADD HL,SP
	CALL ccgint
	LD DE,10
	CALL ccmult
	PUSH HL
	LD HL,10
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	DEC HL
	LD L,(HL)
	LD H,0
	LD DE,-48
	ADD HL,DE
	POP DE
	ADD HL,DE
	POP BC
	PUSH HL
;    if ((t >= 0) & (*nbr < 0))
	LD HL,0
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,0
	POP DE
	CALL ccge
	PUSH HL
	LD HL,8
	ADD HL,SP
	CALL ccgint
	CALL ccgint
	PUSH HL
	LD HL,0
	POP DE
	CALL cclt
	POP DE
	CALL ccand
	LD A,L
	OR H
	JP Z,cc4
;    	return (-2);
	LD HL,-2
	POP BC
	POP BC
	RET
;    d++; *nbr=t;
cc4:
	LD HL,2
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	DEC HL
	LD HL,6
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,2
	ADD HL,SP
	CALL ccgint
	CALL ccpint
;    }
	JP cc2
cc3:
;  return d;
	LD HL,2
	ADD HL,SP
	CALL ccgint
	POP BC
	POP BC
	RET
;  }

; --- End of Compilation ---
