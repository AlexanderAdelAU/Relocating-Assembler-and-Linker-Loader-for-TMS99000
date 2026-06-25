/*
 ============================================================================
 Name        : link99.c
 Authors     : J. E. Hendrix (original Small-MAC linker, 1985)
               Alex Cameron (TMS9900 port 1984; Eclipse cross-compiler 2015)
 Version     : 3.9.10
 Copyright   : Free to use
 Description : Relocatable linker/loader for the TMS9900 architecture.
               Based on the Small-MAC linkage editor (ver 1.0).

 Version history:
   1.0  J.E. Hendrix   Original Small-MAC linker (8080/CP/M)
   2.0  A. Cameron     TMS9900 port (1984)
   3.0  A. Cameron     Eclipse cross-compiler version (June 2015)
   3.1  A. Cameron     COMBASE moved to 0x500 for extended memory
   3.2  A. Cameron     Disk overflow fully implemented; code tidied
   3.3  A. Cameron     First positional arg may be output name only (no .R99)
   3.4  A. Cameron     Relocation tables moved to disk (.CT$ .DT$); frees
                       ~30KB on 64K systems; fixes DREL resolve() chain bug
   3.5  A. Cameron     Entry-index scheme for CT$/DT$: crelptr/drelptr are
                       now entry indices (x CRELSIZE for seek), breaking the
                       64K relocation table limit. All XPOFF entries padded
                       to CRELSIZE. Multi-pass library search added.
   3.6  A. Cameron     Memory manager support: -P# page tagging per module,
                       page-map table (5 bytes/entry) appended after data,
                       cross-page trampoline stubs prepended as loader prefix
                       [word-count][stubs]. Loader extension peels prefix to
                       common 0x0400 before segment-1 copy. -K# sets CRU
                       base (MEMBASE) for SBO/SBZ page switching. -T# sets
                       trampoline load address (default 0x0400).
   3.7  A. Cameron     Transparent mapping: GAL22V10 now maps all paged
                       accesses without PSEL. Trampolines eliminated entirely
                       - cross-page calls are plain BL/RT. -T#/-K#/-W#
                       switches removed. Segment collision detection added:
                       two modules on different pages sharing a virtual
                       segment is a hard link error. -P# extended to 0-15
                       (SA0-SA3 support 16 physical pages). pagemap table
                       retained: loader uses it to initialise 6116 map
                       registers before jumping to program.
   3.8  A. Cameron     Page-separated output blocks: code segment now emitted
                       as one block per page, each prefixed with a size word.
                       File layout:
                         [M x 6-byte pagemap entries]
                         [FFFF FFFF FFFF sentinel]
                         [size:word][page 0 code+data]
                         [size:word][page 1 code+data]
                         ...
                         [sector padding]
                       Non-paged programs emit a single block as before
                       (sentinel only, then [size:word][all code+data]).
   3.9  A. Cameron     Cleaner command syntax:
  3.9.1 A. Cameron     512KB buffer; paged AORG fixup; flat raw binary output
  3.9.2 A. Cameron     Fix resolve() direct patch for assembly externals
  3.9.3 A. Cameron     Fix resolve() xrloc is virtual addr - subtract cbase for buffer access
  3.9.4 A. Cameron     Fix cmod: save cloc at ENAME before SETLC records corrupt it
  3.9.5 A. Cameron     Fix pmmin_global: only track first module PRels for AORG detection
  3.9.6 A. Cameron     Fix PREL: always add cmod even when field=0 (offset 0 in module)
                       - Output name requires explicit .COM extension
                       - Input modules require explicit .R99 extension
                       - Address flags accept 0x prefix (e.g. -O0x2000)
                       - Fixed: -O flag was silently ignored (cbase reset in phase1)
                       - COMBASE removed: cbase defaults to 0, AORG addresses
                         are absolute and require no base offset
                       - oflag added: distinguishes -O0 from no -O flag
                       - pmstart/pmend now store absolute addresses (cbase+cmod)
                         so page map entries correctly reflect AORG-based modules
                       - pageof() and in_page() updated for absolute pmstart/pmend
                       Shell loader reads one page block at a time, programs
                       6116 map registers, then loads directly to virtual
                       address — no staging area needed, supports programs
                       larger than available common memory.
  3.9.7 A. Cameron     Fix cmod stale for library modules: search() consumes the
                       ENAME record before load() runs, leaving cloc_at_ename set
                       to the previous module's load base. Fix: initialise
                       cloc_at_ename = cloc at the top of load() so PSIZE always
                       gets the correct module base even when ENAME was pre-consumed.
  3.9.8 A. Cameron     Fix mid-commandline -P# page tagging: getsw() was setting
                       curpage globally from the last -P# seen, corrupting page
                       assignments for modules before that flag. Fix: getsw() now
                       only sets pagemode=YES; curpage is set exclusively in the
                       phase1 argument loop at the point each -P# is encountered.
                       This enables single-step overlay builds:
                         link99 -M out.COM base.R99 -P2 ovla.R99 -P3 ovlb.R99
  3.9.10 A. Cameron    Paged output: replace sentinel+pagemap+blocks format with
                       a self-describing linked-list block chain. Each block has
                       an 8-byte header: next_offset, page, start, size — followed
                       immediately by its data. next_offset is the byte distance
                       from this header to the next (0 = last block). The loader
                       does a single forward pass: read header, MAP_SET, copy data,
                       follow next_offset. No sentinels, no rewind, no two pointers.
                       Paged output now emits .EXE extension (not .COM) so the
                       shell loader can distinguish chain format from flat binary
                       by file type alone — no magic numbers needed.
                       Applies only to paged (-P#) output; flat .COM path unchanged.
  3.9.9 A. Cameron     Suppress ABS/PREL/DREL/CREL items from -M monitor output.
                       Only special items (ENAME and above, code >= 4) are printed,
                       eliminating the blizzard of g=0/g=1 lines per code byte.
  3.9.6 A. Cameron     Fix PREL: always add cmod even when field=0 (offset 0 in module)
                       - Output name requires explicit .COM extension
                       - Input modules require explicit .R99 extension
                       - Address flags accept 0x prefix (e.g. -O0x2000)
                       - Fixed: -O flag was silently ignored (cbase reset in phase1)
                       - COMBASE removed: cbase defaults to 0, AORG addresses
                         are absolute and require no base offset
                       - oflag added: distinguishes -O0 from no -O flag
                       - pmstart/pmend now store absolute addresses (cbase+cmod)
                         so page map entries correctly reflect AORG-based modules
                       - pageof() and in_page() updated for absolute pmstart/pmend
                       Shell loader reads one page block at a time, programs
                       6116 map registers, then loads directly to virtual
                       address — no staging area needed, supports programs
                       larger than available common memory.

 ============================================================================
 LINKER OPERATION - THREE PHASES
 ============================================================================

 Phase 1 - Loading (phase1())
 ----------------------------
 Processes each command-line argument in order:

   Object modules (.R99):
     - Loaded into the code buffer (or overflow file .O$ if buffer full)
     - PREL/DREL relocatable words recorded in disk-based relocation tables
       (.CT$ for code, .DT$ for data) using a 5-byte entry-index scheme
     - External references (XCHAIN) added to the xr symbol table as chains
     - Entry points (EPOINT) added to the ep symbol table
     - link() called after each module to resolve any newly satisfiable
       external references

   Library files (.LIB):
     - Searched via their companion .NDX index file
     - Each module's ENAME records scanned for unresolved externals
     - Matching modules loaded and linked on demand
     - Library search is MULTI-PASS: after all command-line arguments are
       processed, all libraries are re-searched repeatedly until no new
       modules are loaded in a complete pass. This resolves cross-library
       dependencies (e.g. iolib references printf in clib) regardless of
       library order on the command line.

   Temporary files created during Phase 1:
     .CT$   code relocation table (entry-index scheme, 5 bytes/entry)
     .DT$   data relocation table (entry-index scheme, 5 bytes/entry)
     .O$    code segment overflow (when buffer fills up)
     .R$    code relocation reference stream (parallel to .O$)
     .D$    data segment image
     .DR$   data relocation reference stream

 Phase 2 - Writing (phase2())
 ----------------------------
 Reads back the code image (from buffer and/or .O$ overflow file) and
 produces the final output file (.COM or .LGO):

   - Walks the code image word by word using the .R$ reference stream
   - For each relocatable word: reads its CT$ entry to get the original
     module-relative value, adds the segment base (cbase or dbase), and
     writes the final absolute value to the output file
   - For XPOFF (external-reference-plus-offset) entries: the base address
     of the resolved symbol is added to the stored positive offset before
     patching
   - Absolute bytes are copied through unchanged
   - The special symbol ?MEMRY, if present, is patched with the address
     of the first free byte above the loaded program (freemem())

 resolve() - External Reference Resolution
 ------------------------------------------
 Called from link() after each module load. For each matched xr/ep pair:
   - Walks the XCHAIN linked list stored in the code image
   - Patches each location with the final absolute address of the ep symbol
   - Handles PREL (code-relative) and DREL (data-relative, falls through
     to PREL chain walk since Small-C DREL XCHAINs live in code segment)
   - Circular chain guard: detects self-referential chains (prev_xrloc)
     to prevent infinite loops from corrupted relocation data

 ============================================================================
 Usage: link99 [-B] [-S] [-G#] [-D#] [-M] program [module/library ...]
 ============================================================================

   -B          Big program: force all code to disk from the start, leaving
               the entire memory buffer free for the symbol table. Required
               when the combined code+symbol table exceeds the buffer.

   -G#         Make program absolute at hex address # and write output
               as "program.LGO" instead of "program.COM".

   -D#         Set absolute data segment base to hex address #.

   -S          Generate a Small-C call wrapper to main().

   -M          Monitor: verbose progress output showing module loads,
               symbol resolutions and library searches.

   program     Output name (.COM or .LGO appended). If a matching .R99
               exists it is loaded first; if not, used as output name only.
               This allows:
                 link99 -M -S -B smallC99 cc1 cc2 ... clib99.LIB

   module/     Object modules (.R99) or library files (.LIB).
   library     Libraries are searched multi-pass until all cross-library
               dependencies are satisfied.

 File extensions:
   .R99/.REL   Relocatable object module (input)
   .LIB        Library of object modules
   .NDX        Library index (companion to .LIB; not user-specified)
   .COM        CP/M-style binary output (default)
   .LGO        Load-and-go output (-G# option)
   .O$         Temporary: code segment overflow
   .R$         Temporary: code relocation reference stream
   .D$         Temporary: data segment image
   .DR$        Temporary: data relocation reference stream
   .CT$        Temporary: code relocation table (entry-index, 5 bytes/entry)
   .DT$        Temporary: data relocation table (entry-index, 5 bytes/entry)

 Notes:
   ?MEMRY      If declared external, patched with address of first free byte
               above the loaded program image.
   Host/target This linker runs on a 32-bit host (Windows/Linux) and
               produces output for a 16-bit TMS9900 target. To build for
               a 16-bit host change #define int32 to #define int16.

 ============================================================================
 REL File Format
 ============================================================================

 The .R99/.REL file is a packed BIT-STREAM, not a simple byte stream.
 Items are read 16 bits at a time (one "chunk") via getrel(), and fields
 are extracted from those chunks without regard to byte boundaries.

 Each item begins with a 2-bit TYPE field:
   00  ABS   - absolute (non-relocatable) byte value follows
   01  PREL  - program (code) relative word follows
   10  DREL  - data relative word follows
   11  special item - an additional field identifies which:
         ENAME  (4)   entry name
         CNAME  (5)   common block name
         PNAME  (6)   program name
         LNAME  (7)   library name
         XCHAIN (10)  external reference chain head + symbol name
         EPOINT (11)  entry point location + symbol name
         XPOFF  (13)  external reference + positive offset
         DSIZE  (14)  data segment size
         SETLC  (15)  set location counter
         PSIZE  (17)  program (code) segment size
         EPROG  (18)  end of program / start address
         EFILE  (19)  end of file

 Symbol names are encoded 4 bits per character (BITPSYM=4), giving a
 maximum symbol length of 10 characters (MAXSYM=10) packed into 40 bits.

 Relocation chains:
   PREL and DREL items carry the HEAD of a singly-linked list of every
   location in the module that references a given external symbol. Each
   location in the code image holds the offset of the next location in
   the chain; zero terminates the chain. resolve() walks this chain at
   link time, patching each location with the final absolute address.

 Memory layout during linking:
   buffer[]  code image grows upward  from buffer[0]      -> bpnext
   symbol    table grows  downward from buffer[memsize]   -> snext
   .CT$      code relocation table  (disk file; entry-index scheme)
   .DT$      data relocation table  (disk file; entry-index scheme)

   When bpnext approaches snext (within CUSHION bytes) the code segment
   overflows to the .O$ temporary file. The -B flag forces this overflow
   from byte 0, dedicating the entire buffer to the symbol table.

 CT$/DT$ entry-index scheme (v3.5):
   Each entry is exactly CRELSIZE (5) bytes: [loc:2][val:2][type:1]
   crelptr/drelptr are entry INDICES (not byte offsets). ctseek()/dtseek()
   multiply by CRELSIZE before seeking. This gives 65535 entries x 5 bytes
   = ~327KB maximum table, vs the old 64K byte-offset limit. All entry
   types (PREL, DREL, XPOFF) are padded to CRELSIZE for uniform stride.
 ============================================================================
*/

#include "rel99.h"
#include "stdio.h"
#include "fcntl.h"

/* O_BINARY is a DOS/Windows flag; not needed on POSIX systems */
#ifndef O_BINARY
#define O_BINARY 0
#endif

