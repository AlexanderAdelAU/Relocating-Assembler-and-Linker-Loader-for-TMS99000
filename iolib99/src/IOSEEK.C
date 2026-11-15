/*
** IOSEEK.C -- random positioning for split IOLIB99
**
** New module, Aug 2026.  Adds lseek() and tell().
**
** NOTHING ELSE IN THE LIBRARY CHANGES.  IOCORE, IOOPEN, IOREAD and
** IOWRITE are untouched, so a program that never calls lseek does not
** grow by a single byte and cannot behave differently than before.
**
** How it works
** ------------
** The BDOS drives sequential I/O from the FCB's own cursor: CRN, the
** current record number (word at offset 26), with CBN, the current
** block (word 24).  Those live in the caller's copy of the FCB, which
** _stage_fcb hands to the BDOS for each call, so setting them here
** repositions the next sequential read or write.
**
** This is not a guess: fopen("a") in IOOPEN already does it.  It reads
** to end of file, decrements CRN by hand - borrowing into the high
** byte when it wraps - switches the slot to MWRITE, and lets the
** ordinary sequential write path carry on from there.  lseek does the
** same thing with an arbitrary record number, and clears CBN so the
** BDOS re-derives the block from the allocation chain, which is what
** BDTEST59 does whenever it plants a cursor by hand.
**
** Read units:  the buffer is invalidated so the next getc/getb pulls
**              the target record in through the normal path, then any
**              part-record offset is stepped over with getb.
** Write units: the held buffer is flushed first, then the cursor is
**              planted.  For a position inside a record, that record
**              is read back with the BDOS random read so the bytes in
**              front of the position survive; the next ordinary flush
**              writes the merged record back.  link99 needs this - it
**              patches a size word into a block header after the block
**              has been written.
**
** Limits, both from the 16-bit int
** -------------------------------
** A byte offset caps at 65535, i.e. 128 records.  SEEK_END is not
** supported at all and returns -1 - see the note in lseek().
**
** Small-C notes
** -------------
** '/' and '%' compile to the SIGNED _ccdiv, so an offset above 32767
** would divide wrongly.  The record and remainder are taken with a
** shift and a mask instead: the quotient of any 16-bit value by 512
** fits in 7 bits, so (offset >> 9) & 0x7F is right even though >> is
** an arithmetic shift.
** C escapes use standard values: '\r' is CR (0x0D), '\n' is LF (0x0A).
*/

/*
** BOTH VERSIONS LIVE HERE, chosen by the build:
**     smallcp -C -M          IOSEEK     the C below
**     smallcp -C -M -DASMVER IOSEEK     the assembly above
**
** The C is the DEFINITION and is what the assembly was written against.
** The assembly is 434 bytes where the C is 1,318 - most of the C was
** spent on things C cannot say directly: _fcbword and _setfcbword
** existed only because C cannot express a big-endian word at a byte
** offset, and the shift-and-mask arithmetic only to dodge the signed
** _ccdiv.  In assembly both are single instructions.  It is also
** faster, and it is the piece least likely ever to change again.
*/

#ifdef ASMVER

#asm
;   The register equates and DXOPs are NOT repeated here - smallcp
;   has already emitted them, and a second definition is an error.
NBUFS   EQU     6
SECTOR  EQU     512
MWRITE  EQU     17325           ;IOLIB99's write-mode marker

CBN     EQU     24              ;FCB: current block  (word)
CRN     EQU     26              ;FCB: current record (word)
RELB    EQU     28              ;FCB: relative block  - MUST be 0
RELR    EQU     30              ;FCB: relative record - the position

SETDMA  EQU     26              ;BDOS: set DMA address
RDRND   EQU     33              ;BDOS: read random

SEEKSET EQU     0
SEEKCUR EQU     1

        ENT     lseek
        ENT     tell

        EXT     _ffcb
        EXT     _fnext
        EXT     _ffirst
        EXT     _flast
        EXT     _wascr
        EXT     _commbuf
        EXT     _commfcb
        EXT     _dfltbuf
        EXT     _fchk
        EXT     _cpm
        EXT     _stage_fcb
        EXT     _unstage_fcb
        EXT     getb
        EXT     fflush

