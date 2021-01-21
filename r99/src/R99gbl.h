/*
 TMS990 Series Cross-Assembler  v. 1.0

 January, 1985

 Copyright (c) 1980 William C. Colley, III.
 Modified for the 990 Series by Alexander Cameron.

 File:	a99.gbl

 Global macro substitutions and external variable declarations.

 */

/*  Set input line length:  */

#define	LINLEN 	512

/*  Size of File line length  */

#define	BUFSIZE 	513

/*  Define symbol table parameters:  REL files now allow 4 bits to define
 * the SYMLEN. The original spec allowed 3 bits
 *
 */

#define	SYMLEN	12	/*  Length of labels (must be an even number).	*/
#define	SYMBOLS	809	/*  Number of symbols in symbol table.(prime no)*/
#define	PADDING	"         "	/*  SYMLEN - 1 blanks.			*/
#define ENTBIT 4	/* Sets ENTRY bit in symflg 			*/
#define EQUBIT 16	/* Equivalence - defined via an equate 		*/
#define DUPBIT 32	/* Equivalence - defined via an equate 		*/
#define EXTBIT 8	/* Sets EXTERNAL bit in symflg			*/
#define	DEFBIT	2	/* Sets DEFINED bit				*/
#define RELBIT 1	/* Sets RELOCATION bit in symflg		*/
#define COMBIT 12	/* Sets COMMON bits in symflg			*/

/*  Number of if statements that can be nested:  */

#define	IFDEPTH	16

/*  BDOS functions called directly:  */

#define	LISTOUT	5	/*  Put character to list device.		*/
#define	LOGDISK	14	/*  Log in disk drive.				*/
#define	GETDISK	25	/*  Get currently logged in disk.		*/

/*  Define flag values:  */

#define	TRUE	1	/*  General use.				*/
#define	FALSE	0
#define	SKIP	TRUE	/*  Used with skip flag in getchr.		*/
#define	NOSKIP	FALSE
#define	BIGST	TRUE	/*  Used to tell kind of string in getitem.	*/
#define	SMALST	FALSE
#define	NOCODE	0	/*  Used to tell the hex generator what to do.	*/
#define	PUTCODE	1
#define	FLUSH	2
#define	NOMORE	3
#define EXTADDR 4

/*  File descriptors:  */

#define	LODISK	0	/*  Lowest numbered disk.			*/
#define	NOFILE	-1	/*  No file specified.				*/
#define	CONO	-2	/*  Console output.				*/
#define	LST	   -3	/*  List device.				*/

/*  Items can have the following attributes:  */

#define	ALPHA	0	/*  Alphabetic character.			*/
#define	NUMERIC	1	/*  Numeric digit (0-9).			*/
#define	END_LIN	2	/*  End-of-Line marker (cr or ;).		*/
#define	COMMA	3	/*  Field separator (,).			*/
#define	OPERATR	4	/*  Operator (* - GE < ( SHR etc.).		*/
#define	BAS_DES	5	/*  Base designator ($ % @).			*/
#define	QUOTE	6	/*  String delimiter (" ').			*/
#define	TRASH	7
#define	VALUE	8	/*  Evaluated expression.			*/
#define	BLANK	10	/*  White space (spc tab).			*/
#define COLON	12
#define HATCH 	13

/*  Some tokens for composite operators:  */

#define GTTKN		'>'
#define EQTKN		'='
#define LTTKN		'<'

#define	NO_OPR	0	/*  No operator.				*/
#define	GRTEQ	1	/*  Greater than or equal to.			*/
#define	NOTEQ	2	/*  Not equal to.				*/
#define	LESEQ	3	/*  Less than or equal to.			*/
#define	AND	4	/*  And.					*/
#define	OR	5	/*  Or.						*/
#define	XOR	6	/*  Exclusive or.				*/
#define	NOT	7	/*  1's complement.				*/
#define	MOD	8	/*  Mod--divide and return remainder.		*/
#define	SHL	9	/*  Shift left.					*/
#define	SHR	10	/*  Shift right.				*/
#define	HIGH	11	/*  High byte of.				*/
#define	LOW	12	/*  Low byte of.				*/

/*  Operator precedence values:  */

