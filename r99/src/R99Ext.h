/*
 TMS9900/99105  Cross-Assembler  v. 1.0

 January, 1985

 Original 6800 version Copyright (c) 1980 William C. Colley, III.
 Modified for the TMS9900/99105  Series by Alexander Cameron.

 Declare externals
 */
extern void setfiles(short int, char *);
extern void getopr(unsigned short, char *, unsigned short *, unsigned char *);
extern void getopc(unsigned short, char *, unsigned short *, unsigned char *);
extern int getopcod(unsigned short *, char *);
extern unsigned char getattr(unsigned short);
extern void entnam();
extern void putlin(char *, struct diskbuf *);
extern char ishex(char);
extern int _aton(char, unsigned short);
extern short int putbits(unsigned char, unsigned char);
extern unsigned char getattr(unsigned short);
extern unsigned char getchr(char *, unsigned char);
extern void putchr(char, struct diskbuf *);
extern void putrchr(unsigned char, struct diskbuf *);
extern short int slookup(char *);
extern short int addsym(char *);
extern short int sortsym(unsigned char);
extern void flshhbf(unsigned short);
extern void _psop(unsigned short, char *);
extern void _normop(unsigned short, char);
extern void getname(char *);
extern short int symcmp(char *, char *);
extern void asmline();
extern unsigned char getitem(unsigned short *, unsigned char);
extern short int getlin();
extern void wipeout(char *);
extern void puthex2(unsigned char, char **);
extern void puthex4(unsigned short, char **);
extern unsigned char getprec(char);
extern void markerr(char);
extern short int hash(char *);
extern char toupper(char);
extern void putabs2();
extern void wipeout(char *);
extern short int eval(unsigned char);
extern short int chkoprat(char *);
extern void flush(struct diskbuf *);
extern void gflush(struct diskbuf *);
extern short int getnum(unsigned short *, unsigned char *);
extern void relsyms();
extern short int putrel();
extern short int putfld();
extern short int putsym();

