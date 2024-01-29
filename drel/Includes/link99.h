/*
** miscellaneous definitions
*/
#define MAXFN      15		/* max file name space */
#define INTSZ       2		/* integer size in bytes */
#define COMMENT   ';'		/* comment delimiter */
#define ANOTHER   '|'		/* another operand option */
#define MAXLINE    81		/* length of source line */
#define MICOUNT   150		/* machine instruction hash space */
#define MIOPNDS   300		/* maximum unique operand formats */
#define MIBUFSZ  4600		/* mit syntax space */
#define OBJEXT  ".R99"		/* object file extension */
#define SRCEXT  ".MAC"		/* source file extension */
#define MAXLAB      8		/* maximum label characters used */
#define STACK    1024		/* reserved for stack space */
#define OHDOPEN   164		/* overhead bytes per open file */
#define MAXOPEN     2		/* maximum open files */

/*
** symbol table
*/
#define STMAX   500			/* maximum lables allowed */
#define STVALUE (MAXLAB + 1)		/* offset to value field */
#define STFLAG  (STVALUE + INTSZ)	/* offset to flag byte */
#define STENTRY (STFLAG + 1)		/* st entry size */
#define STBUFSZ (stmax * STENTRY)	/* st buffer size */
#define LABBIT2 128			/* label flag (pass 2) */
#define LABBIT   64			/* label flag */
#define EQUBIT   32			/* EQU flag (pass 2) */
#define SETBIT   16			/* SET flag */
#define XRBIT     8			/* external-reference flag */
#define EPBIT     4			/* entry-point flag */
#define RELBITS   3			/* relative bits (ABS, PREL) */

/*
** macro table
*/
#define MTNXT       0		/* pointer to next macro */
#define MTNAM   INTSZ		/* macro name */

/*
** assembler instruction codes
*/
#define DW     1
#define DB     2
#define DS     3
#define EX     4
#define SET    5
#define EQU    6
#define ORG    7
#define END    8
#define MACRO  9
#define ENDM  10
#define CALL  11
