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
extern char symbol[9]; /* current string */

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
int getrel() {
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

int getspec() { /* get next special item */
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
	case 12: /* chain addr (fill chain with lc) */
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

int gettyp() {
	if (!getbits(2))
		return (ERR); /* get 2-bit field type */
	return (type = field);
}

int getfld() { /* get type and value of field */
	int low;
	if (!getbits(8))
		return (ERR); /* get first 8 bits */
	low = field; /* save as low order byte */
	if (!getbits(8))
		return (ERR); /* get next 8 bits */
	field = (field << 8) | low; /* combine high & low bytes */
	return (item);
}

int getsym() { /* get symbol */
	int i, save;
	char *cp;
	cp = symbol;
	save = field; /* save field */
	if (!getbits(BITPSYM))
		return (ERR); /* get 4-bit symbol length 2^n = MAXSYM  BITPSYM is 3 or 4*/
	i = field; /* capture symbol length */
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
int getbits(int n) {
	int get, t;
	int *c;
	field = 0; /* initialize result */
	while (n) { /* more bits to fetch */
		if (inrem == 0) { /* need another chunk */
			if ((getbyte(inrel, &inchunk)) != 1) {
				puts("\n- Abnormal End of REL File\n");
				return (NO); /* failure */
			}
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

ifilelbuf() {
	nbr = 0;
}

/*
 * get byte has been added to avoid the issue of the CPMEOF character 0x1a sometimes being
 * embedded in the REL file.  The character can signify the premature end of the file and cause
 * an error.
 * Following are use
 * 	nbytes,  number of bytes in file
 *  nbr,	 number of bytes read
 *  *bp;   current pointer in byte buffer
 */
getbyte(fd, inbyte)
	int fd;char *inbyte; {

	if (read(fd, inbyte, 1) == -1)
		return (NO);
	return (YES);

	/* if (nbr == 0) {
		nbytes = lseek(fd, 0, SEEK_END);
		lseek(fd, 0, SEEK_SET);
		if (read(fd, bytebuf, BUFSIZE) == -1)
			return (NO);
		bp = bytebuf;
	} else {
		if (nbr % (BUFSIZE) == 0) {
			if (nbr > nbytes)
				return (NO);
			lseek(fd, nbr, SEEK_SET);
			if (read(fd, bytebuf, BUFSIZE) == -1)
				return (NO);
			bp = bytebuf;
		}
	}
	nbr++;
	*inbyte = *bp++;
	return (YES); */

}
