/*
 ** DREL.C -- dump and lint REL/LIB files
 **
 **           Copyright 1985 J. E. Hendrix
 **
 ** Version 5.0
 **
 ** v5.0: the overlay include generator (-i, OVLADDR.INC/OVLTABLE.INC) is
 ** removed.  LINK99 3.10.0 builds the overlay table and thunks itself
 ** from the overlay map (link99 -Y), and 3.10.1 makes the -c reference-
 ** slot checks on every link, so no build needs DREL.  It remains the
 ** toolchain's inspector, run by hand.
 **
 ** ROLE (see REL99_FORMAT.TXT section 0)
 ** ----------------------------------------
 ** DREL is the toolchain's INSPECTOR: the reference implementation of
 ** REL99_FORMAT.TXT. It reads object files, verifies them against the
 ** spec, and commands nothing. It is kept independent of LINK99 BY
 ** DESIGN: an inspector that reports to the staff it inspects will one
 ** day approve a misconception both share (this happened - spec 2.3).
 **
 ** Two functions, one principle (read-only format consumer):
 **   dump   (no flag)  human-readable inspection of an R99/LIB
 **   verify (-c)       conformance lint
 **
 ** Chain-lint mode (v3.7) -- implements REL99_FORMAT.TXT checklist 6:
 **
 **   drel -c file1.r99 [file2.r99 ...]
 **   drel -c file.r99=lo-hi ...      (a module declared to span pages)
 **
 ** Reconstructs each module image from the stream, walks every external
 ** chain per the format spec (links are exact PREL operand-slot
 ** addresses, terminator 0, empty chain = doubled ABS-0 XCHAIN records),
 ** reports each chain, and ERRORS on any malformation: slots outside
 ** the module, slots that are not PREL relocation words, cycles, or
 ** links into the pre-AORG relocatable preamble. Also reports each
 ** module's preamble size (spec clause 2.1/2.3 evidence).
 **
 ** Normal dump mode remains unchanged:
 **
 **   drel filename.r99
 */
#include "stdio.h"
#include "rel99.h"
#include "fcntl.h"

#define MAXFN 128

/* common REL decoder variables */
int inrel;
int inrem;
int inchunk;
int outrel;
int outrem;
int outchunk;
int item;
int type;
int field;
char symbol[MAXSYM + 1];

static int lint_mode;
static int file_page;             /* -c: declared first page, or -1     */
static int file_pgend;            /* -c: declared last page of a span   */

static int process_file();
static int parse_lint_arg();
static void lint_reset();
static long chain_to_lc();
static void lint_chain();
static void lint_module();
static void process_lint_item();
static int lint_files();
static void usage();

main(argc, argv)
int argc;
char *argv[];
{
    char fn[MAXFN];

    putls("----------------------------------------------------\n");
    putls("TMS9900 REL/LIB Relocatable Object Library Dump Utility Version 5.0\n");
    putls("CP/M Version by Alexander Cameron January, 1985\n");
    putls("MSDOS Version by Alexander Cameron May 2010 to July, 2026\n");
    putls("----------------------------------------------------\n");

    if (argc > 1 && (!strcmp(argv[1], "-c") || !strcmp(argv[1], "-C"))) {
        if (argc < 3) {
            usage();
            exit(1);
        }
        lint_mode = 1;
        exit(lint_files(argc - 2, &argv[2]));
    }

    if (argc > 1 && (!strcmp(argv[1], "-i") || !strcmp(argv[1], "-I"))) {
        putls("drel -i was removed in DREL 5.0: link99 -Y builds the overlay\n");
        putls("table and thunks from the overlay map.  See the link99 README.\n");
        exit(1);
    }

    /* Existing command-line dump mode. */
    if (argc > 1) {
        if (!process_file(argv[1], YES, YES))
            exit(1);
        puts("\nFinished");
        exit(0);
    }

    /* Existing prompted dump mode. */
    while (YES) {
        if (!reqstr("Library/Module Name: ", fn, MAXFN))
            error("File Open Error\n");
        if (!process_file(fn, YES, YES))
            continue;
        puts("\nFinished");
        exit(0);
    }
}

/*
 * Read one REL/R99 file.
 *
 * display == YES: print the normal DREL listing through seerel().
 * display == NO:  feed the items to the -c lint.
 */
