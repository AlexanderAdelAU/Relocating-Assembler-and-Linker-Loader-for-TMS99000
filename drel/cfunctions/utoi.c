/*
** utoi -- convert unsigned decimal string to integer nbr
**          returns field size, else ERR on error
*/

int utoi(char *decstr, int *nbr)  {
  int d,t;
  d=0;

  *nbr=0;
  while((*decstr>='0')&(*decstr<='9')) {
    t=*nbr;t=(10*t) + (*decstr++ - '0');
    if ((t >= 0) & (*nbr < 0))
    	return (-2);
    d++; *nbr=t;
    }
  return d;
  }
