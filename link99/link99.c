/*
 ============================================================================
 Name        : Link99.c
 Author      : Alex Cameron and J.E. Hendrix
 Version     : 3.0
 Copyright   : Free to use
 Description : A Linker Loader based on the Small-MAC linker.
 ============================================================================
 */

/*
 **		Includes Fix 6
 **
 ** LNK.C -- Small-Mac Linkage Editor (ver. 1.0)
 **
 **                    Copyright 1985 J. E. Hendrix
 **
 **                    Modified by Alex Cameron
 **                    	1. Port to TMS9900 architecture 1984
 **                     2. Eclipse Cross compiler version June 2015
 **
 ** Usage: LNK [-B] [-G#] [-M] program [module/library...]
 **
 ** -B                 A BIG program is being linked, so use all
 **                    of free memory for the symbol table and load the
 **                    program to disk entirely.  This is slower but it
 **                    gets the job done.
 **
 ** -G#                Make program absolute at address # (hex) and
 **                    output as "program.LGO" instead of "program.COM".
 **
 ** -D#                Make program use absolute data locations - Not Tested".
 **
 ** -S				   Generate small C call to main
 **
 **
 ** -M                 Monitor linking activity.
 **
 ** program            A file specifier for the program being linked.
 **                    The default, and only allowed, extension is REL.
 **
 ** module/library...  A list of zero or more module (.REL) and/or
 **                    library (.LIB) files.  Each module is linked to
 **                    the program and the libraries are searched for
 **                    just those modules which satisfy one or more
 **                    unresolved external references.
 **
 ** NOTE: Merely declaring a symbol to be external will cause
 ** it's module to be loaded.  It need not actually be referenced.
 **
 ** NOTE: The symbol ?MEMRY will allow the linker to insert the
 ** free memory pointer into this location.
 **
 ** The absence of an extension, or a .REL extension, identifies a module;
 ** whereas, a .LIB extension identifies a library.  If necessary, a
 ** library is rescanned to resolve backward external references between
 ** modules within the library. Module files and libraries are processed
 ** in the order in which they occur in the command line.
 **
 ** Drive Designators (e.g. B:):
 **    - allowed with module and library names
 **    - progra m drive designator locates the input .REL file
 **    - output goes to the default drive
 **
 ****
 ** Filename Extensions:
 **    - must specify .LIB with library name
 **    - standard extensions are:
 **
 **     .REL = relocatable object module
 **     .R99 = relocatable object module
 **     .LIB = library of object modules
 **     .NDX = index to library (not user specified)
 **     .COM = CP/M command file (default output)
 **     .LGO = load-and-go file (-G# output)
 **     .O$  = temporary overflow file
 **     .R$  = temporary reference file
 **
 ** Enter control-S to pause and control-C to abort.
 **
 ** NOTE: Compile only with Small-C 2.1 (edit level 63) or later.
 ** Edit 63 fixes CSYSLIB so that when it overflows a buffer while
 ** writing into a file it will no longer assume that it is at the
 ** end of the file.  This prevents it from padding a sector with
 ** 1A (hex) in the middle of a file when random access is being used.
 */

#include "rel99.h"
#include "stdio.h"
#include "fcntl.h"

/*  Size of File  */

#define	BUFSIZE 	512
#define DEBUG 			/* don't compile debug displays */
/*#define DEBUG2	 *//* don't compile debug detailed displays */

/* current string */
#define MAXFILES 20
#define MAXMEM 49000		/* 49kBytes max memory */
#define CRELSIZE 5			/* Cretble row size */

#define NOCCARGC		/* don't pass arg counts to functions */

#define NAMESIZE   15
#define MAXFIL     20
#define STACK     512			/* allow for stack space */
#define AUXBUF   8096			/* aux buffer for reference file */
#define RELMEM   (2000*CRELSIZE)	/* use for rel table 1000 entries 5 bytes each */
#define MAXOPEN     6			/* maximum files opened */
#define OHDOPEN   1024			/* memory overhead per open file */
#define COMBASE   0x0104		/* 0100H + 3 for 8080.  For 9900 => 0100H + 4*/
#define RET		  0x2DC0
#define JMP       0x0460		/* JMP instruction (0C3H) BRANCH for 9900 => 0460 Branch */
#define RES        -1			/* value of resolved ext ref */
#define XRPLUS     -2			/* ext-ref-plus-offset flag */
#define MODEXT  ".R99"
#define LIBEXT  ".LIB"
#define NDXEXT  ".NDX"
#define COMEXT  ".COM"
#define LGOEXT  ".LGO"
#define OFLEXT   ".O$"
#define REFEXT   ".R$"
#define DATEXT	 ".D$"
#define DRFEXT  ".DR$"
#define TRUE	 1
#define FALSE    0
/*
 * This is the development architecture.
 * If running this  on a 16bit platform alter this to Int16
 */
#define int32 1
/*
 ** symbol table definitions
 */
#if int16
#define NXT    0			/* next-entry pointer */
#define VAL    2			/* offset value */
#define SYM    4			/* symbol 4th byte in*/
#define FLG    (SYM+MAXSYM+1)		/* Assumes 10 bytes symbol length */
#define SSZ 	(SYM+MAXSYM+2)	/* size of table entry */
#define HIGH 	0x7f			/* high-value byte , i.e. end of ASCII Table*/
#define CUSHION  (200*SSZ)	/* reserved for table at overflow point */
#define	FLGEXT 0x80		/* Used by FLG to indicate the symbol is also external symbol  */
char high[] = {HIGH,0}; 	/* high-value symbol */
#endif

/*
 * Not FLG values are:
 * ASCII - normal symbol
 * HIGH - Last value in ASCII Table means entry is empty
 * FLGEXT - Symbol is an external symbol
 * PREL - Least Significant bit set, i.e. 0x01;
 */
