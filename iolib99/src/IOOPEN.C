/*
** IOOPEN.C -- FCB construction, open, close and delete for split IOLIB99
*/
#include "stdio.h"

/* Constants are local deliberately: Small-C/Plus 1.06a has limited include handling. */
/*
 * NBUFS - the number of file units.
 *
 * *** THIS CONSTANT IS DEFINED IN SIX FILES: IODEFS.H, IOCORE.C,
 * *** IOOPEN.C, IOREAD.C, IOWRITE.C and IOSEEK.C.  THEY MUST ALL AGREE.
 *
 * Per-unit arrays are sized by it.  If one file thinks it is 5 and
 * another thinks it is 6, the shorter arrays are indexed past their end
 * and the corruption is silent.
 *
 * Raised from 5 to 6 because link99 holds five at once in phase 1 -
 * DATA.DT$, the .R$, the .CT$, the module and DATA.D$ - which left no
 * spare at all, and the sixth open reported "ERROR: NO BUFFERS".
 *
 * The cost is 14 bytes of BSS per unit.  The 548-byte buffer itself
 * comes from the heap and only when the unit is actually used.
 */
#define NBUFS   6
#define LGH     512
#define BUFLGH  (LGH + 36)
#define SECTOR  512
#define MFREE   11387
#define MREAD   22489
#define MWRITE  17325
#define COMM_FCB    0xAC
#define NAMSIZ      11
#define EXTSIZ       3
#define FCBSIZ      36
#define FCB_CRN      27
#define FCB_LBUFCNT  35
/* FCB byte 18 is the BDOS folder selector: >20 or >FF means "use the
 * active folder (CFLD)", >FE means "the root, explicitly", any other
 * value is a literal folder id.  _newfcb zeroes the whole FCB, so it
 * must put the blank back or every name resolves against the root. */
#define FCB_CFLD     18

extern int _eof_marker;
extern int _ffcb[NBUFS];
extern int _fnext[NBUFS];
extern int _ffirst[NBUFS];
extern int _flast[NBUFS];
extern int _fmode[NBUFS];
extern int _ferr[NBUFS];
extern int _ftext[NBUFS];
extern int _wascr[NBUFS];
extern char *_commfcb;

_upper(c) char c; {
  if(c < 'a') return c;
  if(c > 'z') return c;
  return c - 32;
  }

_newfcb(name, fcb) char *name, *fcb; {
  char c;
  int i;

  i = FCBSIZ;
  while(i--) fcb[i] = 0;
  for(i = 0; i < NAMSIZ; i++) fcb[i] = ' ';
  fcb[FCB_CFLD] = ' ';

  if(*name == 0) return 0;
  if(name[1] == ':') name += 2;
  if(*name == 0) return 0;

  i = 0;
  while((c = _upper(*name++)) && c != '.') {
    if(i < NAMSIZ - EXTSIZ) fcb[i++] = c;
    }

  if(c == '.') {
    i = NAMSIZ - EXTSIZ;
    while((c = _upper(*name++)) && i < NAMSIZ)
      fcb[i++] = c;
    }
  return 1;
  }

_textmode(mode) char *mode; {
  char c;
  while(c = *mode++) {
    c = _upper(c);
    if(c == 'B') return 0;
    }
  return 1;
  }

fopen(name, mode) char *name, *mode; {
  char c;
  char *fcb;
  int index, unit, r15, ch, marker;

  index = NBUFS;
  while(index--) if(_fmode[index] == MFREE) break;
  if(index == -1) {
    err("NO BUFFERS");
    exit(-1);
    }

  unit = index + 5;
  _ferr[index] = 0;
  _ftext[index] = _textmode(mode);
  _wascr[index] = 0;

  if(_ffcb[index] == 0) {
    _ffcb[index] = alloc(BUFLGH);
    if(_ffcb[index] == -1) return 0;
    }

  _ffirst[index] = _ffcb[index] + FCBSIZ;
  _flast[index] = _ffirst[index] + LGH;
  fcb = _ffcb[index];

  if(_newfcb(name, fcb) == 0) return 0;
  c = _upper(*mode);

  if(c == 'R' || c == 'A') {
    r15 = _cpm_fcb(15, index);
    if(r15 < 0 || r15 == 0xFF) return 0;

    _fmode[index] = MREAD;
    _fnext[index] = _flast[index];

    if(c == 'A') {
      /* Find the physical append point.  In text mode an old-style ^Z
         marker is a logical EOF; in binary mode every byte is data. */
      marker = 0;
      while((ch = getb(unit)) != EOF) {
        if(_ftext[index] && ch == _eof_marker) {
          marker = 1;
          break;
          }
        }

      if(fcb[FCB_CRN] == 0) {
        fcb[FCB_CRN] = 0xFF;
        fcb[FCB_CRN - 1] = fcb[FCB_CRN - 1] - 1;
        }
      else fcb[FCB_CRN] = fcb[FCB_CRN] - 1;

      if(marker && _fnext[index] > _ffirst[index])
        _fnext[index] = _fnext[index] - 1;
      _wascr[index] = 0;
      _fmode[index] = MWRITE;
      }
    return unit;
    }

  if(c == 'W') {
    _cpm_fcb(19, index);
    if(_cpm_fcb(22, index) < 0) return 0;
    _fmode[index] = MWRITE;
    _fnext[index] = _ffirst[index];
    return unit;
    }

  return 0;
  }

/* Delete uses the same common-memory FCB staging address as fopen. */
delete(name) char *name; {
  /* FCBSIZ+1, NOT FCBSIZ: smallcp places a char local at frame+1 but
   * does not add that byte to the frame, so an even-length char array
   * declared first overruns by one.  fcb[35] landed on the high byte
   * of this function's own return address, and _newfcb zeroes it on
   * every call, so delete() always returned to >00xx.  The odd length
   * takes the word-rounding pad instead. */
  char fcb[FCBSIZ+1];
  char *src, *dst, *commfcb;
  int *p;
  int i, result;

  if(_newfcb(name, fcb) == 0) return -1;

  p = COMM_FCB;
  commfcb = *p;
  src = fcb;
  dst = commfcb;
  i = FCBSIZ;
  while(i--) *dst++ = *src++;

  result = _cpm(19, commfcb);
  if(result < 0 || result == 0xFF) return -1;
  return 0;
  }

fclose(unit) int unit; {
  int index, werror, nbytes, result;
  char *end, *ptr, *fcb;

  index = unit - 5;
  if(_fchk(index) == MREAD) {
    _fmode[index] = MFREE;
    _wascr[index] = 0;
    return 0;
    }

  nbytes = _fnext[index] - _ffirst[index];
  if(nbytes == 0) nbytes = SECTOR;

  ptr = _fnext[index];
  end = _flast[index];
  while(ptr < end) *ptr++ = _eof_marker;

  werror = fflush(unit);
  fcb = _ffcb[index];
  fcb[FCB_LBUFCNT] = nbytes;

  result = _cpm_fcb(16, index);
  _fmode[index] = MFREE;
  _wascr[index] = 0;
  if(result < 0 || result == 0xFF || werror) {
    _ferr[index] = 1;
    return EOF;
    }
  return 0;
  }