/*
 * lseek() origin constants - not defined in the Small-C stdio.h or fcntl.h
 * Standard values: 0=from start, 1=from current, 2=from end
 */
#define SEEK_SET  0     /* seek from beginning of file  */
#define SEEK_CUR  1     /* seek from current position   */
#define SEEK_END  2     /* seek from end of file        */

/* -------------------------------------------------------------------------
 * Compile-time configuration
 * ---------------------------------------------------------------------- */

#define BUFSIZE     512             /* I/O buffer size                      */
/* #define DEBUG    */             /* enable progress/debug output          */
/* #define DEBUG2   */             /* enable detailed buffer dump output    */

#define MAXFILES    20
#define CRELSIZE    5               /* bytes per relocation table entry     */
#define MAXMEM		48128;			/* start copying to disc 				*/

#define NOCCARGC                    /* do not pass arg counts to functions  */

#define NAMESIZE    15
#define MAXFIL      20
#define STACK       512             /* stack headroom reservation           */
#define AUXBUF      8096            /* aux buffer for reference file        */
/* RELMEM removed: relocation tables now held in disk files .CT$ and .DT$   */
#define MAXOPEN     6               /* maximum simultaneously open files    */
#define OHDOPEN     1024            /* memory overhead per open file        */
/* COMBASE removed - cbase defaults to 0; use -O# to override if needed     */
#define RET         0x2DC0          /* TMS9900 RET instruction              */
#define JMP         0x0460          /* TMS9900 BRANCH instruction           */
#define TPA         0x1000          /* Transient Program Area base          */
#define RES         (-1)            /* value indicating a resolved ext ref  */
#define XRPLUS      (-2)            /* ext-ref-plus-offset flag             */

/* File extensions */
#define MODEXT  ".R99"
#define LIBEXT  ".LIB"
#define NDXEXT  ".NDX"
#define COMEXT  ".COM"
#define EXEEXT  ".EXE"              /* paged chain-format output            */
#define LGOEXT  ".LGO"
#define OFLEXT  ".O$"
#define REFEXT  ".R$"
#define DATEXT  ".D$"
#define DRFEXT  ".DR$"
#define CRLTBEXT ".CT$"             /* code relocation table (disk)         */
#define DRLTBEXT ".DT$"             /* data relocation table (disk)         */

/* -------------------------------------------------------------------------
 * Memory manager / page map
 * ---------------------------------------------------------------------- */
#define MAXPMODS    32              /* max modules tracked in page map      */
#define PMTERM      0xFFFF          /* page-map table terminator word       */

/* v3.7: trampolines removed - GAL transparent mapping makes cross-page
 * calls plain BL/RT. -T#/-K#/-W# switches removed. Segment collision
 * detection replaces trampoline registration in resolve().             */
#define PAGE_SEG    0x1000          /* 4KB virtual segment size             */

#define TRUE    1
#define FALSE   0

/*
 * Host architecture selector.
 * int32: building on a 32-bit host (pointer = 4 bytes).
 * int16: building on a 16-bit host (pointer = 2 bytes).
 */
#define int32 1

/* -------------------------------------------------------------------------
 * Symbol table layout
 * Each entry: [NXT ptr | VAL | SYM string | FLG byte]
 * ---------------------------------------------------------------------- */
#if int16
#define NXT     0                       /* next-entry pointer (2 bytes)     */
#define VAL     2                       /* value field (2 bytes)            */
#define SYM     4                       /* symbol name (MAXSYM+1 bytes)     */
#define FLG     (SYM + MAXSYM + 1)      /* flag byte                        */
#define SSZ     (SYM + MAXSYM + 2)      /* total entry size                 */
#define HIGH    0x7f                    /* sentinel: end of ASCII table     */
#define CUSHION (200 * SSZ)             /* headroom before overflow         */
#define FLGEXT  0x80                    /* flag: symbol is external         */
char high[] = { HIGH, 0 };
#endif

#if int32
#define NXT     0                       /* next-entry pointer (4 bytes)     */
#define VAL     4                       /* value field (4 bytes)            */
#define SYM     8                       /* symbol name (MAXSYM+1 bytes)     */
#define FLG     (SYM + MAXSYM + 1)      /* flag byte                        */
#define SSZ     (SYM + MAXSYM + 2)      /* total entry size                 */
#define HIGH    0x7f                    /* sentinel: end of ASCII table     */
#define CUSHION (400 * SSZ)             /* headroom before overflow         */
#define FLGEXT  0x80                    /* flag: symbol is external         */
char high[] = { HIGH, 0 };
#endif

/* -------------------------------------------------------------------------
 * REL-file I/O state
 * ---------------------------------------------------------------------- */
int            inrel;       /* fd: input REL file                           */
int            outrel;      /* fd: output REL file                          */
unsigned inrem;       /* remaining bits in current input chunk        */
unsigned inchunk;     /* current 16-bit chunk from REL file           */
unsigned outrem;      /* remaining bits in current output chunk       */
unsigned outchunk;    /* current 16-bit chunk for REL file            */
unsigned item;        /* current item code                            */
unsigned type;        /* type field of current item                   */
unsigned field;       /* current bit-field value                      */
char           symbol[10];  /* current symbol name                          */

/* -------------------------------------------------------------------------
 * Global file-name buffers
 * ---------------------------------------------------------------------- */
char modname[MAXSYM + 1];   /* name of current module                       */
char infn[NAMESIZE];        /* input filename                               */
char ndxfn[NAMESIZE];       /* library index filename (.NDX)                */
char csfn[NAMESIZE];        /* code-segment overflow filename (.O$)         */
char crfn[NAMESIZE];        /* code relocation reference filename (.R$)     */
char dsfn[NAMESIZE];        /* data-segment filename (.D$)                  */
char drfn[NAMESIZE];        /* data relocation reference filename (.DR$)    */
char ctfn[NAMESIZE];        /* code relocation table filename (.CT$)        */
char dtfn[NAMESIZE];        /* data relocation table filename (.DT$)        */
char outfn[NAMESIZE];       /* output filename (.COM or .LGO)               */

/*
 * crelptr / drelptr are entry INDICES into the on-disk relocation table
 * files (.CT$ and .DT$).  Byte offset = index * CRELSIZE.
 * Storing a 16-bit index in the 2-byte code image slot gives 65535
 * entries (327KB of table) vs the old 13107-entry 64K byte-offset limit.
 * On a 32-bit host 'unsigned int' is used so the index arithmetic never
 * overflows even at the high end.
 */
unsigned int crelptr;       /* current entry index in code relocation table */
unsigned int drelptr;       /* current entry index in data relocation table */

/* -------------------------------------------------------------------------
 * Library multi-pass support
 * ---------------------------------------------------------------------- */
#define MAXLIBS     10                  /* max library files on command line */
char libfns[MAXLIBS][NAMESIZE];         /* library filenames collected       */
int  nlibfns;                           /* number of libraries               */
int  liblinked;                         /* set by search() when module loads */

/* -------------------------------------------------------------------------
 * Symbol table / buffer pointers
 * ---------------------------------------------------------------------- */
char *bpnext;   /* next free byte in the in-memory code buffer              */
char *sfree;    /* head of the freed symbol-entry list                      */
char *epfirst;  /* first entry-point record in symbol table                 */
char *epprev;   /* previous entry-point record                              */
char *epnext;   /* current entry-point record                               */
char *xrfirst;  /* first external-reference record in symbol table          */
char *xrprev;   /* previous external-reference record                       */
char *xrnext;   /* current external-reference record                        */
char *ep;       /* entry-point address for the current module               */
char *xr;       /* external reference                                       */
char *snext;    /* next free symbol table entry                             */

/* -------------------------------------------------------------------------
 * Location counters and size variables
 * ---------------------------------------------------------------------- */
unsigned pass;    /* current pass number (1 or 2)                     */
unsigned cdisk;   /* code buffer offset where disk overflow begins    */
unsigned dbase;   /* data segment base address                        */
unsigned nxr;     /* next item in external-reference chain            */
unsigned cloc;    /* code location counter                            */
unsigned dloc;    /* data location counter                            */
unsigned csize;   /* total code segment size                          */
unsigned dsize;   /* total data segment size                          */
unsigned cbase;   /* code segment base address                        */
unsigned cmod;    /* code base of the current module                  */
unsigned dmod;    /* data base of the current module                  */
unsigned goloc;   /* start (go) address                               */

/* -------------------------------------------------------------------------
 * Option / state flags
 * ---------------------------------------------------------------------- */
unsigned lgo;             /* non-zero: produce .LGO output            */
unsigned smallC;          /* non-zero: emit Small-C _main preamble    */
unsigned monitor;         /* non-zero: print verbose progress         */
unsigned instr;           /* first instruction planted at output base */
unsigned addr;            /* absolute start address                   */
unsigned ref;             /* current code relocation reference        */
unsigned dref;            /* current data relocation reference        */
unsigned big;             /* non-zero: -B (big program) flag set      */
unsigned xrplus;          /* offset value for the next ext-ref        */
unsigned xrpflag = XRPLUS;/* the XRPLUS sentinel constant             */
unsigned inblock;         /* block number of next library member      */
unsigned inbyte;          /* byte offset within that block            */
unsigned csflag;          /* non-zero: currently in code segment      */
unsigned dsflag;          /* non-zero: currently in data segment      */
unsigned oflag;           /* non-zero: -O flag was explicitly given   */

/* -------------------------------------------------------------------------
 * File descriptors
 * ---------------------------------------------------------------------- */
int ndxfd;      /* library index file (.NDX)                                */
int csfd;       /* code-segment overflow file (.O$); 0 = not yet opened     */
int crfd;       /* code relocation reference file (.R$)                     */
int dsfd;       /* data-segment file (.D$); 0 = not yet opened              */
int drfd;       /* data relocation reference file (.DR$)                    */
int ctfd;       /* code relocation table file (.CT$)                        */
int dtfd;       /* data relocation table file (.DT$)                        */
int outfd;      /* final output file (.COM or .LGO)                         */

/* -------------------------------------------------------------------------
 * In-memory buffers
 * ---------------------------------------------------------------------- */
char *buffer;           /* in-memory code image - allocated at runtime      */
int  memsize;           /* actual runtime size of buffer in bytes           */
char swap99[2];         /* TMS9900 byte-swap temporary                      */

/* -------------------------------------------------------------------------
 * Page map: parallel arrays (no structs - Small-C/K&R compatible).
 * Each entry records the code-segment-relative span of one module and
 * which page it lives on.  Absolute addresses are computed in
 * emit_pagemap() by adding cbase.
 * ---------------------------------------------------------------------- */
unsigned pmstart[MAXPMODS];   /* absolute start address of page module */
unsigned pmend[MAXPMODS];     /* absolute end address (exclusive)      */
unsigned pmcmod[MAXPMODS];    /* code buffer offset (for emit_page_block) */
unsigned pmmin_addr;          /* min PREL value seen in current module */
unsigned cloc_at_ename;        /* cloc saved at ENAME - used for cmod at PSIZE */
unsigned pmmin_global;         /* min PREL value seen across all modules */
unsigned first_module;         /* YES while loading first module for pmmin_global */
unsigned pmmin_valid;         /* non-zero if pmmin_addr is set         */
unsigned pm_modidx;           /* npmods index of current paged module  */
unsigned pm_modsize;          /* size of current paged module          */
int            pmpage[MAXPMODS];    /* page number for this module (0-15)   */
int            npmods;              /* number of page-map entries recorded  */
int            curpage;             /* current page assignment (-P switch)  */
int            pagemode;            /* non-zero: at least one -P flag used  */
int            page1data;           /* non-zero: page1 module has data segment */
int            pages_auto;          /* non-zero: -PAGES auto-assign mode    */
int            pages_min;           /* -PAGES range start                   */
int            pages_max;           /* -PAGES range end                     */
int            pages_next;          /* next page to auto-assign             */

/* v3.7: trampoline arrays removed - see emit_trampolines() deletion */

/* -------------------------------------------------------------------------
 * External declarations (defined in rel99 support modules)
 * ---------------------------------------------------------------------- */
extern int  okay();
extern void putls();
extern int  avail();
extern char *malloc();
extern void poll();
extern int  getarg();
extern int  extend();
extern void error();
extern void error2();
extern int  getrel();
extern void ifilelbuf();
extern void delete();
extern void itox();
extern void itou();
extern int  xtoi();
extern int  utoi();
extern void puts2();
extern void puts3();
extern int  ferror2();
extern int  getint();
extern void putint();
extern int  get16int();
extern void put16int();

/* Forward declaration for emit_page_block_data (defined after emit_chain_blocks) */
unsigned emit_page_block_data();

/* ==========================================================================
 * pageof  -  Return the page number (0 or 1) of absolute address 'addr'.
 *            Returns -1 if the address is not covered by the page map.
 *
 *  Used by resolve() to detect cross-page xr->ep pairs and check
 *  for virtual segment collisions between differently-paged modules.
 * ======================================================================== */
int pageof(addr)
	unsigned addr;
{
	int i;

	for (i = 0; i < npmods; i++) {
		if (addr >= pmstart[i] && addr < pmend[i])
			return (pmpage[i]);
	}
	return (-1);
}

/* ==========================================================================
 * patch_pagemap  -  Patch the ?PAGEMAP symbol with the address of the
 *                   page-map table that emit_pagemap() will write.
 *
 *  The table is appended immediately after the data segment, so its
 *  absolute address is cbase + csize + dsize.
 *
 *  This is structurally identical to freemem(): search the ep table for
 *  the symbol, then write the target value into the code or data image
 *  at the location recorded by the EPOINT record.
 *
 *  The user declares ?PAGEMAP as an external word in their program and the
 *  linker fills it in, just like ?MEMRY.
 * ======================================================================== */