#if int32
#define NXT    0			/* next-entry pointer */
#define VAL    4			/* offset value */
#define SYM    8			/* symbol 4th byte in*/
#define FLG    (SYM+MAXSYM+1)		/* Assumes 10 bytes symbol length */
#define SSZ 	(SYM+MAXSYM+2)	/* size of table entry */
#define HIGH 	0x7f			/* high-value byte , i.e. end of ASCII Table*/
#define CUSHION  (400*SSZ)	/* reserved for table at overflow point */
#define	FLGEXT 	0x80	/* Used by FLG to indicate the symbol is also external symbol  */
char high[] = { HIGH, 0 }; /* high-value symbol */
#endif

int inrel; /* file descriptor for input REL file */
int outrel; /* file descriptor for output REL file */
unsigned short inrem; /* remaining bits in inchunk */
unsigned short inchunk; /* current chunk from REL file */
unsigned short outrem; /* remaining bits in outchunk */
unsigned short outchunk; /* current chunk for REL file */
unsigned short item; /* current item code */
unsigned short type; /* type field */
unsigned short field; /* current bit field */

char symbol[10]; /* current symbol storage */

/*
 ** global variables
 */
char modname[MAXSYM + 1], /* name of current module */
infn[NAMESIZE], /* input filename */
ndxfn[NAMESIZE], /* index filename */
csfn[NAMESIZE], /* code seg filename */
crfn[NAMESIZE], /* code rel filename */
dsfn[NAMESIZE], /* data seg filename */
drfn[NAMESIZE], /* data rel filename */
outfn[NAMESIZE], /* output filename */
*crelptr, /* pointers to code rel table */
*drelptr; /* pointers to data rel table */

/*
 ** global buffer and pointer offsets
 */
char *bpnext, /* next byte in code buffer */
*sfree, /* head of freed entry list */
*epfirst, /* first entry point */
*epprev, /* previous entry point */
*epnext, /* next entry point */
*xrfirst, /* first external reference */
*xrprev, /* previous external reference */
*xrnext, /* next external reference */
*ep, /* entry point address for module*/
*xr; /* external reference */

unsigned short pass, /* pass number */
cdisk, /* disk overflow location */
dbase, /* data base address */
nxr, /* next in ext ref chain */
cloc, /* location counter */
dloc, /* data location counter */
csize, /* program size (fake unsigned) */
dsize, /* data size	*/
cbase, /* base address */
cmod, /* module location */
dmod, /* data module location */
goloc; /* go location */

char *snext; /* next symbol table entry */

char dreltble[RELMEM];
char creltble[RELMEM];
char buffer[MAXMEM];
char swap99[2]; /* TMS9900 temp swap variable */

unsigned short lgo, /* load-and-go format? */
smallC, /* If set we have to generate jmp to small C entry call to main */
monitor, /* monitor activity? */
instr, /* instruction to plant at 0000 */
addr, /* start address */
ref, /* reference to program relative item */
dref, /* reference to a data relative item */
big, /* linking a big program? */
xrplus, /* value of offset for next ext ref */
xrpflag = XRPLUS, /* value of xrplus flag */
inblock, /* block of next library member */
inbyte, /* byte in block of next library member */
csflag, /* code segment flag */
dsflag; /* data segment flag */
int ndxfd, /* index fd */
csfd, /* code segment fd */
crfd, /* code relative index fd */
dsfd, /* data segment fd */
drfd, /* data relative index fd */
outfd; /* output fd */
/* int from_cur[2 * MAXOPEN]; *//* Current position within file  */

extern int okay();
extern void putls();
void getsw();

int temp = 0;

/*
 * Usage: LNK [-B] [-G#] [-M] program [module/library...]
 */

int main(argc, argv)
	int argc;char **argv; {

	putls("----------------------------------------------------\n");
	putls("TMS9900 Reclocatable Object Library utility Version 3.1\n");
	putls("CP/M Version by Alexander Cameron January, 1985\n");
	putls("MSDOS Version by  Alexander Cameron May 2010 to July, 2019\n");
	putls("----------------------------------------------------\n");

	/* Debugging args

	 int argc;
	 int argv[30];
	 char *s1 = "LINK99";
	 char *s2 = "-M";
	 char *s3 = "test";
	 char *s4 = "puts2";
	 argc = 4;
	 argv[0] = s1;
	 argv[1] = s2;
	 argv[2] = s3;
	 argv[3] = s4;

	 */

	getsw(argc, argv); /* fetch and remember switches */
	getmem(); /* acquire maximum memory buffer */
	phase1(argc, argv); /* load and link */
	if (!okay())
		error("\nQuitting with unresolved symbols.");
	phase2(); /* generate final output */
}
/*
 **
 ** use this routine to look for $MEMRY,?MEMRY, etc
 ** and patch free memory value into cell
 **
 */
freemem() {

	char *fmval;
	int et, csch, cspg, dsch, dspg;
	epnext = getint(epfirst); /* first entry point */
	while (strcmp("?MEMRY", epnext + SYM) > 0) /* MEMRY > ent */
		epnext = getint(epnext);
	if (strcmp("?MEMRY", epnext + SYM) < 0) /* not there */
		return;
	et = *(epnext + FLG);
	ep = getint(epnext + VAL);
	fmval = cbase + csize + dsize; /* compute free memory pointer */
	_debug("buffer+ep=", fmval);
	if (et == PREL) {
		if (ep < cdisk)
			putint(buffer + getint(epnext + VAL), fmval);
		else {
			cspg = ctell(csfd);
			csch = ctellc(csfd);
			xrseek(ep - cdisk);
			write(csfd, &fmval, 2);
			lseek(csfd, cspg, SEEK_SET);
			lseek(csfd, csch, SEEK_SET);
		}
	}
	if (et == DREL) { /* if DREL seek pointer position */
		dspg = ctell(dsfd);
		dsch = ctellc(dsfd);
		dxrseek(ep);
		write(dsfd, &fmval, 2);
		lseek(dsfd, dspg, SEEK_SET);
		lseek(dsfd, dsch, SEEK_SET);
	}
}

