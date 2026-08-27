/*
 TMS9900/99105  Cross-Assembler  v. 1.0

 January, 1985

 Original 6800 version Copyright (c) 1980 William C. Colley, III.
 Modified for the TMS9900/99105  and to be relocatable by Alexander Cameron.

 File:	a99put.c

 List and hex output routines.
 */

/*  Get globals:  */

#include "R99gbl.h"
#include "R99Ext.h"

/*
 Function to form the list output line and put it to
 the list device.  Routine also puts the line to the
 console in the event of an error.
 */
lineout() {
	char tbuf[25], *tptr, *bptr, conbuf[LINLEN];
	int count, test;
	memset(tbuf, ' ', 24);
	tbuf[24] = '\0';
	memset(conbuf, '\0', LINLEN);
	tptr = tbuf;
	*tptr++ = errcode;
	tptr++;
	if (hexflg != NOCODE)
		tptr = puthex4(address, tptr);
	else
		tptr += 4;
	tptr += 3;

	/*
	 * BSS uses hexflg == FLUSH and nbytes as the amount by which the
	 * relocatable location counter advances.  Those bytes do not exist
	 * in binbuf, so listing them would read beyond the encoding buffer.
	 */
	if (hexflg == FLUSH) {
		putlin(tbuf, &lstbuf);
		putlin(linbuf, &lstbuf);

		if (lstbuf.fd != CONO && errcode != ' ') {
			strcat(conbuf, tbuf);
			strcat(conbuf, linbuf);
			puts(conbuf);
		}
		return;
	}

	count = 0;
	bptr = binbuf;
	while (TRUE) {
		test = nbytes;
		if ((count == nbytes) || (count != 0) && (count % 4 == 0)) {
			putlin(tbuf, &lstbuf);
			if (count > 4)
				putchr('\n', &lstbuf);
			else
				putlin(linbuf, &lstbuf);
			if (lstbuf.fd != CONO && errcode != ' ') {
				strcat(conbuf, tbuf);
				if (count >= 5)
					putchar('\n');
				else {
					linbuf[28] = '\n';
					linbuf[29] = '\0';
					strcat(conbuf, linbuf);
				}
				puts(conbuf);
			}
			tptr = tbuf + 2;
			tptr = puthex4(address, tptr);
			memset(tptr, ' ', 14);
			tptr += 3;
		}
		if (count == nbytes)
			return;
		count++;
		address++;
		tptr = puthex2(*bptr++, tptr);
		if (count % 2 == 0)
			tptr++;
	}
}

/*
 Function to form the hex output line and put it to
 the hex output device.
 */

hexout8() {
	char *bptr, reclen;
	unsigned count;
	reclen = 16;
	switch (hexflg) {

	case PUTCODE:
		bptr = binbuf;
		for (count = 1; count <= nbytes; count++) {
			hxlnptr = puthex2(*bptr, hxlnptr);
			chksum += *bptr++;

			if (++hxbytes == reclen)
				flshhbf(pc + count);
		}

	case NOCODE:
		return;

	case FLUSH:
		flshhbf(pc);
		return;

	case NOMORE:
		flshhbf(0);
		putlin(":0000000000\n\032", &hexbuf);
		flush(&hexbuf);
		return;
	}
}
/*
 Function to form the hex output line and put it to
 the hex output device.
 We need to add High and Low sements as well
 */

hexout16() {
	char count, *bptr, reclen;
	reclen = 32;
	switch (hexflg) {
	case EXTADDR:
		flshhbf(0);
		putlin(":020000040000FA\n", &hexbuf);
		/*  flush(&hexbuf); */
		return;

	case PUTCODE:
		bptr = binbuf;
		for (count = 1; count <= nbytes; count++) {
			hxlnptr = puthex2(*bptr, hxlnptr);
			chksum += *bptr++;
			if (++hxbytes == reclen)
				flshhbf(pc + count);
		}

	case NOCODE:
		return;

	case FLUSH:
		flshhbf(pc);
		return;

	case NOMORE:
		flshhbf(0);
		putlin(":00000001FF\n\032", &hexbuf);
		flush(&hexbuf);
		return;
	}
}

/*
 Function to put a line of intel hex to the appropriate
 device and get a new line started.
 */

flshhbf(loadaddr)
unsigned loadaddr;
{
	if (hxbytes != 0) {
		hxlnptr = puthex2(-(chksum + hxbytes), hxlnptr);
		*hxlnptr++ = '\n';
		*hxlnptr++ = '\0';
		hxlnptr = hxlnbuf + 1;
		hxlnptr = puthex2(hxbytes, hxlnptr);
		putlin(hxlnbuf, &hexbuf);
	}
	hxbytes = 0;
	hxlnptr = hxlnbuf;
	*hxlnptr++ = ':';
	hxlnptr += 2;
	hxlnptr = puthex4(loadaddr, hxlnptr);
	hxlnptr = puthex2(0, hxlnptr);
	chksum = (loadaddr >> 8) + (loadaddr & 0xff);
}

