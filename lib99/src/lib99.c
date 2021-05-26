/* 
** LIB.C -- Small-Mac Library Manager (ver. 1.0)
**
**                  Copyright 1985 J. E. Hendrix
**
** Usage: LIB -{DPTUX}[A] library [module...]
**
** -D     delete named modules
** -P[A]  print named, or all (-PA), modules on stdout
** -T[A]  table of contents of named, or all (-TA), files on stdout
** -U     update (adding/replace) named modules
**        (gets module names from stdin if not in command line)
** -X[A]  extract named, or all (-XA), modules
**
**        The A suffix obviates prompting stdin for module
**        names when none are in the command line.  This is handy for
**        eliminating operator intervention, especially in batch mode.
**        Ordinarily, when no modules are given in the command line,
**        LIB prompts the user (if stdin is not redirected) and
**        accepts one module name at a time from stdin.  If none
**        are given (CR response to first prompt) and the command
**        switch is -P, -T, or -X then all members of the library are
**        processed.
**
** Drive Designators (e.g. B:):
**     allowed with any library and module names
**     new library and index go on same drive as old
**     will default to the default drive
**
** Filename Extensions:
**     do NOT specify with library or module names
**     standard extensions are:
**
**     .REL = relocatable object module
**     .LIB = library of object modules
**     .NDX = index to library
**     .L$  = temporary new library
**     .N$  = temporary new index
**
** Enter control-S to pause and control-C to abort.
*/


#include "rel99.h"
#include "stdio.h"
#include "fcntl.h"

#define NOCCARGC		/* no argument count passing */
#define NAMESIZE   15
#define MAXMODS   200
#define MODEXT  ".R99"
#define LIBEXT  ".LIB"
#define NDXEXT  ".NDX"
#define L_EXT   ".L$"
#define N_EXT   ".N$"
#define HIGH      127		/* high-value byte */
#define MAXOPEN     6		/* maximum files opened */

/* attribute values for open etc
#define O_RDONLY        _O_RDONLY
#define O_BINARY        _O_BINARY
#define O_CREAT         _O_CREAT
#define O_WRONLY        _O_WRONLY
#define O_TRUNC         _O_TRUNC
#define S_IREAD         _S_IREAD
#define S_IWRITE        _S_IWRITE
#define S_IFDIR         _S_IFDIR
 */

char
  cmd[5],			/* command switch */
  oldlib[NAMESIZE],		/* old library name */
  oldndx[NAMESIZE],		/* old index name */
  newlib[NAMESIZE],		/* new library name (temporary) */
  newndx[NAMESIZE],		/* new index name (temporary) */
 *modname;			/* points to module name buffer */

int
 *mptr,				/* module name pointers */
 *mdone,			/* done with module? */
  modules,			/* count of modules to process */
  all,				/* process all members? */
  inndx,			/* input index fd */
  outndx,			/* output index fd */
  oldblock,			/* block of next input member */
  inblock, 				/* block of next library member */
  oldbyte,			/* byte in block of next input member */
  newblock,			/* block of next output member */
  newbyte;			/* byte in block of next output member */

int inrel; /* file descriptor for input REL file */
int inrem; /* remaining bits in inchunk */
int inchunk; /* current chunk from REL file */
int outrel; /* file descriptor for output REL file */
int outrem; /* remaining bits in outchunk */
int outchunk; /* current chunk for REL file */
int inbyte; /* byte in block of next library member */
int item; /* current item code */
int type; /* type field */
int field; /* current bit field */

char symbol[9]; /* current string */

int  item2, type2, field2, inrel2, inrem2, inch2;
char sym2[NAMESIZE];
int from_cur[2 * MAXOPEN]; /* Current position within file  */

main(argc,argv) int argc, argv[]; {

	putls("----------------------------------------------------\n");
	putls("TMS9900 Reclocatable Object Library utility\n");
	putls("CP/M Version by Alexander Cameron January, 1985\n");
	putls("MSDOS Version by  Alexander Cameron May 2010 to July, 2019\n");
	putls("----------------------------------------------------\n");


  mptr  = calloc(MAXMODS*10, 2);	/* allocate zeroed memory */
  mdone = calloc(MAXMODS*10, 2);
  if(getarg(1, cmd, 5, argc, argv) == EOF) usage();
  cmd[1] = toupper(cmd[1]);
  cmd[2] = toupper(cmd[2]);
  if(cmd[0] != '-' || (cmd[2] && cmd[2] != 'A') || strlen(cmd) > 3) usage();
  if(getarg(2,  oldlib, NAMESIZE, argc, argv) == EOF) usage();
  extend(oldlib, LIBEXT, LIBEXT);
  newfn(oldndx, oldlib, NDXEXT);
  newfn(newlib, oldlib, L_EXT);
  newfn(newndx, oldlib, N_EXT);
  outrem = 8;	/* initialise so first byte is written */
  getmods(argc, argv); 		/* gather switches and module names */

  switch(cmd[1]) {
    case 'D': drop();    break;
    case 'T': table();   break;
    case 'U': update();  break;
    case 'X': extract(); break;
    case 'P': print();   break;
     default: usage();
    }
  }