/*
 ** get as much memory as possible for symbol table
 */
getmem() {
	char sz[9];
	int max;
	max = avail(YES); /* how much available? */
	max -= STACK + RELMEM + AUXBUF + AUXBUF + (MAXOPEN * OHDOPEN);
	bpnext = buffer; /*  malloc(max);	*//* allocate space */
/*	snext = buffer + (max - SSZ); */ /* first entry */
	snext = buffer + MAXMEM - SSZ;  /* first entry */
	sfree = 0; /* no reusable entries yet */

	/* Need to offset the index crelptr from zero due to the method of using pointers
	 * in resolve().  Because we are using a 16bit buffer and table
	 * pointers can't be used so there is a need to use buffer offsets.
	 * The problem to be over come is that an offset of zero would correspond to the
	 * end of and external chain link which is also zero.  Talk about a subtle
	 * bug!  9th May 2020 - During the COVID-19 isolation period :-)
	 *
	 */

	/* crelptr cannot have a zero index value  */
	crelptr = creltble; /* malloc(RELMEM); */
	drelptr = dreltble; /* = malloc(RELMEM); */
	put16int(crelptr, 0); /* null first entries */
	put16int(drelptr, 0);
	crelptr += CRELSIZE; /* bump so that first index is offset from 0 */
	drelptr += CRELSIZE; /* bump so that first index is offset from 0 */

#ifdef DEBUG
	if (monitor) {
		itou(max, sz, 8);
		puts2(sz, " Byte Buffer\n");
	}
#endif
	newtbl(&epfirst); /* set low and high ent pts */
	newtbl(&xrfirst); /* set low and high ext refs */
}

/*
 ** get next module name
 */
getname() {
	if (getrel() == PNAME) {
		strcpy(modname, symbol);
		return (YES);
	}
	if (item == EFILE)
		return (NO);
	error2(infn, " - Corrupted\n");
	return (NO);
}

/*
 ** read next entry from library index file
 */
getndx() {
	if (read(ndxfd, &inblock, 2) != 2 || /* next block */
	read(ndxfd, &inbyte, 2) != 2) { /* next byte in block */
		error2("\n- Error Reading ", infn);
	}
}

/*
 ** get switches from command line
 **
 ** Usage: LNK [-B] [-G#] [-M] program [module/library...]
 */
void getsw(argc, argv)
	int argc;char **argv; {
	char arg[NAMESIZE];
	int argnbr, b, len;
	argnbr = 0;
	dbase = 0;

	/* set from command line  */
	while (getarg(++argnbr, arg, NAMESIZE, argc, argv) != EOF) {
		if (arg[0] != '-')
			continue; /* skip file names */
		if (toupper(arg[1]) == 'G') {   /* load and go code base */
			lgo = YES;
			len = xtoi(arg + 2, &b);
			if (len >= 0 && !arg[len + 2])
				cbase = b;
			else
				usage();
		} else if (toupper(arg[1]) == 'D') {  /* Data base */
			len = xtoi(arg + 2, &b);
			if (len >= 0 && !arg[len + 2])
				dbase = b;
			else
				usage();
		} else if (toupper(arg[1]) == 'B')
			big = YES;
		else if (toupper(arg[1]) == 'M')
			monitor = YES;
		else if (toupper(arg[1]) == 'S')
			smallC = YES;
		else
			usage();
	}
}

/*
 ** is symbol an unresolved ext ref?
 ** on return of true, xrnext -> matching xr entry
 */
isunres() {
	int i;
	xrnext = getint(xrfirst);
	while (xrnext) {
		if ((i = strcmp(symbol, xrnext + SYM)) < 0)
			return (NO);
		if (i == 0) {
			if (!(*(xrnext + FLG) & FLGEXT)) /* Is the external bit set? */
				return (YES); /* acknowledge presence of ext ref's */
			else
				return (NO); /* which have already matched data entry points */
		}
		xrnext = getint(xrnext);
	}
	return (NO);
}

/*
 ** link external references to entry points
 */
link() {
	int cspg, csch, dspg, dsch;
	if (monitor) {
		putls("\nLinking.....");
	}
	cspg = ctell(csfd); /* remember temp file position */
	csch = ctellc(csfd);
	dspg = ctell(dsfd);
	dsch = ctellc(dsfd);
	xrnext = getint(xrprev = xrfirst); /* first external reference */
	epnext = getint(epfirst); /* first entry point */
	while (YES) {
		/* view symbols if debugging
		 *
		 * strcpy(temp, xrnext + SYM);
		 *	strcpy(temp2, epnext + SYM);
		 *
		 */
		if (strcmp(xrnext + SYM, epnext + SYM) > 0) { /* xr > ep */
			epnext = getint(epnext);
			continue;
		}
		if (strcmp(xrnext + SYM, epnext + SYM) < 0) { /* xr < ep */
			xrnext = getint(xrprev = xrnext);
			continue;
		}
		/* Default assumes xr == ep */
		if (*(xrnext + SYM) != HIGH) { /* End of Table */
			if (pass == 2)
				*(xrnext + FLG) &= HIGH; /* clear data flags  */
			if (pass == 1 && *(epnext + FLG) != PREL) { /* if an external symbol has a data entry point */
				*(xrnext + FLG) |= FLGEXT; /*We have a match so set the external bit - associated with it then flag for later linking */
				xrnext = getint(xrprev = xrnext); /* move to next external symbol */
				continue;
			}
			/* We have an external symbol refernece by xrnext so now
			 * match the entry and with an internal entry symbol if it exists
			 */
			resolve();
			putint(xrprev + NXT, getint(xrnext)); /* delink from xr chain */
			putint(xrnext + NXT, sfree); /* link to prev freed entry */
			sfree = xrnext; /* make first freed entry */
			xrnext = getint(xrprev); /* advance to next ext ref */
			continue; /* same ext ref in diff modules? */
		}
		break;
	}
	lseek(csfd, cspg, 0); /* restore temp file position */
	lseek(dsfd, dspg, 0);

}
/*
 ** load a module
 */
