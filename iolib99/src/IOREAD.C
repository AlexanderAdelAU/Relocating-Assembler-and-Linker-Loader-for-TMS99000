/*
** IOREAD.C -- buffered input for split IOLIB99
**
** C text-stream rules used here:
**  - The C newline character is LF (0x0A, '\n').
**  - Text input normalizes external CRLF, CR-only and LF-only line endings
**    to one logical '\n'.  CRLF therefore produces exactly one character.
**  - Binary streams do no CR/LF or ^Z translation.
**  - fgets() stores a terminating '\n' when one is read and room exists,
**    as required by the C stdio contract.
**  - _wascr[] is per-unit CRLF pairing state and is reset by fopen/lseek.
*/
#include "stdio.h"

/* Constants are local deliberately: Small-C/Plus has limited include handling. */
#define NBUFS   6
#define LGH     512
#define SECTOR  512
#define MREAD   22489
#define FCB_LBUFCNT  35

extern int _eof_marker;
extern int _ffcb[NBUFS];
extern int _fnext[NBUFS];
extern int _ffirst[NBUFS];
extern int _flast[NBUFS];
extern int _ftext[NBUFS];
extern char *_dfltbuf;
extern char *_commbuf;
extern char *_commfcb;

int _wascr[NBUFS];   /* text input: previous logical newline came from CR */

_getbuf(index) int index; {
  char *fcb, *dst, *src;
  int sectors_read, i, result, lbufcnt;

  fcb = _ffcb[index];
  dst = _ffirst[index];
  sectors_read = 0;

  while(sectors_read < (LGH / SECTOR)) {
    _stage_fcb(index);
    _cpm(26, _commbuf);
    result = _cpm(20, _commfcb);
    _unstage_fcb(index);
    if(result) break;

    lbufcnt = fcb[FCB_LBUFCNT] & 0xFF;
    if(lbufcnt == 0 || lbufcnt >= SECTOR) lbufcnt = SECTOR;

    src = _commbuf;
    i = lbufcnt;
    while(i--) *dst++ = *src++;
    sectors_read++;
    if(lbufcnt < SECTOR) break;
    }

  _cpm(26, _dfltbuf);
  if(sectors_read == 0) return -1;
  _flast[index] = dst;
  return _ffirst[index];
  }

/*
** Text getc/fgetc returns the C newline character LF for any accepted
** external line ending.  A CRLF pair is consumed as one logical newline.
** Binary mode is byte-for-byte and does not treat ^Z specially.
*/
getc(unit) int unit; {
  int c, index;

  if(unit == stdin) {
    c = getb(unit);
    if(c == CR) {
      _cpm(2, LF);              /* console has already echoed CR */
      c = LF;
      }
    if(c == _eof_marker) return EOF;
    return c;
    }

  index = unit - 5;

  if(!_ftext[index]) return getb(unit);

  while(1) {
    c = getb(unit);
    if(c == EOF) {
      _wascr[index] = 0;
      return EOF;
      }

    if(_wascr[index]) {
      _wascr[index] = 0;
      if(c == LF) continue;     /* swallow LF completing prior CRLF */
      }

    if(c == CR) {
      _wascr[index] = 1;
      return LF;
      }

    if(c == LF) return LF;
    if(c == _eof_marker) return EOF;  /* text compatibility fallback */
    return c;
    }
  }

/* Raw byte input.  No newline or ^Z translation is performed here. */
getb(unit) int unit; {
  int c, index;
  char *next;

  if(unit == stdin) {
    c = _cpm(1, 0);
    return c & 0xFF;
    }

  index = unit - 5;
  if(_fchk(index) != MREAD) {
    err("CAN'T READ OUTFILE");
    exit(-1);
    }

  next = _fnext[index];
  if(next == _flast[index]) {
    next = _getbuf(index);
    if(next == -1) return EOF;
    }

  c = (*next++) & 0xFF;
  _fnext[index] = next;
  return c;
  }

getchar() {
  return getc(stdin);
  }

fgetc(unit) int unit; {
  return getc(unit);
  }

/*
** Read at most size-1 characters.  If a newline is read it is STORED,
** then the buffer is NUL-terminated.  Return NULL only when EOF occurs
** before any character is read (or for a non-positive size here).
*/
fgets(buf, size, unit) char *buf; int size, unit; {
  char *p;
  int c;

  if(size <= 0) return 0;
  p = buf;
  c = EOF;

  while(--size > 0) {
    c = fgetc(unit);
    if(c == EOF) break;
    *p++ = c;
    if(c == LF) break;
    }
  *p = 0;

  if(p == buf && c == EOF) return 0;
  return buf;
  }
