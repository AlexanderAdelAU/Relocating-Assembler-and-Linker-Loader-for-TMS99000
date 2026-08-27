/*
 ============================================================================
 Name        : link99.c K&R version
 Authors     : J. E. Hendrix (original Small-MAC linker, 1985)
               Alex Cameron (TMS9900 port 1984; Eclipse cross-compiler 2015)
 Version     : 3.9.56
 Copyright   : Free to use
 Description : Relocatable linker/loader for the TMS9900 architecture.
               Based on the Small-MAC linkage editor (ver 1.0).

 Version history:
   1.0  J.E. Hendrix   Original Small-MAC linker (8080/CP/M, 1985)
   2.0  A. Cameron     TMS9900 port (1984)
   3.0  A. Cameron     Eclipse cross-compiler version (June 2015)
   3.1  A. Cameron     COMBASE moved to 0x500 for extended memory
   3.2  A. Cameron     Disk overflow fully implemented; code tidied
   3.3  A. Cameron     First positional arg may be output name only
   3.4  A. Cameron     Relocation tables moved to disk (.CT$/.DT$);
                       fixes DREL resolve() chain bug
   3.5  A. Cameron     Entry-index scheme for CT$/DT$; multi-pass
                       library search
   3.6  A. Cameron     Memory manager support: -P# page tagging,
                       page-map table, cross-page trampoline stubs
   3.7  A. Cameron     Transparent mapping: trampolines eliminated;
                       -P# extended to 0-15
   3.8  A. Cameron     Page-separated output blocks; one block per
                       page with sentinel/pagemap prefix
   3.9  A. Cameron     Cleaner command syntax
 3.9.1  A. Cameron     512KB buffer; paged AORG fixup; flat raw binary
 3.9.2  A. Cameron     Fix resolve() direct patch for assembly externals
 3.9.3  A. Cameron     Fix resolve() xrloc virtual-to-buffer conversion
 3.9.4  A. Cameron     Fix cmod: save cloc at ENAME before SETLC corrupts it
 3.9.5  A. Cameron     Fix pmmin_global: first module only for AORG detection
 3.9.6  A. Cameron     Fix PREL: always add cmod for offset-0 references;
                       cbase defaults to 0; oflag added; pmstart/pmend
                       store absolute addresses
 3.9.7  A. Cameron     Fix cmod stale for library modules
 3.9.8  A. Cameron     Fix mid-commandline -P# page tagging
 3.9.9  A. Cameron     Suppress ABS/PREL/DREL/CREL items from -M output
 3.9.10 A. Cameron     AORG paged-output fix: PAGE_SEG guard for PREL/emit
 3.9.11 A. Cameron     Fix xbuf: use cloc not cbase to discriminate buffer
                       offsets from virtual addresses in resolve()
 3.9.12 A. Cameron     Paged EXE chain-block output (Shell V5.7 format):
                       [next_offset][page][start][size][data...]
 3.9.13 A. Cameron     AORG overlay PREL fix: do not add cmod/cbase to
                       fields already >= PAGE_SEG in paged mode
 3.9.14 A. Cameron     Paged EXE emit: use page block virtual start for
                       AORG modules, not common cbase
 3.9.15 A. Cameron     (Diagnostic build - not retained)
 3.9.16 A. Cameron     Paged PREL fix: field=0 must still receive cmod
                       in pagemode (OVLMGR table base reference)
 3.9.17 A. Cameron     SETLC seeds pmmin_addr for AORG detection;
                       emit uses pmstart-pmcmod for correct relocation
 3.9.18 A. Cameron     Capture explicit AORG SETLC base (pm_aorg_base)
                       for EXE chain block virtual start address
 3.9.19 A. Cameron     AORG PREL symbols treated as runtime virtual
                       addresses in paged mode; xbuf_from_runtime()
                       maps virtual addresses back to buffer offsets
 3.9.22 A. Cameron     Resolve chain guard: chains stay within their
                       page-map span to prevent stale CT$ pointer walks
 3.9.23 A. Cameron     EXE block splitting for large modules
 3.9.24 A. Cameron     Fix EXE chunking: split by page-map entry;
                       preserve each chunk original virtual base
 3.9.25 A. Cameron     Fix emit reloc_base: subtract pmcmod to cancel
                       load-time cmod addition; crelptr chain guard
 3.9.26 A. Cameron     EXE_MAX_BLOCK = 0x1F8 (one sector minus header);
                       aligns each block to a sector boundary so Shell
                       V5.8 loader chain arithmetic is always correct;
                       no file size limit
 3.9.26 A. Cameron     Fix EXT placeholder chain corruption: type=3/field=0
                       sentinels must not receive cmod (nxr would be non-zero
                       causing resolve() to walk into wrong buffer locations)
 3.9.27 A. Cameron     New AORG_MARK REL item (reuses CHAIN=16, never emitted
                       by assembler for real chains in r99).  Assembler emits
                       AORG_MARK at every AORG directive so the linker can
                       unambiguously detect absolute-origin modules regardless
                       of their load address (fixes low-AORG modules below
                       PAGE_SEG such as XOP vectors at 0x0040).
                       Corresponding changes: rel99.h (AORG_MARK alias),
                       R99ASMLN.c (emit at AORG), GETREL.c (decode),
                       SEEREL.c (display as "aorg base:").
 3.9.28 A. Cameron     Fix pmstart[]/pmend[] silently page-aligned for any
                       AORG >= PAGE_SEG, discarding the in-page offset (e.g.
                       AORG 0x3A22 was written into the EXE chain block's
                       load address as 0x3000). The rounded value is correct
                       for its original purpose - stripping the page-relative
                       offset out of PREL/SETLC records - but was wrongly
                       being reused as the actual load address too. Added
                       page_align() (the rounding formula now has exactly one
                       definition instead of four separately hand-typed
                       copies) and pm_aorg_true (the real, unrounded address,
                       populated only from AORG_MARK - NOT from PREL, which
                       fires unfiltered for every internal reference in a
                       module and is not a reliable "true start" signal for
                       anything but the smallest single-AORG modules).
                       pmstart[]/pmend[] now use pm_aorg_true throughout.
                       Invisible for -P0/-P2/-P3 (their AORGs already happen
                       to be page-aligned, so rounding was always a no-op);
                       first surfaced on -P5 with AORG 0x3A22, the first
                       build to ever use a page where AORG isn't the page's
                       exact base.
 3.9.29 A. Cameron     Fix is_overlay_chain_base_sentinel excluding page0/
                       common modules from the zero-terminator-as-cmod
                       guard (pmpage[i]==0 and pmstart[i]<PAGE_SEG were both
                       `continue`d past, on the wrong assumption that only
                       AORG overlay modules hit this). A page0 module with
                       several external references hits the identical bug:
                       every symbol's chain spuriously terminates one link
                       too late, at addr==pmcmod (the module's own offset
                       0), so whichever symbol resolves last in the sorted
                       xr/ep walk overwrites that location with its own
                       value - corrupting the module's first instruction
                       word. Removed both exclusions; the underlying
                       addr==pmcmod check was already correct and generic,
                       it just never ran for common modules.

 3.9.35 A. Cameron     Implements REL99_FORMAT.TXT clauses 2.3, 4.2, 6.5.
                       (2.3) The AORG base now binds to the buffer position
                       of the AORG_MARK item (pm_aorg_cloc), not the module
                       start: smallcp emits a 4-byte relocatable preamble
                       (>1000 >1000) before its AORG, which previously
                       shifted every paged image +4 and made every
                       runtime->buffer conversion 4 bytes early (the slot-4
                       chain corruption seen on hardware). Pre-AORG bytes
                       stay in the relocatable stream per the format.
                       (4.2) XCHAIN heads convert to unique buffer offsets
                       at record time; resolve() converts later links via
                       the chain-owning module's span (xbuf_for_chain), so
                       several AORG modules may share one window.
                       (6.5) resolve() never direct-patches inside a paged
                       module: a slot that is not a verified CT entry
                       reports a malformed chain and abandons it. Stored
                       resolution values are UNCHANGED from 3.9.31.
 3.9.36 A. Cameron     Spec clause 2.4 (emit contract): resolved external
                       values patched into a paged module's CT are now
                       stored as FINAL ABSOLUTE addresses (tbase). Emit
                       adds reloc_base to any CT value < PAGE_SEG in a
                       paged module - correct for page-local words,
                       wrong for resolved externals (buffer offsets
                       needing +cbase). Proven on hardware with correct
                       slot addressing (v3.9.35): error epval >0C90
                       emitted as >357E (+>28EE reloc delta) instead of
                       >1C90. Same remedy as withdrawn 3.9.34, now valid
                       because 2.3/4.2 fixed the addressing beneath it.
 3.9.37 A. Cameron     3.9.36 was a no-op: it stored tbase, whose v3.9.30
                       discriminator (epval >= csize) always fires in
                       pagemode because csize = cloc is assigned only
                       AFTER loading while resolve() runs per module
                       DURING loading (csize==0). Hence every resolve
                       trace ever printed shows raw offsets ('to C90').
                       The paged-chain store now discriminates with
                       cloc: the target module is loaded before its
                       entry resolves, so buffer offsets are < cloc and
                       AORG absolutes are not; buffer offsets store
                       epval+cbase (final absolute, spec 2.4), AORG
                       absolutes store epval unchanged.
 3.9.38 A. Cameron     Spec clause 2.5: a paged AORG module crossing its
                       4KB virtual segment is now a fatal link error.
 3.9.39 A. Cameron     Added resident/paged-window overlap protection.
 3.9.40 A. Cameron     Corrected that protection to compare actual page-map
                       runtime spans rather than packed linker-buffer size.
                       Increased MAXPMODS to 128 for large chain-block output.
                       Chain ordering and chain-block writer remain exactly
                       as in the proven 3.9.37 baseline.
 3.9.51 A. Cameron     XREFS scheme: external references are now explicit
                       (EXT "XREFS" / XCHAIN count,name / CHAIN location*).
                       load() records each location as a code-buffer offset
                       on the xr entry; resolve() direct-patches the ABS
                       placeholder with the final absolute address.  Removed
                       the in-code chain walk and all its heuristics
                       (xbuf_for_chain, paged_chain_span, overlay sentinel,
                       circular guard, malformed-chain abandon).
 3.9.52 A. Cameron     Preserve ordinary-PREL versus AORG-PREL provenance in
                       CT$. Ordinary PREL values are link-buffer addresses and
                       Phase 2 applies pmstart-pmcmod; AORG PREL values are
                       already final virtual addresses. Fixes resident-module
                       operands above >1000 (for example IOCORE >1856) being
                       emitted one cbase low instead of >2856.
 3.9.53 A. Cameron     resolve(): the epval>=cloc AORG discriminator broke
                       once total code size passed >8000 (Step 6, csize
                       >B2A6): a buffer offset and an AORG virtual are then
                       numerically indistinguishable, and the SAME symbol
                       resolved differently depending on cloc at resolution
                       time (down1 -> AC80 for CC_EXPR_C refs, BC80 for
                       CC_EXPR_D refs). Provenance, not value range: newsym
                       now sets FLGAORG on the ep entry in the exact branch
                       where it declines to add cmod; resolve() tests the
                       flag. Mirrors how CT_AORG_PREL fixed the identical
                       ambiguity in CT$ (v3.9.52). Also converted the three
                       field>=PAGE_SEG guards in emit_raw_block/
                       emit_page_block/data emit to CT_AORG_PREL provenance
                       (that heuristic corrupts resident PREL words above
                       >1000 -- see the load()-side warning comment). Banner
                       bumped so a stale exe is detectable from build logs.
 ============================================================================
 LINKER OPERATION - THREE PHASES
 ============================================================================
 3.9.55 A. Cameron     -P page numbers are now parsed in full. The
                       explicit-page switch read ONE digit, so -P10
                       silently meant page 1: the module's bytes were
                       written to the wrong physical page while DREL's
                       OVL_TABLE recorded the right one, and the mapper
                       later mapped a page that had never been written.
                       Invisible for eight milestones because no build
                       had used a page above 9. An out-of-range or
                       unparsable -P is now FATAL, not truncated.

 3.9.56 A. Cameron     Fix flat/resident SETLC corruption.  The SETLC
                       handler treated every PREL location above >1000 as
                       an absolute AORG and stripped its 4KB page base, even
                       when no -P page assignment was active.  Large resident
                       modules therefore packed BSS gaps out of the image while
                       retaining logical EPOINT addresses (CC1 main linked as
                       >3106 but its bytes were emitted at >1C56).  AORG SETLC
                       normalization is now restricted to explicitly paged
                       modules (pagemode && curpage != 0), matching the v3.9.54
                       EPOINT/PREL provenance rule.

                       Also (same version): MAXPMODS 128 -> 192 and page-map
                       overflow is now FATAL, not a warning.  The first cut of
                       the phase-1 fatal had a dangling-else bug (the original
                       one-statement else was expanded to three without adding
                       braces), so exit(1) ran for every module and the link
                       died at module 1 regardless of MAXPMODS.  Braced.  The block
                       splitter printed one warning and RETURNED, silently
                       abandoning every remaining chunk -- the EXE linked
                       clean with pieces of the program missing.  Surfaced by
                       the M33a preprocessor image (~141 chunks: 28K resident
                       + nine overlay pages) crossing 128, with crashes that
                       moved as the fixture size changed.  This is the SETLC
                       corruption's sibling: both packed or dropped bytes and
                       left the image structurally short with no error.

 3.9.54 A. Cameron     HARDENING (the CC_DATA incident). (1) The
                       backward-compatible AORG heuristic (EPOINT with
                       PREL value >= PAGE_SEG) misclassified plain
                       RESIDENT modules as AORG the moment their size
                       crossed 0x1000 bytes: entry-point OFFSETS past
                       4KB read as absolute addresses, base became
                       page_align(offset)=0x1000, and the PREL emit
                       fallback then stopped relocating resident words
                       with buffer targets >= 0x1000 - silently zero/
                       garbage-corrupting initialized data (CC_DATA at
                       4338 bytes: op[]/op2[]/usexpr/declared loaded
                       as zeros). The earlier CC_RESIDENT "spec 2.5
                       FATAL" at 5010 bytes was the SAME misfire
                       failing loudly by luck. Both the EPOINT
                       heuristic and the PREL emit fallback are now
                       gated on curpage != 0: only a module under an
                       explicit -P assignment may be heuristically
                       AORG-classified. AORG_MARK detection (r99 v2.1+)
                       is unaffected. Resident PREL modules have NO 4KB
                       size limit - common memory is contiguous.
                       (2) Overflow-to-disk engagement now announces
                       itself loudly with buffer usage numbers (this
                       path is suspect in the 56K-era silent hang).
                       (3) Memory usage report after loading: code,
                       symbol, and free bytes, so buffer exhaustion is
                       a number watched approaching, not a hang.


 Phase 1 - Loading (phase1())
 ----------------------------
 Processes each command-line argument in order:

   Object modules (.R99):
     - Loaded into the code buffer (or overflow file .O$ if buffer full)
     - PREL/DREL relocatable words recorded in disk-based relocation tables
       (.CT$ for code, .DT$ for data) using a 5-byte entry-index scheme
     - External references (XREFS groups) recorded on the xr symbol table:
       one entry per symbol, carrying an explicit list of reference
       locations (no in-code chain)
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
   - Walks the xr entry's list of explicit reference locations (recorded
     from the module's XREFS group at load time and already converted to
     code-buffer offsets while the owning module was unambiguous)
   - Patches each location with the final absolute address of the ep symbol
   - Handles PREL (code-relative) and DREL (data-relative) targets; the
     reference word is an ABS placeholder, so the resolved value is written
     directly and copied through unchanged by COM/page emit
   - No in-code chain, no terminator, no page-map inference, no circular
     guard: the location list is finite and each entry is self-describing

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
         ENAME     (4)   entry name
         CNAME     (5)   common block name (unused by r99)
         PNAME     (6)   program name
         LNAME     (7)   library name (unused by r99)
         EXT       (8)   extension link item
         CSIZE     (9)   common block size (unused by r99)
         XCHAIN    (10)  external reference chain head + symbol name
         EPOINT    (11)  entry point location + symbol name
         XMOFF     (12)  external reference - negative offset
         XPOFF     (13)  external reference + positive offset
         DSIZE     (14)  data segment size
         SETLC     (15)  set location counter
         CHAIN/AORG_MARK (16)  ** v3.9.27: repurposed as AORG_MARK **
                               Assembler emits this at every AORG directive.
                               type=PREL, field=absolute AORG address.
                               Linker uses it to unambiguously identify
                               absolute-origin modules in paged mode,
                               including low-address modules (e.g. XOP
                               vectors at 0x0040) that cannot be detected
                               by the PAGE_SEG threshold heuristic alone.
                               CHAIN was never emitted by r99 for real
                               chain operations so repurposing is safe.
         PSIZE     (17)  program (code) segment size
         EPROG     (18)  end of program / start address
         EFILE     (19)  end of file

 ============================================================================
 Paged Mode (-P# flag)
 ============================================================================

 The TMS99105 SBC uses a hardware memory mapper that maps 4KB segments
 to physical pages. link99 supports paged mode via -P# flags:

   link99 out.exe common.r99 -P2 overlay_a.r99 -P3 overlay_b.r99

 Modules before the first -P# flag are placed in common memory (page 0).
 Each -P# flag assigns subsequent AORG modules to the specified physical
 page. The output is a Shell V5.x EXE chain:

   [next_offset:word][page:word][start:word][size:word][data...]

 Where page=0 means common memory (no mapper programming needed) and
 page>0 means the loader programs the mapper before copying data.

 AORG module detection in paged mode:
   v3.9.13-3.9.26: detected by PREL values >= PAGE_SEG (0x1000). This
     failed for low-address AORG modules (e.g. XOP handlers at 0x0040-
     0x00FF) whose PREL values are below the threshold.
   v3.9.27: AORG_MARK item (see above) provides unambiguous detection.
     The assembler (r99 v2.1+) emits AORG_MARK at every AORG directive.
     Both detection paths are retained for backward compatibility with
     object files assembled by older r99 versions.

 EXE block size limit:
   EXE_MAX_BLOCK = 0x1F8 bytes (one 512-byte sector minus 8-byte header).
   Large modules are automatically split into multiple chain blocks, each
   aligned to a sector boundary for Shell V5.8 loader compatibility.

 Symbol names are encoded as a BITPSYM-bit length count (BITPSYM=4, so
 0-15 characters) followed by that many 8-bit characters. The maximum
 symbol length is MAXSYM=15 characters.

 External references (XREFS scheme):
   Each external symbol used by a module is emitted at end-of-module as an
   explicit group: EXT "XREFS", then XCHAIN <count>,<name>, then <count>
   CHAIN <location> items.  There is no in-code linked list and no zero
   terminator.  load() records each location (converted to a code-buffer
   offset via the module's own AORG anchor) on the symbol's xr entry;
   resolve() later writes the symbol's final absolute address into each.
   The reference word itself is an ABS 0 placeholder with no CT entry.

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
#define MAXMEM		64000;			/* start copying to disc 				*/