load() {
	char str[9];
	unsigned short doffloc, coffloc, creloff;
	if (monitor) {
		puts2("\nLoading.....", modname);
	}
	doffloc = coffloc = -1;
	epprev = epfirst; /* start at the very beginning */
	xrprev = xrfirst;
	do {
		poll(YES);
		switch (getrel()) {
		case ABS:
			if (csflag) { /* Code Segment Flag true ? */
				if (csfd) /* Code Segment File Descripter */
					write(csfd, &field, 1); /* put on disk */
				else
					*bpnext++ = field; /* put in buffer in memory */
				if (coffloc == cloc) { /* revision 24 */
					put16int(crelptr, xrpflag);
					put16int(crelptr + 2, cloc);
					crelptr += 4;
				}
				++cloc;
			}
			if (dsflag) {
				write(dsfd, &field, 1);
				if (doffloc == dloc) {
					put16int(drelptr, xrpflag);
					put16int(drelptr + 2, dloc);
					drelptr += 4;
				}
				++dloc;
			}
			break;
		case DREL:
		case PREL:
			/*
			 * First we must add the the module's code base to the programme relative
			 * reference except for zero fields
			 */
			if (!(field == 0)) {
				if (item == PREL)
					field += cmod;
				if (item == DREL)
					field += dmod;
			}

			if (csflag) {
				put16int(crelptr, cloc); /* reference the code location in buffer for pass 2 */
				put16int(crelptr + 2, field); /* This is the original chain value in the buffer */
				*(crelptr + 4) = item; /* and type */
				creloff = crelptr - creltble;
				if (csfd)
					write(csfd, &creloff, 2); /* put on disk */
				else {
					put16int(bpnext, creloff); /* put the reference to creltbl into buffer */
					bpnext += 2;
				}
				cloc += 2;
				crelptr += CRELSIZE;
			}
			if (dsflag) {
				put16int(drelptr, dloc); /* code reference for pass 2 */
				put16int(drelptr + 2, field); /* locations value */
				*(drelptr + 4) = item; /* and type */
				write(dsfd, &drelptr, 2); /* put on disk */
				dloc += 2;
				drelptr += CRELSIZE;
			}
			break;
		default:
			error("- Unsupported Link Item\n");
			break;
		case ERR:
			error("- Corrupt Module\n");
			break;
		case EPROG:
			if (type == PREL) {
				puts2("Start In ", modname);
				goloc = field + cmod;
			}
			break;
		case ENAME:
			break; /* bypass enames */

		case XCHAIN:
			newsym(&xrprev, xrfirst, "xr");
			break;

		case EPOINT:
			newsym(&epprev, epfirst, "ep");
			break;
		case PSIZE:
			cmod = cloc;
			if (monitor) {
				putls("\n");
				itox(field, str, 8);
				putls(str);
				putls(" Code Bytes at");
				itox(cloc, str, 6);
				putls(str);
				putls("'");
				itox(cloc + cbase, str, 6);
				putls(str);
				puts2(" ", modname);
				putls("\n");
			}
			/* On windows system this is not needed as we have ample memory*/
		/*	break; */
			if (!csfd && (big || (bpnext + field) > (snext - CUSHION))) {
				cdisk = cloc; /* disk overflow point */
				 /* open overflow file */
				csfd = open(csfn, O_CREAT | O_RDWR | O_BINARY);
				/*	from_cur[csfd] = 0; */
				if (monitor) {
					itox(cdisk, str, 8);
					puts2(str, " Overflow Point in code\n");
				}
			}
			break;
		case DSIZE:
			dmod = dloc;
			if (monitor) {
				putls("\n");
				itox(field, str, 8);
				putls(str);
				putls(" Data Bytes at");
				itox(dloc, str, 6);
				putls(str);
				putls("\"");
				itox(dloc + dbase, str, 6);
				putls(str);
				puts2(" ", modname);
				putls("\n");
			}
			if (!dsfd) {
				/*	dsfd = creat(dsfn, _IORW); /* create data object file */
				/*		dsfd = open(dsfn,O_RDWR|O_BINARY); *//* open data object file */
				if ((dsfd = open(dsfn, O_RDWR | O_BINARY)) == -1) {
					if ((dsfd = open(dsfn, O_CREAT | O_BINARY)) == -1)
						error2("Can't open file ", dsfn);
					/*	from_cur[dsfd] = 0; */
				}
			}
			break;
		case SETLC:
			if (type == DREL) {
				dsflag = TRUE;
				csflag = FALSE;
				field += dmod;
				while (dloc < field) { /* adj loc ctr */
					write(dsfd, "\0", 1);
					++dloc;
				}
			}
			if (type == PREL) {
				csflag = TRUE;
				dsflag = FALSE;
				field += cmod; /* Fix 6 */
				while (cloc < field) { /* adj loc ctr  to the align with the code load location*/
					if (csfd)
						write(csfd, "\0", 1);
					else
						*bpnext++ = 0;
					++cloc;
				}
			}
			break;
		case XPOFF:
			if (csflag) {
				put16int(crelptr, xrpflag); /* flag xr plus */
				put16int(crelptr + 2, field); /* xr offset */
				crelptr += 4;
				coffloc = cloc;
			}
			if (dsflag) {
				put16int(drelptr, xrpflag);
				put16int(drelptr + 2, field);
				drelptr += 4;
				doffloc = dloc;
			}
			break;
		}
	} while (item != EPROG);

	/* print out buffer if DEBUG enabled*/
#ifdef DEBUG2
	/* Output Symbol table and buffers etc  */
	unsigned char *bp, *rp, *ep;
	int i;
	char snum[12];

	puts("Outputting external links");
	ep = getint(epfirst); /* first entry point */
	while (YES) {

		//	putls((ep + SYM));
		printf("%s %08X \n", (ep + SYM), getint(ep + VAL));
		if (*(ep + SYM) != HIGH) { /* ext = ent */
			ep = getint(ep);
			continue; /* same ext ref in diff modules? */
		} else
			break;
	}

	bp = buffer;
	puts("Outputting buffer");
	printf("%04X ", 0);
	for (i = 0; i < cloc; i++) {
		printf("%02X ", *bp++);
		if ((i + 1) % 16 == 0) {
			printf("\n%04X ", i + 1);
		}
	}
	puts("\n\nOutputting creltble");
	rp = creltble;
	printf("%04X ", 0);
	for (i = 0; i < crelptr - creltble; i++) {
		printf("%02X ", *rp++);
		if ((i + 1) % 5 == 0) {
			printf("\n%04X ", i + 1);
		}
	}

#endif

}