/*
** add module to library
*/
addmod(name) char *name; {
  char *cp, nam[NAMESIZE];
  saverel();			/* save REL variables */
  strcpy(nam, name);
  extend(nam, MODEXT, MODEXT);
  inrel = open(nam,O_RDWR|O_BINARY);
  ifilelbuf(); /* Initialise file buffers */
  cpymod(NO);			/* do not already have header */
  close(inrel);
  restrel();			/* restore REL variables */
  strcpy(nam, name);
  if(nam[1] == ':') cp = nam + 2; else cp = nam;
  cp[MAXSYM] = NULL;
  }

/*
** close input library and index
*/
closein(mod1, mod2) char *mod1, *mod2 ; {
  close(inrel);
  close(inndx);
  }

/*
** close output library and index
*/
closeup(mod1, mod2) char *mod1, *mod2 ; {
  closein();
  endrel();
  close(outrel);
  putndx(newblock, newbyte);		/* index EFILE */
  putndx(EOF, EOF);					/* terminate new index */
  close(outndx);
  movfil(newlib, oldlib);			/* take original names */
  movfil(newndx, oldndx);
  }

/*
** compare module names ignoring drive designators
*/
cmpmod(mod1, mod2) char *mod1, *mod2; {
  char str1[NAMESIZE], str2[NAMESIZE];
  if(mod1[1] == ':') mod1 += 2; strncpy(str1, mod1, MAXSYM);
  if(mod2[1] == ':') mod2 += 2; strncpy(str2, mod2, MAXSYM);
  return (strcmp(str1, str2));
  }

/*
** copy one module from inrel to outrel
*/
cpymod(hdr) int hdr; {
  if(outndx) putndx(newblock, newbyte);	/* must not be extracting */
  if(hdr && !putrel()) labort(7);	/* already have input header */
  do {
    poll(YES);
    if(getrel() == ERR || !putrel()) labort (7);
  } while(item != EPROG);

  /* fflush(outrel);	/* must empty aux buf for ctell() */

  newblock = ctell(outrel) >> 7 ;	/* remember for next member */
  newbyte = ctellc(outrel); 	/* next byte within the module */
/*  if(newbyte == 128) {++newblock; newbyte = 0;} */
  }

/*
** drop modules from library
*/
drop() {
  char mod[NAMESIZE];
  if(modules == 0) error("- Delete by name only");
  openup();
  while(nxtmod(mod)) {
    if(match(mod, NO)) {
      puts2("Deleted ", mod);
      continue;
      }
    cpymod(YES);
    }
  missing();
  closeup();
  }

/*
** terminate REL or LIB file
*/
endrel() {
  item = EFILE;
  field = 0;
  type = 0;
  if(!putrel()) labort(7);
  }

/*
** extract files from library
*/
extract() {
  char modnam[NAMESIZE];
  openin();
  while(nxtmod(modnam)) {
    if(match(modnam, YES)) {
      extend(modnam, MODEXT, MODEXT);
      outrel = open(modnam, O_CREAT|O_RDWR|O_BINARY);
      from_cur[outrel] = 0;	/* Set the current location */
      cpymod(YES);
      endrel();
      close(outrel);
      puts2("\nCreated ", modnam);
      }
    }
  missing();
  closein();
  }

