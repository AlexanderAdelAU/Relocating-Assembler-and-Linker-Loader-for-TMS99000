/*
** LNK.C -- Small-Mac Linkage Editor
**
**                    Copyright 1985 J. E. Hendrix
**
** Usage: LNK [-B] [-G#] [-L] [-M] program [module/library...]
**
** -B                 A BIG program is being linked, so use all
**                    of free memory for the symbol table and load the
**                    program to disk entirely.  This is slower but it
**                    gets the job done.
**
** -G#                Make program absolute at address # (hex) and
**                    output as "program.LGO" instead of "program.COM".
**
** -L                 List entry point symbols.
**
** -M                 Monitor linking activity.
**
** program            A file specifier for the program being linked.
**                    The default, and only allowed, extension is REL.
**
** module/library...  A list of zero or more module (.REL) and/or
**                    library (.LIB) files.  Each module is linked to
**                    the program and the libraries are searched for
**                    just those modules which satisfy one or more
**                    unresolved external references.
**
** NOTE: Merely declaring a symbol to be external will cause
** it's module to be loaded.  It need not actually be referenced.
**
** NOTE: The symbol TMNAME is defined to be the name of the
** terminal module; i.e., the module which must be loaded last
** of all.  That module contains special code which identifies
** the physical end of the program and the beginning of free
** memory.  The linker is sensitive to its name and waits until
** all other modules are loaded before loading the terminal module.
**
** The absence of an extension, or a .REL extension, identifies a module;
** whereas, a .LIB extension identifies a library.  If necessary, a
** library is rescanned to resolve backward external references between
** modules within the library. Module files and libraries are processed
** in the order in which they occur in the command line.
**
** Drive Designators (e.g. B:):
**    - allowed with module and library names
**    - program drive designator locates the input .REL file
**    - output goes to the default drive
**
** Filename Extensions:
**    - must specify .LIB with library name
**    - standard extensions are:
**
**     .REL = relocatable object module
**     .LIB = library of object modules
**     .NDX = index to library (not user specified)
**     .COM = CP/M command file (default output)
**     .LGO = load-and-go file (-G# output)
**     .O$  = temporary overflow file
**     .R$  = temporary reference file
**
** Enter control-S to pause and control-C to abort.
**
** NOTE: Compile only with Small-C 2.1 (edit level 63) or later.
** Edit 63 fixes CSYSLIB so that when it overflows a buffer while
** writing into a file it will no longer assume that it is at the
** end of the file.  This prevents it from padding a sector with
** 1A (hex) in the middle of a file when random access is being used.
*/
#include <stdio.h>
#include "notice.h"
#include "rel99.h"

#define NODEBUG			/* don't compile debug displays */
#define NOCCARGC		/* don't pass arg counts to functions */

#define NAMESIZE   15
#define MAXFIL     10
#define STACK     512		/* allow for stack space */
/*
 * AUXBUF is 0.  auxbuf() is a no-op - IOLIB99 owns its own buffering and
 * the function says so itself - but getmem was still reserving 2,048
 * bytes for it out of the 4,936 an EXE has.
 */
#define AUXBUF      0
/*
 * Four I/O units are required during a library load once the code buffer
 * spills:
 *   reference .R$ + library .NDX + library .LIB + overflow .O$.
 *
 * IOLIB99 allocates 548 bytes per active unit.  Reserving only three units
 * lets LINK99 consume the fourth unit's heap space as link-buffer RAM; the
 * subsequent fopen(HELLO.O$, "w+") then fails even though CP/M has already
 * created (or may leave) HELLO.O$ in the directory.
 */
#define MAXOPEN     3		/* .R$ + .LIB + .O$; NDX is closed before load */
/*
 * *** 548, NOT 164. ***
 *
 * 164 was CP/M's per-file overhead.  IOLIB99 allocates BUFLGH = 548
 * bytes for every open file - an FCB plus a sector buffer.
 *
 * Reserving 4 x 164 = 656 for files that need 4 x 548 = 2,192 meant
 * getmem's buffer was 1,536 bytes too big, and the third or fourth
 * fopen then found no heap left.  IOLIB99's alloc returns -1 on
 * failure, and IOOPEN stores it without checking:
 *
 *     _ffcb[index]   = alloc(BUFLGH);      -1  = FFFF
 *     _ffirst[index] = _ffcb[index] + FCBSIZ;   = 0023
 *
 * so that unit's buffer pointer becomes 0x0023 and the next write goes
 * over the interrupt vectors - which is exactly the region the memory
 * dump showed destroyed, with the XOP vectors just above it untouched.
 */
