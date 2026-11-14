/*
 ** DREL.C -- dump REL/LIB files or generate OVLADDR.INC / OVLTABLE.INC
 **
 **           Copyright 1985 J. E. Hendrix
 **
 ** Version 4.0
 **
 ** Overlay include generation
 ** --------------------------
 **
 ** DREL can generate the overlay entry-address include used by resident
 ** stubs and the overlay ID -> (segment,page) table consumed by OVLMGR.
 ** Project-specific names and IDs are supplied in each file descriptor:
 **
 **   drel -i OVLADDR.INC cc_decl.R99=8,1,D_DODECL
 **   drel -i OVLADDR.INC olddecl.R99=8,1,dodeclare:D_DODECL
 **
 ** Descriptor form:
 **
 **   file.R99=physical_page,overlay_ID,export[:alias][,export[:alias]...]
 **
 ** Alias omitted means emit the exported name. Alias '-' marks an anchor
 ** without writing an EQU. A plain file name emits all EPOINTs unchanged
 ** and generates no table row. Segments are derived automatically from
 ** entry-point addresses (address >> 12).
 **
 ** Required build sequence:
 **
 **   1. Compile the overlay C modules.
 **   2. Assemble the overlay modules to .R99 files.
 **   3. Run DREL -i to generate OVLADDR.INC (and OVLTABLE.INC) from
 **      their EPOINT records. The =page values MUST match the page
 **      assignments used by the EXE packer -- keep both lists generated
 **      from one variable in the build script.
 **   4. Assemble OVLMGR and any resident source that includes
 **      OVLADDR.INC or OVLTABLE.INC.
 **   5. Run the final LINK99 link.
 **
 ** OVLADDR.INC and OVLTABLE.INC are generated build artifacts. They must
 ** not be edited by hand. Fixed overlay IDs and resident vector locations
 ** belong in the separate source-controlled OVLDEFS.INC file.
 **
 ** IMPORTANT DESIGN NOTE
 ** ---------------------
 ** Include generation is implemented entirely in DREL.C. SEEREL.C remains
 ** a generic shared REL-display module and has no dependency on DREL-only
 ** globals. This prevents LINK99 and other applications that use SEEREL.C
 ** from acquiring unresolved references to DREL-specific state.
 **
 ** ROLE (see REL99_FORMAT.TXT section 0)
 ** ----------------------------------------
 ** DREL is the toolchain's INSPECTOR: the reference implementation of
 ** REL99_FORMAT.TXT. It reads object files, verifies them against the
 ** spec, and commands nothing. It is kept independent of LINK99 BY
 ** DESIGN: an inspector that reports to the staff it inspects will one
 ** day approve a misconception both share (this happened - spec 2.3).
 **
 ** Three functions, one principle (read-only format consumer):
 **   dump   (no flag)  human-readable inspection of an R99/LIB
 **   verify (-c)       conformance lint; the build's quality gate
 **   glue   (-i)       OVLADDR/OVLTABLE generation - TRANSITIONAL:
 **                     this re-derives what LINK99 already knows and
 **                     belongs in a future LINK99 symbol-export option
 **                     (migrate in its own change, per spec section 0)
 **
 ** v4.0 changes:
 **   - Removed all hard-coded overlay IDs, exported names and aliases.
 **   - The -i command line now supplies page, ID and anchor mappings.
 **   - Missing configured anchors remain fatal before final linking.
 **
 ** Chain-lint mode (v3.7) -- implements REL99_FORMAT.TXT checklist 6:
 **
 **   drel -c file1.r99 [file2.r99 ...]
 **   drel -c file.r99=page[-page],ID,anchor[:alias][,...] ...
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

/*
 ** Generic overlay-generation limits.
 **
 ** Project-specific overlay IDs and exported entry names are supplied
 ** on the -i command line. DREL contains no compiler-specific mapping.
 */
#define MAXOVL     31           /* highest supported overlay ID */
#define MAXANCHOR  16           /* exported anchors per module  */
#define TABLENAME "OVLTABLE.INC"

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

