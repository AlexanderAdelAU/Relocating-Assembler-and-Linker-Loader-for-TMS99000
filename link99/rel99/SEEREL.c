/*
 ** SEEREL.C -- display decoded REL items
 **
 ** This is a generic shared REL-display module. It contains no DREL-only
 ** include-generation logic and may be linked by DREL, LINK99 and other
 ** REL utilities without additional globals.
 */
#include "rel99.h"
#include "stdio.h"

int lc, width;

/* common REL decoder variables */
extern int
inrel,
inrem,
inchunk,
outrel,
outrem,
outchunk,
item,
type,
field;
extern char symbol[MAXSYM + 1];

seerel()
{
    char str[MAXSYM + 1];
    int tmp;

    switch(item) {
    case ABS:
        see8(field, ' ');
        lc += 1;
        newlin(NO);
        return;

    case PREL:
    case DREL:
    case CREL:
        see16();
        lc += 2;
        newlin(NO);
        return;

    case XMOFF:
    case XPOFF:
        tmp = type;
        type = item;
        see16();
        type = tmp;
        newlin(NO);
        return;

    case ENAME:
        seenam("     entry: ", NO);
        goto eol;

    case CNAME:
        seenam("    common: ", NO);
        goto eol;

    case PNAME:
        putchar('\n');
        seenam("-  program: ", NO);
        lc = 0;
        goto eol;

    case LNAME:
        seenam("   library: ", NO);
        goto eol;

    case EXT:
        putstring("extension link item\n");
        return;

    case CSIZE:
        seenam(" common sz: ", YES);
        goto eol;

    case XCHAIN:
        seenam(" ext chain: ", YES);
        goto eol;

    case EPOINT:
        seenam("  entry pt: ", YES);
        goto eol;

    case DSIZE:
        putstring(" data size: ");
        goto fld;

    case SETLC:
        putstring("load at: ");
        lc = field;
        goto fld;

    case CHAIN:
        putstring(" aorg base: ");
        goto fld;

    case PSIZE:
        putstring(" prog size: ");
        goto fld;

    case EPROG:
        putstring("- end prog: ");
        goto fld;

    case EFILE:
        putstring("- end file");
        goto eol;

fld:
        see16();
eol:
        newlin(YES);
        return;
    }

    itou(item, str, MAXSYM);
    puts2(str);
    puts2(" is an Unknown Item Code\n");
}

see8(value, suff)
int value;
int suff;
{
    char str[5];

    if (width == 0 && item < CREL) {
        itox(lc, str, 5);
        outz(str);
        putchar(' ');
    }

    itox(value & 255, str, 3);
    outz(str);
    if (suff)
        putchar(suff);
    ++width;
}

see16()
{
    see8(field >> 8, 0);
    see8(field, xtype());
    putchar(' ');
}

seenam(pref, val)
char *pref;
int val;
{
    newlin(YES);
    width = 1;
    putstring(pref);
    if (val)
        see16();
    putstring(symbol);
}

xtype()
{
    switch(type) {
    case ABS:   return(' ');
    case PREL:  return('\'');
    case DREL:  return('"');
    case CREL:  return('~');
    case XPOFF: return('+');
    case XMOFF: return('-');
    }
    return('?');
}

newlin(nl)
int nl;
{
    if (width > 15 || (nl && width)) {
        putchar('\n');
        width = 0;
    }
}

outz(str)
char *str;
{
    char *cp;

    cp = str;
    while (*cp == ' ')
        *cp++ = '0';
    putstring(str);
}
