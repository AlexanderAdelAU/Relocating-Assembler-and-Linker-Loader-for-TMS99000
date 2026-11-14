/*
** BOTH VERSIONS LIVE HERE, chosen by the build:
**     smallcp -C -M          NAME     the C below
**     smallcp -C -M -DASMVER NAME     the assembly above
**
** The C is the DEFINITION - read it to see what the routine does, and it
** is what the assembly was verified against.  The assembly is what gets
** built when space matters.  Frozen: one correct behaviour, no second.
*/

#ifdef ASMVER

#asm
;   The register equates and DXOPs are NOT repeated here -
;   smallcp has already emitted them above this point, and a
;   second definition is a duplicate-symbol error.


        ENT     putstring
        EXT     putchar

;       putstring(str) - every character through putchar.
;       MOVB sets the flags on the byte it moved, so the terminator test
;       is free.
putstring:
        MOV     @2(SP),R1
PS_LOOP:
        CLR     R4
        MOVB    *R1+,R4
        JEQ     PS_DONE
        SRL     R4,8
        DECT    SP
        MOV     R4,*SP
        CALL    @putchar
        AI      SP,2
        JMP     PS_LOOP
PS_DONE:
        RET
#endasm

#else

putstring(str)
char *str;
{
	while(*str)putchar(*str++);
}

#endif
