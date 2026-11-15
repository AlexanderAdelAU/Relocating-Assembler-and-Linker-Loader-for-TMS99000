/*
** xtoi -- hexadecimal ASCII to integer
**
** int xtoi(hexstr, nbr)
**
**   Skips leading zeros, then converts up to four hex digits into
**   *nbr.  Returns the number of characters consumed before the first
**   non-hex character, or -2 if a fifth digit appears.
**
** THE C BELOW IS THE DEFINITION.  The assembly above it does the same
** thing in a fraction of the space and is selected by -DASMVER.  This
** routine is frozen - there is one way to read hex - so the assembly
** should never need revisiting.
*/

#ifdef ASMVER

#asm
        ENT     xtoi

;   Small-C convention: arguments pushed left to right, so the LAST is
;   nearest the stack top.
;       2(SP) = nbr     4(SP) = hexstr
;   The caller removes them.  Result in R4.
;
;   R1 = cp, the walking pointer      R2 = hexstr, kept for the count
;   R3 = the accumulating value       R5 = the current character
;   R6 = digit count                  R7 = scratch

xtoi:
        MOV     @4(SP),R2               ;hexstr
        MOV     R2,R1                   ;cp = hexstr
        MOV     @2(SP),R7               ;nbr
        CLR     *R7                     ;*nbr = 0
        CLR     R3                      ;the value so far
        CLR     R6                      ;d = 0

;   while (*cp == '0') ++cp;
XT_ZERO:
        CLR     R5
        MOVB    *R1,R5
        SRL     R5,8
        CI      R5,'0'
        JNE     XT_LOOP
        INC     R1
        JMP     XT_ZERO

XT_LOOP:
        CLR     R5
        MOVB    *R1,R5
        SRL     R5,8                    ;R5 = *cp

        CI      R5,'0'
        JL      XT_END
        CI      R5,'9'
        JH      XT_ALPHA
        AI      R5,-48                  ;'0'..'9'
        JMP     XT_DIGIT

XT_ALPHA:
        CI      R5,'A'
        JL      XT_END
        CI      R5,'F'
        JH      XT_LOWER
        AI      R5,-55                  ;'A'..'F'
        JMP     XT_DIGIT

XT_LOWER:
        CI      R5,'a'
        JL      XT_END
        CI      R5,'f'
        JH      XT_END
        AI      R5,-87                  ;'a'..'f'

XT_DIGIT:
;   if (d < 4) ++d; else return (-2);
        CI      R6,4
        JHE     XT_TOOMANY
        INC     R6

;   *nbr = (*nbr << 4) + digit
        SLA     R3,4
        A       R5,R3
        MOV     @2(SP),R7
        MOV     R3,*R7

        INC     R1
        JMP     XT_LOOP

XT_END:
;   return (cp - hexstr)
        MOV     R1,R4
        S       R2,R4
        RET

XT_TOOMANY:
        LI      R4,-2
        RET
#endasm

#else

xtoi(hexstr, nbr) char *hexstr; int *nbr;  {
  int d, b;  char *cp;
  d = *nbr = 0; cp = hexstr;
  while(*cp == '0') ++cp;
  while(1) {
    switch(*cp) {
      case '0': case '1': case '2':
      case '3': case '4': case '5':
      case '6': case '7': case '8':
      case '9':                     b=48; break;
      case 'A': case 'B': case 'C':
      case 'D': case 'E': case 'F': b=55; break;
      case 'a': case 'b': case 'c':
      case 'd': case 'e': case 'f': b=87; break;
       default: return (cp - hexstr);
      }
    if(d < 4) ++d; else return (-2);
    *nbr = (*nbr << 4) + (*cp++ - b);
    }
  }

#endif
