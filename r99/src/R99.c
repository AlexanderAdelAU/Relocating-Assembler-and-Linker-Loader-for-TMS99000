/*
 9900 Cross-Assembler  v. 2.1

 May, 1980

 Copyright (c) 1980 William C. Colley, III.

 File:	r99.c

 It all begins here.
 */

/*  Get globals:  */

#include "stdio.h"
#include "r99gbl.h"
#include "R99ext.h"
#include "fcntl.h"

/*  The assembler starts here.  */

main(short int argc, char *argv[]) {
	short int n, m;
	unsigned short u;
	putls("\n---------------------------------------------");
	putls("\nTMS9900 Relocatable Cross-Assembler  vers 1.0\n");
	putls("Copyright (c) 1980  William C. Colley, III\n");
	putls("(TMS 99105A version by Alexander. Cameron Jan 1984 and May 2015 )\n");
	putls("---------------------------------------------\n");
	setfiles(argc, argv);

	sympoint = &symtbl; /*  Initialize symbol table.	*/
	symend = symtbl[SYMBOLS].symname;
	memset(sympoint, '\0', (SYMBOLS * (sizeof(symtbl) / SYMBOLS))); /* symflg added  */
	memset(itemflg, '\0', 3); /*  Initialize encode buffer	*/
	memset(symptr, '\0', 3);
	ifsp = 0; /*  Initialise if stack.	*/
	ifstack[ifsp] = 0xffff;
	hxbytes = 0; /*  Initialise hex generator.	*/
	pc = errcount = progsize = 0;
	pass = 1;
	puts("Pass 1\n");
	while (pass != 3) /*  The actual assembly starts here.	*/
	{
		errcode = ' ';
		if (!getlin()) {
			strcpy(linbuf, "\tEND\t\t;You forgot this!\n");
			linptr = linbuf;
			markerr('*');
			ifstack[ifsp] = 0xffff;
		}
		asmline(); /*  Get binary from line.	*/
		if (pass > 1) {
			lineout(); /*  In pass 2, list line.	*/
			relout(); /*  In pass 2, build rel file.	*/
		} else
			progsize += nbytes; /* keep track of module size */
		pc += nbytes;
		if (pass == 0) /*  This indicates end of pass 1.	*/
		{
			pc = errcount = nbytes = 0;
			pass = 2;
			puts("Pass 2\n");
			source_rewind();
			relhead(); /* output module rel header */
		}
	}
	/*  print statistics  */

	printf("Programme size = %d  %x%s\n", progsize, progsize, "(Hex)");
	u = (SYMBOLS - nssymbols);
	u = 100 * u / SYMBOLS;
	printf("\nSymbol table use factor = %d%s\n", 100 - u, "%");
	/*	printf("Symbol table use factor = %d%s\n",100-u,"%"); */

	/*  List number of errors.	*/

	linptr = linbuf;
	*linptr++ = '\n';
	if (errcount == 0)
		strcpy(linptr, "No");
	else {
		putdec(errcount, &linptr);
		*linptr = '\0';
	}
	strcat(linbuf, " error(s).\n");
	puts(linbuf);
	if (lstbuf.fd != CONO && lstbuf.fd != NOFILE) {
		putlin(linbuf, &lstbuf);
		/*		putchr('\f',&lstbuf); */
	}
	if (lstbuf.fd != NOFILE) /*  If needed, sort and list
	 symbol table.		*/
	{
		n = sortsym(SORT);

	//	n = nssymbols;
		sympoint = symtbl;
		while (n > 0) {
			linptr = linbuf;
			for (m = 0; m < 4; m++) {
				memcpy(linptr, sympoint->symname, SYMLEN); 
				linptr += SYMLEN-1; /* Not sure why this works */
				*linptr++ = ' ';
				//*linptr++ = ' ';
				puthex4(sympoint->symvalu, &linptr);
				if (sympoint->symflg & EXTBIT)
					*linptr++ = '*'; /* mark external */
				else if (sympoint->symflg & RELBIT)
					*linptr++ = '\''; /* mark relocatable */
				else
					*linptr++ = ' ';
				//*linptr++ = ' ';
				//*linptr++ = ' ';
				*linptr++ = ' ';
				sympoint++;
				if (--n <= 0)
					break;
			}
			linptr -= 4;
			*linptr++ = '\n';
			*linptr = '\0';
			putlin(linbuf, &lstbuf);
		}
		putchr(CPMEOF, &lstbuf);
	}
	flush(&lstbuf);
	close(sorbuf.fd);
	wipeout("\r");
}

