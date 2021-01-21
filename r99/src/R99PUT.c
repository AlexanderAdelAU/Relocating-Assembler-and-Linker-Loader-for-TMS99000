/*
 TMS9900/99105  Cross-Assembler  v. 1.0

 January, 1985

 Original 6800 version Copyright (c) 1980 William C. Colley, III.
 Modified for the TMS9900/99105  and to be relocatable by Alexander Cameron.

 File:	a99put.c

 List and hex output routines.
 */

/*  Get globals:  */

#include "r99gbl.h"
#include "r99ext.h"

/*
 Function to form the list output line and put it to
 the list device.  Routine also puts the line to the
 console in the event of an error.
 */
void lineout() {
	char tbuf[25], *tptr, *bptr, conbuf[LINLEN];
	short int count, test;
	memset(tbuf, ' ', 24);
	tbuf[24] = '\0';
	memset(conbuf, '\0', LINLEN);
	tptr = tbuf;
	*tptr++ = errcode;
	tptr++;
	if (hexflg != NOCODE)
		puthex4(address, &tptr);
	else
		tptr += 4;
	tptr += 3;
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
			puthex4(address, &tptr);
			memset(tptr, ' ', 14);
			tptr += 3;
		}
		if (count == nbytes)
			return;
		count++;
		address++;
		puthex2(*bptr++, &tptr);
		if (count % 2 == 0)
			tptr++;
	}
}

/*
 Function to form the hex output line and put it to
 the hex output device.
 */

void hexout8() {
	char *bptr, reclen;
	unsigned short count;
	reclen = 16;
	switch (hexflg) {

	case PUTCODE:
		bptr = binbuf;
		for (count = 1; count <= nbytes; count++) {
			puthex2(*bptr, &hxlnptr);
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

void hexout16() {
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
			puthex2(*bptr, &hxlnptr);
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

void flshhbf(unsigned short loadaddr) {
	if (hxbytes != 0) {
		puthex2(-(chksum + hxbytes), &hxlnptr);
		*hxlnptr++ = '\n';
		*hxlnptr++ = '\0';
		hxlnptr = hxlnbuf + 1;
		puthex2(hxbytes, &hxlnptr);
		putlin(hxlnbuf, &hexbuf);
	}
	hxbytes = 0;
	hxlnptr = hxlnbuf;
	*hxlnptr++ = ':';
	hxlnptr += 2;
	puthex4(loadaddr, &hxlnptr);
	puthex2(0, &hxlnptr);
	chksum = (loadaddr >> 8) + (loadaddr & 0xff);
}

/*
 Function to put a 4-digit hex number into an output line.
 */

void puthex4(unsigned short number, char **lineptr) {
	puthex2(number >> 8, lineptr);
	puthex2(number, lineptr);
}

/*
 Function to put a 2-digit hex number into an output line.
 */

void puthex2(unsigned char number, char **lineptr) {

	if ((**lineptr = (number >> 4) + '0') > '9')
		**lineptr += 7;
	if ((*++*lineptr = (number & 0x0f) + '0') > '9')
		**lineptr += 7;
	++(*lineptr);
}

/*
 Function to put a decimal number into an output line.
 */

void putdec(unsigned number, char **lineptr)
/* unsigned short number;
 char **lineptr; */
{
	if (number == 0)
		return;
	putdec(number / 10, lineptr);
	*(*lineptr)++ = number % 10 + '0';
}

/*
 Function to move a line to a disk buffer.  The line is pointed to
 by line, and the disk buffer is specified by its disk I/O buffer
 structure dskbuf.
 */

putlin(char *line, struct diskbuf *dskbuf) {
	while (*line != '\0')
		putchr(*line++, dskbuf);
}

/*
 Function to put a character into a disk buffer.  The character
 is sent in char, and the disk buffer is specified by the address
 of its structure.  Newline characters (LF's) are converted to
 CR/LF pairs.
 */

void putchr(char byte, struct diskbuf *dskbuf) {
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
void rputchr(unsigned char byte, struct diskbuf *dskbuf) {
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

void flush(struct diskbuf *dskbuf) {
	unsigned int t;
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

void rflush(struct diskbuf *dskbuf) {
	unsigned int t;
	unsigned short sz;
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

