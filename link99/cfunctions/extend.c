/*
** if fn has no extension, extend it with ext1
** if fn has an extension, require it to match ext1 or ext2
** return true if fn's extension matches ext2, else false
** v2: case-insensitive extension matching
*/
#include "stdio.h"

#define NOCCARGC
#define MAXFN      15		/* max file name space */

extend(fn, ext1, ext2) char *fn, *ext1, *ext2; {
  char *cp, *ep, *p;
  /* find last dot after last path separator - handles .. paths */
  cp = NULL;
  for(p = fn; *p; p++) {
    if(*p == '\\' || *p == '/') cp = NULL;
    else if(*p == '.') cp = p;
  }
  if(cp) {
    for(ep = cp; *ep; ep++)
      *ep = toupper(*ep);
    if(strcmp(cp, ext2) == 0) return (YES);
    if(strcmp(cp, ext1) == 0) return (NO);
    puts2(fn, " - invalid extension");
    error(" - invalid extension");
  }
  if(strlen(fn) > MAXFN-4) error2(fn, " - Too Long");
  strcat(fn, ext1);
  return (NO);
}
