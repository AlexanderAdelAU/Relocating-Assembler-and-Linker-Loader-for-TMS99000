/*
 TMS9900/99105  Cross-Assembler  v. 1.0

 January, 1985

 Original 6800 version Copyright (c) 1980 William C. Colley, III.
 Modified for the TMS9900/99105  Series by Alexander Cameron.

 File:	a99get.c

 Routines to get source text from the disk and return it in
 manageable chunks such as operators, labels, opcodes, etc.
 */

/*  Get Globals:  */
#include "r99gbl.h"
#include "r99ext.h"

/* Function to get an opcode from the present source line and return
 its value and attributes.  The function returns -1 if no opcode was
 on the line, 0 if the opcode was not in the opcode table, and 1
 otherwise.  If the opcode was on the line, but not in the table, the
 error is marked up.
 */

int getopcod(unsigned short *value, char *attrib)

{
	char c, dxop[2 * SYMLEN], temp[SYMLEN + 1];

	switch (getchr(&c, SKIP)) {
	case ALPHA:
		backchr;
		getname(temp);
		if (bbsearch(temp, value, attrib, 0, numopcs(), getopc))
			return 1;
		else {
			strcpy(dxop, temp);
			strcat(dxop, PADDING);
			if (slookup(dxop)) {
				markerr('O');
				return 0;
			}

			*value = sympoint->symvalu; /* DXOP's value */
			*attrib = 0x01; /* uses format 1 */
			return 1;
		}

	default:
		markerr('S');
		return -1;

	case END_LIN:
		return 0;

	}
}

/*
 Function to look up an item in a fixed table by binary search.  The function
 returns 1 if the item was found, 0 if not.  name points to the name to be
 looked up (null terminated).  value and attrib catch two bytes of parameters
 attached to the item if the item is found.  high and low are for the benefit of
 the recursion.  For the first call use the number of items in the table for
 high and 0 for low.  table is a pointer to a function to get an item from the
 appropriate table.  Note that in the comparison, lower case is converted to
 upper case.
 */

int bbsearch(char *name, unsigned *value, unsigned *attrib, int low, int high,
		void (*table)()) {
	char temp[5];
	char *pntr1, *pntr2;
	int t, check;
	if (low > high)
		return 0;
	check = low + ((high - low) >> 1);
	(*table)(check, &temp, value, attrib);
	pntr1 = temp;
	pntr2 = name;
	while ((t = toupper(*pntr2++) - *pntr1) == 0 && *pntr1++ != '\0')
		;
	if (t == 0)
		return 1;
	if (t > 0)
		return bbsearch(name, value, attrib, check + 1, high, table);
	return bbsearch(name, value, attrib, low, check - 1, table);
}

/*
 Function to push back an item so that getitem will retrieve it
 a second time.  Only one level of pushback is supported.
 */

void backitem(int value, unsigned attrib)
/* unsigned short value, attrib; */
{
	oldvalu = value;
	oldattr = attrib;
	backflg = TRUE;
}

/*
 Internal function in getnum to convert a character to a digit.  The base
 is passed so that this routine can check for digits larger than the base.
 */

int _aton(char letter, unsigned short base) {
	char n;
	if (letter >= '0' && letter <= '9')
		n = letter - '0';
	else if (letter >= 'A' && letter <= 'F')
		n = letter - ('A' - 10);
	else {
		markerr('E');
		evalerr = TRUE;
		return 0;
	}
	if (n < base)
		return n;
	markerr('D');
	evalerr = TRUE;
	return 0;
}

/*
 Function to get an Intel format number from the present input line.
 value points to the number to be returned, and base points to an
 unsigned short where the base of the number will be saved during internal
 computations.  The function returns a fixed value of 1 and flags errors
 as it goes along.
 */

short int getnum(unsigned short *number, unsigned char *base) {
	char c1, c2;
	unsigned short b;
	/*	return 0xffff; */
	if ((c1 = getchr(&c2, NOSKIP)) != ALPHA && c1 != NUMERIC) {
		backchr;
		return 0xffff;
	}
	if ((b = getnum(number, base)) == 0xffff) {
		*number = 0;
		switch (toupper(c2)) {
		case 'H':
			*base = 16;
			return 1;
		case 'D':
			*base = 10;
			return 1;
		case 'O':
		case 'Q':
			*base = 8;
			return 1;
		case 'B':
			*base = 2;
			return 1;
		default:
			*base = 10;
			b = 1;
		}
	}
	*number += b * _aton(toupper(c2), *base);
	return b * *base;
}

