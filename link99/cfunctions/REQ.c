/*
** req.c -- request user input
*/
#include "stdio.h"

reqnbr(prompt, nbr)  char prompt[]; int *nbr; {		/* request number */
  char str[20];
  int sz;
 /* if(iscons(stdin)) { */
    puts("");
    fputs(prompt, stdout);
/*    } */
  getstr(str, 20);
  if((sz = utoi(str, nbr)) < 0 || str[sz]) return (NO);
  return (YES);
  }

reqstr(char prompt[], char *str, int sz) {	/* request string */
	char *c;
/*  if(iscons(stdin)) { */
    puts("");
    puts(prompt);
   /*   } */
  getstr(str, sz);
  /*  strcpy(str,"cc1.r99"); */
 return (*str);			/* null name returns false */
  }

getstr(str, sz) char *str; int sz; {	/* get string from user */
  int *cp;
  int c;
  gets(str);
  return;
  /*  if(iscons(stdin) && !iscons(stdout)) */
     puts(str);	 	/* echo */
  cp = str;
  while(*cp) {				/* trim ctl chars & make uc */
    if(*cp == '\n') break;
    c=*str;
    if(isprint(c = toupper(*cp++)))
    	++str;
    }
  *str = NULL;
  }