patch_pagemap()
{
	char *pmval;
	int   et, cspg, dspg;

	epnext = getint(epfirst);
	while (strcmp("?PAGEMAP", epnext + SYM) > 0)
		epnext = getint(epnext);
	if (strcmp("?PAGEMAP", epnext + SYM) < 0)
		return;                         /* symbol not declared - skip         */

	et    = *(epnext + FLG);
	ep    = getint(epnext + VAL);
	pmval = cbase + csize + dsize;      /* table starts here in load image    */

	if (et == PREL) {
		if (ep < cdisk) {
			putint(buffer + getint(epnext + VAL), pmval);
		} else {
			cspg = ctell(csfd);
			xrseek(ep - cdisk);
			write(csfd, &pmval, 2);
			lseek(csfd, cspg, SEEK_SET);
		}
	}
	if (et == DREL) {
		dspg = ctell(dsfd);
		dxrseek(ep);
		write(dsfd, &pmval, 2);
		lseek(dsfd, dspg, SEEK_SET);
	}
}

/* ==========================================================================
 * merge_pagemap  -  Merge overlapping same-page entries in the page map.
 *
 *  Called by emit_pagemap() before emitting.  Adjacent or overlapping
 *  entries on the same page are collapsed into one span.  Dead entries
 *  are marked by setting pmend[i] == pmstart[i] (zero-length span).
 *
 *  This reduces the number of 6116 register writes the loader must do
 *  and keeps the page map table compact.
 * ======================================================================== */
merge_pagemap()
{
	int i, j, merged;

	do {
		merged = 0;
		for (i = 0; i < npmods - 1; i++) {
			if (pmend[i] == pmstart[i]) continue;   /* already dead   */
			for (j = i + 1; j < npmods; j++) {
				if (pmend[j] == pmstart[j]) continue; /* already dead */
				if (pmpage[i] != pmpage[j])  continue; /* different pages */
				/* overlapping or adjacent — merge j into i             */
				if (pmstart[j] <= pmend[i] && pmend[j] >= pmstart[i]) {
					if (pmstart[j] < pmstart[i]) pmstart[i] = pmstart[j];
					if (pmend[j]   > pmend[i])   pmend[i]   = pmend[j];
					pmend[j] = pmstart[j];  /* mark j as dead           */
					merged = 1;
				}
			}
		}
	} while (merged);
}

/* ==========================================================================
 * emit_pagemap  -  Write the page-map table prefix to the output file.
 *
 *  Must be called first in phase2(), before any code or data is emitted.
 *  Also calls merge_pagemap() to collapse duplicate same-page entries.
 *
 *  Table layout (6 bytes per entry, 3 x TMS9900 big-endian words):
 *    word 0 : start  absolute start address of block (inclusive)
 *    word 1 : end    absolute end address (exclusive)
 *    word 2 : page   physical page number (0-15)
 *
 *  Word-aligned 6-byte entries mean the loader can use plain MOV *R4+
 *  to walk the table without byte-address arithmetic.
 *
 *  Always terminated by three 0xFFFF sentinel words (6 bytes).
 *  Non-paged programs emit the sentinel only — loader treats this as
 *  "no page setup needed, load flat to TPA".
 *
 *  File layout written by this function + emit_page_blocks():
 *    [M x 6-byte pagemap entries]   0 entries if non-paged
 *    [FFFF FFFF FFFF sentinel]      always present
 *    [size:word][page 0 code+data]  one block per unique page
 *    [size:word][page 1 code+data]  (non-paged: one block, all code)
 *    ...
 *
 *  The loader (LOADERCODE in shell.asm) reads this layout as follows:
 *    1. Walk pagemap entries, program 6116 for each segment
 *    2. Read sentinel (skip 6 bytes)
 *    3. For each [size][block]: enable PSEL for that page, RDSEQ size
 *       bytes direct to virtual address, disable PSEL, next block
 *    4. Jump to program entry point
 * ======================================================================== */
emit_pagemap()
{
	int            i;
	unsigned abs_s, abs_e, abs_pg, term, zero;
	char           str[6];

	term = PMTERM;
	zero = 0;

	/* Merge overlapping same-page entries before emitting               */
	if (pagemode)
		merge_pagemap();

	/* File format:
	 *   FFFF FFFF FFFF   opening sentinel (= link99 format marker)
	 *   start end page   pagemap entry (repeat, 0 to 16 entries)
	 *   FFFF FFFF FFFF   closing sentinel (= end of pagemap)
	 *   [size][data]     one block per entry in order
	 *
	 * Flat (non-paged): one zero entry between sentinels.
	 * Raw binary: no sentinel at offset 0 - load from LADDR.
	 */

	/* Opening sentinel                                                   */
	write99(outfd, &term, 2);
	write99(outfd, &term, 2);
	write99(outfd, &term, 2);

	if (!pagemode) {
		/* Flat: entry with actual load address, page 0               */
		abs_s  = cbase;              /* 0x1000                        */
		abs_e  = cbase + csize + dsize;
		abs_pg = 0;                  /* page 0 = common memory        */
		write99(outfd, &abs_s,  2);
		write99(outfd, &abs_e,  2);
		write99(outfd, &abs_pg, 2);
		if (monitor) {
			puts("\n\tFLAT LOAD");
			itox(abs_s, str, 5); puts2(str, "-");
			itox(abs_e, str, 5); puts2(str, " pg 0\n");
		}
	} else {
		puts("\n\tPAGE MAP");
		for (i = 0; i < npmods; i++) {
			if (pmend[i] == pmstart[i]) continue;
			abs_s  = pmstart[i];
			abs_e  = pmend[i];
			abs_pg = (unsigned)pmpage[i];
			write99(outfd, &abs_s,  2);
			write99(outfd, &abs_e,  2);
			write99(outfd, &abs_pg, 2);
			if (monitor) {
				itox(abs_s, str, 5);  puts2(str, "-");
				itox(abs_e, str, 5);  puts2(str, " pg ");
				itox(abs_pg, str, 3); puts2(str, "\n");
			}
		}
	}

	/* Closing sentinel                                                   */
	write99(outfd, &term, 2);
	write99(outfd, &term, 2);
	write99(outfd, &term, 2);
}

/* ==========================================================================
 * in_page  -  Return YES if code-segment offset 'off' belongs to page 'pg'.
 *
 *  Scans the merged page-map arrays.  A byte at offset 'off' (relative to
 *  cbase) belongs to page 'pg' if it falls within any pmstart/pmend span
 *  whose pmpage matches 'pg'.
 *
 *  Used by emit_page_block() to select which bytes to emit for each page.
 * ======================================================================== */
int in_page(off, pg)
	unsigned off; int pg;
{
	int i;
	for (i = 0; i < npmods; i++) {
		if (pmend[i] == pmstart[i]) continue;   /* dead entry              */
		if (pmpage[i] != pg)        continue;   /* wrong page              */
		/* Use pmcmod (buffer offset) not pmstart (absolute addr)          */
		if (off >= pmcmod[i] && off < pmcmod[i] + (pmend[i] - pmstart[i]))
			return (YES);
	}
	return (NO);
}

/* ==========================================================================
 * emit_raw_block  -  Emit flat non-paged code as raw bytes (no size word).
 *
 *  For flat programs the loader uses the raw binary path - no header,
 *  no size word, just raw bytes loaded directly to LADDR.
 * ======================================================================== */
emit_raw_block(cloc_p, ref_p)
	unsigned *cloc_p;
	unsigned *ref_p;
{
	unsigned tp;
	char     rtbuf[CRELSIZE];

	while (*cloc_p < csize) {
		if (*cloc_p != *ref_p) {
			if (*cloc_p < cdisk)
				field = *(buffer + *cloc_p);
			else
				read(csfd, &field, 1);
			write(outfd, &field, 1);
			(*cloc_p)++;
			continue;
		}
		if (*cloc_p < cdisk)
			tp = get16int(buffer + *cloc_p);
		else
			read(csfd, &tp, 2);
		ctseek(tp);
		read(ctfd, rtbuf, CRELSIZE);
		field = get16int(rtbuf + 2);
		if (rtbuf[4] == PREL) {
		/* AORG modules: PREL values are absolute (>= PAGE_SEG), don't add cbase */
		if (field < PAGE_SEG)
			field += xrplus + cbase;
		else
			field += xrplus;
	}
		if (rtbuf[4] == DREL) field += xrplus + dbase;
		xrplus = 0;
		write99(outfd, &field, 2);
		*cloc_p += 2;
		*ref_p   = readref();
	}
}

/* ==========================================================================
 * emit_page_block  -  Emit one page's worth of code+data to the output file.
 *
 *  Writes a 2-byte size word (TMS9900 big-endian) followed by exactly
 *  'size' bytes of resolved code belonging to page 'pg'.
 *
 *  For non-paged output (pagemode == 0), pg is ignored and all code+data
 *  is emitted as a single block.
 *
 *  The size word allows the loader to know exactly how many bytes to read
 *  from disk for this page without needing to re-parse the page map.
 *
 *  Byte selection:
 *    - Non-relocatable bytes are copied straight from the buffer or .O$ file.
 *    - Relocatable words are resolved using the .CT$ table (same logic as
 *      the original flat emit loop in phase2).
 *    - Bytes NOT belonging to page 'pg' are skipped (not written).
 *      The loader loads each page to the same virtual base address (TPA),
 *      so gaps between modules on the same page need not be preserved —
 *      but intra-module gaps (SETLC padding) are part of the module and
 *      are included in its pmstart/pmend span, so they ARE emitted.
 *
 *  crelptr and csfd must be reset to the start of the code image before
 *  the first call.  Each successive call continues from where the previous
 *  one left off (sequential scan through the code image).
 *
 *  Entry:  pg       = page number to emit (0-15); ignored if !pagemode
 *          cloc_p   = pointer to the current code location counter
 *          ref_p    = pointer to the current relocation reference
 *  Exit:   *cloc_p and *ref_p updated for the next call
 *          returns  number of bytes written (the size field value)
 * ======================================================================== */
unsigned emit_page_block(pg, cloc_p, ref_p)
	int             pg;
	unsigned *cloc_p;
	unsigned *ref_p;
{
	unsigned  sz, szsave, tp;
	long            szpos;
	char            rtbuf[CRELSIZE];
	char            str[6];

	/* --- Reserve space for the size word; fill it in after emit ---    */
	sz    = 0;
	szpos = lseek(outfd, 0, SEEK_CUR);
	write99(outfd, &sz, 2);             /* placeholder — patched below  */

	szsave = 0;

	while (*cloc_p < csize) {

		/* Skip bytes not belonging to this page (paged mode only)      */
		if (pagemode && !in_page(*cloc_p, pg)) {
			/* Advance past this byte without emitting it               */
			if (*cloc_p < cdisk)
				(*cloc_p)++;
			else {
				read(csfd, &field, 1);
				(*cloc_p)++;
			}
			/* If this was a relocatable word, skip second byte too     */
			if (*cloc_p - 1 == *ref_p) {
				if (*cloc_p < cdisk)
					(*cloc_p)++;
				else {
					read(csfd, &tp, 2);
					(*cloc_p)++;
				}
				*ref_p = readref();
			}
			continue;
		}

		if (*cloc_p != *ref_p) {
			/* Non-relocatable byte: copy as-is                         */
			if (*cloc_p < cdisk)
				field = *(buffer + *cloc_p);
			else
				read(csfd, &field, 1);
			write(outfd, &field, 1);
			szsave++;
			(*cloc_p)++;
			continue;
		}

		/* Relocatable word: read CT$ entry and apply segment base      */
		if (*cloc_p < cdisk)
			tp = get16int(buffer + *cloc_p);
		else
			read(csfd, &tp, 2);

		ctseek(tp);
		read(ctfd, rtbuf, CRELSIZE);
		field = get16int(rtbuf + 2);

		if (rtbuf[4] == PREL) {
		/* AORG modules: PREL values are absolute (>= PAGE_SEG), don't add cbase */
		if (field < PAGE_SEG)
			field += xrplus + cbase;
		else
			field += xrplus;
	}
		if (rtbuf[4] == DREL) field += xrplus + dbase;
		xrplus = 0;

		write99(outfd, &field, 2);
		szsave += 2;
		*cloc_p += 2;                   /* word consumed two bytes      */
		*ref_p   = readref();
	}

	/* --- Patch size word now that we know the actual byte count ---    */
	sz = szsave;
	lseek(outfd, szpos, SEEK_SET);
	write99(outfd, &sz, 2);
	lseek(outfd, 0, SEEK_END);

	if (monitor) {
		putls("\n\tPAGE ");
		itox(pg, str, 3);    putls(str);
		putls(" BLOCK ");
		itox(sz, str, 6);    puts2(str, " bytes");
	}

	return (sz);
}

/* ==========================================================================
 * main
 * ======================================================================== */
int main(argc, argv)
	int argc; char **argv;
{
	putls("----------------------------------------------------\n");
	putls("TMS9900 Relocatable Object Linker  Version 3.9.10\n");
	putls("Original CP/M version: Alexander Cameron, January 1985\n");
	putls("MSDOS/PC port:         Alexander Cameron, May 2010 - July 2019\n");
	putls("Cleaner command syntax: Version 3.9.9\n");
	putls("----------------------------------------------------\n");

	getsw(argc, argv);      /* parse command-line switches  */
	getmem();               /* initialise memory buffers    */
	phase1(argc, argv);     /* pass 1: load and link        */
	if (!okay())
		error("\nQuitting with unresolved symbols.");
	phase2();               /* pass 2: write output file    */
}