/* DREL-only include-generation state */
static FILE *incfp;
static int inc_entries;
static int lint_mode;

/* DREL-only table-generation state */
static int ovl_seg[MAXOVL + 1];  /* first segment per overlay ID       */
static int ovl_page[MAXOVL + 1]; /* first physical page per overlay ID */
#define OVLPAGES 4               /* additional pages per overlay ID    */
static int ovl_seg2[OVLPAGES][MAXOVL + 1];
static int ovl_page2[OVLPAGES][MAXOVL + 1];
static int max_ovlid;             /* highest configured overlay ID      */
static int file_page;             /* physical page, or -1 for no table  */
static int file_pgend;            /* v4.2: last page of a declared span  */
                                  /*   =10    -> file_page=10 pgend=10    */
                                  /*   =10-12 -> file_page=10 pgend=12    */
static int file_expected_id;      /* ID supplied for current file       */
static int file_ovlid;            /* ID confirmed by a matched anchor   */
static int file_seg;              /* segment derived from anchor addr   */
static int file_configured;       /* descriptor has ID/anchor data      */
static int file_anchor_count;
static char file_anchor_name[MAXANCHOR][MAXSYM + 1];
static char file_anchor_alias[MAXANCHOR][MAXSYM + 1];
static int file_anchor_emit[MAXANCHOR];
static int file_anchor_seen[MAXANCHOR];
static int table_wanted;

static int process_file();
static int generate_include();
static void process_overlay_item();
static void emit_overlay_entry();
static void emit_equ();
static void setovl();
static int parse_overlay_arg();
static int emit_table();
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
    putls("TMS9900 REL/LIB Reclocatable Object Library Dump Utility Version 4.0\n");
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
        if (argc < 4) {
            usage();
            exit(1);
        }
        exit(generate_include(argv[2], argc - 3, &argv[3]));
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
 * display == NO:  inspect decoded items locally for OVLADDR generation.
 *
 * SEEREL.C is deliberately not involved in include generation.
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
        else
            process_overlay_item();

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
        /*
         * -c accepts the same overlay descriptor as -i so the lint check
         * knows the declared physical-page span.  A plain filename remains
         * valid and keeps the original one-segment rule.
         */
        if (!parse_overlay_arg(files[i], fname))
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
 * Generate OVLADDR.INC (and, when =page suffixes are supplied,
 * OVLTABLE.INC) from selected EPOINT records in all input files.
 */
