/*
 ** getrel -- read a relocatable-object file
 */
#include "rel99.h"
#include "stdio.h"
#define BUFSIZE 256

/* common variables */
extern int inrel, /* file descriptor for input REL file */
inrem, /* remaining bits in inchunk */
inchunk, /* current chunk from REL file */
item, /* current item code */
type, /* type field */
field; /* current bit field */

char *bp; /* current pointer in byte buffer */
int nbytes; /* number of bytes in file */
int nbr; /* number of bytes read */
int nleft; /* bytes still unread in bytebuf */
extern char symbol[MAXSYM + 1]; /* current string (defined in link99.c) */

char bytebuf[BUFSIZE];

/*
 ** get next REL item
 ** return item code on success, ERR on error
 ** on successful return:
 **    item = item code
 **    type = type of field
 **   field = value of field
 **  symbol = symbol name
 */
getrel()
{
	int c[5];
	if (!getbits(1))
		return (ERR); /* get 1 bit */
	if (field == 0) { /* absolute item */
		if (!getbits(8))
			return (ERR); /* get next 8 bits */
		return (type = item = ABS); /* absolute item */
	}
	if (!getbits(2))
		return (ERR); /* get next 2 bits */
	switch (type = item = field) {
	case 0:
		return (getspec()); /* special link item */
	case 1:
		break; /* program relative item */
	case 2:
		break; /* data relative item */
	case 3:
		break; /* common relative item */
	}
	if (getfld() == ERR)
		return (ERR); /* get next 16 bits */

	/*
	 itox(item, c,4);
	 puts2(c, " = item\n");
	 itox(field, c,6);
	 puts2(c, " = field\n");
	 */

	/* Use this to debug which module we are in  */
	/*  puts(symbol); */

	return (item); /* relative items */
}

getspec()   /* get next special item */
{
	if (!getbits(4))
		return (ERR); /* get 4 bits for control field*/
	type = ABS; /* default type */
	item = field + 4;
	switch (field) {
	case 0: /* entry symbol */
	case 1: /* select common block */
	case 2: /* program name */
	case 3: /* request library search */
	case 4: /* extension link items */
		if (getsym() == ERR)
			return (ERR);
		break;
	case 5: /* define common size */
	case 6: /* head of external reference chain */
	case 7: /* define entry point */
		if (gettyp() == ERR || getfld() == ERR || getsym() == ERR)
			return (ERR);
		break;
	case 8: /* external - offset */
	case 9: /* external + offset */
	case 10: /* size of data area */
	case 11: /* set loading location counter */
	case 12: /* chain addr (fill chain with lc) / AORG_MARK */
	case 13: /* size of program */
		if (gettyp() == ERR || getfld() == ERR)
			return (ERR);
		break;
	case 14: /* end of program */
		if (gettyp() == ERR || getfld() == ERR)
			return (ERR);
		inrem = 0; /* force byte boundary */
		break;
	case 15: /* end of file */
		inrem = 0; /* force byte boundary */
	}
	return (item);
}

gettyp()
{
	if (!getbits(2))
		return (ERR); /* get 2-bit field type */
	return (type = field);
}

getfld()   /* get type and value of field */
{
	int low;
	if (!getbits(8))
		return (ERR); /* get first 8 bits */
	low = field; /* save as low order byte */
	if (!getbits(8))
		return (ERR); /* get next 8 bits */
	field = (field << 8) | low; /* combine high & low bytes */
	return (item);
}

getsym()   /* get symbol */
{
	int i, save;
	char *cp;
	cp = symbol;
	save = field; /* save field */
	if (!getbits(BITPSYM))
		return (ERR); /* get BITPSYM-bit symbol length (4 bits -> 0..15 == MAXSYM) */
	i = field; /* capture symbol length */
	if (i > MAXSYM)
		return (ERR); /* refuse over-length name; never overrun symbol[] */
	while (i--) {
		if (!getbits(8))
			return (ERR); /* get next byte */
		*cp++ = field;
	}
	*cp = NULL; /* terminate symbol */
	field = save; /* restore field */
	return (item);
}

/*
 ** get next n bits from REL file into "field"
 ** return true on success, false on error
 */
getbits(n)
int n;
{
	int get, t;
	int *c;
	char b;         /* getbyte writes ONE BYTE - see below */

	field = 0; /* initialize result */
	while (n) { /* more bits to fetch */
		if (inrem == 0) { /* need another chunk */
			/*
			 * inchunk is a 16-bit unsigned, and getbyte stores a
			 * single BYTE through the pointer it is given.  On a
			 * little-endian host that byte lands in the LOW half,
			 * which is what the arithmetic below expects.  On the
			 * TMS9900 it lands in the HIGH half, so every chunk
			 * came out 256 times too big and the first item never
			 * parsed - the board reported "CALL.R99 - Corrupted".
			 *
			 * So read into a char and widen it explicitly.  The
			 * mask matters too: a char assignment yields the full
			 * word on this compiler.
			 */
			if ((getbyte(inrel, &b)) != 1) {
				puts("\n- Abnormal End of REL File\n");
				return (NO); /* failure */
			}
			inchunk = b & 0xff;
			inrem = 8; /* 8 bits remain */
		}
		if (n > inrem)
			get = inrem;
		else
			get = n; /* how many from this chunk */
		n -= get; /* decrement bits needed */
		inrem -= get; /* decr remaining bits */
		field = (field << get) + ((inchunk >> inrem) & ~(ONES << get));
	}
	return (YES); /* success */
}

/*
 * ifilelbuf -- discard whatever getbyte has buffered.
 *
 * Must be called after ANY reposition of the REL input, and whenever a
 * new file is opened on it, or getbyte would go on serving bytes from
 * the old position.  link99 calls it when it opens a file; seek() calls
 * it after moving to a library member.
 */
ifilelbuf() {
	nbr = 0;
	nleft = 0;
}

/*
 * getbyte -- one byte from the REL file.
 *
 * Buffered BUFSIZE bytes at a time.  It used to do read(fd, inbyte, 1)
 * per byte, which on the SBC is one BDOS call per byte of every object
 * file; this makes it one per 256.
 *
 * The buffer is refilled SEQUENTIALLY - no seeking - so it works the
 * same whether or not the file has been repositioned, provided
 * ifilelbuf() is called after any reposition.
 *
 * A short read is end of file, not an error: the last buffer of a file
 * is almost always partial.  Returns YES with the byte, or NO at end of
 * file.
 *
 * The reason this function exists at all: a CPMEOF (0x1a) can appear as
 * ordinary data inside a REL file, so end of file has to come from the
 * byte count, never from the value of a byte.
 */
getbyte(fd, inbyte)
	int fd;
	char *inbyte;
{
	if (nleft == 0) {
		nleft = read(fd, bytebuf, BUFSIZE);
		if (nleft <= 0) {
			nleft = 0;
			return (NO);
		}
		bp = bytebuf;
	}

	nleft--;
	nbr++;
	*inbyte = *bp++;
	return (YES);
}
