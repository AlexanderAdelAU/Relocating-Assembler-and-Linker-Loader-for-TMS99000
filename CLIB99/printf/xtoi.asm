;* * *  Small-C/Plus  Version 1.06  * * *
;       Cain, Van Zandt, Hendrix, Yorston
;       21st April 1990
;
	module printf\xtoi
;/*
;** xtoi -- convert hex string to integer nbr
;**         returns field size, else ERR on error
;*/
;xtoi(hexstr, nbr) char *hexstr; int *nbr;  {
	global qxtoi
qxtoi:
;  int d, b;  char *cp;
;  d = *nbr = 0; cp = hexstr;
	PUSH BC
	PUSH BC
	PUSH BC
	LD HL,4
	ADD HL,SP
	PUSH HL
	LD HL,10
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,0
	CALL ccpint
	CALL ccpint
	LD HL,10
	ADD HL,SP
	CALL ccgint
	POP BC
	PUSH HL
;  while(*cp == '0') ++cp;
cc2:
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
	JP Z,cc3
	LD HL,0
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	JP cc2
cc3:
;  while(1) {
cc4:
;    switch(*cp) {
	LD HL,0
	ADD HL,SP
	CALL ccgint
	LD L,(HL)
	LD H,0
	JP cc8
;      case '0': case '1': case '2':
cc9:
cc10:
cc11:
;      case '3': case '4': case '5':
cc12:
cc13:
cc14:
;      case '6': case '7': case '8':
cc15:
cc16:
cc17:
;      case '9':                     b=48; break;
cc18:
	LD HL,48
	POP DE
	POP BC
	PUSH HL
	PUSH DE
	JP cc7
;      case 'A': case 'B': case 'C':
cc19:
cc20:
cc21:
;      case 'D': case 'E': case 'F': b=55; break;
cc22:
cc23:
cc24:
	LD HL,55
	POP DE
	POP BC
	PUSH HL
	PUSH DE
	JP cc7
;      case 'a': case 'b': case 'c':
cc25:
cc26:
cc27:
;      case 'd': case 'e': case 'f': b=87; break;
cc28:
cc29:
cc30:
	LD HL,87
	POP DE
	POP BC
	PUSH HL
	PUSH DE
	JP cc7
;       default: return (cp - hexstr);
cc31:
	LD HL,0
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,12
	ADD HL,SP
	CALL ccgint
	POP DE
	CALL ccsub
	POP BC
	POP BC
	POP BC
	RET
;      }
	JP cc7
cc8:
	CALL ccswitch
	DEFW cc9,48
	DEFW cc10,49
	DEFW cc11,50
	DEFW cc12,51
	DEFW cc13,52
	DEFW cc14,53
	DEFW cc15,54
	DEFW cc16,55
	DEFW cc17,56
	DEFW cc18,57
	DEFW cc19,65
	DEFW cc20,66
	DEFW cc21,67
	DEFW cc22,68
	DEFW cc23,69
	DEFW cc24,70
	DEFW cc25,97
	DEFW cc26,98
	DEFW cc27,99
	DEFW cc28,100
	DEFW cc29,101
	DEFW cc30,102
	DEFW 0
	JP cc31
cc7:
;    if(d < 4) ++d; else return (-2);
	LD HL,4
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,4
	POP DE
	CALL cclt
	LD A,L
	OR H
	JP Z,cc32
	LD HL,4
	ADD HL,SP
	PUSH HL
	CALL ccgint
	INC HL
	CALL ccpint
	JP cc33
cc32:
	LD HL,-2
	POP BC
	POP BC
	POP BC
	RET
cc33:
;    *nbr = (*nbr << 4) + (*cp++ - b);
	LD HL,8
	ADD HL,SP
	CALL ccgint
	PUSH HL
	LD HL,10
	ADD HL,SP
	CALL ccgint
	CALL ccgint
	PUSH HL
	LD HL,4
	POP DE
	CALL ccasl
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
	PUSH HL
	LD HL,8
	ADD HL,SP
	CALL ccgint
	POP DE
	CALL ccsub
	POP DE
	ADD HL,DE
	CALL ccpint
;    }
	JP cc4
cc5:
;  }
	POP BC
	POP BC
	POP BC
	RET

; --- End of Compilation ---