/*
** get module names
*/
getmods(argc, argv) int argc, argv[]; {
  char *cp, *mp, name[NAMESIZE], fn[NAMESIZE];
  int err, eof, arg, i, j;
   if(!(mp = modname = malloc(MAXMODS*10))) error("- Memory Overflow");
  if((j = avail(NO)) >= 0 && j < 512) {
	  putls("- Limited Stack Space");
    err = YES;
    }
  all = YES;			/* default to all modules */
  if(argc > 3) arg = 3;		/* get module names from command line */
  else {
    arg = 0;			/* get module names from stdin */
    if(cmd[2] && (cmd[1] == 'P' || cmd[1] == 'T' || cmd[1] == 'X')) {
      modname[0] = HIGH;		/* high value */
      modname[1] = NULL;
      return;
      }
    }
  err = eof = NO;
  while(modules < MAXMODS-1) {
    poll(YES);
    if(arg) {
      if(getarg(arg++, name, NAMESIZE, argc, argv)==EOF) {eof = YES; break;}
      }
    else {
      if(!reqstr("Module Name: ", name, NAMESIZE)) {eof = YES; break;}
      }
    all = NO;				/* do selected modules only */
    if(cp = strchr(name, '.')) {
      fputs(name, stdout); puts2(" - Extension Forced to ", MODEXT);
      *cp = NULL;
      err = YES;
      }
    if(cp = strchr(name, ':')) {
      if(cp == name+1) ++cp;		/* set up next check */
      else {
        puts2(name, " - Invalid Format - Ignored");
        goto ignore;
        }
      }
    else cp = name;			/* set up next check */
    if(strlen(cp) > MAXSYM) {
      strcpy(fn, cp);
      fputs(fn, stdout);
      fn[MAXSYM] = NULL;
      puts2(" - Will be Truncated to ", fn);
      err = YES;	/* assembler does actual truncation */
      }
    if(cmd[1] == 'U') {			/* REL file must exist */
      strcpy(fn, name); extend(fn, MODEXT, MODEXT);
      if(i = open(fn, O_RDONLY)) close(i);
      else {
        puts2(name, " - Can't Find - Ignored");
        goto ignore;
        }
      }
    for(i = 0; i < modules; ++i) {	/* find place for module */
      if(cmpmod(mptr[i], name) > 0) {	/* shift others up */
        for(j = modules; j > i; --j) mptr[j] = mptr[j-1];
        break;
        }
      if(cmpmod(name, mptr[i]) == 0) {	/* already loaded */
        puts2(mp, " - Duplicate Name - Ignored");
        goto ignore;
        }
      }
    mptr[i] = mp;			/* load modname pointer */
    strcpy(mp, name);			/* load modname buffer */
    while(*mp++) ;			/* scoot to next address */
    ++modules;				/* bump number of modules */
    continue;

    ignore:
    err = YES;
    }
  mptr[modules] = mp;			/* load terminal pointer */
  *mp++ = HIGH;				/* high value */
  *mp   = NULL;
  if(!eof) error("- Too Many Modules Specified");
  if(err) {
    fputs("\nContinue? ", stderr);
    fgets(name, NAMESIZE, stderr);
    if(toupper(*name) != 'Y') exit(7);
    }
  }

/*
** read an entry from the old index
*/
getndx() {
  if(read(inndx, &inblock, 2) != 2 ||	/* next block */
     read(inndx, &inbyte,  2) != 2) {	/* next byte in block */
    error("- Error Reading Index");
    }
  }

/*
** check if name matches module list
*/
match(name, quit) char *name; int quit; {
  int i, done;
  char *mp;
  if(all) return(YES);
  done = YES;
  for(i = 0; i < modules; ++i) {
    if(cmpmod(mptr[i], name) == 0) {
      mdone[i] = YES;
      return(YES);
      }
    if(!mdone[i]) done = NO;
    }
  if(quit && done) exit(0);
  return(NO);
  }

/*
** print "not in library" messages
*/
missing() {
  int i;
  for(i = 0; i < modules; ++i)
    if(!mdone[i]) puts2(mptr[i], " Was Not in Library");
  }

/*
** move file1 to file2
*/
movfil(file1, file2) char *file1, *file2; {
 /*  unlink(file2); */
  remove(file2);
  if(file2[1] == ':') file2 += 2;
  if(rename(file1, file2)) error("- Can't Rename Files");
  }

/*
** create new filename from old filename and specified extension
*/
newfn(dest, sour, ext) char *dest, *sour, *ext; {
  while(*sour && *sour != '.') *dest++ = *sour++;
  strcpy(dest, ext);
  }

/*
** get next module name
*/
nxtmod(name) char *name; {
  seek();				/* go straight to next member */
  if(getrel() == PNAME) {
    strcpy(name, symbol);
    return (YES);
    }

  if(item == EFILE) {
    *name++ = HIGH;			/* high value */
    *name   = NULL;
    return (NO);
    }
  error("- Module missing or Library or Index is corrupted");
  labort(7);
  }

