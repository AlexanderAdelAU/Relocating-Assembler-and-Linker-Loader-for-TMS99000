

/*
** BOTH VERSIONS LIVE HERE, chosen by the build:
**     smallcp -C -M          NAME     the C below
**     smallcp -C -M -DASMVER NAME     the assembly above
**
** The C is the DEFINITION - read it to see what the routine does, and
** it is what the assembly was verified against.  The assembly is what
** gets built when space matters.  This routine is frozen: it has one
** correct behaviour and should never need revisiting.
*/

#ifdef ASMVER

#asm
;   The register equates and DXOPs are NOT repeated here -
;   smallcp has already emitted them above this point, and a
;   second definition is a duplicate-symbol error.


        ENT     itou

itou:
        MOV     @2(SP),R2               ;R2 = sz
        MOV     @4(SP),R1               ;R1 = str
        MOV     @6(SP),R5               ;R5 = the running value

        MOV     R2,R2
        JGT     IU_POS
        JLT     IU_NEG

;   sz == 0: use the length of the string already there
        MOV     R1,R3
IU_LSCAN:
        MOVB    *R3+,R4
        JNE     IU_LSCAN
        DEC     R3                      ;back over the terminator
        S       R1,R3
        MOV     R3,R2
        JMP     IU_CONV

;   sz > 0: terminate, then fill in front of the terminator
IU_POS:
        DEC     R2
        MOV     R1,R3
        A       R2,R3
        CLR     R4
        MOVB    R4,*R3
        JMP     IU_CONV

;   sz < 0: fill |sz| bytes, no terminator
IU_NEG:
        NEG     R2

IU_CONV:
        MOV     R2,R2
        JEQ     IU_DONE                 ;no room at all

IU_LOOP:
        CLR     R6                      ;R6:R7 = the 32-bit dividend
        MOV     R5,R7
        LI      R3,10
        DIV     R3,R6                   ;R6 = value/10, R7 = value%10

        MOV     R6,R5                   ;carry the quotient on
        AI      R7,48                   ;the remainder is the digit

        DEC     R2                      ;str[--sz] = digit
        MOV     R1,R3
        A       R2,R3
        SWPB    R7                      ;MOVB stores the HIGH half
        MOVB    R7,*R3

        MOV     R5,R5
        JEQ     IU_PAD                  ;nothing left to convert
        MOV     R2,R2
        JNE     IU_LOOP                 ;and still room for it

IU_PAD:
        MOV     R2,R2
        JEQ     IU_DONE
        LI      R3,2000H                ;' ' in the high half
IU_PLOOP:
        DEC     R2
        MOV     R1,R4
        A       R2,R4
        MOVB    R3,*R4
        MOV     R2,R2
        JNE     IU_PLOOP

IU_DONE:
        MOV     R1,R4                   ;return str
        RET
#endasm

#else

#include "stdio.h"
/*
** itou -- convert nbr to unsigned decimal string of width sz
**         right adjusted, blank filled; returns str
**
**        if sz > 0 terminate with null byte
**        if sz = 0 find end of string
**        if sz < 0 use last byte for data
*/
char *itou(nbr, str, sz)
int nbr;
char str[];
int sz;
{
  int lowbit;
  if(sz>0) str[--sz]=NULL;
  else if(sz<0) sz = -sz;
  else while(str[sz]!=NULL) ++sz;
  while(sz) {
    /* SMALLC-UDIV: safe, and deliberately so.  nbr is halved with a
       shift and the sign bit is masked off BEFORE the divide, so the
       value reaching / and % is never above 32767 and the signed
       _ccdiv gives the right answer.  Do not "simplify" this to
       nbr/10 - that would break for values over 32767. */
    lowbit=nbr&1;
    nbr=(nbr>>1)&32767;  /* divide by 2, unsigned */
    str[--sz]=((nbr%5)<<1)+lowbit+'0';
    if((nbr=nbr/5)==0) break;
    }
  while(sz) str[--sz]=' ';
  return str;
  }

#endif
