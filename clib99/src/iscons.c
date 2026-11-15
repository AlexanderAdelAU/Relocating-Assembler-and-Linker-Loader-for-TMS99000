/*
** Determine if fd is the console.
*/
#include <stdio.h>
extern int _cons[];

iscons(fd) int fd; {
  if(_cons[fd] == NULL) {
    if(_iscons(fd)) _cons[fd] = 2;
    else            _cons[fd] = 1;
    }
  if(_cons[fd] == 1) return (NO);
  return (YES);
  }

/*
** Call BDOS only the first time for a file.
**
**  HL == R4
 *  DE == R3
 *  BC == R2
 *
 *  HL → R4: the Small-C primary/return register, which is why
 *  _iscons leaves its result in R4 and the compiled iscons tests
 *  R4 after the call.
 *  DE → R3, BC → R2: notice this makes _bdos a straight CP/M BDOS
 *  calling convention in disguise — CP/M passes the function code in
 *  C (BC) and the parameter in DE, returns in A/HL. CBDOS's "func in R2,
 *  param in R3, result in R1/R4" is exactly that, register-for-register.
 *
 */
/**
** TMS99000 version -- calls _bdos(func, param) in CBDOS.A99
** (func in R2, param in R3 inside BDOS, result in R1/R4).
** Calling _bdos rather than CALL @BDOS keeps this page-safe;
** the linker trampoline handles page0 entry.
**
** Original 8086 code called DOS int 21h fn 4400h (ioctl get info)
** and tested the device (bit 7) and console (bits 0-1) bits.
** Adjust IOCTL below if CBDOS assigns a different function code.
**
** Returns nonzero in R4 if fd is a character device AND the console.
*/

_iscons(fd) int fd; {
#asm
IOCTL	EQU	044H		;BDOS ioctl get info fn (was 4400h)
	EXT	_bdos
	MOV	@2(SP),R4	; R4 = fd            (was: mov bx,ax)
	LI	R3,IOCTL	; ioctl get info fn  (was: mov ax,4400h)
	DECT	SP
	MOV	R3,*SP		; push func (1st arg to _bdos)
	DECT	SP
	MOV	R4,*SP		; push fd   (2nd arg = param)
	CALL	@_bdos		; call BDOS          (was: int 21h)
	AI	SP,4		; drop the two args
	ANDI	R4,0083H	; keep device and console bits
	CI	R4,0080H	; device and console?
	JGT	__cons1
	CLR	R4		; return false if not device and console
__cons1:
#endasm
  }