static int generate_include(outname, nfiles, files)
char *outname;
int nfiles;
char **files;
{
    char fname[MAXFN];
    int i;

    incfp = fopen(outname, "w");
    if (incfp == NULL) {
        puts3("Error creating file: ", outname, "\n");
        return (1);
    }

    inc_entries = 0;
    table_wanted = 0;
    max_ovlid = 0;
    for (i = 0; i <= MAXOVL; ++i) {
        int j;
        ovl_seg[i] = ovl_page[i] = 0;
        for (j = 0; j < OVLPAGES; ++j)
            ovl_seg2[j][i] = ovl_page2[j][i] = 0;
    }

    fprintf(incfp, ";------------------------------------------------------------\n");
    fprintf(incfp, ";  OVLADDR.INC\n");
    fprintf(incfp, ";\n");
    fprintf(incfp, ";  AUTO-GENERATED BY DREL - DO NOT EDIT\n");
    fprintf(incfp, ";\n");
    fprintf(incfp, ";  Generated from overlay .R99 EPOINT records.\n");
    fprintf(incfp, ";  Overlay IDs and aliases came from the DREL command line.\n");
    fprintf(incfp, ";\n");
    fprintf(incfp, ";  Source descriptors:\n");
    for (i = 0; i < nfiles; ++i)
        fprintf(incfp, ";      %s\n", files[i]);
    fprintf(incfp, ";------------------------------------------------------------\n\n");

    for (i = 0; i < nfiles; ++i) {
        int j;

        file_ovlid = 0;
        file_seg = 0;

        if (!parse_overlay_arg(files[i], fname)) {
            fclose(incfp);
            incfp = NULL;
            return (1);
        }

        if (!process_file(fname, NO, NO)) {
            fclose(incfp);
            incfp = NULL;
            return (1);
        }

        if (file_configured) {
            for (j = 0; j < file_anchor_count; ++j) {
                if (!file_anchor_seen[j]) {
                    printf("*** FATAL: configured anchor %s not exported by %s\n",
                           file_anchor_name[j], fname);
                    fclose(incfp);
                    incfp = NULL;
                    return (1);
                }
            }
        }

        if (file_page >= 0) {
            table_wanted = 1;
            if (file_ovlid == 0) {
                puts3("*** FATAL: no configured overlay anchor found in ",
                      fname, "\n");
                fclose(incfp);
                incfp = NULL;
                return (1);
            }
            else if (ovl_page[file_ovlid]) {
                int k, sg, pg;
                sg = file_seg;
                pg = file_page;

                if (sg < ovl_seg[file_ovlid]) {
                    k = ovl_seg[file_ovlid];
                    ovl_seg[file_ovlid] = sg;
                    sg = k;
                    k = ovl_page[file_ovlid];
                    ovl_page[file_ovlid] = pg;
                    pg = k;
                }

                for (j = 0; j < OVLPAGES; ++j) {
                    if (ovl_seg2[j][file_ovlid] == 0) {
                        ovl_seg2[j][file_ovlid] = sg;
                        ovl_page2[j][file_ovlid] = pg;
                        break;
                    }
                    if (sg < ovl_seg2[j][file_ovlid]) {
                        k = ovl_seg2[j][file_ovlid];
                        ovl_seg2[j][file_ovlid] = sg;
                        sg = k;
                        k = ovl_page2[j][file_ovlid];
                        ovl_page2[j][file_ovlid] = pg;
                        pg = k;
                    }
                }
                if (j == OVLPAGES) {
                    puts3("*** FATAL: too many files for overlay ID in ",
                          fname, "\n");
                    fclose(incfp);
                    incfp = NULL;
                    return (1);
                }
            }
            else {
                ovl_seg[file_ovlid] = file_seg;
                ovl_page[file_ovlid] = file_page;
            }

            /*
             * v4.2: a declared span contributes ONE (segment,page)
             * pair PER PAGE.  Without this the table maps only the
             * module's first page - LINK99 loads all of it, OVLMGR
             * maps a third of it, and the rest is unreachable.  The
             * failure shows up as a wild branch on the board, not as
             * a build error.
             *
             * Placed AFTER the if/else above and outside it, so it
             * applies whether or not this is the first module seen
             * for the overlay ID.
             *
             * Segments are consecutive by construction, satisfying
             * the adjacency check in the table writer.
             */
            if (file_pgend > file_page) {
                int spanpg, k2, sg, pg;
                for (spanpg = 1; spanpg <= file_pgend - file_page; ++spanpg) {
                    sg = file_seg + spanpg;
                    pg = file_page + spanpg;
                    if (sg > 15) {
                        puts3("*** FATAL: page span leaves the window in ",
                              fname, "\n");
                        fclose(incfp);
                        incfp = NULL;
                        return (1);
                    }
                    for (j = 0; j < OVLPAGES; ++j) {
                        if (ovl_seg2[j][file_ovlid] == 0) {
                            ovl_seg2[j][file_ovlid]  = sg;
                            ovl_page2[j][file_ovlid] = pg;
                            break;
                        }
                        if (sg < ovl_seg2[j][file_ovlid]) {
                            k2 = ovl_seg2[j][file_ovlid];
                            ovl_seg2[j][file_ovlid] = sg;
                            sg = k2;
                            k2 = ovl_page2[j][file_ovlid];
                            ovl_page2[j][file_ovlid] = pg;
                            pg = k2;
                        }
                    }
                    if (j == OVLPAGES) {
                        puts3("*** FATAL: too many pages for overlay ID in ",
                              fname, "\n");
                        fclose(incfp);
                        incfp = NULL;
                        return (1);
                    }
                }
            }
        }
    }

    fclose(incfp);
    incfp = NULL;

    if (inc_entries == 0 && !table_wanted) {
        puts3("No EPOINT entries written to ", outname, "\n");
        return (1);
    }

    puts3("Generated ", outname, "\n");

    if (table_wanted && emit_table())
        return (1);

    return (0);
}

