/*
** IOWRITE.C -- buffered and raw output for split IOLIB99
**
** C text-stream rule: logical '\n' is LF (0x0A).  On this target the
** external text representation is CRLF, so text putc/fputc translates
** one LF to CR followed by LF.  Binary streams and putb/write are raw.
*/
#include "stdio.h"

#define NBUFS   6
#define SECTOR  512
#define MWRITE  17325

extern int _fnext[NBUFS];
extern int _ffirst[NBUFS];
extern int _flast[NBUFS];
extern int _ferr[NBUFS];
extern int _ftext[NBUFS];
extern char *_dfltbuf;
extern char *_commbuf;
extern char *_commfcb;

fflush(unit) int unit; {
  int index, i;
  char *next, *going, *src, *dst;

  index = unit - 5;
  if(_fchk(index) != MWRITE) {
    err("CAN'T FLUSH");
    exit(-1);
    }

  next = _fnext[index];
  going = _fnext[index] = _ffirst[index];

  while(going < next) {
    src = going;
    dst = _commbuf;
    i = SECTOR;
    while(i--) *dst++ = *src++;

    _cpm(26, _commbuf);
    _stage_fcb(index);
    if(_cpm(21, _commfcb)) {
      _unstage_fcb(index);
      _cpm(26, _dfltbuf);
      _ferr[index] = 1;
      return EOF;
      }
    _unstage_fcb(index);
    going += SECTOR;
    }

  _cpm(26, _dfltbuf);
  return 0;
  }

putc(c, unit) char c; int unit; {
  int index, value;

  value = c & 0xFF;

  if(unit == stdout || unit == stderr) {
    if(value == LF) _cpm(2, CR);
    _cpm(2, value);
    return value;
    }

  index = unit - 5;
  if(_ftext[index] && value == LF) {
    if(putb(CR, unit) == EOF) return EOF;
    }
  if(putb(value, unit) == EOF) return EOF;
  return value;
  }

/* Raw byte output.  No newline translation is performed here. */
putb(c, unit) char c; int unit; {
  int werror, index, value;
  char *next;

  value = c & 0xFF;

  if(unit == stdout || unit == stderr) {
    _cpm(2, value);
    return value;
    }

  index = unit - 5;
  if(_fchk(index) != MWRITE) {
    err("CAN'T WRITE TO INFILE");
    exit(-1);
    }

  werror = (_fnext[index] == _flast[index]) ? fflush(unit) : 0;
  if(werror) {
    _ferr[index] = 1;
    return EOF;
    }

  next = _fnext[index];
  *next++ = c;
  _fnext[index] = next;
  return value;
  }

putchar(c) char c; {
  return putc(c, stdout);
  }

/* Standard puts: write the string, then exactly one newline. */
/*  puts() writes exactly the bytes it was given and nothing else.
    It does NOT append a newline.  Standard C puts() does, but every
    caller in this toolchain - SMALLC99, CC_CL99, SBC_R99, LINK99 and
    the test programs - already supplies its own "\n", so appending
    one printed every message double spaced.  */
puts(buf) char *buf; {
  if(fputs(buf, stdout) == EOF) return EOF;
  return 0;
  }

fputc(c, unit) char c; int unit; {
  return putc(c, unit);
  }

/* Standard fputs: no implicit newline; non-negative on success. */
fputs(buf, unit) char *buf; int unit; {
  char c;
  while(c = *buf++) {
    if(putc(c, unit) == EOF) return EOF;
    }
  return 0;
  }

/* Raw binary block write: never perform CR/LF translation. */
write(unit, buf, n) int unit, n; char *buf; {
  int count;
  count = n;
  while(n--) {
    if(putb(*buf++, unit) == EOF) return EOF;
    }
  return count;
  }
