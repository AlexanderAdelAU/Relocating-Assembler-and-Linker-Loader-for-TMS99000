/*
** putrel -- write a relocatable-object file
*/
#include "rel99.h"
#include "stdio.h"


			/* common variables */
extern int
 outrel,		/* file descriptor for output REL file */
 outrem,		/* remaining bits in outchunk */
 outchunk,		/* current chunk for REL file */
 item,			/* current item code */
 type,			/* type field */
 field;			/* current bit field */
extern char symbol[9];		/* current string */


/*
** put next REL item
** return true on success, false on error
** on call:
**    item = item code
**    type = type of field
**   field = value of field
**  symbol = symbol name
*/ 
int putrel() {
  switch(item) {
    case ABS:
      if(!putbits(0, 1) || !putbits(field, 8)) return (NO);
      return (YES);
    case PREL:  case DREL:  case CREL:
      if(!putbits(1, 1) || !putbits(item, 2) || !putfld()) return (NO);
      return (YES);
    }
  /* item -4 gives special link item 15 (End of File, ie all 1s) */
  /* This dual state first puts 001 and then the next field xxxx */
  if(!putbits(4, 3) || !putbits(item-4, 4)) return (NO);
  switch(item) {

  	case CSIZE:  case XCHAIN:  case EPOINT:
  	  if(!putbits(type, 2) || !putfld()) return (NO);
  	  if(!putsym()) return (NO);
  	  return (YES);


    case ENAME:  case CNAME:  case PNAME:  case LNAME:  case EXT:
      if(!putsym()) return (NO);
      return (YES);

    case XMOFF:  case XPOFF:  case DSIZE:
    case SETLC:  case CHAIN:  case PSIZE:
      if(!putbits(type, 2) || !putfld()) return (NO);
      return (YES);

    case EPROG:
      if(!putbits(type, 2) || !putfld()) return (NO);
      if(outrem < 8 && !putbits(0, outrem)) return (NO);	/* finish byte */
      return (YES);

    case EFILE:
      if(outrem < 8 && !putbits(0, outrem)) return (NO);	/* finish byte */
      return (YES);
    }
  return (NO);
  }

int puttyp() {
  if(putbits(type, 2)) return (YES);	/* put 2-bit field type */
  return (NO);
  }

int putfld() {				/* put low then high byte */
  if(putbits(field, 8) && putbits(field >> 8, 8)) return (YES);
  return (NO);
  }

int putsym() {				/* put symbol */
  int i; char *cp;
  if((i = strlen(symbol)) > MAXSYM) i = MAXSYM;	/* enforce max length */
  if(!putbits(i, BITPSYM)) return (NO);	/* put symbol length */
  cp = symbol;
  while(i--) {
    if(!putbits(*cp++, 8)) return (NO);	/* put next byte */
   }
  *cp = NULL; /* terminate symbol */
  return (YES);
  }

/*
** put next n bits from fld into REL file
** return true on success, false on error
** Must initialise outrem to 8 otherwise leading zero is written to library.
*/
int putbits(int fld, int n) {
  int put;
  while(n) {					/* more bits to put */
    if(n > outrem) put = outrem; else	put = n;	/* how many for this chunk */
    /* First move the current bits over by the amount of the new put */
    outchunk = (outchunk << put);
    outchunk |= ((fld >> (n-put)) & ~(0xff << put));
    n      -= put;				/* decrement bits to put */
    outrem -= put;				/* decr remaining bits */
    if(outrem == 0) {				/* need another chunk */

      if (outchunk == 0xC1)
    	  outrem = 8;
    	if(write(outrel, &outchunk, 1) != 1) {	/* put next bit cluster */
    	  puts("\n\7- Write Error in REL File\n");
    	  return (NO);				/* failure */
      }
      outrem = 8;				/* 8 bits remain */
      outchunk = 0;
     }
  }
  return (YES);					/* success */
  }
  
