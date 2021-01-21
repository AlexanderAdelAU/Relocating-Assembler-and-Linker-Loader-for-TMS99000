#include "r99gbl.h"
#include "rel99.h"
#include "R99ext.h"

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

static char *opcode_table[] = { "A", "AB", "ABS", "AI", "ANDI", "AORG", /*6*/
"B", "BIND", "BL", "BLSK", "BLWP", "BSS", "BYTE", /*7*/
"C", "CB", "CI", "CKOF", "CKON", "CLR", "COC", "CZC",/*8*/
"DEC", "DECT", "DIV", "DIVS", "DXOP",/*5*/
"ELSE", "END", "ENDI", "ENT", "EQU", "EVEN", "EXT",/*7*/
"IDLE", "IF", "INC", "INCT", "INV", /*5*/
"JEQ", "JGT", "JH", "JHE", "JL", "JLE", "JLT", "JMP", "JNC", "JNE", "JNO",
		"JOC", "JOP",/*13*/
		"LDCR", "LI", "LIMI", "LREX", "LST", "LWP", "LWPI",/*7*/
		"MOV", "MOVB", "MPY", "MPYS",/*4*/
		"NAM", "NEG", "NOP", "ORG", "ORI", "RSET", "RT", "RTWP",/*8*/
		"S", "SB", "SBO", "SBZ", "SET", "SETO", "SLA", "SOC", "SOCB", "SRA",
		"SRC", "SRL", "STCR", "STST", "STWP", "SWPB", "SZC", "SZCB",/*18*/
		"TB", "TCMB", "TEXT", "TMB", "TROF", "TRON", "TSMB",/*7*/
		"WORD", "WREN", "X", "XOP", "XOR" /*5*/

};
static unsigned char opcode_attr[] = { 0x09, 0x09, 0x01, 0x35, 0x35, 0x80, 0x01,
		0x01, 0x01, 0x35, 0x01, 0x80, 0x80, 0x09, 0x09, 0x35, 0x00, 0x00, 0x01,
		0x19, 0x19, 0x01, 0x01, 0x19, 0x01, 0xC0, 0xC0, 0x80, 0xC0, 0x80, 0x80,
		0x80, 0x80, 0x00, 0xC0, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02,
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x19, 0x35, 0x06, 0x00,
		0x05, 0x05, 0x06, 0x09, 0x09, 0x19, 0x01, 0x80, 0x01, 0x00, 0x80, 0x35,
		0x00, 0x00, 0x00, 0x09, 0x09, 0x04, 0x04, 0x80, 0x01, 0x2D, 0x09, 0x09,
		0x2D, 0x2D, 0x2D, 0x09, 0x05, 0x05, 0x01, 0x09, 0x09, 0x04, 0x07, 0x80,
		0x07, 0x00, 0x00, 0x07, 0x80, 0x00, 0x01, 0x19, 0x19 };
static unsigned short opcode_opcode[] = { 0xA000, 0xB000, 0x0740, 0x0220,
		0x0240, 0x0000, 0x0440, 0x0140, 0x0680, 0x00B0, 0x0400, 0x0005, 0x0002,
		0x8000, 0x9000, 0x0280, 0x03C0, 0x03A0, 0x04C0, 0x2000, 0x2400, 0x0600,
		0x0640, 0x3C00, 0x0180, 0x000C, 0x0007, 0x0006, 0x0008, 0x000E, 0x0001,
		0x000B, 0x000D, 0x0340, 0x0009, 0x0580, 0x05C0, 0x0540, 0x1300, 0x1500,
		0x1B00, 0x1400, 0x1A00, 0x1200, 0x1100, 0x1000, 0x1700, 0x1600, 0x1900,
		0x1800, 0x1C00, 0x3000, 0x0200, 0x0300, 0x03E0, 0x0080, 0x0090, 0x02E0,
		0xC000, 0xD000, 0x3800, 0x01C0, 0x000F, 0x0500, 0x1000, 0x0000, 0x0260,
		0x0360, 0x045B, 0x0380, 0x6000, 0x7000, 0x1D00, 0x1E00, 0x000A, 0x0700,
		0x0A00, 0xE000, 0xF000, 0x0800, 0x0B00, 0x0900, 0x3400, 0x02C0, 0x02A0,
		0x06C0, 0x4000, 0x5000, 0x1F00, 0x0C0A, 0x0004, 0x0C09, 0x03C0, 0x03E0,
		0x0C0B, 0x0003, 0x03A0, 0x0480, 0x2C00, 0x2800 };

static char *opr_table[] = { "AND", "EQ", "GE", "GT", "HIGH", "LE", "LOW", "LT",
		"MOD", "NE", "NOT", "OR", "SHL", "SHR", "XOR" };

static unsigned char opr_attr[] = {
ANDTKN, EQTKN, GETKN, GTTKN, HITKN, LETKN, LOWTKN, LTTKN, MODTKN, NETKN, NOTTKN,
ORTKN, SHLTKN, SHRTKN, XORTKN };

int numopcs() {

	return 100;

}

int numoprs() {

	return 14;

}

unsigned char getattr(unsigned short c) {
	static unsigned char attrib[] = {
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
	return attrib[c];
}

void getopc(unsigned short index, char *opcode_m, unsigned short *opcode,
		unsigned char *attrib) {
	char *s;
	char *d;
	s = opcode_table[index];
	d = opcode_m;
	while (*s != '\0') {
		*d++ = *s++;
	}
	*d = '\0';
	*attrib = opcode_attr[index];
	*opcode = opcode_opcode[index];

}

void getopr(unsigned short index, char *opr_m, unsigned short *opr_code,
		unsigned char *opr_attrib) {

	char *s;
	char *d;
	s = opr_table[index];
	d = opr_m;
	while (*s != '\0') {
		*d++ = *s++;
	}
	*d = '\0';
	*opr_attrib = opr_attr[index];
}