#define OHDOPEN   548
/* The SBC loads a COM at >1000.  259 = 0100H+3 was CP/M's origin, and
   phase1 assigns cbase = COMBASE unconditionally for COM output, so
   every address the linker resolved was based 0x103. */
#define COMBASE   0x1000
#define RET       201		/* RET instruction (0C9H) */
#define JMP       195		/* JMP instruction (0C3H) */
#define RES        -1		/* value of resolved ext ref */
#define XRPLUS     -2		/* ext-ref-plus-offset flag */
#define TMNAME   "END"		/* terminal module name */
#define MODEXT  ".R99"		/* was ".REL" - CP/M naming */
#define LIBEXT  ".LIB"
#define NDXEXT  ".NDX"
#define COMEXT  ".COM"
#define LGOEXT  ".LGO"
#define OFLEXT   ".O$"
#define REFEXT   ".R$"

/*
** SBC banked code image.
**
** The target program image is no longer kept in the malloc heap and is
** never spilled to .O$.  It is stored in banked 4K pages mapped through
** the same segment-8 overlay window.  Resident code maps the needed page
** before each code byte/word access.
*/
#define CODEWIN   0x8000
#define CODEMASK  0x0FFF
#define CODEBANKS 10		/* pages 3..12, bank numbers 0..9 */
#define CODEMAX   0xA000	/* documentation only: 10 x 4K bytes */
#define SYMWIN    0x9000
#define SYMSIZE   0x3000	/* fixed symbol/XREF table window: >9000-BFFF */

/*
** symbol table definitions
*/
#define NXT    0		/* next-entry pointer */
#define VAL    2		/* offset value */
#define SYM    4		/* symbol */
#define SSZ (SYM+MAXSYM+1)	/* size of table entry */
#define HIGH 127		/* high-value byte */
/*
 * *** 20 ENTRIES, NOT 200. ***
 *
 * CUSHION is the headroom between the code growing UP and the symbol
 * table growing DOWN, before the linker spills code to the .O$ file.
 * At 200 entries it is 4,200 bytes - LARGER THAN THE WHOLE BUFFER,
 * which on an EXE is 2,232 because the shell caps the heap below the
 * overlay window at 8000.
 *
 * So the test fired on the FIRST code byte, every byte went to CALL.O$,
 * and phase2 then had to READ a file it also has to WRITE.  IOLIB99 has
 * no update mode - getb() refuses any unit that is not MREAD - which is
 * the "ERROR: CAN'T READ OUTFILE" the board reported.
 *
 * 200 was sensible when the buffer was tens of kilobytes on CP/M.  Here
 * it has to be a small fraction of the buffer.
 *
 * NOTE: the .O$ overflow path CANNOT WORK with this library at all,
 * because it needs read and write on one unit.  If a program is ever
 * big enough to trigger it, the linker must stop with a message rather
 * than spill.
 */
/*
 * SBC: keep only four symbol slots of emergency headroom here.
 * newsym() still performs the hard collision check on every allocation.
 * The old 20-slot cushion costs 420 bytes of the very small EXE heap and
 * causes premature .O$ spill; that spill cannot safely resolve XREFS with
 * the current write-only IOLIB handle.
 */
#define CUSHION  (4*SSZ)	/* 84-byte emergency table headroom */
char high[] = {HIGH,0};		/* high-value symbol */

/*
** global variables
*/
char
 *xr,				/* external reference */
 *nxt,				/* next in ext ref chain */
 *ep,				/* entry point */
 *buffer,			/* beginning of code buffer */
 *bnext,			/* next byte in code buffer */
 *sfree,			/* head of freed entry list */
 *snext,			/* next symbol table entry */
 *cloc,				/* location counter */
 *cmod,				/* module location */
 *cbase,			/* base address */
 *csize,			/* program size (fake unsigned) */
 *goloc,			/* go location */
 *cdisk,			/* disk overflow location */
 *epfirst,			/* first entry point */
 *epprev,			/* previous entry point */
 *epnext,			/* next entry point */
 *xrfirst,			/* first external reference */
 *xrprev,			/* previous external reference */
 *xrnext,			/* next external reference */
  modname[MAXSYM+1],		/* name of current module */
  infn   [NAMESIZE],		/* input filename */
  ndxfn  [NAMESIZE],		/* index filename */
  tmfn   [NAMESIZE],		/* terminal-module library name */
  csfn   [NAMESIZE],		/* code seg filename */
  crfn   [NAMESIZE],		/* code rel filename */
  outfn  [NAMESIZE];		/* output filename */