static int process_file(fn, announce, display)
char *fn;
int announce;
int display;
{
    inrel = open(fn, _O_RDONLY | _O_BINARY);
    if (inrel == -1) {
        puts3("Error opening file:  ", fn, "\n");
        return (NO);
    }

    if (announce)
        puts3("Opening ", fn, "\n");

    inrem = 0;
    inchunk = 0;
    ifilelbuf();

    do {
        poll(YES);
        if (getrel() == ERR) {
            close(inrel);
            error("Error reading rel item");
            return (NO);
        }

        if (display)
            seerel();
        else if (lint_mode)
            process_lint_item();

    } while (item != EFILE);

    close(inrel);
    return (YES);
}

/* ======================================================================
 * Chain-lint mode (v3.6). See REL99_FORMAT.TXT.
 * Reconstructs the module image at file-lc positions, records AORG
 * marks (clause 2.2/2.3), then walks each XCHAIN per clauses 3.1-3.5
 * and validates per checklist 6.3-6.5. Read-only: reports and errors,
 * never writes -- the property LINK99 lacked.
 * ==================================================================== */

#define CF_WRIT  1
#define CF_PREL  2
#define CF_XREF  4
#define MAXMARK  8
#define MAXCHN   96

static unsigned char cimage[65536];
static unsigned char cflags[65536];
static unsigned long mlc;              /* current file-lc               */
static unsigned int  marklc[MAXMARK];  /* file-lc where AORG took hold  */
static unsigned int  markbase[MAXMARK];/* AORG base declared there      */
static int  nmarks;
static unsigned int  msize;            /* PSIZE                         */
static char chname[MAXCHN][MAXSYM + 1];
static unsigned int  chhead[MAXCHN];
static int  nchains;
static char lmodname[MAXSYM + 1];
static int  lint_errors;
static int  in_module;

/* XREFS-scheme group state */
static int  in_xrefs;                  /* inside an EXT "XREFS" group    */
static int  xref_left;                 /* CHAIN locations still expected */
static int  xref_count;                /* declared count for this group  */
static char cur_xrname[MAXSYM + 1];    /* symbol owning the group        */

static void lint_reset()
{
    long i;
    for (i = 0; i < 65536L; ++i) cflags[i] = 0;
    mlc = 0;
    nmarks = 0;
    msize = 0;
    nchains = 0;
    lmodname[0] = 0;
    in_module = 0;
    in_xrefs = 0;
    xref_left = 0;
    xref_count = 0;
    cur_xrname[0] = 0;
}

/* Map a chain address (virtual for AORG modules, file-lc otherwise)
 * to a file-lc, per spec clause 4.1. Returns -1L if unmappable. */
static long chain_to_lc(addr)
unsigned int addr;
{
    int i;
    long lc;

    if (nmarks == 0)
        return ((long) addr);

    for (i = nmarks - 1; i >= 0; --i) {
        if (addr >= markbase[i]) {
            lc = (long) marklc[i] + (long) (addr - markbase[i]);
            if (lc < (long) mlc)
                return (lc);
        }
    }
    return (-1L);
}

/* Validate one explicit XREFS reference location (XREFS scheme).
 * The reference word must be an emitted ABS 0 placeholder inside the
 * module; the linker patches it with the resolved absolute address. */
static void lint_xref_loc(name, loc)
char *name;
unsigned int loc;
{
    long lc;
    long prelim;

    printf(">%04X ", loc & 0xffff);
    lc = chain_to_lc(loc);
    prelim = (nmarks > 0) ? (long) marklc[0] : 0L;

    if (lc < 0L || lc + 1 >= (long) mlc) {
        printf("*** REF LOCATION OUTSIDE MODULE (6.5) [%s]\n", name);
        ++lint_errors;
        return;
    }
    if (nmarks > 0 && lc < prelim) {
        printf("*** REF LOCATION IN PRE-AORG PREAMBLE (2.1) [%s]\n", name);
        ++lint_errors;
        return;
    }
    if (!(cflags[lc] & CF_WRIT)) {
        printf("*** REF SLOT NEVER EMITTED (6.5) [%s]\n", name);
        ++lint_errors;
        return;
    }
    if (cflags[lc] & CF_PREL) {
        printf("*** REF SLOT IS PREL, EXPECTED ABS PLACEHOLDER (3.4) [%s]\n",
               name);
        ++lint_errors;
        return;
    }
    if (cimage[lc] != 0 || cimage[lc + 1] != 0)
        printf("(note: placeholder = %02X%02X, not 0) ",
               cimage[lc], cimage[lc + 1]);
    /* mark visited to trap the same slot appearing in two groups */
    if (cflags[lc] & CF_XREF) {
        printf("*** REF SLOT ALREADY CLAIMED BY ANOTHER SYMBOL [%s]\n", name);
        ++lint_errors;
        return;
    }
    cflags[lc] |= CF_XREF;
    printf("ok\n");
}

