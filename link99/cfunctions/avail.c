
/*
** Return the number of bytes of available memory.

** In case of a stack overflow condition, if 'abort'
** is non-zero the program aborts with an 'S' clue,
** otherwise zero is returned.
*/
#include "stdio.h"
#define MAXMEM 64000
avail(int abort) {
/*  char x;
  if(&x < _memptr) {
    if(abort) exit(1);
    return (0);
    }
  return (&x - _memptr); */
return MAXMEM;
  }

