/*
** STAGE.C -- the code/data staging area, in paged memory
**
** link99 builds the output image in a buffer as it loads each module,
** then writes it out in phase 2.  On the board that buffer cannot live
** in page 0: the linker's own image leaves under 2K there.  It lives in
** the paged memory behind segment D instead.
**
** PAGE PARTITION - THE ONE THING TO GET RIGHT
**   There is ONE window, segment D, shared with the symbol table at
**   different PAGE numbers.  They must take different pages or they
**   will quietly overwrite each other.
**
**   Segment D and not C because the linker's own image reaches into
**   segment C; the first board run mapped C out from under the code
**   that was executing there.
**
**       pages 2..4    symbol table   (SYMTAB.C: FIRSTPG 2, LASTPG 4)
**       pages 5..15   staging area   (here: STGFIRST 5, STGLAST 15)
**
**   That is 12,288 bytes of symbol table - 585 entries, against the 256
**   that linking R99 needs - and 45,056 bytes of staging, which holds a
**   44K image without touching the .O$ overflow file at all.
**
**   To re-balance, move the boundary in both files together.
**
** ADDRESSING
**   Callers use a flat offset from the start of the staging area, 0 to
**   STGSIZE-1, which is exactly what link99 already has: cloc, xbuf and
**   the rest are all offsets from buffer[0].  The page and the offset
**   within it are worked out here.
**
** Small-C note: the offset can exceed 32767, and '>>' compiles to an
** ARITHMETIC shift, so the page number is masked after shifting rather
** than divided.  '/' would go through the signed _ccdiv and be wrong.
*/

#include "stdio.h"

#define STGFIRST  5                     /* first staging page            */
#define STGLAST   15                    /* last staging page             */
#define STGPAGES  11                    /* STGLAST - STGFIRST + 1        */
#define STGSIZE   45056                 /* STGPAGES * 4096               */

extern stgget();
extern stgput();
extern stggetb();
extern stgputb();

/*
** Split a flat staging offset into its page.  The mask is what makes
** this right for offsets above 32767, where the arithmetic shift would
** otherwise drag the sign bit down.
*/
stgpage(off)
int off;
{
	return (STGFIRST + ((off >> 12) & 0x0F));
}

/*
** Read one byte at a flat staging offset.
*/
stgrdb(off)
int off;
{
	return (stggetb(stgpage(off), off & 0x0fff));
}

/*
** Write one byte at a flat staging offset.
*/
stgwrb(off, value)
int off;
int value;
{
	stgputb(stgpage(off), off & 0x0fff, value);
}

/*
** Read a 16-bit word held big-endian at a flat staging offset.
**
** Done a byte at a time deliberately.  A word access would be wrong at
** an odd offset - the 99105 forces word accesses even - and the image
** being built is a byte stream in which words land wherever they land.
*/
stgrdw(off)
int off;
{
	int hi, lo;

	hi = stgrdb(off) & 0xff;
	lo = stgrdb(off + 1) & 0xff;
	return ((hi << 8) | lo);
}

/*
** Write a 16-bit word big-endian at a flat staging offset.
*/
stgwrw(off, value)
int off;
int value;
{
	stgwrb(off, value >> 8);
	stgwrb(off + 1, value);
}

/*
** How big the staging area is, for the caller's overflow test.
*/
stgsize()
{
	return (STGSIZE);
}
