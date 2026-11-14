/*
** avail -- how much heap is left
**
** ONE FILE, TWO TARGETS.
**
** On the SBC this asks IOLIB99, which knows: heaptop() is where the heap
** has reached and memlimit() is the ceiling the shell set.  That is a real
** answer, and link99 sizes itself from it.
**
** The Windows tools - drel, lib99, link99.exe, r99.exe - are built with
** gcc and do not link IOLIB99, so heaptop() and memlimit() do not exist
** there.  They get the old fixed figure instead, which is all they ever
** had: avail() is only used for a diagnostic print on that side.
**
** __GNUC__ picks between them, so nothing has to be configured.  gcc
** defines it automatically and Small-C does not.
**
** THE HISTORY, because it explains the shape of this file.  The original
** returned MAXMEM unconditionally, with the real calculation commented
** out just above the return - so every caller was being told 64000 no
** matter what.  On the SBC that is not merely imprecise, it is wrong by
** tens of thousands of bytes, and link99 would have believed it.
*/

#include "stdio.h"

#define MAXMEM 64000            /* the figure the host side always had */

#ifndef __GNUC__
extern int heaptop();
extern int memlimit();
#endif

avail(abort)
int abort;
{
#ifdef __GNUC__

	/*
	 * Host build.  There is no IOLIB99 here and nothing depends on the
	 * answer, so give the figure this always returned.
	 */
	return (MAXMEM);

#else

	/*
	 * SBC build.  The real thing.  abort is honoured because callers
	 * pass it and expect to be stopped rather than handed a zero.
	 */
	int top, lim;

	top = heaptop();
	lim = memlimit();

	if (top >= lim) {
		if (abort)
			exit(1);
		return (0);
	}
	return (lim - top);

#endif
}
