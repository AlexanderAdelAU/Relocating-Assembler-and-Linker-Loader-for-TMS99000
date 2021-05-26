/*
** seerel -- show REL items
*/
#include "rel99.h"
#include "stdio.h"


int lc, width;
/*
** display REL item
** on call:
**    item = item code
**    type = type of field
**   field = value of field
**  symbol = symbol name
*/ 

/* common variables */
extern int
inrel,			/* file descriptor for input REL file */
inrem,			/* remaining bits in inchunk */
inchunk,		/* current chunk from REL file */
outrel,		/* file descriptor for output REL file */
outrem,		/* remaining bits in outchunk */
outchunk,		/* current chunk for REL file */
item,			/* current item code */
type,			/* type field */
field;			/* current bit field */
extern char symbol[9];		/* current string */

seerel() {
  char str[MAXSYM]; int tmp;
  switch(item) {
    case   ABS: see8(field, ' '); lc += 1; newlin(NO); return;
    case   PREL: 
    case   DREL: 
    case   CREL: see16(); lc += 2; newlin(NO); return;
    case  XMOFF:
    case  XPOFF: tmp = type; type = item; see16();
                 type = tmp; newlin(NO); return;

    case  ENAME: seenam("     entry: ", NO); goto eol;
    case  CNAME: seenam("    common: ", NO); goto eol;
    case  PNAME: putchar('\n');
                 seenam("-  program: ", NO);
                 lc = 0;
                 goto eol;
    case  LNAME: seenam("   library: ", NO); goto eol;
    case    EXT: putstring("extension link item\n"); return;

    case  CSIZE: seenam(" common sz: ", YES); goto eol;
    case XCHAIN: seenam(" ext chain: ", YES); goto eol;
    case EPOINT: seenam("  entry pt: ", YES); goto eol;

    case  DSIZE:  putstring(" data size: "); goto fld;
    case  SETLC:  putstring("load at: "); lc = field; goto fld;
    case  CHAIN:  putstring(" ld chn at: "); goto fld;
    case  PSIZE:  putstring(" prog size: "); goto fld;

    case  EPROG:  putstring("- end prog: "); goto fld;
    case  EFILE:  putstring("- end file");   goto eol;

            fld: see16();
            eol: newlin(YES);
                 return;
    }
  itou(item, str, MAXSYM);
  puts2(str);
  puts2(" is an Unknown Item Code\n");
  }

see8(int value, int suff) {	/* display 8-bits */
  char str[5];
  if(width == 0 && item < CREL) {	/* need loc ctr pref */
    itox(lc, str, 5);
    outz(str);				/* output loc ctr */
    putchar(' ');			/* output spacer */
    }
  itox(value & 255, str, 3);		/* convert to hex string */
  outz(str);				/* output hex byte */
  if(suff) putchar(suff);		/* output suffix? */
  ++width;				/* bump line width */
  }

 see16() {				/* display field */
  see8(field >> 8, 0);			/* display high byte  */
  see8(field, xtype());			/* display low byte & type */
  putchar(' ');			/* output spacer */
  }
/* display symbol */
 seenam(char *pref, int val) {
  newlin(YES);
  width = 1;				/* avoid address prefix */
  putstring(pref);
  if(val) see16();			/* output a value */
  putstring(symbol);
  }

 xtype() {
  switch(type) {
    case   ABS: return(' ');
    case  PREL: return('\'');
    case  DREL: return('\"');
    case  CREL: return('~');
    case XPOFF: return('+');
    case XMOFF: return('-');
    }
  return('?');
  }

 newlin(int nl) {			/* decide about new line */
  if(width > 15 || (nl && width)) {
	 putchar('\n');
    width = 0;
    }
  }

 outz(char *str) {			/* zero fill and output str */
  char *cp;
  cp = str;
  while(*cp == ' ') *cp++ = '0';	/* supply leading zeroes */
  putstring(str);
  }