int
  lgo,		/* load-and-go format? */
  list,		/* list symbols? */
  monitor,	/* monitor activity? */
  instr,	/* instruction to plant at 0000 */
  addr,		/* start address */
  ref,		/* reference to program relative item */
  big,		/* linking a big program? */
  xrplus,	/* value of offset for next ext ref */
  xrpflag=XRPLUS,	/* value of xrplus flag */
  ndxfd,	/* index fd */
  inblock,	/* block of next library member */
  inbyte,	/* byte in block of next library member */
  tmblock,	/* block of terminal module in tmfn */
  tmbyte,	/* byte of terminal module in tmblock */
  csfd,		/* code segment fd */
  crfd,		/* code relative index fd */
  outfd;	/* output fd */

/*
 * These MUST be unsigned, not int.  They exist only so that the two
 * buffer-pointer comparisons are UNSIGNED ones: the buffer sits above
 * 0x8000 on this machine, and a signed compare reads snext (~D363) as
 * negative.  Declaring them in the int list above would have changed
 * nothing at all.
 */
unsigned ubn, usn;

/*
 * cdisk is set to -1 in phase1 and the comment there calls it a "high
 * value for pointer" - every code offset should be BELOW it, meaning
 * "still in memory".  As 0xFFFF unsigned that is exactly right.
 *
 * But Small-C's '<' is SIGNED, and -1 is the LOWEST signed value, so
 * every test came out false and every reference took the ON DISK
 * branch.  With the overflow file never opened - which is now the
 * normal case - that reached xrseek(csfd) with csfd = 0 and IOLIB99
 * reported "INVALID UNIT NUMBER".
 *
 * Same fault as the buffer-pointer comparisons: on this machine
 * anything above 32767 is negative in a signed compare.
 */
unsigned ucl, ucd;

int xrefs;      /* an EXT "XREFS" marker has been seen           */
int xrsym;      /* the xr entry the following CHAIN items feed    */
int cbank;      /* currently mapped code-image bank, or -1         */


/*
** CommonLibrary GETREL state.
** LINK99 owns this resident state; GETREL accesses it as externals.
*/
int inrel;
int inrem;
int inchunk;
int item;
int type;
int field;

char symbol[MAXSYM+1];


main(argc,argv) int argc, argv[]; {
  ovlinit();                     /* same overlay startup used by FILEEDIT/FILESTAT */
  puts("SBC LINK99");
  getsw(argc, argv);		/* fetch and remember switches */
  getmem();			/* acquire maximum memory buffer */
  phase1(argc, argv);		/* load and link */
  if(!final()) abort(7);		/* resident final pass */
  }

/*
** get as much memory as possible for symbol table
*/
getmem() {
  char sz[12];

  mapsym();
  buffer = SYMWIN;
  bnext = 0;
  cbank = -1;
  snext = SYMWIN + (SYMSIZE - SSZ);
  sfree = 0;
#ifdef DEBUG
  if(monitor) {itou(SYMSIZE, sz, 8); puts2(sz, " Symbol Bytes");}
#endif
  newtbl(&epfirst);
  newtbl(&xrfirst);
  }

/*
** get next module name
*/
getname() {
  if(getrel() == PNAME) {
    strcpy(modname, symbol);
    return (YES);
    }
  if(item == EFILE) return (NO);
  error2(infn, " - Corrupted");
  }

/*
** read next entry from library index file
*/
getndx() {
  /*
  ** Small-C/TMS9900 local byte arrays need spare frame room.
  ** As documented in phase1(), using the nominal final byte of a local
  ** char array can overlap the saved return address.  We only need four
  ** bytes from the .NDX file, but reserve eight here deliberately.
  */
  char b[8];

  /*
  ** Library .NDX files inherit the original CP/M/Z80 on-disk format:
  ** each block/byte value is a 16-bit LITTLE-ENDIAN word.  A direct
  ** read into an int worked on Z80/x86 hosts, but reverses every word
  ** on the big-endian TMS9900 (e.g. 0A 00 became >0A00, not >000A).
  ** Decode the file format explicitly so native LINK99 seeks to the
  ** same member offsets as the Windows linker.
  */
  if(read(ndxfd, b, 4) != 4) {
    error2("- Error Reading ", infn);
    }

  inblock = (b[0] & 255) | ((b[1] & 255) << 8);
  inbyte  = (b[2] & 255) | ((b[3] & 255) << 8);
  }