/* ==========================================================================
 * freemem  -  Patch the ?MEMRY symbol with the free-memory pointer value.
 * ======================================================================== */
freemem()
{
	char *fmval;
	int   et, cspg, dspg;

	epnext = getint(epfirst);
	while (strcmp("?MEMRY", epnext + SYM) > 0)
		epnext = getint(epnext);
	if (strcmp("?MEMRY", epnext + SYM) < 0)
		return;                         /* symbol not present - nothing to do */

	et    = *(epnext + FLG);
	ep    = getint(epnext + VAL);
	fmval = cbase + csize + dsize;      /* free-memory pointer value          */

	if (et == PREL) {
		if (ep < cdisk) {
			/* Symbol is in the in-memory code buffer */
			putint(buffer + getint(epnext + VAL), fmval);
		} else {
			/* Symbol is in the code overflow file (.O$)
			 * FIX: save position with ctell(), then restore with SEEK_SET
			 * after the targeted write so callers' file position is intact.
			 */
			cspg = ctell(csfd);
			xrseek(ep - cdisk);
			write(csfd, &fmval, 2);
			lseek(csfd, cspg, SEEK_SET);
		}
	}

	if (et == DREL) {
		/* Symbol is in the data segment file (.D$) */
		dspg = ctell(dsfd);
		dxrseek(ep);
		write(dsfd, &fmval, 2);
		lseek(dsfd, dspg, SEEK_SET);
	}
}

/* ==========================================================================
 * getmem  -  Initialise the in-memory code buffer and symbol table.
 *            The buffer size is determined at runtime from avail() so the
 *            linker uses all available memory automatically regardless of
 *            the host system size.
 * ======================================================================== */
getmem()
{
	int max;
	char sz[9];

	/* avail() returns CP/M-era TPA size (~40KB). For the Windows/modern
	 * port use a large fixed buffer so the linker can handle big programs. */
	max = 524288;   /* 512KB - sufficient for large C compiler links */
	max -= STACK + AUXBUF + AUXBUF + (MAXOPEN * OHDOPEN);

	if (max < SSZ * 4)
		error("- Not enough memory to run linker\n");

	buffer  = malloc(max);          /* allocate all usable memory at runtime  */
	if (!buffer)
		error("- Memory allocation failed\n");
	memsize = max;                  /* remember actual size for snext calc    */

	bpnext = buffer;
	snext  = buffer + memsize - SSZ; /* symbol table grows downward           */
	sfree  = 0;

	itou(memsize, sz, 8);
	puts2(sz, " Byte Buffer\n");

	newtbl(&epfirst);   /* insert sentinel entry-point records              */
	newtbl(&xrfirst);   /* insert sentinel external-reference records       */
}

/* ==========================================================================
 * getname  -  Read the next module PNAME record from the current REL file.
 * ======================================================================== */
getname()
{
	if (getrel() == PNAME) {
		strcpy(modname, symbol);
		return (YES);
	}
	if (item == EFILE)
		return (NO);
	error2(infn, " - Corrupted\n");
	return (NO);
}

/* ==========================================================================
 * getndx  -  Read the next library-member location from the index file.
 * ======================================================================== */
getndx()
{
	if (read(ndxfd, &inblock, 2) != 2 ||
	    read(ndxfd, &inbyte,  2) != 2)
		error2("\n- Error Reading ", infn);
}

/* ==========================================================================
 * getsw  -  Parse command-line switches.
 * ======================================================================== */
void getsw(argc, argv)
	int argc; char **argv;
{
	char  arg[NAMESIZE];
	char *hexptr;                   /* points past any 0x/0X prefix         */
	int   argnbr, b, len;

	argnbr  = 0;
	dbase   = 0;
	monitor = NO;

	while (getarg(++argnbr, arg, NAMESIZE, argc, argv) != EOF) {
		if (arg[0] != '-')
			continue;               /* skip file-name arguments               */
		if (toupper(arg[1]) == 'G') {           /* load-and-go absolute base  */
			lgo = YES;
			hexptr = (arg[2]=='0' && toupper(arg[3])=='X') ? arg+4 : arg+2;
			len = xtoi(hexptr, &b);
			if (len >= 0 && !hexptr[len])
				cbase = b;
			else
				usage();
		} else if (toupper(arg[1]) == 'O') {   /* origin: set cbase           */
			hexptr = (arg[2]=='0' && toupper(arg[3])=='X') ? arg+4 : arg+2;
			len = xtoi(hexptr, &b);
			if (len >= 0 && !hexptr[len]) {
				cbase = b;
				oflag = YES;
			} else
				usage();
		} else if (toupper(arg[1]) == 'D') {    /* data segment base          */
			hexptr = (arg[2]=='0' && toupper(arg[3])=='X') ? arg+4 : arg+2;
			len = xtoi(hexptr, &b);
			if (len >= 0 && !hexptr[len])
				dbase = b;
			else
				usage();
		} else if (toupper(arg[1]) == 'B')
			big = YES;
		else if (toupper(arg[1]) == 'M')
			monitor = YES;
		else if (toupper(arg[1]) == 'S')
			smallC = YES;
		else if (toupper(arg[1]) == 'P') {      /* page assignment            */
			if (toupper(arg[2]) == 'A') {          /* -PAGES#-# auto-assign      */
				pagemode = YES;
			} else {                               /* -P# explicit page          */
				/* Only set pagemode here; curpage is set sequentially
				 * in the phase1 loop so each module gets the right page. */
				int pg = arg[2] - '0';
				if (pg < 0 || pg > 15)
					usage();
				pagemode = YES;
			}
		} else
			usage();
	}
}

/* ==========================================================================
 * isunres  -  Return YES if 'symbol' is an unresolved external reference.
 *             On return of YES, xrnext points at the matching xr entry.
 * ======================================================================== */
isunres()
{
	int i;

	xrnext = getint(xrfirst);
	while (xrnext) {
		if ((i = strcmp(symbol, xrnext + SYM)) < 0)
			return (NO);
		if (i == 0) {
			if (!(*(xrnext + FLG) & FLGEXT))
				return (YES);
			else
				return (NO);
		}
		xrnext = getint(xrnext);
	}
	return (NO);
}

/* ==========================================================================
 * link  -  Link external references to entry points.
 * ======================================================================== */
link()
{
	int cspg, dspg;

	if (monitor)
		putls("\nLinking.....");

	/* Save current overflow-file positions so resolve() can seek freely     */
	cspg = csfd ? ctell(csfd) : 0;
	dspg = dsfd ? ctell(dsfd) : 0;

	xrnext = getint(xrprev = xrfirst);
	epnext = getint(epfirst);

	while (YES) {
		if (strcmp(xrnext + SYM, epnext + SYM) > 0) {  /* xr > ep          */
			epnext = getint(epnext);
			continue;
		}
		if (strcmp(xrnext + SYM, epnext + SYM) < 0) {  /* xr < ep          */
			xrnext = getint(xrprev = xrnext);
			continue;
		}
		/* xr == ep */
		if (*(xrnext + SYM) != HIGH) {
			if (pass == 2)
				*(xrnext + FLG) &= HIGH;    /* clear data flags on pass 2     */

			if (pass == 1 && *(epnext + FLG) != PREL) {
				/* External symbol has a data entry point - flag for later    */
				*(xrnext + FLG) |= FLGEXT;
				xrnext = getint(xrprev = xrnext);
				continue;
			}

			resolve();
			putint(xrprev + NXT, getint(xrnext)); /* delink from xr chain    */
			putint(xrnext + NXT, sfree);          /* add to free list        */
			sfree  = xrnext;
			xrnext = getint(xrprev);
			continue;
		}
		break;
	}

	/* Restore overflow-file positions */
	if (csfd) lseek(csfd, cspg, SEEK_SET);
	if (dsfd) lseek(dsfd, dspg, SEEK_SET);
}

/* ==========================================================================
 * load  -  Load one module from the current REL file into the code buffer
 *          (or into the overflow file once in-memory space is exhausted).
 * ======================================================================== */