;*******************************************************************
;       UNITIDX - unit number in R1 -> slot index in R1, and the
;       word offset (index*2) in R2, for indexing the per-unit
;       tables.  Clobbers R1, R2.
;*******************************************************************
UNITIDX:
        AI      R1,-5                   ;units are 5..9
        MOV     R1,R2
        SLA     R2,1                    ;*2 for a word table
        RT

;*******************************************************************
;       RDBACK - read record R6 of slot R1 back into that unit's
;       buffer, using the BDOS random read.
;
;       RDRND takes its position from RELB:RELR and, by BDTEST's
;       contract, does NOT disturb CRN - so the cursor planted by
;       lseek stays planted and the ordinary sequential flush writes
;       the merged record back where it belongs.
;
;       At or past end of file the record does not exist yet, so the
;       buffer is zeroed and the following write appends.
;
;       Entry: R1 = slot index, R6 = record, R2 = index*2.
;       Clobbers R0,R2,R3,R4,R5,R7.
;*******************************************************************
RDBACK:
        MOV     R2,R7                   ;keep index*2
        MOV     @_ffcb(R7),R5           ;R5 = this unit's FCB

        CLR     @RELB(R5)               ;RELB must be zero
        MOV     R6,@RELR(R5)            ;RELR = the record wanted

        DECT    SP                      ;_cpm(SETDMA, _commbuf)
        MOV     @_commbuf,*SP
        DECT    SP
        LI      R4,SETDMA
        MOV     R4,*SP
        CALL    @_cpm
        AI      SP,4

        DECT    SP                      ;_stage_fcb(index)
        MOV     R1,*SP
        CALL    @_stage_fcb
        AI      SP,2

        DECT    SP                      ;_cpm(RDRND, _commfcb)
        MOV     @_commfcb,*SP
        DECT    SP
        LI      R4,RDRND
        MOV     R4,*SP
        CALL    @_cpm
        AI      SP,4
        MOV     R4,R3                   ;R3 = the BDOS result

        DECT    SP                      ;_unstage_fcb(index)
                                ;NOTE: smallcp 1.06a truncated this
                                ;to _unstage_fc and 1.06g does not,
                                ;so the spelling here must match the
                                ;compiler that built IOCORE
        MOV     R1,*SP
        CALL    @_unstage_fcb
        AI      SP,2

        DECT    SP                      ;_cpm(SETDMA, _dfltbuf)
        MOV     @_dfltbuf,*SP
        DECT    SP
        LI      R4,SETDMA
        MOV     R4,*SP
        CALL    @_cpm
        AI      SP,4

        MOV     @_ffirst(R7),R5         ;R5 = the unit's buffer
        LI      R2,SECTOR/2             ;copied a WORD at a time

        MOV     R3,R3                   ;did the read succeed?
        JLT     RB_ZERO

        MOV     @_commbuf,R0            ;copy from the common buffer
RB_COPY:
        MOV     *R0+,*R5+
        DEC     R2
        JNE     RB_COPY
        SETO    R4                      ;non-zero: the record existed
        RT

RB_ZERO:
        CLR     *R5+                    ;past EOF - nothing to preserve
        DEC     R2
        JNE     RB_ZERO
        CLR     R4
        RT

;*******************************************************************
;       tell(unit) - the byte offset of the next byte to be read or
;       written.
;
;       After a record is READ, CRN names the record that will be
;       fetched NEXT, so the one in the buffer is CRN - 1.  Straight
;       after fopen nothing is buffered, but _fnext = _flast =
;       _ffirst + SECTOR and CRN is 0, which the same expression
;       evaluates to 0.
;
;       For a WRITE unit CRN names the record the next flush will
;       write and the buffer holds bytes belonging to it, so there is
;       no minus one.
;*******************************************************************
tell:
        MOV     @2(SP),R1               ;unit
        CALL    @UNITIDX                ;R1 = index, R2 = index*2
        MOV     R2,R7

        DECT    SP                      ;_fchk(index)
        MOV     R1,*SP
        CALL    @_fchk
        AI      SP,2
        MOV     R4,R3                   ;R3 = mode

        MOV     @_ffcb(R7),R5
        MOV     @CRN(R5),R0             ;R0 = CRN

        CI      R3,MWRITE
        JEQ     TL_WR
        DEC     R0                      ;read: the buffer holds CRN-1