static void lint_module()
{
    int i, dup;

    if (!in_module)
        return;

    printf("\nMODULE %s  size=%04X  aorg-marks=%d",
           lmodname, msize & 0xffff, nmarks);
    for (i = 0; i < nmarks; ++i)
        printf("  [lc=%04X base=%04X]", marklc[i], markbase[i]);
    if (nmarks > 0 && marklc[0] > 0)
        printf("\n  NOTE: %u pre-AORG preamble byte(s) (clause 2.1/2.3)",
               marklc[0]);
    printf("\n");

    /* v3.7 (spec 2.5): a paged module's extent must stay inside one
     * 4KB segment - one mapping register. Overrun bytes land in the
     * NEXT segment's register domain and corrupt whatever page is
     * mapped there. Checked per AORG mark against the module bytes
     * that follow it. Fatal at the lint gate, minutes before LINK99
     * would refuse the same module.                                  */
    for (i = 0; i < nmarks; ++i) {
        unsigned int span_end, seg_end, next_lc;

        next_lc  = (i + 1 < nmarks) ? marklc[i + 1] : (unsigned int) mlc;
        span_end = markbase[i] + (next_lc - marklc[i]);
        /* v4.2: a module that DECLARED a span (=lo-hi) may reach the
         * end of its last declared page - LINK99 -Plo-hi breaks its
         * blocks at each boundary and pages each correctly.  With no
         * declaration the one-segment rule stands exactly as before,
         * so every existing module lints identically. */
        seg_end  = (markbase[i] & 0xF000) + 0x1000;
        if (file_page >= 0 && file_pgend > file_page)
            seg_end = seg_end + (unsigned int)(file_pgend - file_page) * 0x1000;
        if (span_end > seg_end) {
            printf("  *** SEGMENT OVERRUN (2.5): AORG %04X content ends "
                   "%04X, %u byte(s) past %04X\n",
                   markbase[i], span_end, span_end - seg_end, seg_end);
            ++lint_errors;
        }
    }

    /* XREFS groups are validated inline as their CHAIN items arrive
     * (see process_lint_item); nothing to walk at module close. */
    if (in_xrefs) {
        printf("  *** UNTERMINATED XREFS GROUP: %s expected %d more location(s)\n",
               cur_xrname, xref_left);
        ++lint_errors;
        in_xrefs = 0;
    }
    in_module = 0;
}

static void process_lint_item()
{
    switch (item) {
    case PNAME:
        lint_module();          /* close previous module, if any */
        lint_reset();
        strcpy(lmodname, symbol);
        in_module = 1;
        return;

    case ABS:
        if (mlc < 65535L) {
            cimage[mlc] = field & 0xff;
            cflags[mlc] |= CF_WRIT;
        }
        ++mlc;
        return;

    case PREL:
    case DREL:
    case CREL:
        if (mlc + 1 < 65535L) {
            cimage[mlc]     = (field >> 8) & 0xff;
            cimage[mlc + 1] = field & 0xff;
            cflags[mlc]     |= CF_WRIT;
            cflags[mlc + 1] |= CF_WRIT;
            if (item == PREL)
                cflags[mlc] |= CF_PREL;
        }
        mlc += 2;
        return;

    case XMOFF:
    case XPOFF:
        mlc += 2;
        return;

    case SETLC:
        /* mlc is a FILE-LC: the byte offset into this module's image.
         * cimage[]/cflags[] are indexed by it, chain_to_lc() maps
         * virtual->file-lc through it, and lint_module()'s clause-2.5
         * span check reconstructs the virtual extent from it.
         *
         * In an AORG module the assembler emits SETLC with an ABSOLUTE
         * address, not a file-lc (link99 logs it as "AORG SETLC fixup:
         * base=B000 end=BEF2"). Assigning it raw makes mlc absolute and
         * breaks all three consumers at once:
         *
         *   - lint_module(): span_end = markbase + (mlc - marklc) adds
         *     the base a second time -> phantom SEGMENT OVERRUN (2.5)
         *     reporting ~45000 bytes for a module that fits.
         *   - cimage[]/cflags[]: bytes recorded at absolute indices.
         *   - chain_to_lc(): still returns relative offsets, so every
         *     reference looks unwritten -> REF SLOT NEVER EMITTED (6.5)
         *     for every symbol in the module.
         *
         * Rebase to a file-lc using the AORG mark in force. Modules with
         * no AORG, and any SETLC that is genuinely already a file-lc
         * (field below the AORG base), are left untouched.
         *
         * Latent until now: SETLC appears when a module reserves
         * uninitialised module-level storage. No overlay had any until
         * an overlay first gained module-level uninitialised
         * storage; modules without such storage did not expose it.     */
        if (nmarks > 0 && field >= markbase[nmarks - 1])
            mlc = (unsigned long) marklc[nmarks - 1]
                + (unsigned long) (field - markbase[nmarks - 1]);
        else
            mlc = field;
        return;

    case EXT:
        /* XREFS marker; the XCHAIN header that follows carries the count. */
        if (!strcmp(symbol, "XREFS"))
            printf("\n  XREFS group:");
        return;

    case CHAIN:
        if (in_xrefs) {             /* reference location inside a group   */
            lint_xref_loc(cur_xrname, field);
            if (xref_left > 0 && --xref_left == 0)
                in_xrefs = 0;
            return;
        }
        /* otherwise: genuine AORG_MARK, clause 2.2 */
        if (nmarks < MAXMARK) {
            marklc[nmarks]   = (unsigned int) mlc;
            markbase[nmarks] = field;
            ++nmarks;
        }
        return;

    case PSIZE:
        msize = field;
        return;

    case XCHAIN:
        /* XREFS group header: field = reference count, symbol = name. */
        if (in_xrefs) {
            printf("  *** NESTED XREFS GROUP (%s still open)\n", cur_xrname);
            ++lint_errors;
        }
        strcpy(cur_xrname, symbol);
        xref_count = field;
        xref_left  = field;
        in_xrefs   = (field != 0);
        printf("  %-15s count=%d: ", cur_xrname, xref_count);
        if (field == 0)
            printf("(empty group)\n");
        return;

    case EPROG:
    case EFILE:
        lint_module();
        return;
    }
}