/*
** get switches from command line
*/
getsw(argc, argv) int argc, *argv; {
  char arg[NAMESIZE];
  int argnbr, b, len;
  argnbr = 0;
  while(getarg(++argnbr, arg, NAMESIZE, argc, argv) != EOF) {
    if(arg[0] != '-') continue;			/* skip file names */
    if(toupper(arg[1]) == 'G') {
      lgo = YES;
      len = xtoi(arg + 2, &b);
      if(len >= 0 && !arg[len + 2]) cbase = b; else runusage();
      }
    else if(toupper(arg[1]) == 'B') big = YES;
    else if(toupper(arg[1]) == 'L') list = YES;
    else if(toupper(arg[1]) == 'M') monitor = YES;
    else runusage();
    }
  }

/*
** is symbol an unresolved ext ref?
** on return of true, xrnext -> matching xr entry
*/
isunres() {
  int i;
  xrnext = getint(xrfirst);
  while(xrnext) {
    if((i = strcmp(symbol, xrnext + SYM)) < 0) return (NO);
    if(i == 0)  return (YES);
    xrnext = getint(xrnext);
    }
  return (NO);
  }

/*
** link external references to entry points
*/
link() {
  int cspg, csch;

  /*
   * csfd is 0 when the code fits in memory, which is now the normal
   * case - CUSHION used to be bigger than the whole buffer, so the .O$
   * file always existed and these calls always had a real unit.
   *
   * ctell(0) reaches IOLIB99's _fchk with index -5 and it reports
   * "INVALID UNIT NUMBER".  Every use of csfd has to be guarded.
   */
  cspg = csch = 0;
  if(csfd) {
    cspg = ctell(csfd);			/* remember temp file position */
    csch = ctellc(csfd);
    }
  xrnext = getint(xrprev = xrfirst);	/* first external reference */
  epnext = getint(epfirst);		/* first entry point */
  while(YES) {
    if(strcmp(xrnext + SYM, epnext + SYM) > 0) {	/* xr > ep */
      epnext = getint(epnext);
      continue;
      }
    if(strcmp(xrnext + SYM, epnext + SYM) < 0) {	/* xr < ep */
      xrnext = getint(xrprev = xrnext);
      continue;
      }
    if(*(xrnext + SYM) != HIGH) {			/* xr = ep */
      resolve();			/* resolve this ext ref */
      putint(xrprev, getint(xrnext));	/* delink from xr chain */
      putint(xrnext, sfree);		/* link to prev freed entry */
      sfree = xrnext;			/* make first freed entry */
      xrnext = getint(xrprev);		/* advance to next ext ref */
      continue;				/* same ext ref in diff modules? */
      }
    break;
    }
  if(csfd) cseekc(csfd, cspg, csch);	/* restore exact temp file position */
  }

/*
** write a 16-bit word to the phase-2 reference file.
**
** IOLIB99 write() treats a returned byte value of 0xFF as -1.
** That is indistinguishable from error to write(), even though putb()
** has already stored the byte.  Therefore never write multi-byte binary
** values with one write() call when they may contain 0xFF.
*/
refword(value)
int value;
{
  char b[8];

  b[0] = value >> 8;
  write(crfd, b, 1);
  b[0] = value;
  write(crfd, b, 1);
}