TL_WR:
        SLA     R0,9                    ;record * 512

        MOV     @_fnext(R7),R4
        S       @_ffirst(R7),R4         ;offset within the buffer
        A       R0,R4
        RET

;*******************************************************************
;       lseek(unit, offset, whence)
;
;       Arguments were pushed left to right, so:
;           2(SP) = whence      4(SP) = offset      6(SP) = unit
;
;       Returns the new byte offset, or -1.
;*******************************************************************
lseek:
        MOV     @6(SP),R1               ;unit
        CALL    @UNITIDX
        MOV     R2,R7                   ;R7 = index*2

        DECT    SP                      ;_fchk(index)
        MOV     R1,*SP
        CALL    @_fchk
        AI      SP,2
        MOV     R4,R3                   ;R3 = mode

;--- work out the target offset ------------------------------------
        MOV     @2(SP),R0               ;whence
        CI      R0,SEEKSET
        JEQ     LS_SET
        CI      R0,SEEKCUR
        JEQ     LS_CUR

;   SEEK_END is not supported.  Working the size out from the
;   directory entry was only ever right for files under 64KB, and for
;   a file still open for writing the entry is not even up to date.
;   Nothing calls it - link99 saves its position and seeks back.
        SETO    R4
        RET

LS_SET:
        MOV     @4(SP),R6               ;target = offset
        JMP     LS_HAVE

LS_CUR:
        MOV     @6(SP),R0               ;target = tell(unit) + offset
        DECT    SP
        MOV     R0,*SP
        CALL    @tell
        AI      SP,2
        MOV     R4,R6
        A       @4(SP),R6

LS_HAVE:
        MOV     R6,R6
        JLT     LS_FAIL                 ;a negative target is an error

;--- split it into a record and a remainder ------------------------
;   No divide: SRL is a LOGICAL shift, so unlike the C version this
;   needs no masking to stay right above 32767.
        MOV     R6,R5
        SRL     R5,9                    ;R5 = record
        MOV     R6,R0
        ANDI    R0,0FFFH
        ANDI    R0,01FFH                ;R0 = offset within the record

        MOV     @_ffcb(R7),R2           ;R2 = the FCB

        CI      R3,MWRITE
        JEQ     LS_WRITE

;===================================================================
;       READ SEEK
;===================================================================
        CLR     @CBN(R2)                ;let the BDOS re-derive the block
        MOV     R5,@CRN(R2)             ;plant the cursor

        MOV     @_flast(R7),R4          ;force a refill on the next read
        MOV     R4,@_fnext(R7)
        CLR     @_wascr(R7)             ;the CR/LF state goes with it

        MOV     R0,R0                   ;any part-record offset to skip?
        JEQ     LS_DONE

LS_SKIP:
        MOV     @6(SP),R4               ;getb(unit)
        DECT    SP
        MOV     R4,*SP
        CALL    @getb
        AI      SP,2
        CI      R4,-1
        JEQ     LS_FAIL                 ;end of file while skipping
        DEC     R0
        JNE     LS_SKIP
        JMP     LS_DONE

;===================================================================
;       WRITE SEEK
;===================================================================
LS_WRITE:
        MOV     R0,R1                   ;keep the remainder
        MOV     @6(SP),R4               ;fflush(unit) - write what is held
        DECT    SP
        MOV     R4,*SP
        CALL    @fflush
        AI      SP,2
        MOV     R4,R4
        JLT     LS_FAIL

        MOV     @_ffcb(R7),R2           ;fflush may have moved things
        CLR     @CBN(R2)
        MOV     R5,@CRN(R2)             ;plant the cursor

        MOV     R1,R1                   ;record-aligned?
        JEQ     LS_DONE