/*
 * Inspect one decoded REL item. Only EPOINT records are relevant.
 */
static void process_overlay_item()
{
    if (item == EPOINT)
        emit_overlay_entry(symbol, field);
}

/*
 * Map actual exported overlay names onto the stable names consumed by
 * OVLSTUBS and the harness, and tag the file with the overlay class ID
 * each name belongs to. Add new resident-callable overlay entry points
 * here explicitly.
 */
static void emit_overlay_entry(name, value)
char *name;
unsigned value;
{
    int i;

    /* Plain file name: emit every EPOINT unchanged. */
    if (!file_configured) {
        emit_equ(name, value, name);
        return;
    }

    /* Configured descriptor: only named anchors are relevant. */
    for (i = 0; i < file_anchor_count; ++i) {
        if (!strcmp(name, file_anchor_name[i])) {
            file_anchor_seen[i] = 1;
            if (file_anchor_emit[i])
                emit_equ(file_anchor_alias[i], value, name);
            if (file_expected_id)
                setovl(file_expected_id, value);
            return;
        }
    }
}

static void emit_equ(alias, value, comment)
char *alias;
unsigned value;
char *comment;
{
    unsigned v;

    v = value & 0xffff;

    /* TMS9900 assembler convention: a hex literal whose FIRST digit is
     * A-F must carry a leading zero, otherwise the assembler parses the
     * token as a symbol name and reports 'U' (undefined) -- e.g.
     *     ENTRY      EQU B81AH      -> U 0000
     *     ENTRY      EQU 0B81AH     -> correct
     * With %04X only values >= 0xA000 begin with A-F, so those get the
     * prefix. Values below 0x1000 already print a leading '0'.
     *
     * This became reachable when the codegen window moved to >B000;
     * every generated address there starts with 'B'. The same applies
     * to any future overlay placed in segments A-F.                  */
    fprintf(incfp, "%-10s EQU %s%04XH\t; %s\n",
            alias, (v >= 0xA000) ? "0" : "", v, comment);
    ++inc_entries;
}

/*
 * Record which overlay class the file now being read belongs to, and
 * derive its window segment from the entry-point address (addr >> 12).
 * Warns if one file exports entry points from two different classes.
 */
static void setovl(id, value)
int id;
unsigned value;
{
    int seg;

    seg = (value >> 12) & 15;
    if (file_ovlid && file_ovlid != id)
        puts3("WARNING: file matched two overlay IDs", "", "\n");
    if (file_ovlid && file_seg != seg)
        puts3("WARNING: configured anchors occupy different segments", "", "\n");
    file_ovlid = id;
    file_seg = seg;
}

/*
 * Parse:
 *
 *   file.R99
 *   file.R99=page,ID,export[:alias][,export[:alias]...]
 *
 * A plain file emits all EPOINTs and has no table row. Page '-' permits
 * selected/aliased EPOINT output without creating a table row. Alias '-'
 * makes the export an anchor only and suppresses its EQU.
 */
