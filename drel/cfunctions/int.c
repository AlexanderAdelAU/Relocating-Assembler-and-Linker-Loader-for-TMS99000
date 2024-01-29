/*
** integer manipulation
*/
/*
** get integer of length len from address addr
** (byte sequence set by "putint")
** Only used for symbols and 16 bit addresses
*/
unsigned short get16int(a) char *a;{
	unsigned short hi,lo;
	hi = *a++ << 8;
	lo = *a  & 0x00ff; /* Needed to cater for sign extending when msb bit is set */
	return ((hi | lo));

}
put16int(a,i) char *a; int i;{
	*a++ = i >> 8;
	*a = i;
}
/* This is necessary for working with 32bit implementations while targeting
 * a 16 bit architecture.   The buffer uses 32bit pointers but only 16 bit
 * pointers or addresses are stored in the buffer
 */
getaddr(a) char *a; {
	unsigned int lo16, hi16, addr;
	lo16 = *a++;
	lo16 = lo16 << 8;
	lo16  = *a;
	hi16 = a;
	hi16 &= 0xffff0000;  /* get hi word of address */
 	addr = hi16 | lo16;		/* Now 16 bit address is converted */
	return (addr);
}

/*
 * The purpose of this is to convert higher order integers to
 * 16bit suitable for the TMS99000 etc
 */
 putaddr(a,i) char *a; int i;{
	int low16;
	low16 = i & 0x0000ffff;
	*a++ = low16>>8;
	*a = low16;
}

getint(a) int *a;{
	return (*a);
}


putint(a,i) int *a; int i;{
	*a = i;
}
