#include "R99gbl.h"
#include "REL99.h"
#include "R99Ext.h"

#define GETKN	    1
#define NETKN		2
#define LETKN		3
#define ANDTKN		4
#define ORTKN		5
#define XORTKN		6
#define NOTTKN		7
#define MODTKN		8
#define SHLTKN		9
#define SHRTKN		10
#define HITKN		11
#define LOWTKN		12
#define GTTKN		'>'
#define EQTKN		'='
#define LTTKN		'<'

#define OPCODE_SLOT 5
char opcode_table[505] = {
    65, 0, 0, 0, 0, 65, 66, 0, 0, 0, 65, 66,
    83, 0, 0, 65, 73, 0, 0, 0, 65, 78, 68, 73,
    0, 65, 79, 82, 71, 0, 66, 0, 0, 0, 0, 66,
    73, 78, 68, 0, 66, 76, 0, 0, 0, 66, 76, 83,
    75, 0, 66, 76, 87, 80, 0, 66, 83, 83, 0, 0,
    66, 89, 84, 69, 0, 67, 0, 0, 0, 0, 67, 66,
    0, 0, 0, 67, 73, 0, 0, 0, 67, 75, 79, 70,
    0, 67, 75, 79, 78, 0, 67, 76, 82, 0, 0, 67,
    79, 67, 0, 0, 67, 90, 67, 0, 0, 68, 69, 67,
    0, 0, 68, 69, 67, 84, 0, 68, 73, 86, 0, 0,
    68, 73, 86, 83, 0, 68, 88, 79, 80, 0, 69, 76,
    83, 69, 0, 69, 78, 68, 0, 0, 69, 78, 68, 73,
    0, 69, 78, 84, 0, 0, 69, 81, 85, 0, 0, 69,
    86, 69, 78, 0, 69, 88, 84, 0, 0, 73, 68, 76,
    69, 0, 73, 70, 0, 0, 0, 73, 78, 67, 0, 0,
    73, 78, 67, 76, 0, 73, 78, 67, 84, 0, 73, 78,
    86, 0, 0, 74, 69, 81, 0, 0, 74, 71, 84, 0,
    0, 74, 72, 0, 0, 0, 74, 72, 69, 0, 0, 74,
    76, 0, 0, 0, 74, 76, 69, 0, 0, 74, 76, 84,
    0, 0, 74, 77, 80, 0, 0, 74, 78, 67, 0, 0,
    74, 78, 69, 0, 0, 74, 78, 79, 0, 0, 74, 79,
    67, 0, 0, 74, 79, 80, 0, 0, 76, 68, 67, 82,
    0, 76, 73, 0, 0, 0, 76, 73, 77, 73, 0, 76,
    82, 69, 88, 0, 76, 83, 84, 0, 0, 76, 87, 80,
    0, 0, 76, 87, 80, 73, 0, 77, 79, 86, 0, 0,
    77, 79, 86, 66, 0, 77, 80, 89, 0, 0, 77, 80,
    89, 83, 0, 78, 65, 77, 0, 0, 78, 69, 71, 0,
    0, 78, 79, 80, 0, 0, 79, 82, 71, 0, 0, 79,
    82, 73, 0, 0, 82, 83, 69, 84, 0, 82, 84, 0,
    0, 0, 82, 84, 87, 80, 0, 83, 0, 0, 0, 0,
    83, 66, 0, 0, 0, 83, 66, 79, 0, 0, 83, 66,
    90, 0, 0, 83, 69, 84, 0, 0, 83, 69, 84, 79,
    0, 83, 76, 65, 0, 0, 83, 79, 67, 0, 0, 83,
    79, 67, 66, 0, 83, 82, 65, 0, 0, 83, 82, 67,
    0, 0, 83, 82, 76, 0, 0, 83, 84, 67, 82, 0,
    83, 84, 83, 84, 0, 83, 84, 87, 80, 0, 83, 87,
    80, 66, 0, 83, 90, 67, 0, 0, 83, 90, 67, 66,
    0, 84, 66, 0, 0, 0, 84, 67, 77, 66, 0, 84,
    69, 88, 84, 0, 84, 77, 66, 0, 0, 84, 82, 79,
    70, 0, 84, 82, 79, 78, 0, 84, 83, 77, 66, 0,
    87, 79, 82, 68, 0, 87, 82, 69, 78, 0, 88, 0,
    0, 0, 0, 88, 79, 80, 0, 0, 88, 79, 82, 0,
    0
};
unsigned char opcode_attr[101] = { 0x09, 0x09, 0x01, 0x35, 0x35, 0x80, 0x01,
		0x01, 0x01, 0x35, 0x01, 0x80, 0x80, 0x09, 0x09, 0x35, 0x00, 0x00, 0x01,
		0x19, 0x19, 0x01, 0x01, 0x19, 0x01, 0xC0, 0xC0, 0x80, 0xC0, 0x80, 0x80,
		0x80, 0x80, 0x00, 0xC0, 0x01, 0x80, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02,
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x19, 0x35, 0x06, 0x00,
		0x05, 0x05, 0x06, 0x09, 0x09, 0x19, 0x01, 0x80, 0x01, 0x00, 0x80, 0x35,
		0x00, 0x00, 0x00, 0x09, 0x09, 0x04, 0x04, 0x80, 0x01, 0x2D, 0x09, 0x09,
		0x2D, 0x2D, 0x2D, 0x09, 0x05, 0x05, 0x01, 0x09, 0x09, 0x04, 0x07, 0x80,
		0x07, 0x00, 0x00, 0x07, 0x80, 0x00, 0x01, 0x19, 0x19 };