/*
 ** create new file specifier from an old one
 */
newfn(char *dest, char *sour, char *ext) {
	if (sour[1] == ':' && strcmp(ext, NDXEXT))
		sour += 2;
	while (*sour && *sour != '.')
		*dest++ = *sour++;
	strcpy(dest, ext);
}

/*
 ** store new symbol table entry
 ** they arrive in alphanumeric order
 */
newsym(prev, first, ts)
	int *prev;int first;char *ts; {

	char at[9], *cp, *new;

	if ((new = sfree)) /* sfree is head of reusable or free entries */
		sfree = getint(sfree); /* use old entry */
	else {
		new = snext;
		snext -= SSZ; /* Bump for next symbol */
		if (snext < bpnext)
			error("- Must Specify -B Switch\n");
	}
	if (strcmp(symbol, *prev + SYM) < 0)
		*prev = first; /* tolerate M80 blunder fix 29 */
	cp = *prev;
	while (strcmp(symbol, cp + SYM) >= 0) { /* find position */
		*prev = cp; /* this becomes prev entry */
		cp = getint(cp + NXT);
	}
	putint(new, cp + NXT); /* point new entry ahead */
	putint(*prev, new); /* point prev entry here */
	*prev = new; /* this becomes prev entry */

	if (type == PREL)
		field += cmod; /* adjust for code module location */
	if (type == DREL)
		field += dmod; /* adjust for data module location */

	/* XChain enteries only the last link the the chain is used
	 * and the resolver() walks up the chain
	 */
	putint(new + VAL, field); /* load value */
	strcpy(new + SYM, symbol); /* load symbol */
	*(new + FLG) = type; /* type in symbol table */

#ifdef DEBUG
	if (monitor) {
		itox(getint(new + VAL), at, MAXSYM); /* Print the location of the Symbols in memory */
		putls(at);
		putls(" -t-");
		putls(ts);
		putls(" -s-");
		puts(symbol);
	}
#endif
}

/*
 ** initial table entries
 */
newtbl(low)
	int *low; {

	*low = snext; /* always points to low entry */
	strcpy(snext + SYM, ""); /* store low symbol */
	putint(snext + NXT, snext - SSZ); /*link to next (high) symbol */
	snext -= SSZ; /* 13 */
	strcpy(snext + SYM, high); /* store high symbol */
	putint(snext + NXT, 0); /* end of chain */
	snext -= SSZ; /* bump to next entry */
}

/*
 ** get next module name
 */
nxtmod() {
	getndx(); /* get location and */
	seek(); /* go straight to next member */
	return (getname());
}

/*
 ** report the outcome and decide whether to quit
 */
okay() {
	int err;
	char *eplast, *xr, *ep;

	err = eplast = 0;
	xr = xrfirst;
	ep = epfirst;
	xrnext = getint(xrfirst); /* first external reference */
	epnext = getint(epfirst); /* first entry point */

	/* Okay we grab xrnext and look for entry point symbols
	 * If xrnext points to HIGH then we are at the end
	 * */
	while (YES) {
		poll(YES);
		if (strcmp(xrnext + SYM, epnext + SYM) > 0) { /* ext > ent */
			if (epnext == eplast) {
				puts2("\n-  Redundant: ", xrnext + SYM);
				err = YES;
			}
			eplast = epnext;
			epnext = getint(epnext);
			continue;
		}
		if (strcmp(xrnext + SYM, epnext + SYM) < 0) { /* ext < ent */
			puts2("\n- Unresolved: ", xrnext + SYM);
			err = YES;
			xrnext = getint(xrnext);
			continue;
		}
		if (*(xrnext + SYM) != HIGH) { /* ext = ent */
			xrnext = getint(xrnext);
			continue; /* same ext ref in diff modules? */
		}
		break;
	}
	if (err)
		return (NO);
	return (YES);
}

/*
 ** load input files and library members
 */
