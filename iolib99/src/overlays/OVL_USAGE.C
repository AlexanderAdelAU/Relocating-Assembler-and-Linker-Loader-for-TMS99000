/*
** OVL_USAGE.C -- LINK99 usage/error overlay.
**
** First real LINK99 overlay, following the proven FILEEDIT/FILESTAT model.
** Publish an explicit assembler entry anchor at >8000 so DREL does not
** have to infer the callable overlay address from the Small-C symbol.
*/
#asm
        AORG    08000H
        ENT     OVL_USAGE_ENTRY

OVL_USAGE_ENTRY:
        B       @ovlusage
#endasm

extern error();

ovlusage()
{
    error("Usage: LNK [-B] [-G#] [-L] [-M] program [module/library...]");
}