/*
** load a module
*/
load() {
  char str[12], *offloc;
  offloc = -1;
  epprev = epfirst;
  xrprev = xrfirst;
  xrsym = 0;
  do {
    poll(YES);
    switch(getrel()) {
      case    ABS: codeneed(1);
                   codeputb(cloc, field);
                   if(offloc == cloc) {
                     refword(xrpflag);
                     refword(cloc);
                     }
                   ++cloc;
                   break;

      case   PREL: field = field + cmod;
                   codeneed(2);
                   codeputw(cloc, field);
                   refword(cloc);
                   cloc += 2;
                   break;

      case  DSIZE: if(!field) break;
          default: error("- Unsupported Link Item");
      case    ERR: error("- Corrupt Module");
      case  EPROG: if(type == PREL) {
                     puts2("Start In ", modname);
                     goloc = field + cmod;
                     }
      case  ENAME: break;

      case    EXT: if(!strcmp(symbol, "XREFS")) xrefs = YES;
                   break;

      case XCHAIN: if(xrefs) {
                     xrefs = NO;
                     field = 0;
                     type  = ABS;
                     }
                   newsym(&xrprev, xrfirst, "xr");
                   xrsym = xrprev;
                   break;

      case  CHAIN: if(!xrsym) error("- CHAIN without XCHAIN");
                   field = field + cmod;
                   codeputw(field, getint(xrsym + VAL));
                   putint(xrsym + VAL, field);
                   break;

      case EPOINT: newsym(&epprev, epfirst, "ep");
                   break;

      case  PSIZE: cmod = cloc;
                   if(monitor) {
                     itox(field, str, 8);
                     fputs(str, stdout); fputs(" Bytes at", stdout);
                     itox(cloc,  str, 6);
                     fputs(str, stdout); fputs("'", stdout);
                     itox(cloc+cbase,  str, 6);
                     fputs(str, stdout); puts2(" ", modname);
                     }
                   codeneed(field);
                   break;

      case  SETLC: field = field + cmod;
                   ucl = cloc;
                   ucd = field;
                   while(ucl < ucd) {
                     codeneed(1);
                     codeputb(cloc, 0);
                     ++cloc;
                     ucl = cloc;
                     }
                   break;

      case  XPOFF: refword(xrpflag);
                   refword(field);
                   offloc = cloc;
                   break;
      }
    } while(item != EPROG);
  }

/*
** create new file specifier from an old one
*/
newfn(dest, sour, ext) char *dest, *sour, *ext; {
  if(sour[1] == ':' && strcmp(ext, NDXEXT)) sour += 2;
  while(*sour && *sour != '.') *dest++ = *sour++;
  strcpy(dest, ext);
  }

/*
** store new symbol table entry
** they arrive in alphanumeric order
*/
newsym(prev, first, ts) int *prev, first; char *ts; {
  char at[12], *cp, *new;
  if(new = sfree) sfree = getint(sfree);	/* use old entry */
  else {
    new = snext;
    snext -= SSZ;
    usn = snext;  ubn = buffer;
    if(usn < ubn) error("- Symbol table full");
    }
  if(strcmp(symbol, *prev + SYM) < 0)
    *prev = first;				/* tolerate M80 blunder */
  cp = *prev;
  while(strcmp(symbol, cp + SYM) >= 0) {	/* find position */
    *prev = cp;
    cp = getint(cp);
    }
  putint(new, cp);			/* point new entry ahead */
  putint(*prev, new);			/* point prev entry here */
  *prev = new;				/* this becomes prev entry */
  if(type == PREL) field = field + cmod;/* adjust for module location */
  putint(new + VAL, field);		/* load value */
  strcpy(new + SYM, symbol);		/* load symbol */
#ifdef DEBUG
  if(monitor) show(new, ts);
#endif
  }

/*
** initial table entries
*/
newtbl(low) int *low; {
  *low = snext;				/* always points to low entry */
  strcpy(snext + SYM, "");		/* store low symbol */
  putint(snext, snext - SSZ);		/* link to next (high) symbol */
  snext -= SSZ;				/* now point to next entry */
  strcpy(snext + SYM, high);		/* store high symbol */
  putint(snext, 0);			/* end of chain */
  snext -= SSZ;				/* bump to next entry */
  }

/*
** get next module name
*/
nxtmod() {
  getndx();				/* get location and */
  seek();				/* go straight to next member */
  return (getname());
  }

