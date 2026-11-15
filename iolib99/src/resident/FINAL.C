/*
** FINAL.C -- LINK99 resident final pass.
**
** Phase 2 must be resident because the target code image is now stored
** in banked pages mapped through >8000.  Executing phase2 from the same
** window would replace its own code whenever it reads a target-code bank.
*/
#include <stdio.h>
#include "rel99.h"

#define NOCCARGC
#define SYM       4
#define HIGH    127
#define XRPLUS   -2

extern getint();
extern puts2();
extern show();
extern poll();
extern error2();
extern codegetb();
extern codegetw();
extern outbyte();
extern outword();

extern char *xrnext, *epnext, *xrfirst, *epfirst;
extern char *cloc, *cbase, *csize, *goloc;
extern char outfn[], crfn[];
extern int list, outfd, instr, addr, lgo, ref, field;
extern int crfd, xrplus;
extern unsigned ucl, ucd;

final()
{
  if(!okay()) return 0;
  phase2();
  return 1;
}

okay()
{
  int err;
  char *eplast;

  err = eplast = 0;
  xrnext = getint(xrfirst);
  epnext = getint(epfirst);
  if(list) puts("\nEntry Points:");

  while(1) {
    poll(1);
    if(strcmp(xrnext + SYM, epnext + SYM) > 0) {
      if(epnext == eplast) {
        puts2("-  Redundant: ", xrnext + SYM);
        err = 1;
        }
      if(list) show(epnext, "");
      eplast = epnext;
      epnext = getint(epnext);
      continue;
      }
    if(strcmp(xrnext + SYM, epnext + SYM) < 0) {
      puts2("- Unresolved: ", xrnext + SYM);
      err = 1;
      xrnext = getint(xrnext);
      continue;
      }
    if(*(xrnext + SYM) != HIGH) {
      xrnext = getint(xrnext);
      continue;
      }
    break;
    }
  if(err) return 0;
  return 1;
}

phase2()
{
  outfd = open(outfn, "w");
  if(!outfd) error2("- cannot open ", outfn);

  /*
   * Normal SBC .COM is a raw TMS9900 image at cbase.  Do not write the
   * old CP/M/Z80 three-byte JMP prefix; it corrupts word alignment.
   *
   * LGO keeps its historical header for now, but bytes/words are emitted
   * explicitly so host integer byte order cannot leak into the file.
   */
  if(lgo) {
    outbyte(instr);
    addr = cbase + goloc;
    outword(addr);
    outword(cbase);
    outword(csize);
    }

  cloc = -1;
  readref();
  while(1) {
    ++cloc;
    ucl = cloc;
    ucd = csize;
    if(ucl >= ucd) break;

    if(cloc != ref) {
      field = codegetb(cloc);
      outbyte(field);
      continue;
      }

    field = codegetw(cloc);
    field = field + cbase + xrplus;
    xrplus = 0;
    outword(field);
    readref();
    ++cloc;
    }

  if(ferror(outfd)) error2("- Error Writing ", outfn);
  if(close(outfd) == EOF) error2("- Error Closing ", outfn);

  if(ferror(crfd)) error2("- Error Reading ", crfn);
  close(crfd);
}


outbyte(value)
int value;
{
  char b[8];

  b[0] = value;
  write(outfd, b, 1);
}

outword(value)
int value;
{
  outbyte(value >> 8);
  outbyte(value);
}

readref()
{
  read(crfd, &ref, 2);
  if(ref == XRPLUS) {
    read(crfd, &xrplus, 2);
    read(crfd, &ref, 2);
    if(ref == XRPLUS) {
      xrplus -= cbase;
      read(crfd, &ref, 2);
      }
    }
}