/*
 Function to put a 4-digit hex number into an output line.
 */

puthex4(number, lineptr)
unsigned number;
char *lineptr;
{
	lineptr = puthex2(number >> 8, lineptr);
	return puthex2(number, lineptr);
}

/*
 Function to put a 2-digit hex number into an output line.
 */

puthex2(number, lineptr)
unsigned char number;
char *lineptr;
{
	if ((*lineptr = (number >> 4) + '0') > '9')
		*lineptr += 7;
	lineptr++;
	if ((*lineptr = (number & 0x0f) + '0') > '9')
		*lineptr += 7;
	lineptr++;
	return lineptr;
}

/*
 Function to put a decimal number into an output line.
 */

putdec(number, lineptr)
unsigned number;
char *lineptr;
{
	if (number == 0)
		return lineptr;
	lineptr = putdec(number / 10, lineptr);
	*lineptr++ = number % 10 + '0';
	return lineptr;
}

/*
 Function to move a line to a disk buffer.  The line is pointed to
 by line, and the disk buffer is specified by its disk I/O buffer
 structure dskbuf.
 */

putlin(line, dskbuf)
char *line;
struct diskbuf *dskbuf;
{
	while (*line != '\0')
		putchr(*line++, dskbuf);
}

/*
 Function to put a character into a disk buffer.  The character
 is sent in char, and the disk buffer is specified by the address
 of its structure.  Newline characters (LF's) are converted to
 CR/LF pairs.
 */

putchr(byte, dskbuf)
char byte;
struct diskbuf *dskbuf;
{
	char c;
	/* byte &= 0x7f; */
	if (kbhit()) {
		c = getchar();
		if (c == CTLC)
			wipeout("\n");
		if (c == CTLS)
			while (kbhit() == 0)
				c = getchar();
	}
	switch (dskbuf->fd) {
	case CONO:
		if (byte != CPMEOF)
			putchar(byte);

	case NOFILE:
		return;

	case LST:
		if (byte != CPMEOF) {
			/*	if (byte == '\n') bdos(LISTOUT,'\r');
			 bdos(LISTOUT,byte);
			 */
		} else
			putchr('\n', dskbuf);
		return;

	default:
		if (dskbuf->fd >= 20) {
			printf("In putchr fd=%u\n", dskbuf->fd);
			return;
		}
		*(dskbuf->pointr)++ = byte;
		if (dskbuf->pointr >= dskbuf->space + BUFSIZE) {
			if (write(dskbuf->fd, dskbuf->space, BUFSIZE) == -1) {
				printf("\nDisk write error ++%s\n", "a99_0001");
				wipeout("\n");
			}
			dskbuf->pointr = dskbuf->space;
		}
		return;

	}
}
rputchr(byte, dskbuf)
unsigned char byte;
struct diskbuf *dskbuf;
{
	if (dskbuf->fp >= 20) {
		printf("In rputchr fd=%u\n", dskbuf->fp);
		return;
	}
	*(dskbuf->pointr)++ = byte;
	if (dskbuf->pointr >= dskbuf->space + BUFSIZE) {
		if (write(dskbuf->fd, dskbuf->space, BUFSIZE) == -1) {
			//	if (fwrite(dskbuf->space,sizeof(char),BUFSIZE,dskbuf->fp) == -1) {
			printf("\nDisk write error ++%s\n", "a99_0002");
			wipeout("\n");
		}
		dskbuf->pointr = dskbuf->space;
	}
}

/*
 Function to flush a disk buffer.
 */

flush(dskbuf)
struct diskbuf *dskbuf;
{
	unsigned t;
	if (dskbuf->fd < LODISK)
		return;
	t = dskbuf->pointr - dskbuf->space;
	t = (t % BUFSIZE == 0) ? t / BUFSIZE + 1 : t;
	while (dskbuf->pointr < &dskbuf->space[BUFSIZE])
		*(dskbuf->pointr)++ = 0;
	if (write(dskbuf->fd, dskbuf->space, t) == -1)
		wipeout("\nDisk write error in hexflush.\n");
	if (close(dskbuf->fd) == -1)
		wipeout("\nError closing file.\n");
}

/*
 Function to flush a disk buffer.
 */

rflush(dskbuf)
struct diskbuf *dskbuf;
{
	unsigned t;
	unsigned sz;
	sz = sizeof(char);
	if (dskbuf->fp < LODISK)
		return;
	t = dskbuf->pointr - dskbuf->space;
	t = (t % BUFSIZE == 0) ? t / BUFSIZE + 1 : t;
	while (dskbuf->pointr < &dskbuf->space[BUFSIZE])
		*(dskbuf->pointr)++ = 0;
//	if ((t=fwrite(dskbuf->space,sz,t,dskbuf->fp)) <= 0 ) wipeout("\nDisk write error in rflush.\n");
	if (write(dskbuf->fd, dskbuf->space, t) == -1)
		wipeout("\nDisk write error in rflush.\n");
	if (close(dskbuf->fp) == -1)
		wipeout("\nError closing file.\n");
}

