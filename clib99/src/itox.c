/*
** itox -- converts nbr to hex string of length sz
**         right adjusted and blank filled, returns str
**
**        if sz > 0 terminate with null byte
**        if sz = 0 find end of string
**        if sz < 0 use last byte for data
*/

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


        ENT     itox

itox:
        MOV     @2(SP),R2               ;R2 = sz
        MOV     @4(SP),R1               ;R1 = str
        MOV     @6(SP),R0               ;R0 = nbr

        MOV     R2,R2
        JGT     IX_POS                  ;sz > 0
        JLT     IX_NEG                  ;sz < 0

;   sz == 0: use the length of the string already there
IX_LEN:
        MOV     R1,R3
IX_LSCAN:
        MOVB    *R3+,R5
        JNE     IX_LSCAN                ;MOVB sets the flags on the byte
        DEC     R3                      ;back over the terminator
        S       R1,R3
        MOV     R3,R2                   ;R2 = strlen(str)
        JMP     IX_CONV

;   sz > 0: terminate the string and fill the sz-1 bytes in front
IX_POS:
        DEC     R2
        MOV     R1,R3
        A       R2,R3
        CLR     R5
        MOVB    R5,*R3                  ;str[--sz] = 0
        JMP     IX_CONV

;   sz < 0: fill |sz| bytes, no terminator
IX_NEG:
        NEG     R2

;-------------------------------------------------------------------
;       Fill from the right with hex digits, most significant last.
;-------------------------------------------------------------------
IX_CONV:
        MOV     R2,R2
        JEQ     IX_DONE                 ;no room at all

IX_LOOP:
        MOV     R0,R3
        ANDI    R3,000FH                ;digit = nbr & 15
        SRL     R0,4                    ;nbr >>= 4, logically - no mask

        AI      R3,48                   ;'0'
        CI      R3,58                   ;digit > 9?
        JLT     IX_STORE
        AI      R3,7                    ;'A'-'9'-1, so 10 -> 'A'

IX_STORE:
        DEC     R2                      ;str[--sz] = digit
        MOV     R1,R5
        A       R2,R5
        SWPB    R3                      ;MOVB stores the HIGH half
        MOVB    R3,*R5
        SWPB    R3

        MOV     R0,R0
        JEQ     IX_PAD                  ;nothing left to convert

        MOV     R2,R2
        JNE     IX_LOOP                 ;room for another digit

;-------------------------------------------------------------------
;       Blank-pad whatever is left on the left.
;-------------------------------------------------------------------
IX_PAD:
        MOV     R2,R2
        JEQ     IX_DONE
        LI      R3,2000H                ;' ' in the high half, ready for MOVB
IX_PLOOP:
        DEC     R2
        MOV     R1,R5
        A       R2,R5
        MOVB    R3,*R5
        MOV     R2,R2
        JNE     IX_PLOOP

IX_DONE:
        MOV     R1,R4                   ;return str
        RET
#endasm

#else

itox(nbr, str, sz) int nbr; char str[]; int sz; {
  int digit, offset;
  if(sz>0) str[--sz]=0;
  else if(sz<0) sz = -sz;
  else while(str[sz]!=0) ++sz;
  while(sz) {
    digit=nbr&15; nbr=(nbr>>4)&4095;
    if(digit<10) offset=48; else offset=55;
    str[--sz]=digit+offset;
    if(nbr==0) break;
    }
  while(sz) str[--sz]=' ';
  return str;
  }

#endif