/*
** load input files and library members
*/
phase1(argc, argv) int argc, *argv; {
  /*
   * *** sz IS 12 BYTES FOR AN 8-BYTE JOB, DELIBERATELY. ***
   *
   * A local char array is placed one byte higher in the frame than its
   * size accounts for.  With char sz[8] and three ints the frame is 14
   * bytes - locals S+0..S+13, RETURN ADDRESS AT S+14 - and the array
   * base comes out at S+7, so sz[7] IS S+14.
   *
   * itox() writes str[--sz] = NULL first, so itox(csize, sz, 8) zeroed
   * the HIGH BYTE OF THE RETURN ADDRESS before writing a single digit.
   * phase1 then returned to 0x0042 instead of 0x1042 - main is at the
   * start of the image - and every earlier crash was the same thing at
   * a different address: 0024, 0002, 00A2, 00D4, 00D8, all low memory,
   * all a return address with its top byte cleared.
   *
   * It also explains why exit(0) from here worked perfectly: exit never
   * uses the return address.
   *
   * Four extra bytes keep the written range clear of the boundary.  Any
   * local char array written to its last element needs the same slack.
   */
  char sz[12];
  int i, lib, eof;
  eof = EOF;
  cdisk = CODEMAX;			/* old disk-spill limit: no .O$ */
  if(lgo) instr = RET;			/* load and go format */
  else {instr = JMP; cbase = COMBASE;}	/* COM file format */
  i = 0;
  while(getarg(++i, infn, NAMESIZE, argc, argv) != EOF) {
    if(infn[0] == '-') continue;	/* skip switches */
    if(extend(infn, MODEXT, LIBEXT))
         lib = YES;
    else lib = NO;
    if(!*outfn) {			/* first file name */
      if(lgo) newfn(outfn, infn, LGOEXT);
      else    newfn(outfn, infn, COMEXT);
      newfn(crfn, infn, REFEXT);
      crfd = open(crfn, "w+");

      if(!crfd) error2("- cannot open ", crfn);		/* open reference file */
      auxbuf(crfd, AUXBUF);	/* extra buffering lowers head movement */
      }
    if(lib) search();	/* search library if unresolved ext refs */
    else {
      inrel = open(infn, "r");
      if(!inrel) error2(infn, " - cannot open");
      ifilelbuf();                 /* new REL file: discard GETREL byte buffer */
      inrem = 0;                   /* and restart at a byte boundary */
      getname();			/* program name */
      load();				/* load module */
      link();				/* link previous modules */
      close(inrel);			/* must close */
      }
    }
  if(!*outfn) runusage();
  if(*tmfn) {				/* must get terminal module */
    inrel = open(tmfn, "r");
    ifilelbuf();                   /* new REL file */
    inrem = 0;
    inblock = tmblock; inbyte = tmbyte;
    seek(); getname(); load(); link();
    close(inrel);
    }
  csize = cloc;
  if(ferror(crfd)) error2("- Error Writing ", crfn);
  refword(eof);
  /*
   * IOLIB99 HAS NO READ/WRITE UPDATE MODE.
   *
   * A file opened "w+" is MWRITE, and rewind() only moves the position -
   * it does not change the mode.  getb() then refuses:
   *
   *     if(_fchk(index) != MREAD) { err("CAN'T READ OUTFILE"); exit(-1); }
   *
   * which is exactly what the board reported once phase2 started reading
   * back the reference file.  CP/M's "w+" allowed write-then-read; this
   * library does not.
   *
   * So close the file and reopen it for reading.  The data is already on
   * disc - fclose flushes it - and the reopen gives a unit in MREAD mode.
   */
  close(crfd);
  crfd = open(crfn, "r");
  if(!crfd) error2("- cannot reopen ", crfn);
  itox(csize, sz, 8); puts2(sz, " Bytes (hex)");
  itou(csize, sz, 8); puts2(sz, " Bytes (dec)");
  /*
   * The LAST statement of phase1.  If [end1] prints and [p1] does not,
   * the crash is in the RET itself - XOP 7 taking a corrupted return
   * address off the XOP stack, which lives in segment 0 where PC 0024
   * points.
   */

  /*
   * TEST ONLY.  phase1 has finished its work at this point - it has
   * printed the byte counts.  Exiting HERE, instead of returning to
   * main, separates two things that look identical from outside:
   *
   *   "[end1] then a clean %" prompt   the work is done and the RETURN
   *                                    is what fails
   *   "[end1] then the same failure"   the return is fine and the fault
   *                                    is in the file state phase1 leaves
   *                                    behind - which exit() then trips
   *                                    over when it closes the units
   *
   * exit() closes every unit still in MWRITE mode before calling
   * _shell, so if a file control block is bad this is where it shows.
   */
  }

/*
** generate absolute output in COM or LGO format
**
** COM format: JMP <start> <program>
**
** LGO format: RET <start> <prog-base> <prog-size> <program>
*/
/*
** resolve external references to a given symbol
*/
resolve() {
  char at[12];
  if(!(xr = getint(xrnext + VAL))) return;

  ep = getint(epnext + VAL) + cbase;
  do {
#ifdef DEBUG
    if(monitor) {
      poll(YES);
      fputs("Resolving ", stdout);
      itox(xr, at, 5); fputs(at, stdout);
      fputs(" to ", stdout);
      itox(ep, at, 5); fputs(at, stdout);
      puts2(" ", xrnext + SYM);
      }
#endif
    nxt = codegetw(xr);
    codeputw(xr, ep);
    } while(xr = nxt);
  }