load()
{
	char           str[9];
	char           rtbuf[CRELSIZE]; /* scratch for relocation table entries   */
	unsigned int   doffloc, coffloc, creloff, dreloff;
	int            gval;            /* return value from getrel()             */
	unsigned base;            /* temp for page map base calculation     */

	if (monitor)
		puts2("\nLoading module: ", modname);

	doffloc = coffloc = -1;
	epprev  = epfirst;
	xrprev  = xrfirst;
	cloc_at_ename = cloc;        /* v3.9.7: initialise here in case ENAME was pre-consumed
	                              * by search()'s while(getrel()==ENAME) loop before load()
	                              * was called. Without this, cmod stays at the previous
	                              * module's load base, miscalculating all EPOINT values
	                              * for library modules. The ENAME case below will update
	                              * this again if ENAME is encountered inside load().     */
	cloc_at_ename = cloc;        /* default: set now in case ENAME was pre-consumed */

	do {
		poll(YES);
		gval = getrel();

		if (monitor && gval >= ENAME) {    /* skip noise: ABS/PREL/DREL/CREL */
			putls(" g=");
			itox(gval, str, 2);
			putls(str);
		}

		switch (gval) {

		/* ---- Absolute (non-relocatable) byte ----------------------------- */
		case ABS:
			if (csflag) {
				if (csfd)
					write(csfd, &field, 1);     /* spill to overflow file     */
				else
					*bpnext++ = field;          /* store in memory buffer     */

				if (coffloc == cloc) {
					/* Write XPOFF marker into code relocation table          */
					if (crelptr >= 65535U)
						error("- Code relocation table full (65535 entries)\n");
					put16int(rtbuf,     xrpflag);
					put16int(rtbuf + 2, cloc);
					rtbuf[4] = 0;           /* pad to CRELSIZE               */
					ctseek(crelptr);
					write(ctfd, rtbuf, CRELSIZE);
					crelptr++;
				}
				++cloc;
			}
			if (dsflag) {
				write(dsfd, &field, 1);
				if (doffloc == dloc) {
					/* Write XPOFF marker into data relocation table          */
					if (drelptr >= 65535U)
						error("- Data relocation table full (65535 entries)\n");
					put16int(rtbuf,     xrpflag);
					put16int(rtbuf + 2, dloc);
					rtbuf[4] = 0;           /* pad to CRELSIZE               */
					dtseek(drelptr);
					write(dtfd, rtbuf, CRELSIZE);
					drelptr++;
				}
				++dloc;
			}
			break;

		/* ---- Program-relative or data-relative word ---------------------- */
		case DREL:
		case PREL:
			/* Add the module's segment base to relocatable references.
			 * field=0 means offset 0 within module - still needs cmod.    */
			/* AORG modules: PREL values are already absolute (>= PAGE_SEG).
			 * Don't add cmod - it would double-relocate them.           */
			if (item == PREL && field < PAGE_SEG) field += cmod;
			if (item == PREL && field >= PAGE_SEG) ; /* already absolute */
			if (item == DREL) field += dmod;

			/* Track minimum PREL value to detect AORG base address.
			 * PSIZE fires before PREL in the file; EPROG fixes up pmstart. */
			if (monitor && item == PREL && type != 3) {
				putls("  PREL seen: field=");
				itox(field, str, 4);
				putls(str);
				putls(" pagemode=");
				putls(pagemode ? "YES" : "NO");
				putls(" pmmin_valid=");
				putls(pmmin_valid ? "YES" : "NO");
				putls("\n");
			}
			if (item == PREL && type != 3 && pagemode && field != 0) {
				if (!pmmin_valid || field < pmmin_addr) {
					pmmin_addr  = field;
					pmmin_valid = YES;
				}
			}
			/* Track global min PREL from first module only (for AORG cbase detection) */
			if (item == PREL && type != 3 && !pagemode && field != 0 && first_module && field < pmmin_global)
				pmmin_global = field;

			if (csflag) {
				/* Build 5-byte entry: [loc:2][chainval:2][type:1]           */
				if (crelptr >= 65535U)
					error("- Code relocation table full (65535 entries)\n");
				put16int(rtbuf,     cloc);
				put16int(rtbuf + 2, field);
				rtbuf[4] = item;
				ctseek(crelptr);
				write(ctfd, rtbuf, CRELSIZE);

				/* Store entry INDEX (not byte offset) in code image         */
				creloff = crelptr;
				if (csfd)
					write(csfd, &creloff, 2);   /* spill to overflow file   */
				else {
					put16int(bpnext, creloff);  /* store in memory buffer   */
					bpnext += 2;
				}
				cloc    += 2;
				crelptr += 1;               /* index, not byte offset       */
			}

			if (dsflag) {
				/* Same 5-byte layout for the data relocation table          */
				if (drelptr >= 65535U)
					error("- Data relocation table full (65535 entries)\n");
				put16int(rtbuf,     dloc);
				put16int(rtbuf + 2, field);
				rtbuf[4] = item;
				dtseek(drelptr);
				write(dtfd, rtbuf, CRELSIZE);

				dreloff = drelptr;
				write(dsfd, &dreloff, 2);

				dloc    += 2;
				drelptr += 1;               /* index, not byte offset       */
			}
			break;

		default:
			if (monitor) {
				putls("  unknown getrel()=");
				itox(item, str, 2);
				putls(str);
				putls("\n");
			}
			error("- Unsupported Link Item\n");
			break;

		case ERR:
			error("- Corrupt Module\n");
			break;

		case EPROG:
			first_module = NO;  /* no longer in first module */
			/* Synthesise pagemap entry for pure AORG modules with no PSIZE.
			 * These have no PSIZE record so npmods==0 at EPROG time.     */
			if (pagemode && pmmin_valid && npmods == 0 && pmmin_addr > PAGE_SEG) {
				base = pmmin_addr & ~(PAGE_SEG-1);
				pmstart[npmods] = base;
				pmend[npmods]   = base + cloc;
				pmcmod[npmods]  = 0;
				pmpage[npmods]  = curpage;
				pm_modidx = npmods;
				++npmods;
				if (monitor) {
					putls("  AORG synth: base=");
					itox(base, str, 5); putls(str);
					putls(" end=");
					itox(base+cloc, str, 5); putls(str);
					putls(" pg ");
					itox(curpage, str, 3); puts2(str, "\n");
				}
			}
			/* Fix up page map entry for AORG modules: PSIZE fires before
			 * PREL records so we stored a placeholder. Now we have seen
			 * all PREL values and can derive the correct AORG base.       */
			if (pagemode && pmmin_valid && pm_modidx < npmods
					&& pmmin_addr > PAGE_SEG) { /* AORG only: relocatable modules have small PREL values */
				base = pmmin_addr & ~(PAGE_SEG-1);
				if (base != pmstart[pm_modidx]) {
					pmstart[pm_modidx] = base;
					/* pm_modsize is the virtual span from the REL PSIZE record.
					 * For AORG modules with a BSS gap before the first instruction
					 * this includes the BSS in the virtual span but the linker's
					 * SETLC handler only zero-fills up to the SETLC target, so
					 * cloc ends at the real content end, not the virtual span end.
					 * Use cloc - pmcmod to get actual bytes deposited.            */
					pmend[pm_modidx]   = base + (cloc - pmcmod[pm_modidx]);
					if (monitor) {
						putls("  AORG fixup: base=");
						itox(base, str, 5);
						putls(str);
						putls(" end=");
						itox(pmend[pm_modidx], str, 5);
						putls(str);
						putls("\n");
					}
				}
			}
			if (type == PREL) {
				puts2("Start In ", modname);
				goloc = field + cmod;
			}
			break;

		case ENAME:
			/* Reset per-module PREL tracking at start of each module      */
			pmmin_valid = NO;
			pmmin_addr  = 0xFFFF;
			cloc_at_ename = cloc;  /* save cloc before SETLC records */
			break;  /* entry names handled during search/library phase       */

		case XCHAIN:
			newsym(&xrprev, xrfirst, "xr");
			break;

		case EPOINT:
			newsym(&epprev, epfirst, "ep");
			break;

		/* ---- Program-segment size record --------------------------------- */
		case PSIZE:
		do_psize:
			cmod = cloc_at_ename;  /* use cloc BEFORE SETLC records, not after */
			/* Store placeholder now; EPROG will fix up pmstart/pmend once
			 * we have seen the PREL values that tell us the AORG base.
			 * For non-AORG modules (cloc>0 after SETLC) cbase+cmod is fine. */
			if (npmods < MAXPMODS) {
				base = cbase + cmod;         /* placeholder (correct if SETLC) */
				pmstart[npmods] = base;
				pmend[npmods]   = base + field;
				pmcmod[npmods]  = cmod;      /* actual code buffer offset      */
				/* Auto-assign next available page if -PAGES active
				 * and no explicit -P# was given for this module.  */
				if (pages_auto && curpage == 0) {
					if (pages_next > pages_max) {
						putls("\n- Error: -PAGES range exhausted\n");
						exit(1);
					}
					int pages_needed;
					pmpage[npmods]  = pages_next;
					/* Virtual start = page * 4KB segment size                */
					base            = (unsigned)(pages_next << 12);
					pmstart[npmods] = base;
					pmend[npmods]   = base + field;
					pmcmod[npmods]  = cmod;
					/* Advance pages_next by number of 4KB segments needed    */
					pages_needed = (field + PAGE_SEG - 1) / PAGE_SEG;
					if (pages_needed < 1) pages_needed = 1;
					pages_next += pages_needed;
				} else {
					pmpage[npmods] = curpage;
				}
				pm_modidx  = npmods;         /* remember for EPROG fixup       */
				pm_modsize = field;
				++npmods;
			} else
				puts("\n- Warning: page map full, module not recorded\n");
			/* Reset per-module PREL tracking */
			pmmin_valid = NO;
			pmmin_addr  = 0xFFFF;
			/* Check 4KB segment alignment */
			if (pagemode && npmods > 0) {
				base = pmstart[npmods-1];
				if ((base >> 12) != ((base + field - 1) >> 12)) {
					putls("\n- Note: module ");
					putls(modname);
					putls(" spans more than one 4KB segment.\n");
				}
			}
			if (monitor) {
				putls("\n");
				itox(field, str, 8);
				putls(str);
				putls(" Code Bytes at ");
				itox(cloc, str, 6);
				putls(str);
				putls("' ");
				itox(cloc + cbase, str, 6);
				putls(str);
				puts2(" ", modname);
				putls("\n");
			}
			/* Open the overflow file if memory is running short             */
			if (!csfd && (big || (bpnext + field) > (snext - CUSHION))) {
				cdisk = cloc;
				csfd  = open(csfn, O_CREAT | O_TRUNC | O_RDWR | O_BINARY, 0644);
				if (csfd < 0)
					error2("- Cannot create overflow file ", csfn);
				if (monitor) {
					itox(cdisk, str, 8);
					puts2(str, " Overflow Point in code\n");
				}
			}
			break;

		/* ---- Data-segment size record ------------------------------------ */
		case DSIZE:
			dmod = dloc;
			/* Warn if a page1 module has data (strings/globals).
			 * Data in page1 is inaccessible from page0 functions
			 * (pointer dereferences after page switch read page0 memory).
			 * Move string literals and globals to page0 modules.          */
			if (curpage != 0 && field > 0) {
				page1data = YES;
				puts2("\n- Warning: page", curpage ? "1" : "0");
				puts2(" module has data segment: ", modname);
				putls("\n  String literals in non-page0 modules are");
				putls(" inaccessible from page0.\n");
				putls("  Move string literals and globals to page0 modules.\n");
			}
			if (monitor) {
				putls("\n");
				itox(field, str, 8);
				putls(str);
				putls(" Data Bytes at ");
				itox(dloc, str, 6);
				putls(str);
				putls("\" ");
				itox(dloc + dbase, str, 6);
				putls(str);
				puts2(" ", modname);
				putls("\n");
			}
			/* Open the data-segment file on first data encountered          */
			if (!dsfd) {
				dsfd = open(dsfn, O_RDWR | O_BINARY);
				if (dsfd < 0)
					dsfd = open(dsfn, O_CREAT | O_RDWR | O_BINARY, 0644);
				if (dsfd < 0)
					error2("- Cannot open data file ", dsfn);
			}
			break;

		/* ---- Set location counter ---------------------------------------- */
		case SETLC:
			if (type == DREL) {
				dsflag = TRUE;
				csflag = FALSE;
				field += dmod;
				while (dloc < field) {
					write(dsfd, "\0", 1);
					++dloc;
				}
			}
				if (type == PREL) {
				csflag = TRUE;
				dsflag = FALSE;
				/* AORG modules emit absolute virtual addresses in SETLC
				 * (BSS) records. Detect and strip the page base BEFORE
				 * adding cmod; otherwise cmod inflates the already-absolute
				 * address and produces a wildly oversized BSS region.
				 * The AORG base is derived from pmmin_addr (lowest PREL seen
				 * so far) if available, or from field itself.
				 * Applied unconditionally: AORG produces absolute addresses
				 * regardless of whether pagemode is active.                  */
				if (field > PAGE_SEG) {
					unsigned aorg_base;
					if (pmmin_valid)
						aorg_base = (unsigned)(pmmin_addr & ~((unsigned)(PAGE_SEG - 1)));
					else
						aorg_base = (unsigned)(field    & ~((unsigned)(PAGE_SEG - 1)));
					field = (unsigned)(field - aorg_base);
				}
				field += cmod;
				while (cloc < field) {
					if (csfd)
						write(csfd, "\0", 1);
					else
						*bpnext++ = 0;
					++cloc;
				}
			}
			break;

		/* ---- External-reference-plus-offset record ----------------------- */
		case XPOFF:
			if (csflag) {
				if (crelptr >= 65535U)
					error("- Code relocation table full (65535 entries)\n");
				put16int(rtbuf,     xrpflag);
				put16int(rtbuf + 2, field);
				rtbuf[4] = 0;           /* pad to CRELSIZE                   */
				ctseek(crelptr);
				write(ctfd, rtbuf, CRELSIZE);
				crelptr++;
				coffloc  = cloc;
			}
			if (dsflag) {
				if (drelptr >= 65535U)
					error("- Data relocation table full (65535 entries)\n");
				put16int(rtbuf,     xrpflag);
				put16int(rtbuf + 2, field);
				rtbuf[4] = 0;           /* pad to CRELSIZE                   */
				dtseek(drelptr);
				write(dtfd, rtbuf, CRELSIZE);
				drelptr++;
				doffloc  = dloc;
			}
			break;
		}
	} while (item != EPROG);

#ifdef DEBUG2
	{
		unsigned char *bp, *rp;
		int i;

		puts("Outputting external entry points:");
		ep = getint(epfirst);
		while (*(ep + SYM) != HIGH) {
			printf("%s %08X\n", (ep + SYM), getint(ep + VAL));
			ep = getint(ep);
		}

		puts("Outputting code buffer:");
		bp = buffer;
		printf("%04X  ", 0);
		for (i = 0; i < cloc; i++) {
			printf("%02X ", *bp++);
			if ((i + 1) % 16 == 0)
				printf("\n%04X  ", i + 1);
		}

		puts("\nOutputting creltble (disk .CT$ file, offsets only):");
		printf("crelptr offset = %u\n", crelptr);
	}
#endif
}

/* ==========================================================================
 * newfn  -  Build a new filename by replacing the extension of 'sour'.
 * ======================================================================== */
newfn(dest, sour, ext)
	char *dest; char *sour; char *ext;
{
	/* Strip drive designator unless building an index filename */
	if (sour[1] == ':' && strcmp(ext, NDXEXT))
		sour += 2;
	while (*sour && *sour != '.')
		*dest++ = *sour++;
	strcpy(dest, ext);
}

/* ==========================================================================
 * newsym  -  Insert a new symbol into the sorted symbol table.
 * ======================================================================== */
newsym(prev, first, ts)
	char **prev; char *first; char *ts;
{
	char *cp, *newent;
#ifdef DEBUG
	char at[9];
#endif
	if ((newent = sfree))
		sfree = getint(sfree);  /* recycle a previously freed entry          */
	else {
		newent  = snext;
		snext  -= SSZ;
		if (snext < bpnext)
			error("- Must Specify -B Switch\n");
	}

	/* Tolerate M80-style out-of-order symbols (fix 29)                      */
	if (strcmp(symbol, *prev + SYM) < 0)
		*prev = first;

	cp = *prev;
	while (strcmp(symbol, cp + SYM) >= 0) {
		*prev = cp;
		cp = getint(cp + NXT);
	}

	putint(newent,       cp + NXT);     /* link new entry ahead               */
	putint(*prev,        newent);       /* link previous entry to here        */
	*prev = newent;

	if (type == PREL) field += cmod;
	if (type == DREL) field += dmod;

	putint(newent + VAL, field);
	strcpy(newent + SYM, symbol);
	*(newent + FLG) = type;

#ifdef DEBUG
	if (monitor) {
		itox(getint(newent + VAL), at, MAXSYM);
		putls(at);
		putls(" -t-");
		putls(ts);
		putls(" -s-");
		puts(symbol);
	}
#endif
}

/* ==========================================================================
 * newtbl  -  Initialise a symbol table with low and high sentinel entries.
 * ======================================================================== */
newtbl(low)
	char **low;
{
	*low = snext;
	strcpy(snext + SYM, "");            /* low sentinel (empty string)        */
	putint(snext + NXT, snext - SSZ);   /* link to high sentinel              */
	snext -= SSZ;
	strcpy(snext + SYM, high);          /* high sentinel (0x7f)               */
	putint(snext + NXT, 0);             /* end of chain                       */
	snext -= SSZ;
}

/* ==========================================================================
 * nxtmod  -  Advance to the next library member and read its name.
 * ======================================================================== */
nxtmod()
{
	getndx();
	seek();
	return (getname());
}

/* ==========================================================================
 * okay  -  Scan for unresolved or redundant symbols; return YES if clean.
 * ======================================================================== */
okay()
{
	int   err;
	char *eplast;

	err    = 0;
	eplast = 0;
	xrnext = getint(xrfirst);
	epnext = getint(epfirst);

	while (YES) {
		poll(YES);
		if (strcmp(xrnext + SYM, epnext + SYM) > 0) {  /* ext > ent        */
			if (epnext == eplast) {
				puts2("\n-  Redundant: ", xrnext + SYM);
				err = YES;
			}
			eplast = epnext;
			epnext = getint(epnext);
			continue;
		}
		if (strcmp(xrnext + SYM, epnext + SYM) < 0) {  /* ext < ent        */
			puts2("\n- Unresolved: ", xrnext + SYM);
			err = YES;
			xrnext = getint(xrnext);
			continue;
		}
		if (*(xrnext + SYM) != HIGH) {                  /* ext == ent       */
			xrnext = getint(xrnext);
			continue;
		}
		break;
	}
	return (err ? NO : YES);
}

/* ==========================================================================
 * phase1  -  Pass 1: load all modules and libraries; link external refs.
 * ======================================================================== */
