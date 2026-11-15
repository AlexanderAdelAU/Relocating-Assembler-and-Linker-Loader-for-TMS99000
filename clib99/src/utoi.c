/*
** utoi -- decimal ASCII to unsigned integer
**
** int utoi(decstr, nbr)
**
**   Reads decimal digits into *nbr and returns how many were consumed,
**   or -2 on overflow.  The C detects overflow by watching for the sign
**   bit appearing - "if ((t >= 0) & (*nbr < 0)) return -2" - which is
**   the only tool it has without an unsigned compare.
**
**   The assembly can do better: it multiplies by ten and checks the
**   real carry, so it accepts the full unsigned range up to 65535 where
**   the C stops at 32767.  For every value the C accepts, both give the
**   same answer.
**
** THE C BELOW IS THE DEFINITION.  Frozen - selected by -DASMVER.
*/

#ifdef ASMVER

#asm
        ENT     utoi

;       2(SP) = nbr     4(SP) = decstr
;   R1 = the walking pointer   R2 = the accumulating value
;   R3 = digit count           R5 = the current character
;   R6:R7 = the multiply pair - MPY Rs,Rd puts the 32-bit product in
;   Rd:Rd+1, so nothing else may live in R7.

utoi:
        MOV     @4(SP),R1               ;decstr
        MOV     @2(SP),R5
        CLR     *R5                     ;*nbr = 0
        CLR     R2                      ;the value so far
        CLR     R3                      ;d = 0

UT_LOOP:
        CLR     R5
        MOVB    *R1,R5
        SRL     R5,8                    ;R5 = *decstr
        CI      R5,'0'
        JL      UT_END
        CI      R5,'9'
        JH      UT_END
        AI      R5,-48                  ;the digit itself

;   value = value*10 + digit, refusing to wrap past 65535
        MOV     R2,R6
        LI      R7,10
        MPY     R7,R6                   ;R6:R7 = value * 10
        MOV     R6,R6
        JNE     UT_OVER                 ;high half set: over 65535
        A       R5,R7
        JOC     UT_OVER                 ;the add carried
        MOV     R7,R2

        MOV     @2(SP),R5
        MOV     R2,*R5                  ;*nbr = value
        INC     R3                      ;d++
        INC     R1
        JMP     UT_LOOP

UT_END:
        MOV     R3,R4                   ;return d
        RET

UT_OVER:
        LI      R4,-2
        RET
#endasm

#else

utoi(decstr, nbr)
char *decstr;
int *nbr;
{
  int d,t;
  d=0;

  *nbr=0;
  while((*decstr>='0')&(*decstr<='9')) {
    t=*nbr;t=(10*t) + (*decstr++ - '0');
    if ((t >= 0) & (*nbr < 0))
    	return (-2);
    d++; *nbr=t;
    }
  return d;
  }

#endif