/*
** search a library
*/
search() {
  int linked, ndxpos;
  linked = NO;
  newfn(ndxfn, infn, NDXEXT);
  ndxfd = open(ndxfn, "r");

  if(!ndxfd) error2("- cannot open ", ndxfn);
  inrel = open(infn, "r");
  if(!inrel) error2(infn, " - cannot open");
  ifilelbuf();                     /* new library file */
  inrem = 0;
  while(YES) {					/* rescan till done */
    while(nxtmod()) {
      if(strcmp(modname, TMNAME) == 0) {	/* will load this one last */
        strcpy(tmfn, infn);
        tmblock = inblock;
        tmbyte = inbyte;
        continue;
        }
      while(getrel() == ENAME) {
        poll(YES);
        if(isunres()) {				/* unresolved reference? */
          /*
           * SBC/IOLIB99: do not keep the .NDX unit open while loading.
           * IOLIB allocates 548 bytes per open unit.  Keeping .R$, .LIB,
           * .NDX and .O$ open together leaves only 52 bytes for LINK99's
           * code/symbol buffer.
           *
           * Save the next-index byte position, close NDX, load/link the
           * selected member, then reopen NDX and restore that position.
           * This caps simultaneous units at three and returns 548 bytes
           * to the linker buffer.
           */
          ndxpos = tell(ndxfd);
          close(ndxfd);
          ndxfd = 0;
          load();				/* load module */
          link();				/* link to previous ones */
          ndxfd = open(ndxfn, "r");
          if(!ndxfd) error2("- cannot reopen ", ndxfn);
          if(lseek(ndxfd, ndxpos, 0) < 0) error2("- NDX seek error ", ndxfn);
          linked = YES;
          break;
          }
        }
      }
    if(!linked) break;
    linked = NO;
    rewind(ndxfd);
    }
  if(ndxfd) close(ndxfd);
  close(inrel);
  }

/*
** seek to next member in old library
*/
seek() {
  if(inblock == EOF) error("- Premature End of Index");
  if(cseekc(inrel, inblock, inbyte) == EOF)
    error("- Corrupt Library or Index");
  ifilelbuf();                     /* cseekc invalidates GETREL's 256-byte buffer */
  inrem = 0;                       /* force getrel() to read a fresh byte */
  }

/*
** show a symbol
*/
show(table, type) char *table, *type; {
  char str[12];  int abs;
  itox(abs = getint(table + VAL), str, 8);
  fputs( str, stdout); fputs("'", stdout);
  itox(abs + cbase, str, 6);
  fputs( str, stdout); fputs(" ", stdout);
  fputs(type, stdout); fputs(" ", stdout);
  puts(table + SYM);
  }

/*
** seek external reference
*/
xrseek(byte) int byte; {
  if(cseekc(csfd, (byte >> 7) & 511, byte & 127) == EOF)
    error2("- Seek Error in ", csfn);
  }



/*
** banked target-code image
*/
codebank(addr)
int addr;
{
  return ((addr >> 12) & 15);
}

codeneed(n)
int n;
{
  int b;

  if(n <= 0) return;
  b = codebank(cloc + n - 1);
  if(b >= CODEBANKS) error("- Program too large for code banks");
}

codptr(addr)
int addr;
{
  int b, off;
  char *p;

  b = codebank(addr);
  if(b != cbank) {
    switch(b) {
      case  0: mapc0();  break;
      case  1: mapc1();  break;
      case  2: mapc2();  break;
      case  3: mapc3();  break;
      case  4: mapc4();  break;
      case  5: mapc5();  break;
      case  6: mapc6();  break;
      case  7: mapc7();  break;
      case  8: mapc8();  break;
      case  9: mapc9();  break;
      default: error("- Program too large for code banks");
      }
    cbank = b;
    }

  ucl = addr;
  off = ucl & CODEMASK;
  p = CODEWIN;
  p = p + off;
  return p;
}

codeputb(addr, value)
int addr, value;
{
  char *p;
  p = codptr(addr);
  *p = value;
}

codegetb(addr)
int addr;
{
  char *p;
  p = codptr(addr);
  return (*p & 255);
}

codeputw(addr, value)
int addr, value;
{
  codeputb(addr, value >> 8);
  codeputb(addr + 1, value);
}