phase1(argc, argv)
	int argc;int *argv; {
	char sz[9];
	int i, lib, eof;
	/* Initialise variables */
	pass = 1;
	csflag = TRUE;
	dsflag = FALSE; /* default state */
	eof = EOF;
	cdisk = 48000; /* high value for pointer - must declare to stop sign extenstion */
	puts("\nPhase 1 - Loading object and library files");
	if (lgo)
		instr = JMP; /* load and go format */
	else
		instr = JMP;
	cbase = COMBASE;
	i = 0;
	newfn(dsfn, "DATA", DATEXT); /*  DATA.$D  */
	newfn(drfn, "DATA", DRFEXT); /*   DATA.$DR */
	while (getarg(++i, infn, NAMESIZE, argc, argv) != EOF) {
		if (infn[0] == '-')
			continue; /* skip switches */
		if (extend(infn, MODEXT, LIBEXT)) /* check for .REL or .LIB extension */
			lib = YES;
		else
			lib = NO;
		if (!*outfn) { /* first file name */
			if (lgo)
				newfn(outfn, infn, LGOEXT); /* create cs, cr filenames from infn */
			else
				newfn(outfn, infn, COMEXT);

			newfn(csfn, infn, OFLEXT);  /*  O$  */
			newfn(crfn, infn, REFEXT); /*  R$  */
			/* Delete if they already exist */
			delete(csfn);
			delete(crfn);

			crfd = open(crfn, O_CREAT | O_RDWR | O_BINARY);
		}

		if (lib) {
			putls("\nSearching Library for symbol-> ");
			/* ifilelbuf(); /* Initialise file buffers */
			search(); /* search library if unresolved ext refs */
		} else {
			if ((inrel = open(infn, O_RDONLY | O_BINARY)) == -1) {
				error2("\nError opening source file: ", infn); /* must open */
			}
			puts3("\nOpening R99 File ", infn, "\n");
			ifilelbuf(); /* Initialise file buffers */
			getname(); /* program name */
			load(); /* load module */
			link(); /* link previous modules */
			close(inrel); /* must close */
		}
	}
	/*
	 *  now try to link data entry points to externals
	 */
	csize = cloc; /* discover module sizes */
	dsize = dloc;
	dbase = cbase + csize; /* compute data segment base */
	pass = 2; /* tell link() it's pass 2 */
	if (dsize)
		link(); /* link data blocks ,if present to code module */
	if (!*outfn)
		usage();
	freemem(); /* path free memory cell */

	if (ferror2(crfd))
		error2("\n- Error Writing ", crfn);

	write(crfd, &eof, 2);
	rewind(crfd);

	if (ferror2(csfd))
		error2("\n- Error Writing ", csfn);
	rewind(csfd);

	if (ferror2(dsfd))
		error2("\n- Error Writing ", dsfn);
	rewind(dsfd); /* rewind */

	crelptr = creltble + CRELSIZE; /* rewind rel table */
	drelptr = dreltble + CRELSIZE; /* and data segment file */

	itox(csize, sz, 5);
	lout("\n\tCODE SIZE   ");
	lout(sz);
	if (csize) {
		lout(" ("); /* output statistics */
		itox(cbase, sz, 5);
		lout(sz);
		lout("-");
		itox(cbase + csize - 1, sz, 5);
		puts2(sz, ")");
	}
	itox(dsize, sz, 5);
	lout("\n\tDATA SIZE   ");
	lout(sz);
	if (dsize) {
		lout(" ("); /* output statistics */
		itox(dbase, sz, 5);
		lout(sz);
		lout("-");
		itox(dbase + dsize, sz, 5);
		puts2(sz, ")");
	}
}

/*
 ** generate absolute output in COM or LGO format
 **
 ** COM format: JMP <start> <program>
 **
 ** LGO format: RET <start> <prog-base> <prog-size> <program>
 */