/*
 Function to get an item from the present source line.  Items may be
 looked-up symbols, numeric constants, string constants, operators, etc.
 The function returns the attribute of the item gotten.  value is a
 pointer to where the value of the item will be stored, and quoteflg
 determines whether getitem will look for one- and two-character string
 constants or not.
 */

unsigned char getitem(unsigned short *value, unsigned char quoteflg)

{
	char c, c1, *tmpptr, tempbuf[2 * SYMLEN];
	unsigned char base, attrib;
	if (backflg) {
		backflg = FALSE;
		*value = oldvalu;
		return oldattr;
	}
	switch ((attrib = getchr(&c, SKIP))) {
	case END_LIN:
	case COMMA:
		return attrib;

	case NUMERIC:
		backchr;
		getnum(value, &base);
		return VALUE;

	case BAS_DES:
		switch (c) {
		case '#':
			base = 16;
			break;

		case '%':
			base = 2;
		}
		*value = 0;
		while ((attrib = getchr(&c, NOSKIP)) == NUMERIC || attrib == ALPHA) {
			*value = *value * base + _aton(toupper(c), base);

		}
		backchr;
		return VALUE;

	case OPERATR:
		switch (c) {
		case '>':
		case '=':
		case '<':
			getchr(&c1, NOSKIP);
			if (c == '>' && c1 == '=')
				c = GRTEQ;
			else if (c == '>' && c1 == '<')
				c = NOTEQ;
			else if (c == '=' && c1 == '>')
				c = GRTEQ;
			else if (c == '=' && c1 == '<')
				c = LESEQ;
			else if (c == '<' && c1 == '>')
				c = GRTEQ;
			else if (c == '<' && c1 == '=')
				c = LESEQ;
			else
				backchr;

		default:
			*value = c;
			return attrib;
		}

	case QUOTE:
		if (quoteflg) {
			*value = c;
			return attrib;
		}
		tmpptr = tempbuf;
		attrib = 0;
		while ((getchr(&c1, NOSKIP), c1) != '\n' && c1 != c)
			if (++attrib <= 2)
				*tmpptr++ = c1;
		if (c1 == '\n' || attrib > 2) {
			if (c1 == '\n')
				backchr;
			evalerr = TRUE;
			markerr('"');
			return VALUE;
		}
		tmpptr = tempbuf;
		for (*value = 0; attrib != 0; attrib--)
			*value = (*value << 8) + *tmpptr++;
		return VALUE;

	case ALPHA:
		backchr;
		getname(tempbuf);
		if ((*value = chkoprat(tempbuf) & 0xff) != NO_OPR)
			return OPERATR;
		strcat(tempbuf, PADDING);
		switch (slookup(tempbuf)) {
		case 0:
			/* this returns the original value - for EXTBITs it will be zero */
			*value = sympoint->symvalu;
			if (pass == 2) {
				if (sympoint->symflg & EXTBIT) { /* If it was marked external now make it relocatable */
					sympoint->symvalu = pc + nbytes - 2; /*  set new chain link - 2 is for opcode */
					sympoint->symflg |= RELBIT;
				/*	if (*value) */
				/*		sympoint->symflg |= RELBIT;  */	/* last item in chain link must remain zero */
				} 		/* however other links in chain must be relocatable */
				itemflg[0] |= sympoint->symflg; /* assume REL & XXX = REL */
				symptr[0] = &(sympoint->symname);
				relflg = sympoint->symflg & RELBIT;
			}
			if (sympoint->symname[0] > 0x7f)
				directok = FALSE;
			return VALUE;

		case -1:
			if (extflg == TRUE && pass == 1) {
				if (addsym(tempbuf)) {
					sympoint->symvalu = 0; /* show as last link in chain */
					sympoint->symflg &= EXTBIT; /* Assume not found is external and Relocatable */
					relflg = sympoint->symflg & RELBIT;
					return VALUE;
				}
			} else {
				evalerr = TRUE;
				markerr('U');
				return VALUE;
			}
		}
	}
}

/*
 Function to get a symbol name, opcode, or label from the present source
 line.  The name is returned (null terminated) in buffer.  Names are
 terminated by any non-alphanumeric character.

 Names terminated by '##' are flagged as 'external' symbols and thus
 generate a zero value in the object, and an 'X' indicator in the REL
 file output.
 */
