/*
 **  General rel interface routines
 */
#include "r99gbl.h"
#include "rel99.h"
#include "R99ext.h"

#define NO 0
#define YES 1
#define NULL 0
#define ERR -1

/*
 Function to form the hex output line and put it to
 the hex output device.
 */

short int relout() {
	char count, count2, *bptr, code, tflg, *sptr;
	if (pass < 2)
		return (YES);
	switch (hexflg) {
	case PUTCODE:
		/*	bptr = binbuf; */
		return YES;
		for (count = 0; count < nbytes; count++) {

			tflg = itemflg[count]; /* r only detected */
			sptr = symptr[count];
			itemflg[count] = '\0'; /* zero again */
			/*	puthex2(*bptr++,&hxlnptr); */
			/*	item = ABS; *//* put out opcode */
			/*	field = *bptr++; */
			/*	putrel();*/
			/*	if ( tflg & RELBIT ){
			 item = PREL;
			 field = *bptr++;
			 putrel();
			 count++;
			 }
			 */
			if (tflg & EXTBIT) {
				puthex2(*bptr++, &hxlnptr); /* put out lsb of word */
				count++;
				for (count2 = 0; count2 < SYMLEN; count2++)
					puthex2(*sptr++, &hxlnptr);
				*hxlnptr++ = 'X'; /* show the symbol is external */
			}
		}

	case NOCODE:
		return NO;

		/*	case FLUSH:	flshhbf(pc);
		 return NO;
		 */
	case NOMORE:
		rputchr(0x1A, &hexbuf); /* end of file */
		rflush(&hexbuf);
		return NO;
	}
}

/*
 Output the module header consisting of Common size and 
 Programme Indicator.
 */
void relhead() {
	outrem = 8; /* initialise */
	outchunk = 0;
	item = PNAME; /* programme name */
	field = 0;
	/* strcpy(symbol,progname); *//* copy name over */
	memcpy(symbol, progname, SYMLEN);
	putrel();

	/* now output all the entry symbols this module can resolve */
	entnam();

	/* data size */
	item = DSIZE; /* zero this not supported */
	type = 0;
	field = 0;
	putrel();

	/*  programme size */

	item = PSIZE;
	type = PREL;
	field = progsize; /* high value of programme counter */
	putrel();

}
/*
 Output the entry symbols - assume symbols not sorted
 */
void entnam() {
	short int n, t;
	/*	n = nssymbols;  /*sortsym(NOSORT);	/*Get number of sorted symbols */
	n = SYMBOLS;
	sympoint = symtbl; /* begin at start of table */
	while (n > 0) { /* begin looking for ent symbols */
		if (sympoint->symflg & ENTBIT) {
			item = ENAME; /* entry table for this module */
			memcpy(symbol, sympoint->symname, SYMLEN);
			putrel();
		}
		sympoint++;
		--n;
	}
}
/*
 This function will output all the referenced externals
 and entry points into the object file
 */
void relsyms() {
	short int n;

	n = nssymbols;
	sympoint = symtbl; /* point to the start of the table */
	while (n > 0) {
		if (sympoint->symflg & (EXTBIT | ENTBIT)) {
			if (sympoint->symflg & EXTBIT) /* type of symbol to be defined */
				item = XCHAIN;
			else
				item = EPOINT;

			field = sympoint->symvalu; /* offset of last symbol */
			memcpy(symbol, sympoint->symname, SYMLEN);
			if (!(sympoint->symvalu) && (sympoint->symflg & EXTBIT)) {
				type = ABS;
				putabs2();
			} else {
				type = PREL;
				putrel();
			}
		}
		sympoint++;
		--n;
	}
}

/*--------------------------------------------------------
 new rel section
 ---------------------------------------------------------*/

/*
 ** putrel -- write a relocatable-object file
 */

/*
 ** put next REL item
 ** return true on success, false on error
 ** on call:
 **    item = item code
 **    type = type of field
 **   field = value of field
 **  symbol = symbol name
 */
/* putabs2() will place 2 bytes onto the output stream.
 This saves calling putrel() twice as would be the case
 using the normal putrel() ABS sequence.
 */
void putabs2() {
	unsigned short f;
	f = field;
	field = f >> 8;
	putrel();
	field = f;
	putrel();
}

