/*
** rel.h -- header for REL file processing
**
** REL99 EXPLICIT XCLINK EXTENSION
===============================

Encoding:
    EXT   "XCLINK"
    PREL  <next-link>

The marker immediately precedes every PREL word used as an external-reference
chain link.

Semantics:
    XCLINK + PREL 0000  = terminal chain sentinel; never relocate.
    XCLINK + PREL nnnn  = module-relative next-chain slot; add cmod.
    PREL without marker = ordinary program-relative relocation.

The existing XCHAIN special item still supplies the chain head and symbol name.

GETREL needs no parser change because EXT already returns item EXT and places
its string payload in symbol.

Compatibility:
    LINK99 3.9.46 reads explicit markers.
    Legacy object files remain accepted through the existing raw-zero fallback.
    Newly assembled explicit-XCLINK objects require LINK99 3.9.46 or later.

*/

			/* item-type codes */

#define ABS       0	/* absolute item */
#define PREL      1	/* program (code) relative item */
#define DREL      2	/* data relative item */
#define CREL      3	/* common relative item */

#define ENAME     4	/* entry name */
#define CNAME     5	/* common block name */
#define PNAME     6	/* program name */
#define LNAME     7	/* library name */
#define EXT       8	/* extension link-item */

/* Explicit external-chain marker carried by EXT. */
#define REL99_EXT_XCLINK "XCLINK"

/*
** REL99 EXPLICIT XREFS EXTERNAL-REFERENCE SCHEME
** ==============================================
**
** Replaces the legacy in-code external chain (a singly-linked list threaded
** through the code image, headed by an XCHAIN item and terminated by a zero
** word).  That scheme could not bind a reference to its owning module once
** several modules (resident + overlays) shared a virtual address window.
**
** Encoding (emitted once per external symbol used in a module, at END):
**
**     EXT     "XREFS"                     marker; introduces one group
**     XCHAIN  <count>, <symbol>           field = reference count, + name
**     CHAIN   <location 1>                field = module PC of a reference
**     CHAIN   <location 2>
**     ...                                 exactly <count> CHAIN items
**
** Semantics:
**   - The group is self-delimiting: <count> gives the number of CHAIN
**     items that follow, so no terminator word is needed.
**   - A CHAIN item is a reference LOCATION only while inside an XREFS group
**     (i.e. after EXT "XREFS" + XCHAIN, until <count> locations are read).
**     Outside a group CHAIN retains its AORG_MARK meaning.  The two never
**     interleave: AORG_MARK is emitted in the code body, XREFS groups at END.
**   - Each reference WORD in the code is left as ABS 0 (no PREL, no in-code
**     next-pointer).  The linker converts every location to a buffer offset
**     using the owning module's org anchor at read time - unambiguous, since
**     the module is known then - and writes the symbol's final absolute
**     address into it.  ABS words are copied through by the emit stage.
**   - CHAIN <location> is a module program-counter value: 0-based for a
**     relocatable module, the absolute AORG address for an AORG module.
**
** GETREL needs no change: EXT, XCHAIN and CHAIN are all existing items.
** Tools that DISPLAY or INTERPRET external records (e.g. SEEREL) must treat
** CHAIN-within-a-group as a location rather than an AORG base.
**
** Compatibility: new objects require the XREFS-aware LINK99; they are not
** interchangeable with legacy-chain objects.  Reassemble to convert.
*/

/* Explicit grouped external-reference list marker carried by EXT. */
#define REL99_EXT_XREFS  "XREFS"


#define CSIZE     9	/* common size & name */
#define XCHAIN   10	/* external-reference-chain head & name */
#define EPOINT   11	/* entry point location & name */

#define XMOFF    12	/* external - offset */
#define XPOFF    13	/* external + offset */
#define DSIZE    14	/* data area size */
#define SETLC    15	/* set location counter for loading */
#define CHAIN    16	/* chain address (fill chain with loc ctr)  */
#define PSIZE    17	/* program (code) size */
#define EPROG    18	/* end of program */
#define EFILE    19	/* end of file */
#define AORG_MARK CHAIN	/* absolute ORG marker reuses CHAIN (never emitted by assembler) */

#define MAXSYM    15	/* maximum symbol length allowed in REL file (2^BITPSYM-1) */
#define BITPSYM	  4 /* bits in the symbol-length count field: 4 -> max 15 chars (== MAXSYM). Original spec is 3 (max 7) */
#define ONES     0xffff	/* all one bits */