phase1(argc, argv)
	int argc; char **argv;
{
	char sz[9];
	char rtbuf[CRELSIZE];           /* scratch buffer for relocation entries  */
	char firstfn[NAMESIZE];         /* filename that set outfn (may be name-only) */
	int  i, lib, eof, prev_was_page;
	prev_was_page = NO;

	pass   = 1;
	csflag = TRUE;
	dsflag = FALSE;
	eof    = EOF;
	firstfn[0] = 0;
	nlibfns    = 0;

	/*
	 * cdisk is set high so all addresses are treated as in-memory until
	 * the overflow file is actually opened.
	 */
	cdisk = MAXMEM;

	puts("\nPhase 1 - Loading object and library files");

	instr = lgo ? RET : JMP;
	/* cbase defaults to 0 if -O flag not given.
	 * For non-paged output the loader loads to FLATBASE (0x1000)
	 * so default cbase to 0x1000 when no -O flag and no -P flag.
	 * AORG modules set their own absolute addresses and override
	 * this via the -O flag explicitly if required.                 */
	if (!oflag) {
		/* Non-paged flat output: loader loads to 0x1000 so relocate there.
		 * Paged or AORG output: cbase=0, modules set their own addresses
		 * via AORG or the -PAGES auto-assign mechanism.                   */
		if (pagemode || pages_auto)
			cbase = 0;
		else
			cbase = 0x1000;
	}
	page1data  = NO;
	i     = 0;
	pmmin_valid  = NO;
	pmmin_addr   = 0xFFFF;
	pmmin_global = 0xFFFF;
	pm_modidx    = 0;
	first_module = YES;
	pm_modsize  = 0;
	npmods      = 0;

	/*
	 * Open the data relocation table file now (named after "DATA" like dsfn).
	 * The code relocation table file is opened once the first input filename
	 * is known (below).
	 *
	 * Guard entry at offset 0: zero is the end-of-chain sentinel so the
	 * first real entry must be at offset CRELSIZE (non-zero).
	 */
	newfn(dsfn,  "DATA", DATEXT);
	newfn(drfn,  "DATA", DRFEXT);
	newfn(dtfn,  "DATA", DRLTBEXT);
	delete(dtfn);
	dtfd = open(dtfn, O_CREAT | O_RDWR | O_BINARY, 0644);
	if (dtfd < 0)
		error2("- Cannot create data relocation table ", dtfn);
	rtbuf[0] = rtbuf[1] = rtbuf[2] = rtbuf[3] = rtbuf[4] = 0;
	write(dtfd, rtbuf, CRELSIZE);   /* guard entry at offset 0               */
	drelptr = 1;  /* first usable index (0 = guard) */             /* first usable offset                   */

	while (getarg(++i, infn, NAMESIZE, argc, argv) != EOF) {
		if (infn[0] == '-') {
			/* Process position-sensitive switches inline so -P takes
			 * effect for the modules that follow it on the command line */
			if (toupper(infn[1]) == 'P') {
				if (toupper(infn[2]) == 'A') {
					/* -PAGES min-max: auto-assign pages to modules
					 * Modules before -PAGES: cbase=0x1000, page 0, common
					 * Modules after -PAGES:  cbase=0x2000+, auto pages
					 * Format: -PAGESmin-max e.g. -PAGES2-15             */
					char *p;
					int  nlen;
					p = infn + 6;
					nlen = utoi(p, &pages_min);
					if (nlen > 0) p += nlen;
					if (*p == '-') p++;
					nlen = utoi(p, &pages_max);
					if (pages_max == 0) pages_max = pages_min;
					pages_next = pages_min;
					pages_auto = YES;
					pagemode   = YES;
					prev_was_page = NO;
					/* Shift cbase to start of paged region (0x2000)
					 * cloc stays where it is - paged modules placed
					 * relative to new cbase from this point on.      */
					cbase = (unsigned)(pages_min << 12);
					cloc  = 0;
					if (monitor) {
						putls("\nAuto page mode: pages ");
						sz[0] = '0' + (pages_min / 10);
						sz[1] = '0' + (pages_min % 10);
						sz[2] = '-';
						sz[3] = '0' + (pages_max / 10);
						sz[4] = '0' + (pages_max % 10);
						sz[5] = 0;
						putls(sz);
						putls(" paged base=");
						itox(cbase, sz, 5);
						putls(sz);
					}
				} else {
					/* -P#: explicit page for following module(s) */
					curpage       = infn[2] - '0';
					pages_auto    = NO;
					pagemode      = YES;
					prev_was_page = YES;
					if (monitor) {
						itox(curpage, sz, 3);
						puts2("\nPage mode: curpage=", sz);
					}
				}
			} else {
				prev_was_page = NO;
			}
			continue;               /* skip all switch arguments              */
		}

		/* If .COM or .LGO treat as output filename, not input module.
		 * extend() would error on these extensions so check directly.     */
		{
			char *dot;
			dot = strchr(infn, '.');
			if (dot && (stricmp(dot, COMEXT) == 0 || stricmp(dot, EXEEXT) == 0 || stricmp(dot, LGOEXT) == 0)) {
				if (!*outfn) {
					strcpy(outfn, infn);
					strcpy(firstfn, infn);
					newfn(csfn, infn, OFLEXT);
					newfn(crfn, infn, REFEXT);
					newfn(ctfn, infn, CRLTBEXT);
					delete(csfn);
					delete(crfn);
					delete(ctfn);
					crfd = open(crfn, O_CREAT | O_RDWR | O_BINARY, 0644);
					if (crfd < 0)
						error2("- Cannot create reference file ", crfn);
					ctfd = open(ctfn, O_CREAT | O_RDWR | O_BINARY, 0644);
					if (ctfd < 0)
						error2("- Cannot create code relocation table ", ctfn);
					rtbuf[0] = rtbuf[1] = rtbuf[2] = rtbuf[3] = rtbuf[4] = 0;
					write(ctfd, rtbuf, CRELSIZE);
					crelptr = 1;
				}
				prev_was_page = NO;
				continue;           /* skip — already handled as output name  */
			}
		}

		lib = extend(infn, MODEXT, LIBEXT) ? YES : NO;

		/* Libraries default to page0 unless explicitly tagged with -P.
		 * This prevents the common mistake of -P1 module.OBJ lib.LIB
		 * accidentally tagging the entire library as page1.
		 * A -P flag immediately before the library overrides this.      */
		if (lib) {
			/* Check if the PREVIOUS argument was a -P flag by testing
			 * whether curpage was just set — we do this by saving the
			 * last switch seen. Simple approach: reset to 0 for libs
			 * unless the immediately preceding arg was -P.              */
			if (!prev_was_page)
				curpage = 0;
		}
		prev_was_page = NO;

		if (!*outfn) {              /* first input file determines output name */
			strcpy(firstfn, infn);  /* remember which arg set outfn           */
			newfn(outfn, infn, lgo ? LGOEXT : (pagemode ? EXEEXT : COMEXT));
			newfn(csfn,  infn, OFLEXT);
			newfn(crfn,  infn, REFEXT);
			newfn(ctfn,  infn, CRLTBEXT);
			delete(csfn);
			delete(crfn);
			delete(ctfn);
			crfd = open(crfn, O_CREAT | O_RDWR | O_BINARY, 0644);
			if (crfd < 0)
				error2("- Cannot create reference file ", crfn);
			ctfd = open(ctfn, O_CREAT | O_RDWR | O_BINARY, 0644);
			if (ctfd < 0)
				error2("- Cannot create code relocation table ", ctfn);
			rtbuf[0] = rtbuf[1] = rtbuf[2] = rtbuf[3] = rtbuf[4] = 0;
			write(ctfd, rtbuf, CRELSIZE);   /* guard entry at offset 0       */
			crelptr = 1;  /* first usable index (0 = guard) */             /* first usable offset           */
		}

		if (lib) {
			/* Save filename for multi-pass and do first search             */
			if (nlibfns < MAXLIBS)
				strcpy(libfns[nlibfns++], infn);
			putls("\nSearching Library for symbol-> ");
			search();
		} else {
			inrel = open(infn, O_RDONLY | O_BINARY);
			if (inrel < 0) {
				/*
				 * If the file that couldn't be opened is the same one used to
				 * set the output name (i.e. the first positional argument), treat
				 * it as an output-name-only specifier rather than a hard error.
				 * This supports:
				 *   link99 -M -S -B myapp mod1 mod2 clib99.LIB
				 * where "myapp" names the output but has no matching myapp.R99.
				 */
				if (!strcmp(infn, firstfn)) {
					if (monitor)
						puts3("\nOutput name: ", outfn, " (no object file)\n");
					continue;
				}
				error2("\nError opening source file: ", infn);
			}
			puts3("\nOpening R99 File ", infn, "\n");
			ifilelbuf();
			getname();
			load();
			link();
			close(inrel);
		}
	}

	/* Multi-pass library search: repeat all libraries until no new links.
	 * This handles forward references between libraries (e.g. iolib pulls
	 * in printf from clib, which then needs to be resolved from clib).    */
	do {
		int j;
		liblinked = NO;
		for (j = 0; j < nlibfns; j++) {
			strcpy(infn, libfns[j]);
			putls("\nSearching Library for symbol-> ");
			search();
		}
	} while (liblinked);

	/* Establish segment sizes and data base address for pass 2              */
	/* For flat AORG modules: global min PREL >= 0x1000 means addresses
	 * are already absolute - cbase must be 0 to avoid double-adding.        */
	{
		char sz[6];
		putls("\n  Searching for main in cbase=");
		itox(cbase, sz, 5);
		putls(sz);
		putls(" pmmin_global=");
		itox(pmmin_global, sz, 5);
		putls(sz);
		putls("\n");
	}
	if (!oflag && !pagemode && !pages_auto && pmmin_global < 0xFFFF && pmmin_global >= 0x1000)
		cbase = 0;
	csize = cloc;
	dsize = dloc;
	dbase = cbase + csize;
	pass  = 2;

	if (dsize)
		link();                     /* link data blocks to code module        */

	if (!*outfn)
		usage();

	freemem();                      /* patch ?MEMRY if present                */
	/* patch_pagemap() removed: page map is now in the file prefix,
	 * not appended after data, so ?PAGEMAP is no longer meaningful. */

	if (ferror2(crfd))
		error2("\n- Error Writing ", crfn);
	write(crfd, &eof, 2);
	rewind(crfd);

	if (csfd) {
		if (ferror2(csfd))
			error2("\n- Error Writing ", csfn);
		rewind(csfd);
	}

	if (dsfd) {
		if (ferror2(dsfd))
			error2("\n- Error Writing ", dsfn);
		rewind(dsfd);
	}

	crelptr = 1;  /* first usable index (0 = guard) */             /* reset to first real entry for phase 2  */
	drelptr = 1;  /* first usable index (0 = guard) */

	itox(csize, sz, 5);
	lout("\n\tCODE SIZE   ");
	lout(sz);
	if (csize) {
		lout(" (");
		itox(cbase, sz, 5);           lout(sz);
		lout("-");
		itox(cbase + csize - 1, sz, 5); puts2(sz, ")");
	}

	itox(dsize, sz, 5);
	lout("\n\tDATA SIZE   ");
	lout(sz);
	if (dsize) {
		lout(" (");
		itox(dbase, sz, 5);           lout(sz);
		lout("-");
		itox(dbase + dsize, sz, 5);   puts2(sz, ")");
	}
}

/* ==========================================================================
 * emit_data_block  -  Emit the data segment as a [size:word][bytes] block.
 *
 *  Mirrors emit_page_block() but reads from the data segment file (.D$)
 *  and resolves relocations via the data relocation table (.DT$).
 *
 *  Called from phase2() for non-paged output after the code block has
 *  been emitted.  Using the same size-word placeholder/patch-back pattern
 *  as emit_page_block() ensures the loader always sees a correctly-sized
 *  block header regardless of how much data there is.
 *
 *  Entry: dsfd and dtfd rewound to start; drelptr reset to 1.
 *  Exit:  returns number of data bytes written (the size field value).
 * ======================================================================== */
unsigned emit_data_block()
{
	unsigned        sz, szsave;
	long            szpos;
	char            rtbuf[CRELSIZE];
	char            str[6];

	/* --- Reserve space for the size word; fill it in after emit ---    */
	sz    = 0;
	szpos = lseek(outfd, 0, SEEK_CUR);
	write99(outfd, &sz, 2);             /* placeholder — patched below  */

	szsave = 0;
	dloc   = 0;
	dref   = readdref();
	xrplus = 0;

	while (dloc < dsize) {

		if (dloc != dref) {
			/* Non-relocatable byte: copy as-is                         */
			read(dsfd, &field, 1);
			write(outfd, &field, 1);
			szsave++;
			dloc++;
			continue;
		}

		/* Relocatable word: read DT$ entry and apply segment base      */
		read(dsfd, &field, 2);
		dtseek(field);
		read(dtfd, rtbuf, CRELSIZE);
		field = get16int(rtbuf + 2);

		if (rtbuf[4] == PREL) {
		/* AORG modules: PREL values are absolute (>= PAGE_SEG), don't add cbase */
		if (field < PAGE_SEG)
			field += xrplus + cbase;
		else
			field += xrplus;
	}
		if (rtbuf[4] == DREL) field += xrplus + dbase;
		xrplus = 0;

		write99(outfd, &field, 2);
		szsave += 2;
		dloc   += 2;
		dref    = readdref();
	}

	/* --- Patch size word now that we know the actual byte count ---    */
	sz = szsave;
	lseek(outfd, szpos, SEEK_SET);
	write99(outfd, &sz, 2);
	lseek(outfd, 0, SEEK_END);

	if (monitor) {
		putls("\n\tDATA BLOCK ");
		itox(sz, str, 6);    puts2(str, " bytes");
	}

	return (sz);
}