#define NOCCARGC                    /* do not pass arg counts to functions  */

#define NAMESIZE    15
#define MAXFIL      30
#define STACK       512             /* stack headroom reservation           */
#define AUXBUF      18192            /* aux buffer for reference file        */
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
#define EXEEXT  ".EXE"
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
/*
** v3.9.56: 128 was not enough and overflowing it CORRUPTED THE OUTPUT
** SILENTLY -- split_page_blocks() printed a warning and RETURNED,
** abandoning every remaining chunk unwritten, so the EXE linked clean
** with pieces of the program missing and the board executed whatever
** the loader left.
**
** 192 covers the real need with margin: the M33a image is ~141 chunks
** (28KB resident + nine overlay pages). Raise if a future image needs
** more; the arrays cost 10 bytes/slot in global static plus 8/slot in
** write_exe's stack frame.
*/
#define MAXPMODS    192             /* max modules/chunks tracked in page map */
#define PMTERM      0xFFFF          /* page-map table terminator word       */
#define EXE_MAX_BLOCK 0x01F8          /* one sector minus header; aligns blocks to sector boundaries */

/*
 * Internal CT$ type, never present in an R99 input stream.
 *
 * PREL values from an AORG body are already final virtual addresses.
 * Ordinary PREL values are converted by load() to link-buffer addresses
 * and still require the page-map entry's runtime-minus-buffer delta when
 * emitted. Keeping these cases distinct prevents a resident value such
 * as >1856 from being mistaken for an AORG address merely because it is
 * numerically above PAGE_SEG.
 */