codegetw(addr)
int addr;
{
  return ((codegetb(addr) & 255) << 8) | (codegetb(addr + 1) & 255);
}


/*
** ------------------------------------------------------------------
** SBC runtime adaptation
**
** The original 1985 linker used an older Small-C runtime interface.
** These routines adapt that interface directly to the current SBC
** runtime.  They are part of LINK99 itself, not a compatibility
** library/module.
** ------------------------------------------------------------------
*/

abort(code)
int code;
{
    exit(code);
}

malloc(size)
int size;
{
    return alloc(size);
}

getint(p)
char *p;
{
    return ((*p & 255) << 8) | (*(p + 1) & 255);
}

putint(p, value)
char *p;
int value;
{
    *p = value >> 8;
    *(p + 1) = value;
}

puts2(a, b)
char *a, *b;
{
    fputs(a, stdout);
    puts(b);
}

error(s)
char *s;
{
    puts(s);
    exit(1);
}

error2(a, b)
char *a, *b;
{
    puts(a);
    puts(b);
    exit(1);
}

poll(flag)
int flag;
{
    return 0;
}

/*
** Old descriptor API -> current IOLIB stream API.
*/
open(name, mode)
char *name, *mode;
{
    return fopen(name, mode);
}

close(unit)
int unit;
{
    return fclose(unit);
}

/*
** Raw binary read.
** REL input and linker temporary files must not pass through text
** translation.
*/
read(unit, buf, n)
int unit, n;
char *buf;
{
    int count, c;

    count = 0;
    while(count < n) {
        c = getb(unit);
        if(c == EOF) break;
        *buf++ = c;
        ++count;
    }
    return count;
}

rewind(unit)
int unit;
{
    if(lseek(unit, 0, 0) < 0) return EOF;
    return 0;
}

/*
** Historical cseek() addresses 128-byte blocks.
** LINK99 uses mode zero for absolute positioning.
*/
cseek(unit, block, mode)
int unit, block, mode;
{
    int pos;

    if(mode != 0) return EOF;
    pos = block << 7;
    if(lseek(unit, pos, 0) < 0) return EOF;
    return block;
}

/*
** Historical tell pair:
**      ctell()  = 128-byte block
**      ctellc() = byte offset within that block
*/
ctell(unit)
int unit;
{
    int pos;

    pos = tell(unit);
    if(pos < 0) return EOF;
    return (pos >> 7) & 511;
}

ctellc(unit)
int unit;
{
    int pos;

    pos = tell(unit);
    if(pos < 0) return EOF;
    return pos & 127;
}

/*
** Set an exact historical block+byte position using current byte seek.
*/
cseekc(unit, block, byte)
int unit, block, byte;
{
    int pos;

    pos = (block << 7) + (byte & 127);
    if(lseek(unit, pos, 0) < 0) return EOF;
    return 0;
}

/*
** Current IOLIB owns its buffering.  The old auxbuf() request was a
** performance hint only.
*/
auxbuf(unit, size)
int unit, size;
{
    return 0;
}

/*
** LINK99-local command-line accessor.
**
** IOLIB startup calls _setargs() and then main(_argc,_argv), so argv[0]
** is the program name and argv[1] is the first real argument.  Both
** LINK99 argument loops begin with ++i from zero and therefore request
** getarg(1) first.
**
** Keep this helper private to LINK99.  SIZE is the total destination
** buffer size, so reserve one byte for the terminating NUL.
*/
getarg(n, s, size, argc, argv)
int n;
char *s;
int size;
int argc;
char **argv;
{
    char *str;
    int i;

    if(size <= 0) return EOF;

    if(n < 0 | n >= argc) {
        *s = NULL;
        return EOF;
    }

    str = argv[n];
    i = 0;

    while(i < size - 1) {
        if((s[i] = str[i]) == NULL) return i;
        ++i;
    }

    s[i] = NULL;
    return i;
}



/*
** Supply MODEXT when no extension is present.
** Return YES when the resulting name is a library name.
*/
extend(name, modext, libext)
char *name, *modext, *libext;
{
    char *p, *dot, *end;

    p = name;
    dot = 0;

    if(p[0] && p[1] == ':') p += 2;

    end = p;
    while(*end) {
        if(*end == '.') dot = end;
        ++end;
    }

    if(!dot) {
        p = modext;
        while(*p) *end++ = *p++;
        *end = 0;
        return NO;
    }

    if(strcmp(dot, libext) == 0) return YES;
    return NO;
}
