/*
 * itoo -- convert nbr to octal string of width sz
 *         right adjusted, blank filled
 *
 *         if sz > 0 terminate with null byte
 *         if sz  = 0 find end of string
 *         if sz < 0 use last byte for data
 */

itoo(nbr, str, sz)
    int nbr; char str[]; int sz;
{
    int i;

    if (sz > 0)
        str[--sz] = '\0';
    else if (sz < 0)
        sz = -sz;
    else {
        while (str[sz] != '\0')
            ++sz;
    }

    i = sz;
    if (nbr == 0) {
        if (i > 0) str[--i] = '0';
    } else {
        while (nbr && i > 0) {
            str[--i] = (nbr & 7) + '0';
            nbr = (nbr >> 3) & 0x1FFF;  /* logical shift - mask sign bits */
        }
    }
    while (i > 0) str[--i] = ' ';
    return str;
}
