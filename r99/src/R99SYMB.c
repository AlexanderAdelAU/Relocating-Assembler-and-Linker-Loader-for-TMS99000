/*
 TMS9900/99105  Cross-Assembler  v. 1.0

 January, 1985

 Original 6800 version Copyright (c) 1980 William C. Colley, III.
 Modified for the TMS9900/99105  Series by Alexander Cameron.

 File:	a99symb.c

 Routines to manipulate the symbol table.
 */

/*  Get Globals:  */

#include "r99gbl.h"
#include "r99ext.h"

/*
 This function checks the symbol table for a given symbol.  The function returns
 one of three values as follows:

 1 = symbol not found and symbol table full.
 0 = symbol found.  sympoint points to the matching entry.
 -1 = symbol not found.  sympoint points to where the symbol
 should have been.

 Jan 85 - Added code to cater for quadratic probing when handling
 collisions. (See Wirth's Algorithms + Data Structures = Programs
 pp 268)
 */
short int slookup(char *symbol) {
	short int h, d;
	d = 2;
	h = hash(symbol);
	sympoint = &symtbl[h];
	while ((sympoint->symname[0] & 0x7f) != '\0') {
		if (symcmp(symbol, sympoint->symname) == 0)
			return (0);
		h += d;
		d += 2;
		sympoint = &symtbl[h];
		if (h >= SYMBOLS)
			h -= SYMBOLS;
		if (d == SYMBOLS)
			return (1);
	}
	return (-1);
}

/*
 This function returns a hash value for a given symbol.  The hash value
 is calculated by folding the symbol name up into 16 bits (2 bytes, thus
 the symbol length must be even) mod the number of symbols.

 The earlier versions of the cross-assembler assigned j as an int short
 consequently if j took on a value greater than 32367 the entry pointer
 overflowed and the assembler wound up stuffing symbols into the  middle
 of the programme.  Assigning j to unsigned short solved this bug.
 */

short int hash(char *symbol) {
	short int i;
	unsigned short j, tsymbr, tsymbl;
	j = 0;
	for (i = 0; i < (SYMLEN / 2); i++) {
		//	tsymbl = (*symbol++ << 9);
		//	tsymbr = (*symbol++ >> 1);
		tsymbl = (*symbol++ << 7);
		tsymbr = (*symbol++) << 1;
		/* The above fixed  a problem with certain symbols not being found using
		 j += (*symbol++ << 8) + (*symbol++);

		 */
		j += (tsymbl + tsymbr);

	}
	return (j % SYMBOLS);
}

/*
 Function to sort the symbol table.  The function
 returns the number of entries in the table.

 Set flag if you just want a count
 */
short int sortsym(unsigned char flag) {
	short int n, t;
	struct symbtbl *tptr;
	n = 0;
	t = sizeof(symtbl) / SYMBOLS;
	for (tptr = sympoint = symtbl; tptr < symend; tptr++) {
		if ((tptr->symname[0] & 0x7f) != '\0') {

			if (!flag) {
				/*	strcpy(sympoint->symname,tptr->symname);
				 sympoint->symvalu = tptr->symvalu;
				 sympoint->symflg=tptr->symflg; */
				memcpy(sympoint->symname, tptr->symname, t);
				sympoint++;
			}
			n++;
		}
	}
	if (!flag)
		qsort(&symtbl, n, t, symcmp); /* 2 Bytes for addr 1 Byte for flags */
	return n;
}

/*
 This function compares two symbols.  It returns zero if the
 symbols are the same, not zero if they are different.
 */

short int symcmp(char *sym1, char *sym2) {
	char i;
	short int t;
	for (i = 0; i < SYMLEN; i++) {
		t = ((*sym1++ & 0x7f) - (*sym2++ & 0x7f));
		if (t != 0)
			break;
	}
	return (t);
}
/*
 This function adds a new entry to the symbol table.  The function
 returns values of either 0 or -1.  If the value is 0, the symbol is
 already in the table and the global variable sympoint points to the
 existing entry.  If the value is -1, the symbol has just been entered
 into the table and sympoint points to the new entry.  If the symbol
 table is full, the function triggers an abort of the assembly.
 */

short int addsym(char *symbol) {
	short int t;
	if ((t = slookup(symbol)) > 0) {
		wipeout("\nSymbol Table Overflow.\n");
	}
	if (t != 0) {
		memcpy(sympoint, symbol, SYMLEN);
		if (entflg)
			sympoint->symflg |= ENTBIT;
		if (extflg)
			sympoint->symflg |= EXTBIT;
		sympoint->symflg |= RELBIT; /* All symbols begin as Relocatable and are set by the PSUDO or OPs */
	}
	return (t);
}

/*
 Function to abort an assembly.  The parameter reason holds a string that
 will be printed to explain why the assembly bombed.  Note that I can't just
 call exit since this will not restore the currently logged disk drive, and
 software that changes the currently logged disk drive annoys me greatly.
 */

void wipeout(char *reason) {
	puts(reason);
	exit(-1);
}