/*
 Function to set up the file structure.  Routine is called with
 the original argc and argv from main().
 */

setfiles(short int argc, char *argv[]) {

	char sorfname[24], lstfname[24], hexfname[24], *tptr;
	FILE fp;
	if (--argc == 0)
		wipeout("\nNo file info supplied.\n");
	argv++;
	sorbuf.fd = lstbuf.fd = hexbuf.fd = NOFILE;
	lstbuf.pointr = lstbuf.space;
	hexbuf.pointr = hexbuf.space;
	sorfname[0] = curdrive + 'A';
	sorfname[1] = ':';
	sorfname[2] = '\0';
	strcpy(progname, *argv); /* copy programme name */
	strcat(sorfname, *argv++);
	for (tptr = sorfname; *tptr != '\0'; tptr++)
		if (*tptr == '.')
			*tptr = '\0';
	strcpy(lstfname, sorfname);
	strcpy(hexfname, lstfname);
	strcat(sorfname, ".A99");
	strcat(lstfname, ".L99");
	strcat(hexfname, ".R99");
	if (--argc == 0)
		goto defsorf;
	while (**argv != '\0') {
		switch (*(*argv)++) {
		case 'S':
			switch (*(*argv)++) {
			case 'A':
			case 'B':
			case 'C':
			case 'D':
				sorfname[0] = *(*argv - 1);

			case '-':
				if ((sorbuf.fd = open(sorfname, _O_RDWR)) == -1)
					wipeout("\nCan't open source.\n");
				source_rewind();
				break;

			default:
				goto badcomnd;
			}
			break;

		case 'L':
			switch (*(*argv)++) {
			case 'A':
			case 'B':
			case 'C':
			case 'D':
				lstfname[0] = *(*argv - 1);
			case '-':
				if ((lstbuf.fd = open(lstfname, _O_RDWR | _O_CREAT | _O_BINARY,
						0664)) == -1)
					wipeout("\nCan't open list.\n");
				break;

			case 'X':
				lstbuf.fd = CONO;
				break;

			case 'Y':
				lstbuf.fd = LST;
				break;

			default:
				goto badcomnd;
			}
			break;

		case 'H':
			switch (*(*argv)++) {
			case 'A':
			case 'B':
			case 'C':
			case 'D':
				hexfname[0] = *(*argv - 1);
			case '-': /*if (( hexbuf.fd = open(hexfname,_O_RDWR,_O_BINARY|_O_CREAT|_O_TRUNC|_O_EXCL)) == -1)*/
				if ((hexbuf.fd = open(hexfname, _O_RDWR | _O_CREAT | _O_BINARY,
						0664)) == -1)
					wipeout("\n Can't open R99 file.\n");
				// 	if ( (hexbuf.fp=fopen(hexfname, "wb+")) == NULL)
				//		wipeout("\n Can't open hex.\n");
				break;

			case 'X':
				hexbuf.fd = CONO;
				break;

			case 'Y':
				hexbuf.fd = LST;
				break;

			default:
				goto badcomnd;
			}
			break;

			badcomnd: default:
			wipeout("\nIllegal command line.\n");
		}
	}
	/* if (sorbuf.fd != NOFILE) return 0; */

	defsorf:

	if ((sorbuf.fd = open(sorfname, 0)) == -1)
		wipeout("\nCan't open source.\n");
	source_rewind();

}