unsigned opcode_opcode[101] = { 0xA000, 0xB000, 0x0740, 0x0220,
		0x0240, 0x0000, 0x0440, 0x0140, 0x0680, 0x00B0, 0x0400, 0x0005, 0x0002,
		0x8000, 0x9000, 0x0280, 0x03C0, 0x03A0, 0x04C0, 0x2000, 0x2400, 0x0600,
		0x0640, 0x3C00, 0x0180, 0x000C, 0x0007, 0x0006, 0x0008, 0x000E, 0x0001,
		0x000B, 0x000D, 0x0340, 0x0009, 0x0580, 0x0010, 0x05C0, 0x0540, 0x1300, 0x1500,
		0x1B00, 0x1400, 0x1A00, 0x1200, 0x1100, 0x1000, 0x1700, 0x1600, 0x1900,
		0x1800, 0x1C00, 0x3000, 0x0200, 0x0300, 0x03E0, 0x0080, 0x0090, 0x02E0,
		0xC000, 0xD000, 0x3800, 0x01C0, 0x000F, 0x0500, 0x1000, 0x0000, 0x0260,
		0x0360, 0x045B, 0x0380, 0x6000, 0x7000, 0x1D00, 0x1E00, 0x000A, 0x0700,
		0x0A00, 0xE000, 0xF000, 0x0800, 0x0B00, 0x0900, 0x3400, 0x02C0, 0x02A0,
		0x06C0, 0x4000, 0x5000, 0x1F00, 0x0C0A, 0x0004, 0x0C09, 0x03C0, 0x03E0,
		0x0C0B, 0x0003, 0x03A0, 0x0480, 0x2C00, 0x2800 };

#define OPR_SLOT 5
char opr_table[75] = {
    65, 78, 68, 0, 0, 69, 81, 0, 0, 0, 71, 69,
    0, 0, 0, 71, 84, 0, 0, 0, 72, 73, 71, 72,
    0, 76, 69, 0, 0, 0, 76, 79, 87, 0, 0, 76,
    84, 0, 0, 0, 77, 79, 68, 0, 0, 78, 69, 0,
    0, 0, 78, 79, 84, 0, 0, 79, 82, 0, 0, 0,
    83, 72, 76, 0, 0, 83, 72, 82, 0, 0, 88, 79,
    82, 0, 0
};

unsigned char opr_attr[15] = {
ANDTKN, EQTKN, GETKN, GTTKN, HITKN, LETKN, LOWTKN, LTTKN, MODTKN, NETKN, NOTTKN,
ORTKN, SHLTKN, SHRTKN, XORTKN };

unsigned char char_attr[128] = {
	/*BLOCK TRASH 9 */
	TRASH, TRASH, TRASH, TRASH, TRASH, TRASH, TRASH, TRASH, TRASH,
	BLANK, END_LIN, TRASH, TRASH, TRASH,
	TRASH, TRASH, TRASH, TRASH, TRASH, TRASH, TRASH, TRASH, TRASH,
	TRASH, TRASH, TRASH, TRASH, TRASH, TRASH, TRASH, TRASH, TRASH,
	BLANK, ALPHA, QUOTE, HATCH, OPERATR, BAS_DES, ALPHA, QUOTE,
	OPERATR, OPERATR, OPERATR, OPERATR, COMMA, OPERATR, ALPHA, OPERATR,
	/* 0x30 decimal 48 */
	NUMERIC, NUMERIC, NUMERIC, NUMERIC, NUMERIC, NUMERIC, NUMERIC, NUMERIC,
	NUMERIC, NUMERIC,
	/* 3A, 58 */
	COLON, END_LIN, OPERATR, OPERATR, OPERATR, ALPHA, OPERATR,
	ALPHA, ALPHA, ALPHA, ALPHA, ALPHA, ALPHA, ALPHA, ALPHA, ALPHA,
	ALPHA, ALPHA, ALPHA, ALPHA,
	ALPHA, ALPHA, ALPHA, ALPHA, ALPHA, ALPHA, ALPHA, ALPHA, ALPHA,
	ALPHA, ALPHA, ALPHA, ALPHA,
	ALPHA, ALPHA, ALPHA, ALPHA, ALPHA, ALPHA,
	ALPHA, ALPHA, ALPHA, ALPHA, ALPHA, ALPHA, ALPHA, ALPHA, ALPHA,
	ALPHA, ALPHA, ALPHA, ALPHA,
	ALPHA, ALPHA, ALPHA, ALPHA, ALPHA, ALPHA, ALPHA, ALPHA, ALPHA,
	ALPHA, ALPHA, ALPHA, ALPHA,
	ALPHA, ALPHA, ALPHA, ALPHA, TRASH };

numopcs()
{

	return 100;

}

numoprs()
{

	return 14;

}

getattr(c)
unsigned c;
{
	return char_attr[c];
}

getopc(index, opcode_m, opcode, attrib)
unsigned index;
char *opcode_m;
unsigned *opcode;
unsigned char *attrib;
{
	char *s;
	char *d;
	s = opcode_table + index * OPCODE_SLOT;
	d = opcode_m;
	while (*s != '\0') {
		*d++ = *s++;
	}
	*d = '\0';
	*attrib = opcode_attr[index];
	*opcode = opcode_opcode[index];

}

getopr(index, opr_m, opr_code, opr_attrib)
unsigned index;
char *opr_m;
unsigned *opr_code;
unsigned char *opr_attrib;
{

	char *s;
	char *d;
	s = opr_table + index * OPR_SLOT;
	d = opr_m;
	while (*s != '\0') {
		*d++ = *s++;
	}
	*d = '\0';
	*opr_attrib = opr_attr[index];
}