static int lint_files(nfiles, files)
int nfiles;
char **files;
{
    char fname[MAXFN];
    int i;

    lint_errors = 0;
    for (i = 0; i < nfiles; ++i) {
        /* file.r99 or file.r99=lo-hi; anything after a ',' (the old
         * -i descriptor) is ignored, so old command lines still lint. */
        if (!parse_lint_arg(files[i], fname))
            return (1);

        lint_reset();
        printf("\n==== CHAIN LINT: %s ====\n", fname);
        if (!process_file(fname, NO, NO))
            return (1);
    }
    if (lint_errors) {
        printf("\n*** CHAIN LINT: %d error(s)\n", lint_errors);
        return (1);
    }
    printf("\nCHAIN LINT: all chains conform to REL99_FORMAT.TXT\n");
    return (0);
}


/*
 * Parse a -c argument:  file.r99  or  file.r99=lo[-hi][,anything].
 * A declared span lets the module run past its first 4K segment, as
 * link99 -Plo-hi (or a lo-hi page in the overlay map) allows.
 */
static int parse_lint_arg(arg, fname)
char *arg, *fname;
{
    int i, n;

    file_page = file_pgend = -1;
    for (i = 0; arg[i] && arg[i] != '=' && i < MAXFN - 1; ++i)
        fname[i] = arg[i];
    fname[i] = 0;
    if (arg[i] && arg[i] != '=') {
        puts3("File name too long: ", arg, "\n");
        return (0);
    }
    if (arg[i] != '=')
        return (1);
    ++i;
    if (arg[i] < '0' || arg[i] > '9') {
        puts3("Expected file=page or file=lo-hi: ", arg, "\n");
        return (0);
    }
    n = 0;
    while (arg[i] >= '0' && arg[i] <= '9')
        n = n * 10 + (arg[i++] - '0');
    file_page = file_pgend = n;
    if (arg[i] == '-') {
        ++i;
        if (arg[i] < '0' || arg[i] > '9') {
            puts3("Expected file=lo-hi: ", arg, "\n");
            return (0);
        }
        n = 0;
        while (arg[i] >= '0' && arg[i] <= '9')
            n = n * 10 + (arg[i++] - '0');
        if (n < file_page) {
            puts3("Page span runs backwards: ", arg, "\n");
            return (0);
        }
        file_pgend = n;
    }
    return (1);
}

static void usage()
{
    putls("Usage:\n");
    putls("  drel file.r99                 dump an object file or library\n");
    putls("  drel -c file1.r99 [...]       lint: reference slots, segment overrun\n");
    putls("  drel -c file.r99=lo-hi ...    lint a module declared to span pages\n");
}