phase2() {
	char at[6], sz[8], c[6];
	short tp;
	char *tp2;
	char *epnext2;
	char main[MAXSYM + 1] = "main";
	char name[MAXSYM + 1] = "NAM";
	/* FILE *fp; */

	puts("\n\nPhase 2 - Writing execution files\n");

	epnext2 = getint(epfirst); /* first entry point */
	while (YES) {
		if (strcmp(StringPadRight(main, MAXSYM, " "), epnext2 + SYM) == 0) {
			_debug("Found main with value (goloc) = ", getint(epnext2 + VAL));
			goloc = getint(epnext2 + VAL);

		}
		/*
		 if (strcmp(StringPadRight(name, MAXSYM, " "), epnext2 + SYM) == 0) {
		 _debug("Found NAM with value (goloc) = ", getint(epnext2 + VAL));
		 goloc = getint(epnext2 + VAL);
		 break;
		 }
		 */
		if (*(epnext2 + SYM) != HIGH) { /* ext = ent */
			epnext2 = getint(epnext2);
			continue; /* same ext ref in diff modules? */
		} else
			break;
	}
	/* fp = fopen(outfn,"wb+");
	 fclose(fp); */

	if (!outfd) {
		if ((outfd = open(outfn, O_WRONLY | O_BINARY)) == -1) {
			if ((outfd = open(outfn, O_CREAT | O_WRONLY | O_BINARY)) == -1)
				error2("Error opening destination: ", outfn);
		}
	}
	/* Place branch instructions at begining of code to jump over data
	 * unless the "-C" option is chosen - that is small C calls _main## in the
	 * IOLIB which will (after the linking process) call back to main function.
	 * main: after the arguments have been determined.
	 *
	 */
	if (!smallC) {
		if (write99(outfd, &instr, 2) == -1) /* plant first instruction */
			error2("Error writing to: ", outfn);
		addr = cbase + goloc;
		if (write99(outfd, &addr, 2) == EOF) /* with its address */
			error2("Error writing to: ", outfn);
		if (lgo) {
			write99(outfd, &cbase, 2); /* where to load for execution */
			write99(outfd, &csize, 2); /* how many bytes to load */
		}
	}
	else {
		cbase = 0x100;
	}

	cloc = -1; /* allow efficient pre-increment offset by CRELSIZE as we don't want enteries at zero */
	crelptr = creltble + CRELSIZE; /* rewind rel table */
	drelptr = dreltble + CRELSIZE; /* and data segment file */

	ref = readref(); /* get first reference */
	while (++cloc < csize) { /* while more code */

		if (cloc != ref) { /* not relative reference */
			if (cloc < cdisk)
				field = *(buffer + cloc);
			else
				read(csfd, &field, 1);
			write(outfd, &field, 1); /* copy one byte as is */
			continue; /* bypass readref section */
		}
		if (cloc < cdisk) { /* get next 2-byte relative item */
			/*
			 * Modification due to 16 bit offset being used
			 */
			tp = get16int(buffer + cloc);
			field = get16int(creltble + tp + 2); /* and rel value from creltble */
			//	if (tp == 0) field = 0x6E;

		} else {
			read(csfd, &tp, 2);
			field = get16int(tp + 2);
		}
		if (*(tp + creltble + 4) == PREL)
			field += xrplus + cbase; /* make absolute revision 24 */
		if (*(tp + creltble + 4) == DREL)
			field += xrplus + dbase;
		xrplus = 0;
		write99(outfd, &field, 2); /* copy 2 bytes adjusted */
		++cloc; /* need additional increment */
		ref = readref(); /* get next reference */
	}
	/*  now do data section   */

	if (dsize) {
		dloc = -1; /* allow efficient pre-increment */
		ref = readdref(); /* get first reference */
		while (++dloc < dsize) { /* while more code */
			if (dref != dloc) { /* not relative reference */
				read(dsfd, &field, 1);
				write(outfd, &field, 1); /* copy one byte as is */
				continue; /* bypass readref section */
			}

			/* assume data relative item */
			read(dsfd, &tp, 2);
			field = get16int(tp + 2); /* get table value */
			tp2 = tp;
			if (*(tp2 + 4) == PREL) /*** AC **???*/
				field += xrplus + cbase; /* make absolute */
			if (*(tp2 + 4) == DREL)
				field += xrplus + dbase;
			xrplus = 0;
			write(outfd, &field, 2); /* copy 2 bytes adjusted */
			ref = readdref(); /* get next reference */
			++dloc; /* need additional increment */
		}
	}
	/*
	 de-allocate code ref files
	 */

	if (ferror2(outfd))
		error2("\n- Error Writing ", outfn);
	close(outfd);
	if (csfd) {
		if (ferror2(csfd))
			error2("\n- Error Reading ", csfn);
		close(csfd);
		delete(csfn);
	}
	if (ferror2(crfd))
		error2("- Error Reading ", crfn);
	close(crfd);
	delete(crfn);

	/*
	 de-allocate data ref files
	 */
	if (dsfd) {
		if (ferror2(dsfd))
			error2("\n- Error Reading ", dsfn);

		close(dsfd);
		delete(dsfn);
		if (remove(dsfn) != 0)
			putls("Error deleting file dsfn");
		else
			putls("dsfn File successfully deleted");

	}
}

/*
 ** read next reference
 */
int readref() {
	ref = get16int(crelptr); /* get next reference */
	if (ref == XRPLUS) { /* ext ref offset flag? */
		xrplus = get16int(crelptr + 2); /* yes, get offset value */
		crelptr += 4; /* point to next ref */
		ref = get16int(crelptr); /* then get reference */
		if (ref == XRPLUS) { /* revision 24 */
			xrplus -= cbase;
			crelptr += 2;
			ref = get16int(crelptr);
		}
	}
	crelptr += CRELSIZE; /* point to next entry */
	return ref;
}

/*
 ** read next data reference
 */
readdref() {

	dref = get16int(drelptr); /* get next reference */
	if (dref == XRPLUS) { /* ext ref offset flag? */
		xrplus = get16int(drelptr + 2); /* yes, get offset value */
		drelptr += 4; /* point to next dref */
		dref = get16int(drelptr); /* then get reference */
		if (dref == XRPLUS) { /* revision 24 */
			xrplus -= dbase;
			drelptr += 2;
			dref = get16int(drelptr);
		}
	}
	drelptr += 5; /* point to next entry */
	return dref;
}

/*
 ** resolve all external references to a given symbol
 ** Note resolving takes place in the 16 bit TMS9000 address space.
 */