#define CT_AORG_PREL 0x41

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
#ifdef int16
#define NXT     0                       /* next-entry pointer (2 bytes)     */
#define VAL     2                       /* value field (2 bytes)            */
#define SYM     4                       /* symbol name (MAXSYM+1 bytes)     */
#define FLG     (SYM + MAXSYM + 1)      /* flag byte                        */
#define SSZ     (SYM + MAXSYM + 2)      /* total entry size                 */
#define HIGH    0x7f                    /* sentinel: end of ASCII table     */
#define CUSHION (200 * SSZ)             /* headroom before overflow         */
#define FLGEXT  0x80                    /* flag: symbol is external         */
#define FLGAORG 0x40                    /* flag: paged AORG entry point --  */
                                        /* VAL is a final virtual address,  */
                                        /* never add cbase (see newsym)     */
char high[] = { HIGH, 0 };
#endif

#ifdef int32
#define NXT     0                       /* next-entry pointer (4 bytes)     */
#define VAL     4                       /* value field (4 bytes)            */
#define SYM     8                       /* symbol name (MAXSYM+1 bytes)     */
#define FLG     (SYM + MAXSYM + 1)      /* flag byte                        */
#define SSZ     (SYM + MAXSYM + 2)      /* total entry size                 */
#define HIGH    0x7f                    /* sentinel: end of ASCII table     */
#define CUSHION (400 * SSZ)             /* headroom before overflow         */
#define FLGEXT  0x80                    /* flag: symbol is external         */
#define FLGAORG 0x40                    /* flag: paged AORG entry point --  */
                                        /* VAL is a final virtual address,  */
                                        /* never add cbase (see newsym)     */
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
char           symbol[MAXSYM + 1]; /* current symbol name (sized off MAXSYM) */

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
unsigned crelptr;       /* current entry index in code relocation table */
unsigned drelptr;       /* current entry index in data relocation table */

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
unsigned pmbase[MAXPMODS];    /* original virtual base for EXE split chunks */
unsigned pmmin_addr;          /* min PREL value seen in current module */
unsigned cloc_at_ename;        /* cloc saved at ENAME - used for cmod at PSIZE */
unsigned pmmin_global;         /* min PREL value seen across all modules */
unsigned first_module;         /* YES while loading first module for pmmin_global */
unsigned pmmin_valid;         /* non-zero if pmmin_addr is set         */
unsigned pm_modidx;           /* npmods index of current paged module  */
unsigned pm_modsize;          /* size of current paged module          */
unsigned pm_aorg_base;        /* explicit AORG base captured from AORG_MARK */
int      pm_aorg_valid;       /* non-zero if current module used AORG   */
unsigned pm_aorg_cloc;        /* v3.9.35 (2.3): buffer offset (cloc) at the
                               * accepted AORG_MARK - where absolute code
                               * actually begins. Pre-mark bytes are
                               * relocatable preamble, NOT page content.  */
unsigned pm_aorg_true;        /* TRUE (unrounded) AORG address - the real
                                * load address. pm_aorg_base is page_align()'d
                                * and must stay that way for PREL/SETLC offset
                                * stripping - never use it as a load address. */
int      pm_saw_abs_prel;     /* v3.9.30: YES if this module has a genuinely
                                * absolute (AORG) PREL - field was >= PAGE_SEG
                                * BEFORE cmod was added. Relocatable modules
                                * never do; gates the AORG-fixup heuristic so a
                                * relocatable module loaded high in the buffer
                                * (e.g. call.r99) is not mis-based.            */
int            pmpage[MAXPMODS];    /* page number for this module (0-15)   */
int            pmpgend[MAXPMODS];   /* v3.9.57: last page this module spans  */
int            pmspan[MAXPMODS];    /* v3.9.57: 1 = module DECLARED a span   */
int            npmods;              /* number of page-map entries recorded  */
int            curpage;             /* current page assignment (-P switch)  */
int            curpgend;            /* v3.9.57: last page of a -Plo-hi span  */
                                    /*   -P10    -> curpage=10 curpgend=10   */
                                    /*   -P10-12 -> curpage=10 curpgend=12   */
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
extern putls();
extern int  avail();
extern char *malloc();
extern poll();
extern int  getarg();
extern int  extend();
extern error();
extern error2();
extern int  getrel();
extern ifilelbuf();
extern delete();
extern itox();
extern itou();
extern int  xtoi();
extern int  utoi();
extern puts2();
extern puts3();
extern int  ferror2();
extern int  getint();
extern putint();
extern int  get16int();
extern put16int();

