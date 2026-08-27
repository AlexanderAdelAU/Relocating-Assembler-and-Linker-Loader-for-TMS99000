/*
 TMS9900/99105 Cross-Assembler external declarations.

 Refactored for the Small C 2.2 declaration model.  Small C does not
 implement ANSI prototypes, so these declarations intentionally carry
 no parameter lists or return-type promises.
 */

extern setfiles();
extern getopr();
extern getopc();
extern getopcod();
extern getattr();
extern entnam();
extern putlin();
extern ishex();
extern _aton();
extern putbits();
extern getchr();
extern putchr();
extern putrchr();
extern rputchr();
extern slookup();
extern addsym();
extern sortsym();
extern flshhbf();
extern _psop();
extern _normop();
extern getname();
extern symcmp();
extern asmline();
extern getitem();
extern getlin();
extern source_rewind();
extern include_source();
extern push_source();
extern close_includes();
extern wipeout();
extern puthex2();
extern puthex4();
extern putdec();
extern getprec();
extern markerr();
extern hash();
extern toupper();
extern putabs2();
extern putabs();
extern eval();
extern chkoprat();
extern flush();
extern gflush();
extern rflush();
extern getnum();
extern relsyms();
extern putrel();
extern putfld();
extern putsym();
extern xref_add();
extern xref_reset();
extern xref_emit();
extern lineout();
extern hexout8();
extern hexout16();
extern relout();
extern relhead();
extern backitem();
extern bbsearch();
extern numopcs();
extern numoprs();
extern movchr();
extern seek();
extern flushlin();
extern puts2();
extern puts3();
extern putls();
extern cant();
extern error2();
extern error();
