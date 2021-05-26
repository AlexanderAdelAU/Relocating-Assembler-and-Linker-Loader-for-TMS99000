/*
** rel.h -- header for REL file processing
*/

			/* item-type codes */

#define ABS       0	/* absolute item */
#define PREL      1	/* program (code) relative item */
#define DREL      2	/* data relative item */
#define CREL      3	/* common relative item */

#define ENAME     4	/* entry name */
#define CNAME     5	/* common block name */
#define PNAME     6	/* program name */
#define LNAME     7	/* library name */
#define EXT       8	/* extension link-item */

#define CSIZE     9	/* common size & name */
#define XCHAIN   10	/* external-reference-chain head & name */
#define EPOINT   11	/* entry point location & name */

#define XMOFF    12	/* external - offset */
#define XPOFF    13	/* external + offset */
#define DSIZE    14	/* data area size */
#define SETLC    15	/* set location counter for loading */
#define CHAIN    16	/* chain address (fill chain with loc ctr)  */
#define PSIZE    17	/* program (code) size */
#define EPROG    18	/* end of program */
#define EFILE    19	/* end of file */

#define MAXSYM    10	/* maximum symbol length allowed in REL file */
#define BITPSYM	  4 /* Number of bits to cover the length of the symbol.  Original spec is 3 */
#define ONES     0xffff	/* all one bits */


