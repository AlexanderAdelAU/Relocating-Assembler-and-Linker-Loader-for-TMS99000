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


SEEKCUR EQU     1

        ENT     ctell
        ENT     ctellc
        EXT     lseek

;       ctell(fd) - where the file is now.
;       lseek(fd, 0, SEEK_CUR): three arguments pushed left to right, so
;       fd goes down first and whence last, and the caller removes all
;       three.  lseek leaves its result in R4 already.
ctell:
        MOV     @2(SP),R4
        DECT    SP
        MOV     R4,*SP
        CLR     R4
        DECT    SP
        MOV     R4,*SP
        LI      R4,SEEKCUR
        DECT    SP
        MOV     R4,*SP
        CALL    @lseek
        AI      SP,6
        RET

;       ctellc(fd) - the same, masked into the current record.
ctellc:
        MOV     @2(SP),R4
        DECT    SP
        MOV     R4,*SP
        CLR     R4
        DECT    SP
        MOV     R4,*SP
        LI      R4,SEEKCUR
        DECT    SP
        MOV     R4,*SP
        CALL    @lseek
        AI      SP,6
        ANDI    R4,127
        RET
#endasm

#else

#include "stdio.h"

ctell(fd) int fd; {
	return (lseek(fd, 0, SEEK_CUR)); /* Return current file position */
  }

ctellc(fd) int fd;{
  return (lseek(fd, 0, SEEK_CUR) & 127);
  }

#endif