#define	UOP1	0	/*  Unary +, unary -.				*/
#define	MULT	1	/*  *, /, MOD, SHL, SHR.			*/
#define	ADDIT	2	/*  Binary +, binary -.				*/
#define	RELAT	3	/*  >, =, <, >=, <>, <=.			*/
#define	UOP2	4	/*  NOT.					*/
#define	LOG1	5	/*  AND.					*/
#define	LOG2	6	/*  OR, XOR.					*/
#define	UOP3	7	/*  HIGH, LOW.					*/
#define	RPREN	8	/*  ).						*/
#define	LPREN	9	/*  (.						*/
#define	ENDEX	10	/*  CR, ;, ,.					*/
#define	START	11	/*  Start of expression.			*/

/*  Bits of opcode attribute byte.  */

#define	PSOP	0x80	/*  Pseudo-op.					*/
#define	DIROK	0x40	/*  Non-pseudo-op is OK for direct addressing.	*/
#define	IFGROUP	0x40	/*  Pseudo-op is IF, ELSE, ENDI.		*/

/*  Address modes for the 9900 instruction set 				*/

#define WREG		0
#define	INDIRECT 	1
#define SYMBOLIC	2
#define INDEXED 	3
#define AUTOINC		4

/*  Misc control chars */

#define CPMEOF	0x1A
#define CTLS	19	/* Control S for stopping */
#define CTLC    0x03	/* Control C aborts */
#define NOSORT TRUE	/* Controls whether or not we want the table sorted */
#define SORT FALSE
#define INTEL8 TRUE    /* Generate Intel 16 or 32 Format */
#define FILE char

struct diskbuf {
	short int fd;
	unsigned char *pointr;
	unsigned char *endpoint;
	unsigned char space[BUFSIZE];
	FILE *fp;
};
struct diskbuf sorbuf, lstbuf, hexbuf;

/*  Set up the symbol table: */

struct symbtbl {
	char symname[SYMLEN];
	unsigned short symvalu;
	char symflg;
/* unsigned char none; */
};
struct symbtbl symtbl[SYMBOLS], *symend, *sympoint;

/*  If stack and stack pointer.  */

unsigned short ifsp;
unsigned short ifstack[IFDEPTH + 1];

/*  Buffer to hold current line:  */

char *linptr, linbuf[LINLEN];

/*  Buffer to hold the code generated by a line.  */

unsigned char binbuf[LINLEN], *bptr;
short int nbytes;

/*  Buffers for the hex generator.  */

char chksum, hxbytes, *hxlnptr, hxlnbuf[88];

/*  Miscellanious mailboxes:  */

unsigned char extflg; /*  Indicates external symbol			*/
unsigned char entflg; /*  Indicates and entry symbol			*/
unsigned char relflg; /*  Indicates whether inst operand need relocation */
short int errcode; /*  Error records.				*/
short int errcount;
short int evalerr; /*  Flag to tell of error in eval.		*/
unsigned short backflg, oldattr; /*  Item push-back buffer.			*/
unsigned short oldvalu;
short int curdrive; /*  Place to save drive that was current
 disk when assembly started.		*/
short int hexflg; /*  Flag for asmline to tell hex
 generator what to do.			*/
short int directok; /*  All symbols on line pre-defined.		*/
unsigned short pc; /*  Assembly program counter.			*/
unsigned short progsize;
unsigned short address; /*  Address to be put into listed line.		*/
char pass; /*  Which pass the assembly is in.		*/
char addrmode;
char quitflag; /*  Used in eval to detect INDEXED expressions  */
/*added by [AC]  */
unsigned char quoteflg;

/*-----------

 rel variables

 ------------*/
unsigned char outchunk, /* current output chunk in REL file */
outrem, /* remaining bits in outchunk */
item, /* current item code */
type; /* type field */
unsigned short field; /* current bit field */

char symbol[SYMLEN], /* current string */
progname[SYMLEN]; /* programme module's name */
unsigned short nssymbols;
unsigned short itemflg[3]; /* type of item found in getitem
 itemflg[0] = current item found
 itemflg[1] = operand 1 item type
 itemflg[2] = operand 2 item type
 */
unsigned short symptr[3]; /* external or entry symbol pointer
 symptr[0] -> current symbol
 symptr[1] -> operand 1 symbol
 symptr[2] -> operand 2 symbol
 */

/*  Some trivial functions:  */

#define	backchr	linptr--		/*  Push back a character.	*/