/*
** open library and index for input
** Flags specify the file attributes. They must contain one of O_RDONLY, O_WRONLY or O_RDWR and may also have any of the following.
**  O_CREAT, O_EXCL, O_NOCTTY, O_TRUNC, O_APPEND, O_NONBLOCK, O_NDELAY, O_SYNC. See the man page for full details.
**  fopen returns 0 if file does not exist, of file handle if it does
*/
openin() {

  while((inrel = open(oldlib, O_RDONLY|O_BINARY)) == -1) {
		putls("\nCreating New Library");
		outrel = open(oldlib,  O_CREAT|O_RDWR|O_BINARY);
		from_cur[outrel] = 0;	/* Set the current location */
		item = EFILE;
		putrel();
		close(outrel);
		outndx = open(oldndx, O_CREAT|O_RDWR|O_BINARY);
		putndx(0, 0);
		putndx(EOF, EOF);
		close(outndx);
   }
   inndx = open(oldndx, O_RDONLY|O_BINARY);
  }

/*
** open libraries and indices for updating
*/
openup() {
  openin();
  outrel = open(newlib, O_CREAT|O_RDWR|O_BINARY);
  from_cur[outrel] = 0;	/* Set the current location */
  outndx = open(newndx, O_CREAT|O_RDWR|O_BINARY);
/*  auxbuf(outrel, 4096); */
  }

/*
** print files from library
*/
print() {
  char modnam[NAMESIZE];
  openin();
  while(nxtmod(modnam)) {
    if(match(modnam, YES)) {
      while(YES) {
        poll(YES);
       if(item > ENAME) seerel();
        getrel();
        if(item == EPROG) break;
        }
      }
    }
  missing();
  closein();

  }

/*
** write an entry to the new index
*/
putndx(block, byte) int block, byte; {
  if(write(outndx, &block, 2) != 2 ||	/* next block to index */
     write(outndx, &byte, 2) != 2)	/* next byte in block to index */
    error("- Error Writing New Index");
  }

/*
** restore REL variables
*/
restrel() {
  item    = item2;
  type    = type2;
  field   = field2;
  strcpy(symbol, sym2);
  inrel   = inrel2;
  inchunk = inch2;
  inrem   = inrem2;
  }

/*
** save REL variables
*/
saverel() {
  item2  = item;
  type2  = type;
  field2 = field;
  strcpy(sym2, symbol);
  inrel2 = inrel;
  inch2  = inchunk;
  inrem2 = inrem;
  inrem  = 0;		/* force getrel() to read a byte */
  }

/*
** seek to next member in old library
extern int Uchrpos[];	 lives in CSYSLIB
SEEK_SET/SEEK_CUR/SEEK_END are 0/1/2 respectively, you can use the number or definition.
*/

seek() {
  getndx();
  if(inblock == EOF) error("- Premature End of Index");
  if(lseek(inrel, inblock << 7, SEEK_SET ) == EOF)
    error("- Corrupt Library or Index");
  if (lseek(inrel, inbyte, SEEK_CUR) == EOF) /* now offset to current byte */
  		error("lseekc - Corrupt Library or Index");   /* fixed by AC */

  inrem = 0;			/* force getrel() to read a byte */
  }


/*
** print table of contentnxtmods
*/
table() {
  char name[NAMESIZE]; int i, j;
  openin();
  puts("");
  i = 0;
  while(nxtmod(name)) {
    poll(YES);
    if(match(name, YES)) {

      j = 10- strlen(name);
      while(j--) putchar(' ');
      puts(name);
      if (!(++i % 8)) puts("");
      }
    }
  puts("");
  missing();
  closein();
  puts("\nEnd of Library");
  }

/*
** update (add and replace) modules in alphanumeric order
*/
update() {

  char mod[NAMESIZE]; int m;
  openup();
  m = 0;					/* first in module list */
  nxtmod(mod);				/* first in old library */
  while(YES) {
    if(cmpmod(mptr[m], mod) > 0) {	/* module > member */
      cpymod(YES);			/* copy rest of member */
      nxtmod(mod);			/* next in old library */
      continue;
      }
    if(cmpmod(mptr[m], mod) < 0) {	/* module < member */
      addmod(mptr[m]);			/* add new module */
      puts2("   Added ", mptr[m]);
      ++m;						/* next in module list */
      continue;
      }
    if(*mod != HIGH) {			/* equal and not at end */
      addmod(mptr[m]);			/* add new module */
      ++m;						/* next in module list */
      puts2("Replaced ", mod);
      nxtmod(mod);				/* next in old library */
      continue;
      }
    break;
    }
  closeup();
  }

/*
** abort with a usage message
*/
usage() {
  error("Usage: LIB -{DPTUX}[A] library [module...]");
  }