/* 
 Use this for code output
 */

/*putrel2(){ */
/*if(item == PREL);
 *bptr++ = field >> 8;
 *bptr++ = field; */
/* putrel();
 } */

void putabs() {
	/* *bptr++ = field; */
	putrel();
}

short int putrel() {
	char c[3], f, *p;
	if (pass < 2)
		return (YES);
	/*	c[2] = '\0';
	 p = c;
	 if (item == PREL)  {
	 f=field>>8;
	 if ((*p = (f >> 4) + '0') > '9') *p += 7;
	 if ((*++p = (f & 0x0f) + '0') > '9') *p += 7;
	 puts(c);
	 }
	 p = c;
	 f=field;
	 if ((*p = (f >> 4) + '0') > '9') *p += 7;
	 if ((*++p = (f & 0x0f) + '0') > '9') *p += 7;
	 puts(c); */

	switch (item) {
	case ABS:
		if (!putbits(0, 1) || !putbits(field, 8))
			return (NO);
		return (YES);
	case PREL:
	case DREL:
	case CREL:
		if (!putbits(1, 1) || !putbits(item, 2) || !putfld())
			return (NO);
		return (YES);
	}
	/* First put out special link <100>  */
	if (!putbits(4, 3) || !putbits(item - 4, 4))
		return (NO);

	switch (item) {
	case CSIZE:
	case XCHAIN:
	case EPOINT:
		if (!putbits(type, 2) || !putfld())
			return (NO);
		if (!putsym())
			return (NO);
		return (YES);

	case ENAME:
	case CNAME:
	case PNAME:
	case LNAME:
	case EXT:
		if (!putsym())
			return (NO);
		return (YES);

	case XMOFF:
	case XPOFF:
	case DSIZE:
	case SETLC:
	case CHAIN:
	case PSIZE:
		if (!putbits(type, 2) || !putfld())
			return (NO);
		return (YES);

	case EPROG:
		if (!putbits(type, 2) || !putfld())
			return (NO);
		if (outrem < 8 && !putbits(0, outrem))
			return (NO);/* finish byte */
		return (YES);

	case EFILE:
		if (outrem < 8 && !putbits(0, outrem))
			return (NO);/* finish byte */
		return (YES);
	}
	return (NO);
}

short int puttyp() {
	if (putbits(type, 2))
		return (YES); /* put 2-bit field type */
	return (NO);
}

short int putfld() { /* put low then high byte */
	if (putbits(field, 8) && putbits(field >> 8, 8))
		return (YES);
	return (NO);
}

short int putsym() { /* put symbol */
	short int i, j;
	char *cp;
	cp = symbol;
	if ((i = strlen(symbol)) > MAXSYM)
		i = MAXSYM; /* enforce max length */
	/* don't pad with spaces */
	/* for(j=0;(j < i) && (*cp++ != 0x20); ++j); */
	if (!putbits(i, BITPSYM))
		return (NO); /* put n bits for symbol length. 3 for spec, 4 for R99 */
	cp = symbol;
	while (i--) {
		if (!putbits(*cp++, 8))
			return (NO); /* put next byte */
	}
	/* *cp = '\0'; /* terminate symbol What was this for ??? AC */
	return (YES);
}

/*
 ** put next n bits from fld into REL file
 ** return true on success, false on error
 */
short int putbits(unsigned char fld, unsigned char nbits) {
	unsigned char put, n;
	n = nbits;
	while (n) { /* more bits to put */
		if (n > outrem)
			put = outrem;
		else
			put = n;
		/* First move the current bits over by the amount of the new put */
		outchunk = (outchunk << put);
		outchunk |= ((fld >> (n - put)) & ~(0xff << put));
		/* how many for this chunk */
		n -= put; /* decrement bits to put */
		outrem -= put; /* decr remaining bits */
		if (outrem == 0) { /* need another chunk */
			rputchr(outchunk, &hexbuf); /* put next bit cluster */
			outrem = 8; /* 8 bits remain */
			outchunk = 0;
		}
	}
	return (YES); /* success */
}

/*
 Simple hex function which could go into STDLIB2
 */
char ishex(char c) {
	return isdigit(c) || (c >= 'A' && c <= 'F');
}