static int parse_overlay_arg(arg, fname)
char *arg, *fname;
{
    int i, j, n, id, page, pgend, have_digit;
    char token[MAXSYM + 1];

    file_page = -1;
    file_pgend = -1;
    file_expected_id = 0;
    file_configured = 0;
    file_anchor_count = 0;

    i = 0;
    while (arg[i] && arg[i] != '=' && i < MAXFN - 1) {
        fname[i] = arg[i];
        ++i;
    }
    fname[i] = 0;
    if (arg[i] && arg[i] != '=') {
        puts3("File name too long in descriptor: ", arg, "\n");
        return (0);
    }

    if (arg[i] != '=')
        return (1);

    ++i;
    file_configured = 1;

    if (arg[i] == '-') {
        page = -1;
        pgend = -1;
        ++i;
    }
    else {
        page = 0;
        have_digit = 0;
        while (arg[i] >= '0' && arg[i] <= '9') {
            page = page * 10 + (arg[i] - '0');
            ++i;
            have_digit = 1;
        }
        if (!have_digit) {
            puts3("Invalid overlay page in descriptor: ", arg, "\n");
            return (0);
        }

        /*
         * v4.2: optional page SPAN, =lo-hi.
         *
         *   =10     one page - every existing descriptor, unchanged.
         *   =10-12  this module MAY occupy pages 10,11,12.  Declared
         *           by the author, never inferred.  Must match the
         *           -Plo-hi given to LINK99 for the same module.
         */
        pgend = page;
        if (arg[i] == '-') {
            ++i;
            pgend = 0;
            have_digit = 0;
            while (arg[i] >= '0' && arg[i] <= '9') {
                pgend = pgend * 10 + (arg[i] - '0');
                ++i;
                have_digit = 1;
            }
            if (!have_digit || pgend < page) {
                puts3("Invalid overlay page span in descriptor: ", arg, "\n");
                return (0);
            }
        }
    }

    if (arg[i++] != ',') {
        puts3("Expected page,ID,anchors in descriptor: ", arg, "\n");
        return (0);
    }

    id = 0;
    have_digit = 0;
    while (arg[i] >= '0' && arg[i] <= '9') {
        id = id * 10 + (arg[i] - '0');
        ++i;
        have_digit = 1;
    }
    if (!have_digit || id > MAXOVL) {
        puts3("Invalid overlay ID in descriptor: ", arg, "\n");
        return (0);
    }
    if (page >= 0 && id == 0) {
        puts3("Paged overlay ID must be greater than zero: ", arg, "\n");
        return (0);
    }
    if (arg[i++] != ',') {
        puts3("Expected page,ID,anchors in descriptor: ", arg, "\n");
        return (0);
    }

    file_page = page;
    file_pgend = pgend;
    file_expected_id = id;
    if (id > max_ovlid)
        max_ovlid = id;

    while (arg[i]) {
        if (file_anchor_count >= MAXANCHOR) {
            puts3("Too many anchors in descriptor: ", arg, "\n");
            return (0);
        }

        n = 0;
        while (arg[i] && arg[i] != ':' && arg[i] != ',') {
            if (n >= MAXSYM) {
                puts3("Anchor name too long in descriptor: ", arg, "\n");
                return (0);
            }
            token[n++] = arg[i++];
        }
        token[n] = 0;
        if (n == 0) {
            puts3("Empty anchor in descriptor: ", arg, "\n");
            return (0);
        }

        j = file_anchor_count;
        strcpy(file_anchor_name[j], token);
        strcpy(file_anchor_alias[j], token);
        file_anchor_emit[j] = 1;
        file_anchor_seen[j] = 0;

        if (arg[i] == ':') {
            ++i;
            n = 0;
            while (arg[i] && arg[i] != ',') {
                if (n >= MAXSYM) {
                    puts3("Alias name too long in descriptor: ", arg, "\n");
                    return (0);
                }
                token[n++] = arg[i++];
            }
            token[n] = 0;
            if (!strcmp(token, "-"))
                file_anchor_emit[j] = 0;
            else if (n == 0) {
                puts3("Empty alias in descriptor: ", arg, "\n");
                return (0);
            }
            else
                strcpy(file_anchor_alias[j], token);
        }

        ++file_anchor_count;
        if (arg[i] == ',')
            ++i;
    }

    if (file_anchor_count == 0) {
        puts3("Descriptor has no anchors: ", arg, "\n");
        return (0);
    }
    return (1);
}