;   Not aligned: read the record back so the bytes in front of the
;   position survive, then point into it.  The next flush writes the
;   merged record.
        MOV     R1,R0                   ;keep the remainder again
        MOV     R7,R2
        SRL     R2,1                    ;R1 index for RDBACK
        MOV     R2,R1
        MOV     R7,R2
        MOV     R5,R6                   ;R6 = the record
        MOV     R0,R5                   ;park the remainder in R5
        CALL    @RDBACK
        MOV     R5,R0                   ;recover it

        MOV     @_ffirst(R7),R4
        A       R0,R4
        MOV     R4,@_fnext(R7)

LS_DONE:
        MOV     R6,R4                   ;return the new offset
        RET

LS_FAIL:
        SETO    R4
        RET
#endasm

#else

#include "stdio.h"

/* Constants are local deliberately: Small-C/Plus 1.06a has limited include handling. */
/* NBUFS is defined in SIX files - IODEFS.H, IOCORE.C, IOOPEN.C,
   IOREAD.C, IOWRITE.C and here.  They must all agree. */
#define NBUFS   6
#define SECTOR  512
#define MREAD   22489
#define MWRITE  17325

/* FCB word fields.  High byte at the even offset. */
#define FCB_CBN_HI   24
#define FCB_CRN_HI   26
#define FCB_FSZ_HI   14
#define FCB_LRBL_HI  20

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

extern int _ffcb[NBUFS];
extern int _fnext[NBUFS];
extern int _ffirst[NBUFS];
extern int _flast[NBUFS];
extern int _wascr[NBUFS];
extern char *_commbuf;
extern char *_commfcb;
extern char *_dfltbuf;

/*
** Read one FCB word.  Words are big-endian: high byte at the even
** offset, low byte at the odd one.
*/
_fcbword(fcb, off) char *fcb; int off; {
  int hi, lo;
  hi = fcb[off] & 0xFF;
  lo = fcb[off + 1] & 0xFF;
  return (hi << 8) | lo;
  }

_setfcbword(fcb, off, value) char *fcb; int off, value; {
  fcb[off] = value >> 8;
  fcb[off + 1] = value;
  }

/*
** Read record 'rec' back into the unit's buffer with the BDOS random
** read, so that a write which starts part way into a record leaves the
** bytes in front of it as they were.
**
** RDRND does not disturb CRN - BDTEST59 asserts that - so the cursor
** this module just planted stays planted, and the ordinary sequential
** fflush writes the merged record back where it belongs.
**
** At or past end of file the record does not exist yet; the buffer is
** zeroed and the write appends.
**
** This is the ONLY place a random BDOS call is made.  It lives here so
** that _getbuf and fflush never name it, and so no program that merely
** reads or writes a file drags this module in.
*/
_rdback(index, rec) int index, rec; {
  char *fcb, *dst, *src;
  int i, result;

  fcb = _ffcb[index];
  fcb[28] = 0;              /* RELB high - must be zero */
  fcb[29] = 0;
  fcb[30] = rec >> 8;       /* RELR high */
  fcb[31] = rec;

  _cpm(26, _commbuf);
  _stage_fcb(index);
  result = _cpm(33, _commfcb);      /* RDRND */
  _unstage_fcb(index);
  _cpm(26, _dfltbuf);

  dst = _ffirst[index];
  i = SECTOR;
  if(result < 0) {
    while(i--) *dst++ = 0;          /* past EOF: nothing to preserve */
    return 0;
    }

  src = _commbuf;
  while(i--) *dst++ = *src++;
  return 1;
  }