/* ==========================================================================
 * emit_chain_blocks  -  Emit paged output as a self-describing linked-list
 *                       block chain (v3.9.10).
 *
 *  Replaces emit_pagemap() + emit_page_block() loop for paged output.
 *
 *  Each block is written as:
 *    next_offset  word  byte distance from start of THIS header to next header
 *                       (0 = this is the last block)
 *    page         word  physical page number (0-15)
 *    start        word  virtual load address
 *    size         word  byte count of data that follows
 *    [data...]
 *
 *  The loader does a single forward pass:
 *    1. Read 8-byte header: next_offset, page, start, size
 *    2. Capture start from first block as launch address
 *    3. MAP_SET(page, start>>12) if page != 0
 *    4. PSEL on, copy size bytes to start, PSEL off
 *    5. If next_offset == 0 -> launch; else R6 += next_offset -> loop
 *
 *  Format detection: first word is next_offset. For valid paged output
 *  this is always >= 8 (header) + data, so never FFFF. The loader can
 *  distinguish this from the old sentinel format (first word = FFFF) or
 *  raw binary (first word = program instruction) if needed — but since we
 *  are replacing the format entirely, the loader simply always reads it
 *  as a chain.
 *
 *  Entries are emitted in the order they appear in the pmods arrays
 *  (i.e. the order modules appeared on the command line), skipping dead
 *  (merged-away) entries.  merge_pagemap() is called first so same-page
 *  spans are already collapsed.
 *
 *  File positions of each header are saved so next_offset and size can
 *  be patched back after each block's data is written.
 * ======================================================================== */
emit_chain_blocks()
{
	int      i, p, pg;
	int      live[MAXPMODS];    /* indices of live (non-dead) pagemap entries */
	int      nlive;             /* number of live entries                     */
	long     hdrpos[MAXPMODS];  /* file position of each block's header       */
	unsigned blksz[MAXPMODS];   /* byte count written for each block          */
	unsigned cloc_arr[MAXPMODS];/* cloc at start of each page scan            */
	unsigned ref_arr[MAXPMODS]; /* ref  at start of each page scan            */
	unsigned zero, next_off, pg_word, start_word, size_word;
	long     cur;
	char     str[8];

	/* Collect live entries in emit order */
	merge_pagemap();
	nlive = 0;
	for (i = 0; i < npmods; i++) {
		if (pmend[i] != pmstart[i])
			live[nlive++] = i;
	}

	if (nlive == 0)
		return (0);

	/* For each live entry: scan the code image once to emit its block.
	 * emit_page_block() already does the right thing per-page, including
	 * the size-word placeholder/patch-back. We just need to prepend our
	 * own header words (next_offset, page, start) around it.
	 *
	 * We emit ALL headers+blocks sequentially. After each block we know
	 * its size. After all blocks we patch next_offset for each block.   */

	for (p = 0; p < nlive; p++) {
		i = live[p];

		/* Save position of this block's header */
		hdrpos[p] = lseek(outfd, 0, SEEK_CUR);

		/* Write header placeholder words (patched below) */
		zero = 0;
		write99(outfd, &zero, 2);   /* next_offset  - patched after all blocks */
		pg_word    = (unsigned)pmpage[i];
		start_word = pmstart[i];
		write99(outfd, &pg_word,    2);   /* page  */
		write99(outfd, &start_word, 2);   /* start */
		write99(outfd, &zero,       2);   /* size  - patched below               */

		/* Reset code image scan to start of this page's data */
		cloc    = 0;
		crelptr = 1;
		ref     = readref();
		xrplus  = 0;
		if (csfd) rewind(csfd);

		/* Emit this page's block data (size word already handled inside
		 * emit_page_block via szpos, but we wrote our own size placeholder
		 * above — so we need the raw byte count back).
		 * emit_page_block writes its own size word first, which we don't
		 * want here (we already wrote our header size slot). We call it
		 * and let it write its internal size word as a temporary, then
		 * patch ours from the return value and seek back to remove the
		 * internal one.
		 *
		 * Cleaner: call emit_page_block but patch back over its size word
		 * with ours. Since emit_page_block writes [size:2][data], and our
		 * header already has the size slot, we record the file position
		 * before the call, call it (which writes size+data), then copy
		 * the size value into our header slot and shift data up — too
		 * complex.
		 *
		 * Simplest correct approach: save the file position after our
		 * header, call emit_page_block (which writes its own [size][data]),
		 * read back the size word it wrote, patch our header size slot,
		 * then the data bytes are already in the right place — the only
		 * redundancy is the extra size word emit_page_block wrote.
		 * We remove it by seeking back and shifting, which is messy.
		 *
		 * Best approach: inline the emit loop here without the internal
		 * size word, using the same logic as emit_page_block().           */

		blksz[p] = emit_page_block_data(i, &cloc, &ref);

		/* Patch our header's size word */
		cur = lseek(outfd, 0, SEEK_CUR);
		lseek(outfd, hdrpos[p] + 6, SEEK_SET);   /* size is at offset 6 in header */
		size_word = blksz[p];
		write99(outfd, &size_word, 2);
		lseek(outfd, cur, SEEK_SET);

		if (monitor) {
			putls("\n\tCHAIN BLOCK pg=");
			itox(pmpage[i],  str, 3); putls(str);
			putls(" start=");
			itox(pmstart[i], str, 5); putls(str);
			putls(" size=");
			itox(blksz[p],   str, 6); putls(str);
		}
	}

	/* Now patch next_offset for each block.
	 * next_offset[p] = distance in bytes from hdrpos[p] to hdrpos[p+1].
	 * Last block gets next_offset = 0.                                   */
	for (p = 0; p < nlive; p++) {
		if (p < nlive - 1)
			next_off = (unsigned)(hdrpos[p + 1] - hdrpos[p]);
		else
			next_off = 0;
		cur = lseek(outfd, 0, SEEK_CUR);
		lseek(outfd, hdrpos[p], SEEK_SET);
		write99(outfd, &next_off, 2);
		lseek(outfd, cur, SEEK_SET);
	}
}

/* ==========================================================================
 * emit_page_block_data  -  Emit one page's data bytes only (no size word).
 *
 *  Same logic as emit_page_block() but without the leading size word.
 *  Called by emit_chain_blocks() which manages its own header.
 *
 *  Entry:  idx      = index into pmods arrays for the page to emit
 *          cloc_p   = pointer to current code location counter (reset by caller)
 *          ref_p    = pointer to current relocation reference  (reset by caller)
 *  Exit:   returns number of data bytes written
 * ======================================================================== */
unsigned emit_page_block_data(idx, cloc_p, ref_p)
	int      idx;
	unsigned *cloc_p;
	unsigned *ref_p;
{
	unsigned  szsave, tp;
	char      rtbuf[CRELSIZE];
	int       pg;

	pg     = pmpage[idx];
	szsave = 0;

	while (*cloc_p < csize) {

		/* Skip bytes not belonging to this page */
		if (!in_page(*cloc_p, pg)) {
			if (*cloc_p < cdisk)
				(*cloc_p)++;
			else {
				read(csfd, &field, 1);
				(*cloc_p)++;
			}
			if (*cloc_p - 1 == *ref_p) {
				if (*cloc_p < cdisk)
					(*cloc_p)++;
				else {
					read(csfd, &tp, 2);
					(*cloc_p)++;
				}
				*ref_p = readref();
			}
			continue;
		}

		if (*cloc_p != *ref_p) {
			/* Non-relocatable byte */
			if (*cloc_p < cdisk)
				field = *(buffer + *cloc_p);
			else
				read(csfd, &field, 1);
			write(outfd, &field, 1);
			szsave++;
			(*cloc_p)++;
			continue;
		}

		/* Relocatable word */
		if (*cloc_p < cdisk)
			tp = get16int(buffer + *cloc_p);
		else
			read(csfd, &tp, 2);

		ctseek(tp);
		read(ctfd, rtbuf, CRELSIZE);
		field = get16int(rtbuf + 2);

		if (rtbuf[4] == PREL) {
		/* AORG modules: PREL values are absolute (>= PAGE_SEG), don't add cbase */
		if (field < PAGE_SEG)
			field += xrplus + cbase;
		else
			field += xrplus;
	}
		if (rtbuf[4] == DREL) field += xrplus + dbase;
		xrplus = 0;

		write99(outfd, &field, 2);
		szsave  += 2;
		*cloc_p += 2;
		*ref_p   = readref();
	}

	return (szsave);
}

/* ==========================================================================
 * phase2  -  Pass 2: generate the final absolute output file.
 *
 *  File layout produced (v3.8):
 *    [M x 6-byte pagemap entries]   see emit_pagemap()
 *    [FFFF FFFF FFFF sentinel]      always present
 *    [size:word][page 0 code+data]  one block per unique page number
 *    [size:word][page 1 code+data]  non-paged: single block, all code
 *    ...
 *    [sector padding]               to 512-byte boundary
 *
 *  LGO header (RET <start> <base> <size>) is written before the page
 *  blocks when -G# is specified.
 *
 *  The page-block format lets the shell loader process one page at a
 *  time without staging the entire program in memory first:
 *    for each [size][block]:
 *      program 6116 for this page, enable PSEL,
 *      RDSEQ 'size' bytes to virtual TPA address,
 *      disable PSEL, advance to next block.
 * ======================================================================== */
phase2()
{
	char  at[6], sz[8];
	char *epnext2;                  /* walks ep table to find main()        */
	char  mainname[MAXSYM + 1];
	strcpy(mainname, "main");

	puts("\n\nPhase 2 - Writing execution files\n");

	/* Locate the entry point of 'main' in the symbol table                  */
	epnext2 = getint(epfirst);
	while (YES) {
		if (strcmp(StringPadRight(mainname, MAXSYM, " "), epnext2 + SYM) == 0) {
			_debug("Found main at goloc = ", getint(epnext2 + VAL));
			goloc = getint(epnext2 + VAL);
		}
		if (*(epnext2 + SYM) != HIGH) {
			epnext2 = getint(epnext2);
			continue;
		}
		break;
	}

	/* Open output file                                                       */
	if (!outfd) {
		outfd = open(outfn, O_WRONLY | O_TRUNC | O_BINARY);
		if (outfd < 0)
			outfd = open(outfn, O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, 0644);
		if (outfd < 0)
			error2("Error opening destination: ", outfn);
	}

	/* Paged output uses emit_chain_blocks() below — self-describing format.
	 * Non-paged output uses emit_raw_block() below.                        */

	/*
	 * Write the program header.
	 * TMS9900: STWP WP / B @entry is generated by the assembler or
	 * Small-C compiler (via iolib). link99 does NOT insert it — doing
	 * so shifts all code addresses by 6 bytes breaking relocations.
	 * LGO: RET <start> <load-base> <code-size> (unchanged)
	 */
	if (lgo) {
		if (write99(outfd, &instr, 2) < 0)
			error2("Error writing to: ", outfn);
		addr = cbase + goloc;
		if (write99(outfd, &addr, 2) < 0)
			error2("Error writing to: ", outfn);
		write99(outfd, &cbase, 2);
		write99(outfd, &csize, 2);
	}

	/* ----- Emit code+data as page-separated blocks ---------------------- */
	/* v3.8: one [size:word][bytes] block per unique page number.
	 * Non-paged: single block containing all code+data.
	 * The shell loader reads one block at a time, programs the 6116
	 * map registers for that page, then loads directly to virtual TPA.  */

	cloc    = 0;
	crelptr = 1;  /* first usable index (0 = guard) */
	drelptr = 1;  /* first usable index (0 = guard) */
	ref     = readref();
	xrplus  = 0;

	if (!pagemode) {
		/* --- Non-paged: raw binary - no size word, no header ---      */
		emit_raw_block(&cloc, &ref);

		/* Data is a separate [size:word][bytes] block via emit_data_block().
		 * This gives the loader a correct size header and keeps the same
		 * patch-back pattern as emit_page_block() - no raw appending.   */
		if (dsize) {
			rewind(dsfd);
			drelptr = 1;
			emit_data_block();
		}
	} else {
		/* --- Paged: linked-list block chain (v3.9.10) ---             */
		/* emit_pagemap() call removed: format is now self-describing.  */
		emit_chain_blocks();
	}



	/* ----- Cleanup: close and delete temporary files --------------------- */

	if (ferror2(outfd))
		error2("\n- Error Writing ", outfn);
	close(outfd);

	if (csfd) {
		if (ferror2(csfd))
			error2("\n- Error Reading ", csfn);
		close(csfd);
		delete(csfn);
	}

	if (ferror2(crfd))
		error2("- Error Reading ", crfn);
	close(crfd);
	delete(crfn);

	if (ferror2(ctfd))
		error2("- Error Reading ", ctfn);
	close(ctfd);
	delete(ctfn);

	if (ferror2(dtfd))
		error2("- Error Reading ", dtfn);
	close(dtfd);
	delete(dtfn);

	if (dsfd) {
		if (ferror2(dsfd))
			error2("\n- Error Reading ", dsfn);
		close(dsfd);
		delete(dsfn);
	}
}

/* ==========================================================================
 * readref  -  Read the next code relocation entry from the table file.
 *             crelptr is an entry INDEX; ctseek multiplies by CRELSIZE.
 * ======================================================================== */