/* v3.9.12: SHELL v57 linked-list EXE block writer for paged output. */
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
 * page_align  -  Round a virtual address down to its 4KB page boundary.
 *
 *  This is a pure calculation with no side effects - it exists only to
 *  give the rounding formula ONE definition instead of four separately
 *  hand-typed copies (which had drifted into inconsistent use: two
 *  sites correctly kept the TRUE address around for use as a load
 *  address, two others didn't).
 *
 *  IMPORTANT: the return value of this function must NEVER be used as
 *  a load address (pmstart[]/pmend[]). It exists ONLY for stripping
 *  the page-relative offset out of PREL/SETLC records, where the
 *  in-page offset needs to be measured from the page's own start.
 *  Using it as a load address silently discards everything below the
 *  page boundary - e.g. an AORG of 0x3A22 becomes 0x3000, which is
 *  wrong for any module whose AORG isn't already page-aligned.
 * ========================================================================== */
unsigned page_align(addr)
	unsigned addr;
{
	if (addr >= PAGE_SEG)
		return (addr & ~((PAGE_SEG - 1)));
	return addr;
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
				/* overlapping or adjacent -- merge j into i             */
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
 *  Non-paged programs emit the sentinel only -- loader treats this as
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
			abs_pg = pmpage[i];
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
		/* v3.9.53 (revised): the first attempt guarded on
		 * field >= PAGE_SEG here, repeating at emit time exactly the
		 * heuristic the load()-side comment forbids: an ORDINARY
		 * resident PREL word whose target sits past buffer offset
		 * >1000 (IOCORE fmptr >1856; in Step 6 most of the resident
		 * image) is numerically >= PAGE_SEG too, and would have been
		 * emitted one cbase LOW.  Provenance already exists: load()
		 * tags AORG-body PREL words CT_AORG_PREL (v3.9.52), so use
		 * it, matching the phase-2 chain emitter's contract:
		 *
		 *   PREL          CT value is a link-buffer address: + cbase
		 *   CT_AORG_PREL  CT value is final virtual: preserve
		 *
		 * (The 2026-07 double-relocation of level13/zerojump/store/
		 * level1/primary was in resolve(), not here -- see newsym()
		 * FLGAORG.)  xrplus always applies: it is an offset WITHIN
		 * the target, not a relocation.                            */
		if (rtbuf[4] == PREL)
			field += xrplus + cbase;
		if (rtbuf[4] == CT_AORG_PREL)
			field += xrplus;
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
 *      so gaps between modules on the same page need not be preserved --
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
	int             szpos;
	char            rtbuf[CRELSIZE];
	char            str[6];

	/* --- Reserve space for the size word; fill it in after emit ---    */
	sz    = 0;
	szpos = lseek(outfd, 0, SEEK_CUR);
	write99(outfd, &sz, 2);             /* placeholder -- patched below  */

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

		/* v3.9.53 (revised): the first attempt guarded on
		 * field >= PAGE_SEG here, repeating at emit time exactly the
		 * heuristic the load()-side comment forbids: an ORDINARY
		 * resident PREL word whose target sits past buffer offset
		 * >1000 (IOCORE fmptr >1856; in Step 6 most of the resident
		 * image) is numerically >= PAGE_SEG too, and would have been
		 * emitted one cbase LOW.  Provenance already exists: load()
		 * tags AORG-body PREL words CT_AORG_PREL (v3.9.52), so use
		 * it, matching the phase-2 chain emitter's contract:
		 *
		 *   PREL          CT value is a link-buffer address: + cbase
		 *   CT_AORG_PREL  CT value is final virtual: preserve
		 *
		 * (The 2026-07 double-relocation of level13/zerojump/store/
		 * level1/primary was in resolve(), not here -- see newsym()
		 * FLGAORG.)  xrplus always applies: it is an offset WITHIN
		 * the target, not a relocation.                            */
		if (rtbuf[4] == PREL)
			field += xrplus + cbase;
		if (rtbuf[4] == CT_AORG_PREL)
			field += xrplus;
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
 * Multi-line command line support
 *
 *  A trailing '^' on the last command-line argument tells link99 that the
 *  command continues on the next input line, e.g.
 *
 *      link99 -O1000 -M skeltest.EXE ^
 *          skeltest.R99 ^
 *          ovlmgr.R99 ^
 *          ...
 *          -P5 cc_codegen.R99
 *
 *  A trailing comma (,) works the same way as '^', e.g.
 *
 *      link99 -O1000 -M skeltest.EXE ,
 *          skeltest.R99 ,
 *          -P5 cc_codegen.R99
 *
 *  Continuation lines are read from standard input; each line that itself
 *  ends with '^' or ',' continues onto the following line.  The tokens collected
 *  are appended to argv before getsw()/phase1() ever see the arguments,
 *  so the rest of the linker is completely unaware of the mechanism.
 * ======================================================================== */
#define MAXXARG  128                /* max total arguments after expansion  */
#define XARGBUF  4096               /* text pool for continuation tokens    */
#define ISCONT(c) ((c) == '^' || (c) == ',')   /* continuation characters   */

char *xargv[MAXXARG];               /* expanded argument vector             */
char  xargbuf[XARGBUF];             /* backing store for new tokens         */

/* --------------------------------------------------------------------------
 * getcmdline  -  Read one line from standard input (fd 0).
 *                Strips CR/LF.  Returns the line length, or EOF on
 *                end of input.
 * ------------------------------------------------------------------------ */
int getcmdline(buf, max)
	char *buf; int max;
{
	int  n, got;
	char c;

	n = 0;
	got = NO;
	while (read(0, &c, 1) == 1) {
		got = YES;
		if (c == '\n')
			break;
		if (c != '\r' && n < max - 1)
			buf[n++] = c;
	}
	buf[n] = 0;
	return (got ? n : EOF);
}

/* --------------------------------------------------------------------------
 * expand_args  -  Build the expanded argument vector in xargv[].
 *                 Returns the new argument count.
 * ------------------------------------------------------------------------ */
int expand_args(argc, argv)
	int argc; char **argv;
{
	char  line[256];
	char *p;
	int   n, i, len, cont;

	n = 0;
	for (i = 0; i < argc && n < MAXXARG; i++)
		xargv[n++] = argv[i];

	/* Does the last original argument request continuation?              */
	cont = NO;
	if (n > 0) {
		p   = xargv[n - 1];
		len = 0;
		while (p[len])
			len++;
		if (len && ISCONT(p[len - 1])) {
			p[len - 1] = 0;
			if (!p[0])
				n--;            /* bare "^" or "," argument - drop it   */
			cont = YES;
		}
	}

	p = xargbuf;
	while (cont) {
		cont = NO;
		putls("link99* ");      /* continuation prompt                  */
		len = getcmdline(line, 256);
		if (len == EOF)
			break;

		/* trim trailing blanks */
		while (len && (line[len-1] == ' ' || line[len-1] == '\t'))
			line[--len] = 0;

		/* line ends with '^' or ',' - continue onto the next line       */
		if (len && ISCONT(line[len-1])) {
			line[--len] = 0;
			cont = YES;
			while (len && (line[len-1] == ' ' || line[len-1] == '\t'))
				line[--len] = 0;
		}

		/* tokenize on blanks and append to xargv[]                       */
		i = 0;
		while (i < len) {
			while (line[i] == ' ' || line[i] == '\t')
				i++;
			if (!line[i])
				break;
			if (n >= MAXXARG)
				error("\n- Too many arguments");
			xargv[n++] = p;
			while (line[i] && line[i] != ' ' && line[i] != '\t') {
				if (p >= xargbuf + XARGBUF - 1)
					error("\n- Command line too long");
				*p++ = line[i++];
			}
			*p++ = 0;
		}
	}
	return (n);
}

/* ==========================================================================
 * main
 * ======================================================================== */
int main(argc, argv)
	int argc; char **argv;
{
	putls("----------------------------------------------------\n");
	putls("TMS9900 Relocatable Object Linker  Version 3.9.56\n");
	putls("Original CP/M version: Alexander Cameron, January 1985\n");
	putls("MSDOS/PC port:         Alexander Cameron, May 2010 - July 2019\n");
	putls("Explicit XREFS + PREL/AORG CT provenance\n");
	putls("----------------------------------------------------\n");

	argc = expand_args(argc, argv); /* '^' multi-line continuation  */
	argv = xargv;

	getsw(argc, argv);      /* parse command-line switches  */
	getmem();               /* initialise memory buffers    */
	phase1(argc, argv);     /* pass 1: load and link        */
	if (!okay())
		error("\nQuitting with unresolved symbols.");
	/* v3.9.54: memory usage report - make buffer exhaustion a number
	 * watched approaching, not a silent hang at some future link.     */
	{
		char msz[9];
		putls("Memory: code ");
		itou((bpnext - buffer), msz, 8);
		putls(msz);
		putls(" symbols ");
		itou(((buffer + memsize) - snext), msz, 8);
		putls(msz);
		putls(" free ");
		itou((snext - bpnext), msz, 8);
		putls(msz);
		putls(" bytes\n");
	}

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
getsw(argc, argv)
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
				 * in the phase1 loop so each module gets the right page.
				 *
				 * v3.9.55: parse ALL the digits. This read arg[2] alone,
				 * so "-P10" validated as page 1 and the trailing '0' was
				 * discarded -- see the matching fix in the phase1 loop. */
				int pg;
				if (utoi(arg + 2, &pg) <= 0)
					usage();
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

			if (pass == 1 && (*(epnext + FLG) & ~FLGAORG & 0xFF) != PREL) {
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
 * XREFS scheme: explicit external-reference ingest support
 * ======================================================================== */
int   in_xrefs  = NO;      /* currently inside an EXT "XREFS" group          */
int   xref_left = 0;       /* CHAIN reference locations still expected       */
char *xref_sym  = 0;       /* xr entry receiving the current group's nodes   */

/* Allocate one symbol-table-sized node (reuses the NXT + VAL slots) to hold
 * one converted reference location.  Same allocator newsym() uses.          */
char *xnode_alloc()
{
	char *n;
	if ((n = sfree))
		sfree = getint(sfree);
	else {
		n = snext;
		snext -= SSZ;
		if (snext < bpnext)
			error("- Must Specify -B Switch\n");
	}
	return n;
}

/* ==========================================================================
 * load  -  Load one module from the current REL file into the code buffer
 *          (or into the overflow file once in-memory space is exhausted).
 * ======================================================================== */
load()
{
	char           str[9];
	char           rtbuf[CRELSIZE]; /* scratch for relocation table entries   */
	unsigned   doffloc, coffloc, creloff, dreloff;
	int            gval;            /* return value from getrel()             */
	int            aorg_prel;      /* CT provenance for current PREL word     */
	unsigned base;            /* temp for page map base calculation     */
	unsigned pmceil;          /* v3.9.57: ceiling of the declared span  */

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
	pm_aorg_valid = NO;
	pm_aorg_base  = 0;
	pm_aorg_true  = 0;
	pm_aorg_cloc  = 0;
	pm_saw_abs_prel = NO;

	do {
		poll(YES);
		gval = getrel();

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
			/*
			 * Preserve PREL provenance in CT$.
			 *
			 * Ordinary relocatable PREL:
			 *     CT value = module-relative value + cmod
			 *              = link-buffer target address
			 *
			 * AORG PREL:
			 *     CT value = final virtual target address, unchanged
			 *
			 * The old field>=PAGE_SEG test cannot be repeated during emit:
			 * a normal resident link-buffer target can also be >= PAGE_SEG
			 * (IOCORE fmptr was >1856). CT_AORG_PREL carries the
			 * distinction explicitly to Phase 2.
			 */
			aorg_prel = NO;
			if (item == PREL) {
				if (pagemode && pm_aorg_valid &&
				    cloc >= pm_aorg_cloc) {
					aorg_prel = YES;       /* explicit AORG_MARK provenance */
					/* field is already the final virtual address */
				} else if (pagemode && curpage != 0 &&
				           pm_aorg_valid && field >= PAGE_SEG) {
					/* v3.9.54: curpage gate added - see EPOINT note. */
					/*
					 * Backward compatibility for older R99 files without
					 * AORG_MARK.  Only an AORG (overlay-page) module can
					 * carry a final virtual >= PAGE_SEG here; a page-0
					 * resident word >= PAGE_SEG is just a buffer offset
					 * past the first 4K and must still relocate normally.
					 */
					aorg_prel = YES;
					pm_saw_abs_prel = YES;
				} else if (type != 3 || field != 0) {
					field += cmod;
				}
			}
			if (item == DREL)
				field += dmod;

			/* Track minimum PREL value to detect AORG base address.
			 * PSIZE fires before PREL in the file; EPROG fixes up pmstart. */
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
				if (item == PREL && aorg_prel)
					rtbuf[4] = CT_AORG_PREL;
				else
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

			/*
			 * Finalise the page-map entry for the module just loaded.
			 *
			 * Do NOT use pmmin_addr here.  pmmin_addr is only the lowest
			 * relocated PREL reference seen inside the module; it is not the
			 * module's load origin.  OVLTEST proved this: pmmin_addr became
			 * >1022 instead of the true AORG >1000, and OVLA/OVLB became
			 * >2008 instead of the true AORG >2000.
			 *
			 * Only confirmed AORG state may revise the PSIZE-created page-map
			 * entry.  Confirmed AORG state comes from AORG_MARK when present,
			 * or from absolute PREL EPOINT values for older R99 files.
			 */
			if (pagemode && pm_aorg_valid && pm_modidx < npmods) {
				base = pm_aorg_true ? pm_aorg_true : pm_aorg_base;
				pmstart[pm_modidx] = base;
				/* v3.9.35 (2.3): the page's buffer anchor is where the
				 * AORG took effect, not the module start. Pre-AORG
				 * preamble bytes remain in the relocatable stream.     */
				if (pm_aorg_cloc > pmcmod[pm_modidx])
					pmcmod[pm_modidx] = pm_aorg_cloc;
				pmend[pm_modidx]   = base + (cloc - pmcmod[pm_modidx]);
				/* v3.9.38 (spec 2.5): a paged module's bytes must all
				 * live under ONE mapping register. Bytes past the 4KB
				 * boundary are emitted through - and called through -
				 * whatever page happens to occupy the NEXT segment's
				 * register, silently corrupting it. This was a polite
				 * Note for weeks while it broke builds. Now it is
				 * fatal, with instructions.                            */
				/*
				 * v3.9.57: a module linked -Plo-hi DECLARED that it may
				 * occupy several consecutive pages, so its ceiling is the
				 * end of its LAST declared page.  split_pagemap_for_exe
				 * breaks its blocks at each boundary and pages each one
				 * correctly, so the bytes are NOT emitted through the
				 * wrong register.
				 *
				 * This check runs when the AORG is applied, long before
				 * the splitter sees the module - so without the span
				 * allowance here the module is rejected before the span
				 * support can do anything.
				 *
				 * Undeclared modules keep the one-segment rule exactly.
				 */
				pmceil = (pmstart[pm_modidx] & 0xF000) + 0x1000;
				if (pmspan[pm_modidx])
					pmceil = pmceil +
					    (unsigned)(pmpgend[pm_modidx] -
					               pmpage[pm_modidx]) * 0x1000;
				if (pmend[pm_modidx] > pmceil) {
					putls("\n*** FATAL (spec 2.5): module ");
					putls(modname);
					putls(" crosses a 4KB segment boundary by ");
					itox(pmend[pm_modidx] - pmceil, str, 5);
					putls(str);
					putls(" bytes.\n    Widen -Plo-hi, split the module on a");
					putls(" natural seam, or move shared helpers resident.\n");
					exit(1);
				}
				if (monitor) {
					putls("  AORG SETLC fixup: base=");
					itox(base, str, 5); putls(str);
					putls(" end=");
					itox(pmend[pm_modidx], str, 5); putls(str);
					putls("\n");
				}
			}

			if (type == PREL) {
				puts2("Start In ", modname);
				goloc = field + cmod;
			}
			break;

		case ENAME:
			/* Reset per-module PREL/AORG tracking at start of each module */
			pmmin_valid    = NO;
			pmmin_addr     = 0xFFFF;
			pm_aorg_valid  = NO;
			pm_aorg_base   = 0;
			pm_aorg_true   = 0;
			pm_aorg_cloc   = 0;
			cloc_at_ename = cloc;  /* save cloc before SETLC records */
			pm_saw_abs_prel = NO;
			break;  /* entry names handled during search/library phase       */

		case EXT:
			/* XREFS marker; the XCHAIN header that follows carries the
			 * count and drives group parsing.  Nothing to do here.      */
			break;

		case XCHAIN:
			/* XREFS group header: field = reference count, symbol = name.
			 * Create one xr entry with an EMPTY location list; the CHAIN
			 * items that follow supply the reference locations.          */
			{
				unsigned count = field;
				field = 0;                 /* xr VAL = empty list head    */
				type  = ABS;               /* stop newsym rebasing zero   */
				newsym(&xrprev, xrfirst, "xr");
				xref_sym = xrprev;         /* newsym left *prev = new ent */
				putint(xref_sym + VAL, 0);
				*(xref_sym + FLG) = PREL;  /* treat as a code external    */
				xref_left = count;
				in_xrefs  = (count != 0);
			}
			break;

		case EPOINT:
			/*
			 * Backward-compatible AORG detection for older R99 files that
			 * do not emit AORG_MARK.  Absolute PREL entry points are reliable
			 * load-origin evidence; ordinary relocatable COM-style modules have
			 * small entry offsets here and must not be treated as AORG.
			 */
			/* v3.9.54: gated on curpage != 0. A module NOT under an
			 * explicit -P assignment is resident: its entry points are
			 * module-relative OFFSETS, and any module larger than 4KB
			 * has offsets >= PAGE_SEG that are NOT load-origin
			 * evidence. Without this gate, a resident module was
			 * AORG-classified the moment it outgrew 4KB, and its
			 * initialized data was corrupted (the CC_DATA incident). */
			if (pagemode && curpage != 0 && type == PREL && field >= PAGE_SEG) {
				base = page_align(field);
				if (!pm_aorg_valid || field < pm_aorg_true) {
					pm_aorg_valid = YES;
					pm_aorg_true  = field;
					pm_aorg_base  = base;
				}
			}
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
					base            = (pages_next << 12);
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
				/* v3.9.57: carry the DECLARED span, not an inferred one. */
				pmpgend[npmods] = (pmpage[npmods] == curpage)
				                ? curpgend : pmpage[npmods];
				pmspan[npmods]  = (pmpage[npmods] != 0 &&
				                   pmpgend[npmods] > pmpage[npmods]);
				pm_modidx  = npmods;         /* remember for EPROG fixup       */
				pm_modsize = field;
				++npmods;
			} else {
				/* v3.9.56: FATAL -- an unrecorded module is not emitted.
				** BRACES ARE REQUIRED: without them only the first putls
				** belongs to the else and exit(1) runs for EVERY module,
				** killing the link at module 1. */
				putls("\n*** FATAL: page map full, module not recorded\n");
				putls("    Raise MAXPMODS in link99.c and rebuild.\n");
				exit(1);
			}
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
				/* v3.9.54: overflow engagement is ALWAYS announced.
				 * This path is suspect in the pre-64K silent hang; if
				 * you see this, prefer enlarging getmem() instead.    */
				putls("\n*** WARNING: code overflow to DISK engaged at ");
				itox(cdisk, str, 8);
				putls(str);
				putls("\n    The in-memory buffer is exhausted. The disk\n");
				putls("    overflow path is little-tested on this port -\n");
				putls("    prefer enlarging the buffer in getmem().\n");
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
				/* AORG modules assigned with -P emit absolute virtual
				 * addresses in SETLC (BSS) records. Strip the page base
				 * before adding cmod.
				 *
				 * CRITICAL: ordinary resident modules also have perfectly
				 * valid module-relative SETLC values above >1000 when their
				 * BSS exceeds 4KB. Those values must be preserved in full.
				 * The old unconditional field>PAGE_SEG test packed CC1's
				 * BSS out of the image, while EPOINT main retained its full
				 * logical address. Match the v3.9.54 provenance rule: only
				 * an explicitly paged module may use the AORG heuristic.   */
				if (pagemode && curpage != 0 && field >= PAGE_SEG) {
					unsigned aorg_base;
					/* v3.9.18: remember explicit AORG SETLC base for EXE block
					 * virtual start.  Do not alter resolve() behaviour here.
					 * NOTE: deliberately do NOT touch pm_aorg_true here.
					 * This handler fires for every PREL record in the
					 * module (unfiltered, always-overwrite) - for a large
					 * module that's dozens of unrelated internal addresses,
					 * not a meaningful "true start". Rounding made this
					 * safe by accident (everything in one segment rounds
					 * to the same page base); pm_aorg_true must not adopt
					 * that same unfiltered signal. Only AORG_MARK (fires
					 * once per explicit AORG directive, with proper
					 * lowest-wins comparison) is a reliable source for it. */
					aorg_base = page_align(field);
					if (pagemode) {
						pm_aorg_base  = aorg_base;
						pm_aorg_valid = YES;
					}
					field = (field - aorg_base);
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
		/* ---- AORG marker: explicit absolute ORG address ----------------- */
		case AORG_MARK:                 /* == CHAIN == 16 */
			if (in_xrefs) {
				/* reference location inside an XREFS group: convert the
				 * runtime/AORG location to THIS module's packed code-buffer
				 * offset before resolve() direct-patches it.
				 *
				 * Older R99 files do not emit AORG_MARK.  In that case
				 * SETLC can set pm_aorg_valid/pm_aorg_base before any EPOINT
				 * has supplied pm_aorg_true, while pm_aorg_cloc is still 0.
				 * The old test therefore selected obuf=0, ovrt=0 and stored
				 * the virtual XREF location (e.g. >906E) as though it were a
				 * packed-buffer offset.  resolve() then patched the wrong
				 * bytes and the emitted overlay retained its ABS 0 operand.
				 *
				 * Use the explicit AORG buffer anchor only when it actually
				 * exists; otherwise the module's cmod is the packed-buffer
				 * anchor.  For old page-aligned overlay R99 files, the SETLC
				 * page base is the runtime anchor until EPOINT refines it. */
				unsigned loc = field;
				unsigned xbuf;

				/*
				 * XREFS groups are emitted near the front of older R99
				 * modules, before PSIZE/SETLC has established cmod or
				 * pm_aorg_base.  cloc_at_ename, however, is already the
				 * packed-buffer anchor for THIS module.
				 *
				 * For an explicitly paged module the XREF location is an
				 * absolute virtual address (e.g. >9056).  Old R99 files
				 * have no AORG_MARK, so derive the 4K virtual segment
				 * directly from the reference location and translate to
				 * the packed-buffer address.  Spec 2.5 later enforces that
				 * one paged module cannot cross a 4K segment boundary.
				 *
				 * This must NOT use cmod here: before PSIZE, cmod can still
				 * belong to the previous module (or be zero).  That was why
				 * an XREF such as >9056 was stored literally as buffer
				 * offset >9056.  It appeared to work only when packed code
				 * happened to occupy the same offset; changing an earlier
				 * overlay size then broke unrelated later XREFs.
				 */
				if (pagemode && curpage != 0 && loc >= PAGE_SEG) {
					if (pm_aorg_valid && pm_aorg_cloc && pm_aorg_true)
						xbuf = pm_aorg_cloc + (loc - pm_aorg_true);
					else
						xbuf = cloc_at_ename + (loc - page_align(loc));
				}
				else {
					/* Resident/ordinary module XREF locations are relative
					 * to the module start. */
					xbuf = cloc_at_ename + loc;
				}
				char *node = xnode_alloc();
				putint(node + NXT, getint(xref_sym + VAL)); /* prepend    */
				putint(node + VAL, xbuf);
				putint(xref_sym + VAL, node);
				if (--xref_left == 0)
					in_xrefs = NO;
				break;
			}
			/* v3.9.27: assembler emits this at every AORG directive.
			 * field = absolute virtual address of the AORG.
			 * Use lowest value seen as the module's base address.       */
			if (pagemode) {
				unsigned aorg_base;
				aorg_base = page_align(field);
				if (!pm_aorg_valid || aorg_base < pm_aorg_base) {
					pm_aorg_base  = aorg_base;
					pm_aorg_valid = YES;
					pm_aorg_true  = field;   /* true addr, before rounding */
					/* v3.9.35 (2.3): absolute code begins at the CURRENT
					 * buffer position - bytes loaded before this item are
					 * relocatable preamble and stay out of the page.      */
					pm_aorg_cloc  = cloc;
				}
				if (monitor) {
					putls("  AORG_MARK: addr=");
					itox(field, str, 5); putls(str);
					putls(" base=");
					itox(pm_aorg_base, str, 5); putls(str);
					putls("\n");
				}
			}
			break;
		}
	} while (item != EPROG);

#ifdef DEBUG2
	{
		char *bp, *rp;
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
			printf("%02X ", *bp++ & 0xFF);
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
	int   aorgsym;
#ifdef DEBUG
	char at[MAXSYM + 1];            /* itox() below prints MAXSYM digits       */
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

	/* v3.9.19: In paged mode AORG symbols (>= PAGE_SEG) are already
	 * runtime virtual addresses.  Do not add the code-buffer module
	 * offset to them, or RUN99-style overlay XCHAINs become e.g. >298A
	 * instead of the real runtime location >238A.  Relocatable symbols
	 * and PREL zero still receive cmod as before. */
	aorgsym = 0;
	if (type == PREL) {
		/* Resident modules (curpage == 0) can legitimately have entry
		 * offsets >= PAGE_SEG once the module exceeds 4KB.  Those are
		 * still module-relative and MUST receive cmod here and cbase
		 * later in resolve().  Only an explicitly paged module can use
		 * a >=PAGE_SEG PREL entry as an already-final AORG virtual. */
		if (pagemode && curpage != 0 && field >= PAGE_SEG)
			aorgsym = 1;    /* AORG virtual: cmod not added; remember it */
		else
			field += cmod;
	}
	if (type == DREL) field += dmod;

	putint(newent + VAL, field);
	strcpy(newent + SYM, symbol);
	*(newent + FLG) = type;
	/* v3.9.53: the cmod-skip branch above is the ONLY place the linker
	 * knows a symbol's value is a final AORG virtual rather than a
	 * buffer offset.  Once csize grows past the first AORG page the two
	 * are numerically indistinguishable, so the knowledge must be kept
	 * here, not re-derived from value ranges later (resolve() tried,
	 * and the same symbol resolved differently depending on WHEN it
	 * resolved).                                                      */
	if (aorgsym)
		*(newent + FLG) |= FLGAORG;

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
					cbase = (pages_min << 12);
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
					/* -P#: explicit page for following module(s)
					 *
					 * v3.9.55: PARSE ALL THE DIGITS.
					 *
					 * This was "curpage = infn[2] - '0'" -- a single
					 * digit. "-P10" therefore silently became page 1
					 * and the '0' was dropped. The module's bytes went
					 * to page 1 while DREL's OVL_TABLE (which parses
					 * the number correctly) said page 10, so at run
					 * time the mapper faithfully mapped a page that had
					 * never been written and the CPU executed garbage.
					 *
					 * Cost the M33a preprocessor overlay a debugging
					 * session: CC_MACS at -P10 landed in page 1 and
					 * >9000 held nothing. Pages 0-9 had always worked,
					 * which is why this survived eight milestones.
					 *
					 * A page number out of range is now fatal rather
					 * than silently truncated.                        */
					int pg, pgend, nlen;
					char *pp;
					nlen = utoi(infn + 2, &pg);
					if (nlen <= 0 || pg < 0 || pg > 15) {
						putls("\n*** FATAL: bad -P page number: ");
						putls(infn);
						putls("\n    Valid range is -P0 to -P15.\n");
						exit(1);
					}
					/*
					 * v3.9.57: optional page SPAN, -Plo-hi.
					 *
					 * -P10     one page, exactly as before.  Nothing in the
					 *          emitter behaves differently for a single page,
					 *          so existing images are unchanged.
					 * -P10-12  the module MAY occupy pages 10,11,12.  The
					 *          author is stating the intent; the linker does
					 *          not guess it from addresses.
					 */
					pgend = pg;
					pp    = infn + 2 + nlen;
					if (*pp == '-') {
						pp++;
						if (utoi(pp, &pgend) <= 0 || pgend < pg || pgend > 15) {
							putls("\n*** FATAL: bad -P page span: ");
							putls(infn);
							putls("\n    Use -Plo-hi with 0 <= lo <= hi <= 15.\n");
							exit(1);
						}
					}
					curpgend      = pgend;
					curpage       = pg;
					pages_auto    = NO;
					pagemode      = YES;
					prev_was_page = YES;
					if (monitor) {
						itou(curpage, sz, 3);
						puts2("\nPage mode: curpage=", sz);
						if (curpgend != curpage) {
							itou(curpgend, sz, 3);
							puts2(" spanning to page ", sz);
						}
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
				continue;           /* skip -- already handled as output name  */
			}
		}

		lib = extend(infn, MODEXT, LIBEXT) ? YES : NO;

		/* Libraries default to page0 unless explicitly tagged with -P.
		 * This prevents the common mistake of -P1 module.OBJ lib.LIB
		 * accidentally tagging the entire library as page1.
		 * A -P flag immediately before the library overrides this.      */
		if (lib) {
			/* Check if the PREVIOUS argument was a -P flag by testing
			 * whether curpage was just set -- we do this by saving the
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
	/* v3.9.40: compare actual resident runtime spans with actual
	 * non-zero-page runtime spans. csize is the packed linker-buffer
	 * size and includes overlay images, so cbase+csize is not a valid
	 * resident-runtime end address. */
	if (pagemode) {
		int ri, pi;

		for (ri = 0; ri < npmods; ri++) {
			if (pmpage[ri] != 0 || pmend[ri] <= pmstart[ri])
				continue;

			for (pi = 0; pi < npmods; pi++) {
				if (pmpage[pi] == 0 || pmend[pi] <= pmstart[pi])
					continue;

				/* Half-open ranges [start,end) overlap iff each starts
				 * before the other ends. */
				if (pmstart[ri] < pmend[pi] &&
				    pmstart[pi] < pmend[ri]) {
					putls("\n*** FATAL: resident range ");
					itox(pmstart[ri], sz, 5); putls(sz);
					putls("-");
					itox(pmend[ri] - 1, sz, 5); putls(sz);
					putls(" overlaps paged range ");
					itox(pmstart[pi], sz, 5); putls(sz);
					putls("-");
					itox(pmend[pi] - 1, sz, 5); putls(sz);
					putls(" on page ");
					itox(pmpage[pi], sz, 3); putls(sz);
					putls(".\n");
					exit(1);
				}
			}
		}
	}
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
	int             szpos;
	char            rtbuf[CRELSIZE];
	char            str[6];

	/* --- Reserve space for the size word; fill it in after emit ---    */
	sz    = 0;
	szpos = lseek(outfd, 0, SEEK_CUR);
	write99(outfd, &sz, 2);             /* placeholder -- patched below  */

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

		/* v3.9.53 (revised): the first attempt guarded on
		 * field >= PAGE_SEG here, repeating at emit time exactly the
		 * heuristic the load()-side comment forbids: an ORDINARY
		 * resident PREL word whose target sits past buffer offset
		 * >1000 (IOCORE fmptr >1856; in Step 6 most of the resident
		 * image) is numerically >= PAGE_SEG too, and would have been
		 * emitted one cbase LOW.  Provenance already exists: load()
		 * tags AORG-body PREL words CT_AORG_PREL (v3.9.52), so use
		 * it, matching the phase-2 chain emitter's contract:
		 *
		 *   PREL          CT value is a link-buffer address: + cbase
		 *   CT_AORG_PREL  CT value is final virtual: preserve
		 *
		 * (The 2026-07 double-relocation of level13/zerojump/store/
		 * level1/primary was in resolve(), not here -- see newsym()
		 * FLGAORG.)  xrplus always applies: it is an offset WITHIN
		 * the target, not a relocation.                            */
		if (rtbuf[4] == PREL)
			field += xrplus + cbase;
		if (rtbuf[4] == CT_AORG_PREL)
			field += xrplus;
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



/* ============================================================================
 * split_pagemap_for_exe  -  Split large page-map spans into loader-safe chunks.
 *
 *  SHELL v57 stages two sectors at 0500-08FF.  It can reload during a block,
 *  but the next-header calculation still uses the original staged header
 *  address.  Therefore a block whose data crosses the staging boundary can
 *  leave the next header calculation pointing past the real current staging
 *  position.  OVLTEST never hit this because its blocks are small; BASIC99's
 *  page-0 block is >0x0400 and exposes it.
 *
 *  This routine is generic: it operates only on pmstart/pmend/pmcmod/pmpage.
 *  It does not know module names or symbols.  Each split span becomes another
 *  ordinary EXE chain block with the same physical page and an advanced
 *  virtual start address.
 * ========================================================================== */
split_pagemap_for_exe()
{
	int i, j, oldn;
	unsigned start, end, mod, remain, chunk, vbase;
	unsigned tonext;              /* v3.9.57: bytes to the next page boundary */
	int pg, pgend, spans;

	oldn = npmods;
	for (i = 0; i < oldn; i++) {
		if (pmend[i] == pmstart[i])
			continue;
		start  = pmstart[i];
		end    = pmend[i];
		mod    = pmcmod[i];
		pg     = pmpage[i];
		pgend  = pmpgend[i];
		vbase  = start;
		pmbase[i] = vbase;
		remain = end - start;

		/*
		 * v3.9.57: does this module DECLARE a page span?
		 *
		 * Only a module linked -Plo-hi with hi > lo is split at page
		 * boundaries and has its chunks renumbered.  Everything else -
		 * page 0, -P10, -PAGES - takes exactly the path it always did,
		 * so its output is unchanged.  The linker never infers a
		 * boundary from an address; it is told where they are.
		 */
		spans = pmspan[i];

		chunk = remain;
		if (chunk > EXE_MAX_BLOCK)
			chunk = EXE_MAX_BLOCK;
		if (spans) {
			tonext = (((start >> 12) + 1) << 12) - start;
			if (chunk > tonext)
				chunk = tonext;
		}
		if (chunk >= remain)
			continue;                 /* fits one block - unchanged */

		/* Shrink original entry to first chunk. */
		pmend[i] = start + chunk;
		start  += chunk;
		mod    += chunk;
		remain -= chunk;

		while (remain) {
			if (npmods >= MAXPMODS) {
				/* v3.9.56: FATAL. This used to warn and return, leaving
				** the rest of the image unwritten -- a clean link with
				** the program truncated, and no symptom but one warning
				** line. */
				putls("\n*** FATAL: page map full while splitting EXE block\n");
				putls("    The image needs more than MAXPMODS chunks.\n");
				putls("    Raise MAXPMODS in link99.c and rebuild.\n");
				exit(1);
			}
			chunk = remain;
			if (chunk > EXE_MAX_BLOCK)
				chunk = EXE_MAX_BLOCK;
			if (spans) {
				tonext = (((start >> 12) + 1) << 12) - start;
				if (chunk > tonext)
					chunk = tonext;
			}
			pmstart[npmods] = start;
			pmend[npmods]   = start + chunk;
			pmcmod[npmods]  = mod;
			pmbase[npmods]  = vbase;
			if (spans) {
				/* The page follows the address, within the declared span. */
				pmpage[npmods] = pg + ((start >> 12) - (vbase >> 12));
				if (pmpage[npmods] > pgend) {
					putls("\n*** FATAL: module overruns its declared page span\n");
					putls("    Widen -Plo-hi or shrink the module.\n");
					exit(1);
				}
			} else {
				pmpage[npmods] = pg;
			}
			pmpgend[npmods] = pgend;
			pmspan[npmods]  = spans;
			npmods++;
			start  += chunk;
			mod    += chunk;
			remain -= chunk;
		}
	}

	/* Keep chain order by virtual/buffer order, stable enough for launch block. */
	for (i = 0; i < npmods - 1; i++) {
		for (j = i + 1; j < npmods; j++) {
			if (pmcmod[j] < pmcmod[i]) {
				unsigned ts, te, tm, tb; int tp;
				ts=pmstart[i]; te=pmend[i]; tm=pmcmod[i]; tb=pmbase[i]; tp=pmpage[i];
				pmstart[i]=pmstart[j]; pmend[i]=pmend[j]; pmcmod[i]=pmcmod[j]; pmbase[i]=pmbase[j]; pmpage[i]=pmpage[j];
				pmstart[j]=ts; pmend[j]=te; pmcmod[j]=tm; pmbase[j]=tb; pmpage[j]=tp;
			}
		}
	}
}

/* ============================================================================
 * emit_chain_blocks  -  Emit paged output as the SHELL v57 linked-list EXE
 *                       block chain (v3.9.12).
 *
 *  Each block is written as:
 *    next_offset  word  byte distance from start of THIS header to next header
 *                       (0 = this is the last block)
 *    page         word  physical page number (0-15)
 *    start        word  virtual load address
 *    size         word  byte count of data that follows
 *    [data...]
 *
 *  The SHELL loader does a single forward pass:
 *    1. Read 8-byte header: next_offset, page, start, size
 *    2. Capture start from first block as launch address
 *    3. MAP_SET(page, start>>12) if page != 0
 *    4. Copy size bytes to start
 *    5. If next_offset == 0 launch; otherwise advance by next_offset
 *
 *  This replaces the older sentinel/page-map EXE prefix.  It is used only
 *  when pagemode is active; flat COM/LGO output remains unchanged.
 * ========================================================================== */
emit_chain_blocks()
{
	int      i, p;
	int      live[MAXPMODS];
	int      nlive;
	int      hdrpos[MAXPMODS];
	unsigned blksz[MAXPMODS];
	unsigned realsz[MAXPMODS];    /* v3.9.57: data length BEFORE padding    */
	int      spanblk[MAXPMODS];   /* v3.9.57: block of a DECLARED-span module */
	unsigned zero, next_off, pg_word, start_word, size_word;
	int      cur;
	char     str[8];

	merge_pagemap();
	split_pagemap_for_exe();
	nlive = 0;
	for (i = 0; i < npmods; i++) {
		if (pmend[i] != pmstart[i])
			live[nlive++] = i;
	}

	if (nlive == 0)
		return (0);

	for (p = 0; p < nlive; p++) {
		i = live[p];
		hdrpos[p] = lseek(outfd, 0, SEEK_CUR);

		zero = 0;
		write99(outfd, &zero, 2);             /* next_offset - patched later */
		pg_word    = pmpage[i];
		start_word = pmstart[i];
		write99(outfd, &pg_word,    2);       /* page  */
		write99(outfd, &start_word, 2);       /* start */
		write99(outfd, &zero,       2);       /* size  - patched after data */

		cloc    = 0;
		crelptr = 1;
		ref     = readref();
		xrplus  = 0;
		if (csfd) rewind(csfd);

		blksz[p]   = emit_page_block_data(i, &cloc, &ref);
		realsz[p]  = blksz[p];        /* v3.9.57: length before padding */
		/*
		 * v3.9.57: this block belongs to a module linked -Plo-hi with
		 * hi > lo.  The flag records what was DECLARED - it is never
		 * inferred from an address looking like a boundary.
		 */
		spanblk[p] = pmspan[i];
		/*
		 * Every NON-FINAL chain block must occupy one complete 0x200-byte
		 * disk sector:
		 *
		 *     8-byte header + 0x1F8-byte payload = 0x200
		 *
		 * A short block followed immediately by another header leaves that
		 * header part-way through a sector.  The Shell loader's refill logic
		 * then loses the chain position when the staging buffer is reloaded.
		 *
		 * Do not alter the final block; its NEXT_OFFSET is zero and no
		 * following header has to be found.
		 */
		if (p < nlive - 1) {
			char pad;
			pad = 0;

			while (blksz[p] < EXE_MAX_BLOCK) {
				write(outfd, &pad, 1);       /* <---- PAD TO 01F8 */
				blksz[p]++;
			}
		}
		else if (blksz[p] & 1) {
			char pad;
			pad = 0;
			write(outfd, &pad, 1);           /* <---- FINAL BLOCK WORD ALIGN */
			blksz[p]++;
			realsz[p]++;                     /* the align byte is real data  */
		}


		cur = lseek(outfd, 0, SEEK_CUR);
		lseek(outfd, hdrpos[p] + 6, SEEK_SET);
		/*
		 * A declared-span module's blocks stop at page boundaries, so
		 * their true length must be written even though the record is
		 * still padded to a full sector: the loader maps ONCE per block
		 * and would otherwise copy the padding across the boundary.
		 * Every other module keeps the historic padded size - changing
		 * that for all modules altered every existing image and hung the
		 * Shell loader (v3.9.57 first attempt).
		 */
		if (spanblk[p])
			size_word = realsz[p];
		else
			size_word = blksz[p];
		write99(outfd, &size_word, 2);
		lseek(outfd, cur, SEEK_SET);

		if (monitor) {
			putls("\n\tEXE CHAIN BLOCK pg=");
			itox(pmpage[i],  str, 3); putls(str);
			putls(" start=");
			itox(pmstart[i], str, 5); putls(str);
			putls(" size=");
			itox(blksz[p],   str, 6); putls(str);
		}
	}

	for (p = 0; p < nlive; p++) {
		if (p < nlive - 1)
			next_off = (hdrpos[p + 1] - hdrpos[p]);
		else
			next_off = 0;
		cur = lseek(outfd, 0, SEEK_CUR);
		lseek(outfd, hdrpos[p], SEEK_SET);
		write99(outfd, &next_off, 2);
		lseek(outfd, cur, SEEK_SET);
	}
}

/* ============================================================================
 * emit_page_block_data  -  Emit one page-map entry's data bytes only.
 *
 *  This is the same relocation/copy logic as emit_page_block(), but without
 *  the old leading [size:word].  emit_chain_blocks() owns the 8-byte EXE
 *  header and patches its size field after this function returns.
 * ========================================================================== */
unsigned emit_page_block_data(idx, cloc_p, ref_p)
	int      idx;
	unsigned *cloc_p;
	unsigned *ref_p;
{
	unsigned  szsave, tp, entry_start, entry_end, reloc_base;
	char      rtbuf[CRELSIZE];
	char      str[9];
	int       pg;

	pg          = pmpage[idx];
	entry_start = pmcmod[idx];
	entry_end   = pmcmod[idx] + (pmend[idx] - pmstart[idx]);

	/*
	 * Runtime-minus-buffer delta for this exact EXE chunk.
	 * pmstart and pmcmod advance together when a block is split, so this
	 * remains constant for resident modules and is also correct for
	 * auto-assigned relocatable page modules.
	 */
	reloc_base  = pmstart[idx] - pmcmod[idx];
	szsave      = 0;

	while (*cloc_p < csize) {
		if (*cloc_p < entry_start || *cloc_p >= entry_end) {
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
			if (*cloc_p < cdisk)
				field = *(buffer + *cloc_p);
			else
				read(csfd, &field, 1);
			write(outfd, &field, 1);
			szsave++;
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

		/*
		 * Phase-2 PREL contract:
		 *
		 *   PREL          CT value is a link-buffer address.
		 *                 Convert it with this block's runtime-buffer delta.
		 *
		 *   CT_AORG_PREL CT value is already a final virtual AORG address.
		 *                 Preserve it; apply only an explicit XPOFF addend.
		 *
		 * This replaces the invalid numeric field<PAGE_SEG heuristic.
		 */
		if (rtbuf[4] == PREL)
			field += xrplus + reloc_base;
		if (rtbuf[4] == CT_AORG_PREL)
			field += xrplus;
		if (rtbuf[4] == DREL)
			field += xrplus + dbase;
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
 *  Flat COM/LGO layout is unchanged from the stable 3.9.11 baseline.
 *
 *  Paged EXE layout produced (v3.9.12, SHELL v57):
 *    repeated linked-list blocks:
 *      [next_offset:word][page:word][start:word][size:word][data...]
 *    next_offset is the byte distance from the current block header to
 *    the next block header; zero marks the final block.
 *
 *  LGO header (RET <start> <base> <size>) is written before the page
 *  blocks when -G# is specified.
 *
 *  The SHELL loader walks the chain in one forward pass, maps each
 *  block's virtual segment to the requested physical page, copies 'size'
 *  bytes to 'start', and launches at the first block's start address.
 * ======================================================================== */
phase2()
{
	char  at[6], sz[8];
	char *epnext2;                  /* walks ep table to find main()        */
	char  mainname[MAXSYM + 1];
	int   pglist[16];               /* unique page numbers in emit order    */
	int   npages, p, i, found;     /* page-block loop variables            */
	strcpy(mainname, "main");

	puts("\n\nPhase 2 - Writing execution files\n");

	/* Locate the entry point of 'main' in the symbol table                  */
	epnext2 = getint(epfirst);
	while (YES) {
		if (strcmp(mainname, epnext2 + SYM) == 0) {
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

	/* v3.9.12: Paged EXE no longer writes the old sentinel/page-map
	 * prefix here.  SHELL v57 expects linked-list block headers emitted
	 * by emit_chain_blocks() below.  Flat COM/LGO remains unchanged.    */

	/*
	 * Write the program header.
	 * TMS9900: STWP WP / B @entry is generated by the assembler or
	 * Small-C compiler (via iolib). link99 does NOT insert it -- doing
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
		/* --- Paged EXE: SHELL v57 linked-list block chain ---          */
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
 * resolve  -  Patch every explicit reference location for one xr==ep pair.
 *
 *  XREFS scheme.  The xr entry's VAL is the head of an internal list of
 *  reference locations, each already converted to a code-buffer offset at
 *  ingest time (when the owning module was unambiguous).  Every location
 *  holds an ABS 0 placeholder; we write the entry point's final absolute
 *  address straight into it.  ABS words carry no CT entry, so COM/page
 *  emit copies them through unchanged.  No chain walk, no page-map guessing.
 * ======================================================================== */
resolve()
{
	char    *node, *nextnode;
	unsigned epval, value, xbuf;
	char wb[2];
	char at[7];

	epval = getint(epnext + VAL);

	/* Final absolute runtime address of the resolved entry point.
	 *
	 * v3.9.53: the old discriminator (epval >= cloc: "a buffer offset
	 * is always < cloc") was sound only while total code size stayed
	 * below the first AORG page.  At csize >B2A6 with pages at >8000,
	 * an AORG epval such as >8A2A sits BELOW cloc by the time later
	 * modules resolve against it, and the same symbol then resolved
	 * to different values at different times (down1: AC80 for the
	 * CC_EXPR_C references, BC80 for CC_EXPR_D's).  Use the
	 * provenance flag newsym() records instead -- value ranges
	 * cannot distinguish these populations once they overlap.       */
	if ((*(epnext + FLG) & ~FLGAORG & 0xFF) == DREL)
		value = epval + dbase;
	else if (*(epnext + FLG) & FLGAORG)
		value = epval;              /* paged AORG: already virtual   */
	else
		value = epval + cbase;

	node = getint(xrnext + VAL);
	while (node) {
		xbuf = getint(node + VAL);
		if (xbuf < cdisk)
			put16int(buffer + xbuf, value);
		else {
			wb[0] = value >> 8;             /* TMS order (hi,lo)          */
			wb[1] = value & 0xff;
			xrseek(xbuf - cdisk);
			write(csfd, wb, 2);
		}
		if (monitor) {
			putls("\n  xref ");   putls(xrnext + SYM);
			itox(xbuf, at, 5);    putls(" @");  putls(at);
			itox(value, at, 5);   putls(" = "); putls(at);
		}
		nextnode = getint(node + NXT);
		putint(node + NXT, sfree);          /* recycle the node           */
		sfree = node;
		node  = nextnode;
	}
	putint(xrnext + VAL, 0);
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
	      "  A trailing ^ or , continues the command on the next input line.\n"
	      "  Transparent GAL mapping: cross-page calls are plain BL/RT, no trampolines.\n"
	      "  Segment collision (two pages sharing a 4KB virtual segment) is a link error.");
}

/* ==========================================================================
 * ctseek  -  Seek to byte offset 'off' in the code relocation table file.
 * ======================================================================== */
ctseek(idx)
	unsigned idx;
{
	if (lseek(ctfd, idx * CRELSIZE, SEEK_SET) == EOF)
		error("- Seek error in code relocation table (.CT$)");
}

/* ==========================================================================
 * dtseek  -  Seek to entry index 'idx' in the data relocation table file.
 * ======================================================================== */
dtseek(idx)
	unsigned idx;
{
	if (lseek(dtfd, idx * CRELSIZE, SEEK_SET) == EOF)
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
