#ifndef REL99_FORMAT_H
#define REL99_FORMAT_H

/* Every numeric field in the REL model is a 16-bit word. */
typedef unsigned short WORD;
typedef signed short   SWORD;
typedef unsigned char  BYTE;

#define REL_ABS       ((WORD)0)
#define REL_PREL      ((WORD)1)
#define REL_DREL      ((WORD)2)
#define REL_CREL      ((WORD)3)
#define REL_ENAME     ((WORD)4)
#define REL_CNAME     ((WORD)5)
#define REL_PNAME     ((WORD)6)
#define REL_LNAME     ((WORD)7)
#define REL_EXT       ((WORD)8)
#define REL_CSIZE     ((WORD)9)
#define REL_XCHAIN    ((WORD)10)
#define REL_EPOINT    ((WORD)11)
#define REL_XMOFF     ((WORD)12)
#define REL_XPOFF     ((WORD)13)
#define REL_DSIZE     ((WORD)14)
#define REL_SETLC     ((WORD)15)
#define REL_CHAIN     ((WORD)16)
#define REL_AORG_MARK REL_CHAIN
#define REL_PSIZE     ((WORD)17)
#define REL_EPROG     ((WORD)18)
#define REL_EFILE     ((WORD)19)

#define REL_MAXSYM    ((WORD)15)
#define REL_BITPSYM   ((WORD)4)

#endif