int readref()
{
	char rbuf[CRELSIZE];
	int  nr;

	rbuf[0] = rbuf[1] = rbuf[2] = rbuf[3] = rbuf[4] = 0xFF; /* sentinel if read fails */
	ctseek(crelptr);
	nr = read(ctfd, rbuf, CRELSIZE);
	if (nr < CRELSIZE) {
		ref = 0xFFFF;               /* beyond end of table - no more refs       */
		return (ref);
	}

	ref = get16int(rbuf);

	if (ref == XRPLUS) {
		xrplus   = get16int(rbuf + 2);
		crelptr++;                      /* consume XPOFF entry               */

		ctseek(crelptr);
		read(ctfd, rbuf, CRELSIZE);
		ref = get16int(rbuf);

		if (ref == XRPLUS) {
			xrplus  -= cbase;
			crelptr++;
			ctseek(crelptr);
			read(ctfd, rbuf, CRELSIZE);
			ref = get16int(rbuf);
		}
	}
	crelptr++;
	return (ref);
}

/* ==========================================================================
 * readdref  -  Read the next data relocation entry from the table file.
 *              drelptr is an entry INDEX; dtseek multiplies by CRELSIZE.
 * ======================================================================== */
readdref()
{
	char rbuf[CRELSIZE];
	int  nr;

	rbuf[0] = rbuf[1] = rbuf[2] = rbuf[3] = rbuf[4] = 0xFF; /* sentinel if read fails */
	dtseek(drelptr);
	nr = read(dtfd, rbuf, CRELSIZE);
	if (nr < CRELSIZE) {
		dref = 0xFFFF;
		return (dref);
	}

	dref = get16int(rbuf);

	if (dref == XRPLUS) {
		xrplus   = get16int(rbuf + 2);
		drelptr++;                      /* consume XPOFF entry               */

		dtseek(drelptr);
		read(dtfd, rbuf, CRELSIZE);
		dref = get16int(rbuf);

		if (dref == XRPLUS) {
			xrplus  -= dbase;
			drelptr++;
			dtseek(drelptr);
			read(dtfd, rbuf, CRELSIZE);
			dref = get16int(rbuf);
		}
	}
	drelptr++;
	return (dref);
}

/* ==========================================================================
 * resolve  -  Patch all external-reference chain entries for one symbol.
 *
 *  The code image (buffer or overflow file) stores .CT$ file offsets at
 *  each relocatable word location.  The .CT$ entry at that offset
 *  holds: [code-location:2][chain-value:2][type:1].  We walk the chain
 *  until chain-value reaches zero, writing the resolved absolute address.
 * ======================================================================== */
resolve()
{
	unsigned int   tp;
	unsigned xrloc, epval, xbuf;
	unsigned xt, et, tbase;
	unsigned prev_xrloc;          /* detect repeating chain locations   */
	char rtbuf[CRELSIZE];           /* scratch for one relocation table entry */
	char at[7];
	char str[9];
	int  rdebug;
	rdebug = 1;
	prev_xrloc = 0;

	if (!(xrloc = getint(xrnext + VAL)))    /* head of ext-ref chain         */
		return;

	epval = getint(epnext + VAL);

	xt    = *(xrnext + FLG);
	et    = *(epnext + FLG);

	/* Compute the absolute target address in the runtime image              */
	if (et == PREL) tbase = epval + cbase;
	if (et == DREL) tbase = epval + dbase;

	/* Cross-page collision check (v3.7): with transparent GAL mapping,
	 * cross-page calls are plain BL/RT - no trampolines needed.
	 * However two modules on different pages sharing a virtual 4KB
	 * segment is a hard error: only one mapping can be active.
	 * pageof() returns -1 for untagged (common) addresses; those are
	 * always safe and need no check.                                  */
	{
		int caller_pg, callee_pg;
		int caller_seg, callee_seg;
		caller_pg  = pageof(xrloc + cbase);
		callee_pg  = pageof(tbase);
		caller_seg = (xrloc + cbase) >> 12;  /* top 4 bits = segment index */
		callee_seg = tbase >> 12;
		if (caller_pg >= 0 && callee_pg >= 0 &&
		    caller_pg != callee_pg &&
		    caller_seg == callee_seg) {
			puts2("\n- Error: segment collision: ", xrnext + SYM);
			puts2(" caller page ", caller_pg ? "1" : "0");
			puts2(" callee page ", callee_pg ? "1" : "0");
			puts2(" share segment ", "");
			/* non-fatal: report all collisions then let okay() abort   */
		}
	}

	do {
		/* end-of-chain sentinel                                              */
		if (xrloc == 0)
			break;
		/* repeated location means circular chain - treat as end             */
		if (xrloc == prev_xrloc)
			break;
		prev_xrloc = xrloc;

		if (++rdebug > 5000) {
			puts("Resolving Error: likely duplicate entry names - quitting");
			exit(0);
		}
		if (monitor) {
			poll(YES);
			putls("\n Resolving external ");
			itox(xt, at, 5);
			puts2("xt = ", at);
			itox(xrloc, at, 5);
			puts2(" xrloc=", at);
			putls((xt == PREL) ? "' to " : "\" to ");
			itox(tbase, at, 5);
			putls(at);
			puts2(" for ", xrnext + SYM);
			itox(nxr, at, 5);
			puts2(" nxr=", at);
		}

		switch (xt) {

		/* ---- Data-relative reference ----------------------------------------
		 * In TMS9900 Small-C, DREL XCHAIN references are code instructions that
		 * reference data symbols (e.g. MOV @_var, R0).  The chain locations are
		 * in the CODE segment, not the data segment, so chain-walking is
		 * identical to PREL.  Only tbase differs (uses dbase, set above).
		 * Fall through to PREL.
		 * ------------------------------------------------------------------- */
		case DREL:

		/* ---- Program-relative reference ---------------------------------- */
		case PREL:
			/* xrloc is virtual addr for AORG modules, buffer offset for relocatable */
			/* line 2442 - WRONG: subtracts cbase from a buffer-relative offset */
		/*	xbuf = (xrloc >= cbase) ? (xrloc - cbase) : xrloc; */
			/* xrloc is virtual for AORG/cbase>0 modules; convert to buffer offset */
			xbuf = (xrloc >= cbase) ? xrloc - cbase : xrloc;
			if (xbuf < cdisk) {
				/* Location is in the in-memory code buffer                   */
				tp = get16int(buffer + xbuf);
				if (tp == 0) {
					/* End of chain - direct patch for assembly externals      */
					put16int(buffer + xbuf, tbase);

				} else {
					/* tp is a byte offset into the code relocation table     */
					ctseek(tp);
					read(ctfd, rtbuf, CRELSIZE);
					nxr       = get16int(rtbuf + 2);
					if (nxr == xrloc) nxr = 0;  /* self-ref = end of chain  */
					xt        = rtbuf[4];
					rtbuf[4]  = et;
					put16int(rtbuf + 2, epval);
					ctseek(tp);
					write(ctfd, rtbuf, CRELSIZE);
					if (nxr == 0)
						tp = 0;     /* signal end-of-chain                    */
				}
			} else {
				/* Location is in the code overflow file (.O$)                */
				xrseek(xbuf - cdisk);
				read(csfd, &tp, 2);
				if (tp == 0) {
					xrseek(xbuf - cdisk);
					write(csfd, &tbase, 2);
				} else {
					ctseek(tp);
					read(ctfd, rtbuf, CRELSIZE);
					nxr       = get16int(rtbuf + 2);
					if (nxr == xrloc) nxr = 0;  /* self-ref = end of chain  */
					xt        = rtbuf[4];
					rtbuf[4]  = et;
					put16int(rtbuf + 2, epval);
					ctseek(tp);
					write(ctfd, rtbuf, CRELSIZE);
				}
			}
			break;
		}

		xrloc = nxr;
	} while (tp);

#ifdef DEBUG2
	{
		unsigned i;
		unsigned char *bp, *rp;

		puts("Outputting final buffer:");
		bp = buffer;
		printf("%04X  ", 0);
		for (i = 0; i < cloc; i++) {
			printf("%02X ", *bp++);
			if ((i + 1) % 16 == 0)
				printf("\n%04X  ", i + 1);
		}
		puts("\n\nOutputting final creltble (disk .CT$ file, offsets only):");
		printf("crelptr offset = %u\n", crelptr);
	}
#endif
}

/* ==========================================================================
 * search  -  Search a library for modules that resolve pending externals.
 * ======================================================================== */
search()
{
	int linked;
	linked = NO;

	newfn(ndxfn, infn, NDXEXT);
	ndxfd = open(ndxfn, O_RDONLY | O_BINARY);
	inrel = open(infn,  O_RDONLY | O_BINARY);
	ifilelbuf();

	while (YES) {                       /* rescan until no new links found    */
		while (nxtmod()) {
			while (getrel() == ENAME) {
				poll(YES);
				if (isunres()) {
					putls(symbol);
					putchar('\n');
					load();
					link();
					linked    = YES;
					liblinked = YES;    /* signal phase1 that new links found */
					break;
				}
			}
		}
		if (!linked)
			break;
		linked = NO;
		rewind(ndxfd);
	}

	close(ndxfd);
	close(inrel);
}

/* ==========================================================================
 * seek  -  Position the library REL file at the next member.
 * ======================================================================== */
seek()
{
	if (inblock == EOF)
		error("- Premature End of Index");
	if (lseek(inrel, inblock << 7, SEEK_SET) == EOF)
		error("- Corrupt Library or Index");
	if (lseek(inrel, inbyte, SEEK_CUR) == EOF)
		error("- Corrupt Library or Index (byte offset)");
	inrem = 0;  /* force getrel() to read a fresh byte                       */
}

/* ==========================================================================
 * usage  -  Print usage message and abort.
 * ======================================================================== */
usage()
{
	error("\nUsage: link99 [-B] [-S] [-G0xADDR] [-O0xADDR] [-D0xADDR] [-P#] [-M] outname.com module.r99 [module.r99/library.lib...]\n"
	      "  outname.com  output executable (explicit .COM extension required)\n"
	      "  module.r99   relocatable object module (explicit .R99 extension required)\n"
	      "  -O0xADDR     set code origin address (e.g. -O0x2000 for paged segment 2)\n"
	      "  -P#          page assignment 0-15 for following modules\n"
	      "  -PAGES#-#    auto-assign pages across range (e.g. -PAGES2-15)\n"
	      "  -B           big-endian output\n"
	      "  -S           Small-C mode\n"
	      "  -M           monitor/verbose output\n"
	      "  Transparent GAL mapping: cross-page calls are plain BL/RT, no trampolines.\n"
	      "  Segment collision (two pages sharing a 4KB virtual segment) is a link error.");
}

/* ==========================================================================
 * ctseek  -  Seek to byte offset 'off' in the code relocation table file.
 * ======================================================================== */
ctseek(idx)
	unsigned int idx;
{
	if (lseek(ctfd, (long)idx * CRELSIZE, SEEK_SET) == EOF)
		error("- Seek error in code relocation table (.CT$)");
}

/* ==========================================================================
 * dtseek  -  Seek to entry index 'idx' in the data relocation table file.
 * ======================================================================== */
dtseek(idx)
	unsigned int idx;
{
	if (lseek(dtfd, (long)idx * CRELSIZE, SEEK_SET) == EOF)
		error("- Seek error in data relocation table (.DT$)");
}

/* ==========================================================================
 * xrseek  -  Seek to absolute byte offset 'byte' in the code overflow file.
 *
 *  FIX: was SEEK_CUR (relative to current position), must be SEEK_SET
 *       (absolute).  SEEK_CUR made every seek position-dependent, breaking
 *       all random access into the overflow file.
 * ======================================================================== */
xrseek(byte)
	int byte;
{
	if (lseek(csfd, byte, SEEK_SET) == EOF)
		error2("- Seek error in code overflow file ", csfn);
}

/* ==========================================================================
 * dxrseek  -  Seek to absolute byte offset 'byte' in the data segment file.
 *
 *  FIX: same SEEK_CUR -> SEEK_SET correction as xrseek.
 * ======================================================================== */
dxrseek(byte)
	int byte;
{
	if (lseek(dsfd, byte, SEEK_SET) == EOF)
		error2("- Seek error in data segment file ", dsfn);
}

/* ==========================================================================
 * rewind  -  Seek file descriptor 'fd' back to byte 0.
 *
 *  FIX: the original issued a pointless seek-to-end before seeking to
 *       start.  Removed; now simply seeks to offset 0.
 * ======================================================================== */
rewind(fd)
	int fd;
{
	lseek(fd, 0, SEEK_SET);
}

/* ==========================================================================
 * lout  -  Output a string (strips CP/M line-feed artefacts).
 * ======================================================================== */
lout(string)
	char *string;
{
	putls(string);
}

/* ==========================================================================
 * _debug  -  Print a labelled hex value for diagnostic use.
 * ======================================================================== */
_debug(var, val)
	char *var; int val;
{
	char at[6];
	itox(val, at, 5);
	puts2(var, at);
	puts("\n");
}

/* ==========================================================================
 * write99  -  Write a 16-bit word to 'fd' with TMS9900 byte order.
 * ======================================================================== */
write99(fd, buf, n)
	int fd; char *buf; int n;
{
	swap99[0] = *(buf + 1); /* high byte first */
	swap99[1] = *buf;       /* low byte second */
	return (write(fd, swap99, n));
}

/* ==========================================================================
 * pint99  -  Store integer 'i' into '*addr' in TMS9900 byte order.
 * ======================================================================== */
pint99(addr, i)
	int *addr; int i;
{
	char *t;
	t    = addr;
	*t++ = i >> 8;
	*t   = i;
}

/* ==========================================================================
 * StringPadRight  -  Right-pad 'string' with 'pad' to reach 'padded_len'.
 * ======================================================================== */
StringPadRight(string, padded_len, pad)
	char *string; int padded_len; char *pad;
{
	int len, i;
	len = strlen(string);
	if (len >= padded_len)
		return (string);
	for (i = len; i < padded_len; i++)
		strcat(string, pad);
	return (string);
}