/*
 * Write OVLTABLE.INC: the OVL_TABLE storage block for OVLMGR.A99.
 * Fails (and reports) if any overlay ID never received a table row --
 * an incomplete table would send OVLMGR to segment 0, page 0.
 */
static int emit_table()
{
    FILE *fp;
    int i, bad;

    fp = fopen(TABLENAME, "w");
    if (fp == NULL) {
        puts3("Error creating file: ", TABLENAME, "\n");
        return (1);
    }

    fprintf(fp, ";------------------------------------------------------------\n");
    fprintf(fp, ";  OVLTABLE.INC\n");
    fprintf(fp, ";\n");
    fprintf(fp, ";  AUTO-GENERATED BY DREL - DO NOT EDIT\n");
    fprintf(fp, ";\n");
    fprintf(fp, ";  Overlay ID -> up to %d (segment,page) pairs.\n",
            OVLPAGES + 1);
    fprintf(fp, ";  %d words per row, so byte offset = ID*%d.\n",
            2 * (OVLPAGES + 1), 4 * (OVLPAGES + 1));
    fprintf(fp, ";  A zero segment terminates the row.\n");
    fprintf(fp, ";------------------------------------------------------------\n");
    fprintf(fp, "OVL_TABLE:\n");
    fprintf(fp, "\tWORD\t0,0");
    for (i = 0; i < OVLPAGES; ++i)
        fprintf(fp, ",0,0");
    fprintf(fp, "\t;unused ID 0\n");

    bad = 0;
    for (i = 1; i <= max_ovlid; ++i) {
        int j, np, prev;

        np = ovl_seg[i] ? 1 : 0;
        fprintf(fp, "\tWORD\t%d,%d", ovl_seg[i], ovl_page[i]);
        for (j = 0; j < OVLPAGES; ++j) {
            fprintf(fp, ",%d,%d", ovl_seg2[j][i], ovl_page2[j][i]);
            if (ovl_seg2[j][i])
                ++np;
        }
        if (np > 1)
            fprintf(fp, "\t;ID %d (%d pages)\n", i, np);
        else
            fprintf(fp, "\t;ID %d\n", i);

        if (!ovl_seg[i])
            continue;

        prev = ovl_seg[i];
        for (j = 0; j < OVLPAGES; ++j) {
            if (!ovl_seg2[j][i])
                break;
            if (ovl_seg2[j][i] != prev + 1) {
                printf("ERROR: overlay ID %d pages are not adjacent "
                       "(segment %d then %d)\n",
                       i, prev, ovl_seg2[j][i]);
                ++bad;
            }
            prev = ovl_seg2[j][i];
        }
    }
    fclose(fp);

    if (bad) {
        puts3("ERROR: invalid overlay layout in ", TABLENAME, "\n");
        return (1);
    }
    puts3("Generated ", TABLENAME, "\n");
    return (0);
}

static void usage()
{
    putls("Usage:\n");
    putls("  drel file.r99\n");
    putls("  drel -c file1.r99 [file2.r99 ...]\n");
    putls("  drel -c file.r99=page[-page],ID,anchor[:alias][,...] ...\n");
    putls("  drel -i OVLADDR.INC file.r99 ...\n");
    putls("  drel -i OVLADDR.INC file.r99=page,ID,entry[:alias][,entry[:alias]...] ...\n");
    putls("\n");
    putls("Plain file: emit every EPOINT using its exported name.\n");
    putls("Descriptor: select anchors/aliases and generate OVLTABLE.INC.\n");
    putls("Use :- for an anchor that should not emit an EQU.\n");
    putls("\n");
    putls("Examples:\n");
    putls("  drel -i OVLADDR.INC cc_decl.R99=8,1,D_DODECL\n");
    putls("  drel -i OVLADDR.INC olddecl.R99=8,1,dodeclare:D_DODECL\n");
}
