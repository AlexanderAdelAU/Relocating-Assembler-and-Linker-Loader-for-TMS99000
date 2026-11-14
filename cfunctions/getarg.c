/*
** Get command line argument.
** Entry: n    = Number of the argument.
**        s    = Destination string pointer.
**        size = Size of destination string.
**        argc = Argument count from main().
**        argv = Argument vector(s) from main().
** Returns number of characters moved on success,
** else EOF.
**
** ** Usage: LNK [-B] [-G#] [-M] program [module/library...]
**   argc =3;
  argv[0]="LINK99";
  argv[1] = "-M";
  argv[2]= "Sieve.r99";
*/
#include "stdio.h"

/* n is number of arguments
 * s = name argument
 * size is the size constraint
 * argc is argument count
 * argv is argument vector
 *
 */

int getarg(n, s, size, argc, argv) int n; char *s; int size; int argc; char **argv;{

	 char *str;
  int i;
  if(n < 0 | n >= argc) {
    *s = NULL;
    return EOF;
    }
  i = 0;
  str=argv[n];
  while(i<size) {
    if((s[i]=str[i])==NULL) break;
    ++i;
    }
  s[i]=NULL;
  return i;
  }