void getname(char *buffer)
/* char *buffer; */
{
	char c, d, count;
	count = 0;
	while ((c = getchr(&d, NOSKIP)) == ALPHA || c == NUMERIC)
		if (++count <= SYMLEN)
			*buffer++ = d;
	switch (c) {
	case HATCH:
		if ((c = getchr(&d, NOSKIP)) == HATCH)
			extflg = TRUE;
		else
			backchr;
		*buffer = '\0';
		return;

	case COLON:
		if ((c = getchr(&d, NOSKIP)) == COLON)
			entflg = TRUE;
		else
			backchr;
		*buffer = '\0';
		return;

	case QUOTE:  /* Allow quote ' mark to be used for certain labels */
		*buffer++ = d;

		if ((c = getchr(&d, NOSKIP)) == COLON)
			;
		else
			backchr;

		*buffer = '\0';
		return;

	}
	backchr;
	*buffer = '\0';
}

/*
 Function to check a string against the list of operators that get spelled out.
 This list consists of GE, GT, LE, LT, EQ, NE, AND, OR, XOR, NOT, MOD, SHL,
 SHR, HIGH, LOW.  The function returns the token for the operator if the
 string is an operator, the token NO_OPR otherwise.
 */

short int chkoprat(char *string) {
	short int t;

	t = 0;
	return NO_OPR;
	/*	unsigned short s,t;
	 return (bsearch(string,&s,&t,0,numoprs(),getopr())) ? t : NO_OPR; */
}

/*
 Function to get a character from the present source line.  The function
 returns the attribute of the character fetched.  value is a pointer to
 the location where the character itself will be saved, and skipflg
 determines whether or not white space is skipped.
 */

unsigned char getchr(char *value, unsigned char skipflg) {
	unsigned char c;
	while (TRUE) {
		c = getattr(*value = *linptr++);
		if ((c != TRASH) && (c != BLANK || skipflg != SKIP)) {
			return c;
		}
	}
}

/*
 Function to get a new line of source text from the disk.  Returns 0
 if EOF encountered, 1 otherwise.
 */

short int getlin() {
	char c;
	short int count;
	linptr = linbuf;
	count = 0;
	backflg = FALSE;
	while (movchr(&c)) {
		if (c == '\n') {
			*linptr++ = c;
			*linptr = '\0';
			linptr = linbuf;
			return 1;
		}
		if (count < LINLEN)
			*linptr++ = c;
		count++;
	}

	return 0;
}
/*
 Function to flush rest of line - called after assembly is complete for a
 particular line, for example multiple comment fields on the same line
 */

short int flushlin() {
	char c;
	short int count;
	linptr = linbuf;
	count = 0;
	backflg = FALSE;
	while (movchr(&c)) {
		if (c == '\n') {
			*linptr++ = c;
			*linptr = '\0';
			linptr = linbuf;
			return 1;
		}
		if (count < LINLEN)
			*linptr++ = c;
		count++;
	}

	return 0;
}

/*
 Function to move a character from the source disk buffer to the
 location dest.  Returns 0 if no character available, 1 otherwise.
 */

int movchr(char *dest) {
	int short t;
	t = 0;

	/*	if (sorbuf.pointr >= sorbuf.space + 128) */
	if (sorbuf.pointr >= sorbuf.endpoint) {
		sorbuf.pointr = sorbuf.space;
		if ((t = read(sorbuf.fd, sorbuf.space, BUFSIZE)) == -1) {
			printf("\nDisk read error. ++ %s\n", "TI_READERR");
			wipeout("\n");
		}
		sorbuf.endpoint = sorbuf.space + t; /*  if (t != 128)  sorbuf.space[t+1] = CPMEOF; */
		if (t == 0) {
			sorbuf.space[t] = CPMEOF;
			sorbuf.pointr = sorbuf.space;
			sorbuf.endpoint = sorbuf.space;
			return 0;
		}
	}
	return ((*dest = *sorbuf.pointr++ & 0x7f) == CPMEOF) ? 0 : 1;
}

int seek(int short fd, int short i, int short j) {

	lseek(sorbuf.fd, 0, 0);

}
/*
 Function to rewind the source file.
 */

void source_rewind() {
	seek(sorbuf.fd, 0, 0);
	sorbuf.pointr = sorbuf.space; /* this forces a read */
	sorbuf.endpoint = sorbuf.space + BUFSIZE;
	sorbuf.pointr = sorbuf.endpoint;
}