resolve() {
	char at[7], tsymb[11];
	unsigned short tp, xr, ep;
	unsigned short et, xt, tbase;
	int rdebug = 1;

	/* First get the position of the symbol (sp)  in the code buffer.  Must use
	 * getint as symbols are in 32bit memory space whereas buffer pointers are
	 * character based.
	 * */
/*	strcpy(tsymb, xrnext + SYM); */
	if (!(xr = getint(xrnext + VAL))) /* head of ext ref chain */
		return 0;
	ep = getint(epnext + VAL); /* entry point that the external address is referening in target runtime code segment */
	xt = *(xrnext + FLG); /* determine which segment chain is in */
	et = *(epnext + FLG); /* Entry Type  */
	/* Use ep to get a absolute location where ep resides in the executable image */
	if (et == PREL) {
		tbase = ep + cbase;
	}
	if (et == DREL) {
		tbase = ep + dbase;
	}

	do {

#ifdef DEBUG
		if (rdebug++ > 150) {
			puts("Resolving Error:  Likely duplicate entry names- quitting");
			exit(0);
		}
		if (monitor) {
			poll(YES);
			putls("\n Resolving external ");
			itox(xt, at, 5);
			putls("xt = ");
			putls(at);
			itox(xr, at, 5);
			puts2(" ", at);
			if (xt == PREL)
				putls("' to ");
			else {
				putls("\" to ");
			}
			itox(tbase, at, 5);
			putls(at);
			puts2(" for ", xrnext + SYM);

			itox(nxr, at, 5);
			putls("nxr = ");
			putls(at);
		}
#endif
		switch (xt) {
		case DREL:
			dxrseek(xr);
			read(dsfd, &tp, 2);
			if (tp == 0) {
				dxrseek(xr);
				write(dsfd, &tbase, 2);
			} else {
				nxr = get16int(tp + 2);
				xt = get16int(tp + 4);
				*(creltble + tp + 4) = et;
				put16int(tp + 2, ep);
			}
			break;

		case PREL:
			if (xr < cdisk) {
				/* In the function load() we introduced the creltble offset to cater for the 16bit
				 * architecture.   Here we need to use it to obtain the correct offset.
				 * Use xr to get the pointer to the creltble where the table is organised as
				 * buffer offset (2 Bytes) - xr chain value(2 Bytes) - REL Type (1 Byte)
				 */
				tp = get16int(buffer + xr);

				if (tp == 0) {
					//	put16int(xr + buffer, tbase);

				} else {
					nxr = get16int(creltble + tp + 2); /* next xr in the code chain */
					xt = *(creltble + tp + 4); /* external reference type */
					*(creltble + tp + 4) = et; /* don't know why this is being reset it is already set */
					put16int(creltble + tp + 2, ep); /* Int16 because we are writing to a true 16 bit address space */

					/* if nxr is equal to zero it means that the original buffer value was zero so we
					 * and that there are no further links in the chain. We can exit by setting tp = 0.
					 */
					if (nxr == 0)
						tp = 0;
				}
			} else {
				putls("Overflow.....");
				xrseek(xr - cdisk);
				read(csfd, &tp, 2);
				if (tp == 0) {
					xrseek(xr - cdisk);
					write(csfd, &tbase, 2);
					putls("check write.2.");
				} else {
					nxr = getint(creltble + tp + 2);
					xt = *(creltble + tp + 4);
					*(creltble + tp + 4) = et;
					put16int(creltble + tp + 4, ep);
				}

			}
			break;
		}
		xr = nxr;
	} while (tp);
#ifdef DEBUG2
	unsigned short i;
	unsigned char *bp, *rp;
	bp = buffer;
	puts("Outputting final buffer");
	printf("%04X ", 0);
	for (i = 0; i < cloc; i++) {
		printf("%02X ", *bp++);
		if ((i + 1) % 16 == 0) {
			printf("\n%04X ", i + 1);
		}
	}
	puts("\n\nOutputting final creltble");
	rp = creltble;
	printf("%04X ", 0);
	for (i = 0; i < crelptr - creltble; i++) {
		printf("%02X ", *rp++);
		if ((i + 1) % 5 == 0) {
			printf("\n%04X ", i + 1);
		}
	}
#endif
	return 0;
}
/*
 ** search a library
 */
search() {
	int linked;
	linked = NO;
	newfn(ndxfn, infn, NDXEXT);
	/* ndxfd = open(ndxfn, "r"); */
	ndxfd = open(ndxfn, O_RDONLY | O_BINARY);
	/*	inrel = open(infn, "r"); */
	inrel = open(infn, O_RDONLY | O_BINARY);
	ifilelbuf();
	while (YES) { /* rescan till done */
		while (nxtmod()) {
			while (getrel() == ENAME) {
				poll(YES);
				if (isunres()) { /* unresolved reference? */
					putls(symbol);
					putchar('\n');
					load(); /* load module */
					link(); /* link to previous ones */
					linked = YES;
					break;
				}
			}
		}
		if (!linked) /* AC mod */
			break;
		linked = NO;
		rewind(ndxfd);
	}
	close(ndxfd);
	close(inrel);
}

/*
 ** seek to next member in old library
 */
seek() {
	if (inblock == EOF)
		error("- Premature End of Index");
	/*  rewind(inrel); */
	if (lseek(inrel, inblock << 7, SEEK_SET) == EOF)
		error("- Corrupt Library or Index");
	if (lseek(inrel, inbyte, SEEK_CUR) == EOF)
		error("lseekc - Corrupt Library or Index"); /* fixed by AC */

	inrem = 0; /* force getrel() to read a byte */
}

/*
 ** abort with a usage message
 */
usage() {
	error("\nUsage: LNK [-B] [-S] [-G#] [-D#] [-M] [-D#] program [module/library...]");
}

/*
 ** seek external reference in 128 byte blocks
 */
xrseek(byte)
	int byte; {
	/*	if (lseek(csfd, (byte >> 7) & 511, 0) == EOF) */
	if (lseek(csfd, byte, SEEK_CUR) == EOF)
		error2("- Seek error in xr file ", csfn);
}

/*
 ** seekdata segment external reference
 */
dxrseek(byte)
	int byte; {
	/* if (lseek(dsfd, (byte >> 7) & 511, 0) == EOF) */
	if (lseek(dsfd, byte, SEEK_CUR) == EOF)
		error2("- Seek error in data file ", dsfn);
}

/*
 ** replace library version to get

 ** rid of line feed
 */
lout(string)
	char *string; {
	putls(string);
}

_debug(var, val)
	int var;int val; {
	char at[6];
	itox(val, at, 5);
	puts2(var, at);
	puts("\n");
}
rewind(fd)
	int fd; {
	lseek(fd, 0, 2); /* rewind */
	lseek(fd, 0, 0); /* rewind AC */
}

/*
 addition for the 9900/99105 to keep the bytes the right way around

 */
write99(int fd, char *buf, int n) {
	char *t; /* swap high an low byte */
	t = swap99;
	*t++ = *(buf + 1); /* high byte */
	*t = *buf; /* low byte  */
	return (write(fd, swap99, n)); /* swap complete */

}
pint99(int *addr, int i) { /* integer swap for 9900 */
	char *t;
	t = addr;
	*t++ = i >> 8;
	*t = i;
}
StringPadRight(char *string, int padded_len, char *pad) {
	int len;
	int i;
	len = strlen(string);
	if (len >= padded_len) {
		return string;
	}

	for (i = 0; i < padded_len - len; i++) {
		strcat(string, pad);
	}
	return string;
}
