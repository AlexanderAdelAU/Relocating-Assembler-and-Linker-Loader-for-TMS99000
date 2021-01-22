/*
 ** DREL.C -- dump REL or LIB file
 **
 **           Copyright 1985 J. E. Hendrix
 **
 **  No command line switches are accepted.  The user is prompted
 **  for each file to be dumped.  Output goes to the standard
 **  output file and is, therefore, redirectable to any output
 **  device or to a disk file.  If an input file cannot be found
 **  the user is prompted for another input file.  File names must
 **  be given, complete with extensions.  Drive specifiers may be
 **  given.
 */
#include "stdio.h"
#include "rel99.h"
#include "fcntl.h"

#define MAXFN 15 /* this should be in the mac.h file */

/* common variables */

int	inrel;			/* file descriptor for input REL file */
int	inrem;			/* remaining bits in inchunk */
int	inchunk;		/* current chunk from REL file */
int outrel;		/* file descriptor for output REL file */
int	outrem;		/* remaining bits in outchunk */
int	outchunk;		/* current chunk for REL file */
int	item;			/* current item code */
int	type;			/* type field */
int	field;			/* current bit field */
char symbol[9];		/* current string */


main(argc,argv) int argc, argv[]; {
	char fn[MAXFN];

	putls("----------------------------------------------------\n");
	putls("TMS9900 REL/LIB Reclocatable Object Library Dump Utility Version 3.1\n");
	putls("CP/M Version by Alexander Cameron January, 1985\n");
	putls("MSDOS Version by  Alexander Cameron May 2010 to July, 2019\n");
	putls("----------------------------------------------------\n");

	if (argc > 1) {
		strncpy(fn, argv[1], MAXFN);
		if (!(inrel = open(fn, _O_RDONLY|_O_BINARY ))) {
		}
		puts3("Opening ", fn, "\n");
		ifilelbuf(); /* Initialise file buffers */
		do {
			poll(YES); /* poll for user interrupt */
			if (getrel() == ERR) {
				error("Error reading rel item"); /* get next REL item */
			}
			seerel(); /* display it */
		} while (item != EFILE);
		close(inrel);
		puts("\nFinished");
		exit(0);
	}
	else {
		while (YES) {
			if (!reqstr("Library/Module Name: ", fn, MAXFN))
				error("File Open Error\n");
			if (!(inrel = open(fn, O_RDONLY|O_BINARY ))) {
				continue;
			}
			puts3("Opening ", fn, "\n");
			ifilelbuf(); /* Initialise file buffers */
			do {
				poll(YES); /* poll for user interrupt */
				if (getrel() == ERR) {
					error("Error reading rel item"); /* get next REL item */
				}
				seerel(); /* display it */
			} while (item != EFILE);
			close(inrel);
			puts("\nFinished");
			exit(0);
		}
	}
}
