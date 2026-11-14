/*
** integer manipulation
**
** 16-BIT TARGET VERSION.  The host build carried scaffolding here for
** running 16-bit code inside 32-bit pointers: getaddr() rebuilt an
** address by pasting the stored low half onto the high half of the
** host pointer, and putaddr() masked a host int down to 16 bits.
** On the TMS9900 a pointer IS 16 bits, so both collapse into plain
** getint/putint and are kept only as aliases so link99.c does not have
** to be rewritten around them.
**
** get16int/put16int stay as they are: they pack a 16-bit value into
** two BYTES of the buffer, big-endian, which is a real operation and
** nothing to do with host pointer width.
*/

/*
** Get a 16-bit value from two bytes at 'a', high byte first.
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


        ENT     get16int
        ENT     put16int
        ENT     getaddr
        ENT     putaddr
        ENT     getint
        ENT     putint

;*******************************************************************
;       get16int(a) - the big-endian word held in the two bytes at a.
;
;       a[0]=12 a[1]=34 -> 1234:
;           MOVB *R1+,R4   R4 = 1200
;           SWPB R4        R4 = 0012
;           MOVB *R1,R4    R4 = 3412
;           SWPB R4        R4 = 1234
;*******************************************************************
get16int:
getaddr:
        MOV     @2(SP),R1               ;a
        CLR     R4
        MOVB    *R1+,R4
        SWPB    R4
        MOVB    *R1,R4
        SWPB    R4
        RET

;*******************************************************************
;       put16int(a, i) - store i at a, high byte first.
;       i is the LAST argument, so it is nearest the stack top.
;*******************************************************************
put16int:
putaddr:
        MOV     @2(SP),R2               ;i
        MOV     @4(SP),R1               ;a
        MOVB    R2,*R1+                 ;R2's high half IS the high byte
        SWPB    R2
        MOVB    R2,*R1                  ;then the low byte
        RET

;*******************************************************************
;       getint(a) - the word at a word-aligned location.
;*******************************************************************
getint:
        MOV     @2(SP),R1
        MOV     *R1,R4
        RET

;*******************************************************************
;       putint(a, i) - store it.
;*******************************************************************
putint:
        MOV     @2(SP),R4               ;i
        MOV     @4(SP),R1               ;a
        MOV     R4,*R1
        RET
#endasm

#else

get16int(a)
char *a;
{
	int hi, lo;
	hi = *a++ << 8;
	lo = *a & 0x00ff;   /* stops the sign extending when bit 7 is set */
	return ((hi | lo) & 0xFFFF);  /* no-op on a 16-bit int; stops a 32-bit host sign-extending */
}

/*
** Store a 16-bit value as two bytes at 'a', high byte first.
*/
put16int(a, i)
char *a;
int i;
{
	*a++ = i >> 8;
	*a = i;
}

/*
** Address in, address out.  On a 16-bit target there is nothing to do.
*/
getaddr(a)
char *a;
{
	return (get16int(a));
}

putaddr(a, i)
char *a;
int i;
{
	put16int(a, i);
}

/*
** Word at a word-aligned location.
*/
getint(a)
int *a;
{
	return (*a);
}

putint(a, i)
int *a;
int i;
{
	*a = i;
}

#endif