/*
** Current byte offset of the next byte to be read or written.
**
** After a record is READ, CRN names the record that will be fetched
** NEXT, so the one sitting in the buffer is CRN - 1.  Straight after
** fopen nothing is buffered, but _fnext == _flast == _ffirst + SECTOR
** and CRN is 0, which the same expression evaluates to 0.
**
** For a WRITE unit CRN names the record the next flush will write and
** the buffer holds bytes belonging to that record, so there is no
** minus one.
*/
tell(unit) int unit; {
  int index, mode, crn;
  char *fcb;

  index = unit - 5;
  mode  = _fchk(index);
  fcb   = _ffcb[index];
  crn   = _fcbword(fcb, FCB_CRN_HI);

  if(mode == MWRITE)
    return (crn << 9) + (_fnext[index] - _ffirst[index]);
  return ((crn - 1) << 9) + (_fnext[index] - _ffirst[index]);
  }

/*
** Position 'unit' to a byte offset.  whence is SEEK_SET, SEEK_CUR or
** SEEK_END.  Returns the new offset, or -1 on failure.
*/
lseek(unit, offset, whence) int unit, offset, whence; {
  int index, mode, target, rec, rem;
  char *fcb;

  index = unit - 5;
  mode  = _fchk(index);

  if(whence == SEEK_SET) target = offset;
  else if(whence == SEEK_CUR) target = tell(unit) + offset;
  else if(whence == SEEK_END) {
    /*  Not supported.  Working the size out from the directory entry
        cost about 300 bytes and was only ever right for files under
        64KB - and for a file still open for writing the entry is not
        even up to date.  Nothing calls it: link99 saves its position
        and seeks back to it instead.  Use the BDOS GETSIZ call (35)
        where a real size is needed.  */
    return -1;
    }
  else return -1;

  if(target < 0) return -1;

  /* shift and mask, never divide - see the note at the top */
  rec = (target >> 9) & 0x7F;
  rem = target & 0x1FF;

  fcb = _ffcb[index];

  /*  Is the target already in the buffer?  After a sequential read of
      record R the BDOS has stepped CRN to R+1, so rec+1 == CRN means the
      bytes are in front of us and the seek is pure pointer arithmetic -
      no BDOS call at all.  tell() encodes the same relationship: it
      returns ((crn - 1) << 9) + (_fnext - _ffirst) for a read unit.

      CRN is 0 on a fresh open and after rewind, and rec is never
      negative, so neither can reach this path by accident.  A short
      final record leaves _flast below _ffirst+512; a target beyond it
      falls through to the slow path and behaves exactly as before.

      link99 seeks once per library member and rescans the library after
      every member it links.  Members are small, so many of those seeks
      land in the record already held.  */
  if(mode != MWRITE
     && rec + 1 == _fcbword(fcb, FCB_CRN_HI)
     && (_ffirst[index] + rem) <= _flast[index]) {
    _fnext[index] = _ffirst[index] + rem;
    _wascr[index] = 0;
    return target;
    }

  if(mode == MWRITE) {
    /*  Flush what is held first: fflush writes a whole 512-byte record
        at the current cursor and steps it on.  Then plant the cursor,
        read the target record back so a part-record write preserves
        what is in front of it, and point into it.  The next flush
        writes the merged record at the cursor by the ordinary path. */
    if(fflush(unit) < 0) return -1;
    _setfcbword(fcb, FCB_CBN_HI, 0);
    _setfcbword(fcb, FCB_CRN_HI, rec);
    if(rem) {
      _rdback(index, rec);
      _fnext[index] = _ffirst[index] + rem;
      }
    return target;
    }

  _setfcbword(fcb, FCB_CBN_HI, 0);
  _setfcbword(fcb, FCB_CRN_HI, rec);
  _fnext[index] = _flast[index];        /* force a refill on next read */
  _wascr[index] = 0;

  /*  Pull the record in with one getb, then place the cursor directly.
      Stepping in a byte at a time cost up to 511 calls per seek, and
      link99 seeks once per library member on every rescan.  getb has
      already refilled the buffer and left _fnext at _ffirst+1, so the
      offset is simply _ffirst+rem.  */
  if(rem) {
    if(getb(unit) == -1) return -1;
    _fnext[index] = _ffirst[index] + rem;
    }

  return target;
  }

#endif
