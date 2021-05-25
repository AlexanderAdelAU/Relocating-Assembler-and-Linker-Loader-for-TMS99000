/*
** if fn has no extension, extend it with ext1
** if fn has an extension, require it to match ext1 or ext2
** return true if fn's extension matches ext2, else false
*/
#include "stdio.h"

#define NOCCARGC
#define MAXFN      15		/* max file name space */
extend(fn, ext1, ext2) char *fn, *ext1, *ext2; {
  char *cp;
  if(cp = strchr(fn, '.')) {
    if(strcmp(cp, ext2) == 0) return (YES);
    if(strcmp(cp, ext1) == 0) return (NO);
    puts2(fn, " - invalid extension");
    error(" - invalid extension");
    }
  if(strlen(fn) > MAXFN-4) error2(fn, " - Too Long");
  strcat(fn, ext1);
  return (NO);
  }
