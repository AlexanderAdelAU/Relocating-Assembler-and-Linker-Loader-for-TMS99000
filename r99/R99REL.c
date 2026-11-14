/*
 **  General rel interface routines
 */
#include "R99gbl.h"
#include "REL99.h"
#include "R99Ext.h"

#define NO 0
#define YES 1
#define NULL 0
#define ERR -1

/* ==========================================================================
 * Explicit external-reference accumulator (XREFS scheme - see rel99.h).
 *
 * Every reference to an EXT symbol records (symbol, module-relative location)
 * here during pass 2; at END the grouped list is emitted by xref_emit() as:
 *     EXT "XREFS" / XCHAIN <count>,<name> / CHAIN <loc> * count
 * The reference word itself is left as ABS 0 in the object; the linker
 * patches each recorded location with the symbol's final absolute address.
 * ======================================================================== */

struct xrefent {
	char           name[SYMLEN];
	unsigned loc;
};

struct xrefent xreftab[MAXXREF];
int      nxref;

/* Called at the start of each module's object output (relhead). */
xref_reset() {
	nxref = 0;
}

/* Record one reference site.  name is the SYMLEN space-padded symbol name;
 * loc is the module program-counter location of the operand word. */
xref_add(name, loc)
char *name;
unsigned loc;
{
	if (nxref >= MAXXREF) {
		printf("\nXREF table overflow (>%d external references)\n", MAXXREF);
		wipeout("\n");
		return;
	}
	memcpy(xreftab[nxref].name, name, SYMLEN);
	xreftab[nxref].loc = loc;
	++nxref;
}

/* Emit one EXT "XREFS" + XCHAIN(count,name) + CHAIN(loc)* group. */
xref_group(first)
int first;
{
	int j, count;

	count = 0;
	for (j = first; j < nxref; ++j)
		if (!(xreftab[j].name[0] & 0x80) &&
		    !symcmp(xreftab[j].name, xreftab[first].name))
			++count;

	item = EXT;                        /* EXT "XREFS" marker */
	memcpy(symbol, REL99_EXT_XREFS, SYMLEN);
	putrel();

	item  = XCHAIN;                    /* header: field = count, symbol = name */
	type  = ABS;
	field = count;
	memcpy(symbol, xreftab[first].name, SYMLEN);
	putrel();

	for (j = first; j < nxref; ++j) {  /* one CHAIN item per location */
		if ((xreftab[j].name[0] & 0x80) ||
		    symcmp(xreftab[j].name, xreftab[first].name))
			continue;
		xreftab[j].name[0] |= 0x80;
		item  = CHAIN;
		type  = PREL;
		field = xreftab[j].loc;
		putrel();
	}
}

/* Emit every accumulated group.  Called from relsyms() at END, pass 2. */
xref_emit() {
	int i;

	if (pass < 2)
		return;
	for (i = 0; i < nxref; ++i)
		xreftab[i].name[0] &= 0x7f;
	for (i = 0; i < nxref; ++i)
		if (!(xreftab[i].name[0] & 0x80))
			xref_group(i);
}

/*
 Function to form the hex output line and put it to
 the hex output device.
 */

relout()
{
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
				hxlnptr = puthex2(*bptr++, hxlnptr); /* put out lsb of word */
				count++;
				for (count2 = 0; count2 < SYMLEN; count2++)
					hxlnptr = puthex2(*sptr++, hxlnptr);
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
relhead() {
	xref_reset();       /* start a fresh reference list for this module */
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
entnam() {
	int n, t;
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
relsyms() {
	int n;

	n = nssymbols;
	sympoint = symtbl;
	while (n > 0) {
		/* Entry points only: externals are emitted by xref_emit() as
		 * explicit grouped reference lists (XREFS scheme). */
		if (sympoint->symflg & ENTBIT) {
			item  = EPOINT;
			type  = PREL;
			field = sympoint->symvalu;
			memcpy(symbol, sympoint->symname, SYMLEN);
			putrel();
		}
		sympoint++;
		--n;
	}
	xref_emit();        /* emit grouped EXT "XREFS" reference lists */
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
putabs2() {
	unsigned f;
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

putabs() {
	/* *bptr++ = field; */
	putrel();
}

putrel()
{
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

puttyp()
{
	if (putbits(type, 2))
		return (YES); /* put 2-bit field type */
	return (NO);
}

putfld()
{ /* put low then high byte */
	if (putbits(field, 8) && putbits(field >> 8, 8))
		return (YES);
	return (NO);
}

putsym()
{
	int i;
	char *cp;

	cp = symbol;
	i = 0;

	while (i < MAXSYM && cp[i] != '\0' && cp[i] != ' ')
		++i;

	if (!putbits(i, BITPSYM))
		return NO;

	while (i--) {
		if (!putbits(*cp++, 8))
			return NO;
	}

	return YES;
}

/*
 ** put next n bits from fld into REL file
 ** return true on success, false on error
 */
putbits(fld, nbits)
unsigned char fld;
unsigned char nbits;
{
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
ishex(c)
char c;
{
	return isdigit(c) || (c >= 'A' && c <= 'F');
}
