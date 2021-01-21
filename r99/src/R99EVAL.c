/*
 TMS9900/99105  Cross-Assembler  v. 1.0


 January, 1985

 Original 6800 version Copyright (c) 1980 William C. Colley, III.
 Modified for the TMS9900/99105  Series by Alexander Cameron.

 File:	a99eval.c

 Routines to crunch on source text chunks and give back
 evaluated expressions, opcode parameters, etc.
 */

/*  Get Globals:  */

#include "r99gbl.h"
#include "r99ext.h"

/*
 Function to evaluate the next expression on the present source line.
 Function returns the value of the expression.  In addition, a global
 flag, evalerr, is set to TRUE if an error occurred anywhere in the
 evaluation process.  The precedence parameter, prec, is for the benefit
 of the recursion.  In the first call to the routine, use START.
 */

short int eval(unsigned char prec)

{
	short int valu1, valu2;

	valu1 = valu2 = 0;
	while (TRUE) {
		switch (getitem(&valu1, SMALST)) {

		case COMMA:
			if (prec != START)
				backitem(0, COMMA);
			evalerr = TRUE;
			markerr('E');
			return 0;

		case END_LIN:
			if (prec == ENDEX)
				return 0;
			backitem(0, END_LIN);
			evalerr = TRUE;
			markerr('E');
			return 0;

		case OPERATR:
			if (valu1 == NOT)
				valu1 = ~eval(UOP2);
			else if (valu1 == '-')
				valu1 = -eval(START); /* was UOP1 */
			else if (valu1 == '+')
				valu1 = eval(START); /* was UOP1 */
			else if (valu1 == '(')
				valu1 = eval(LPREN);
			else if (valu1 == HIGH)
				valu1 = eval(UOP3) >> 8;
			else if (valu1 == LOW)
				valu1 = eval(UOP3) & 0xff;
			else if (valu1 == '$') {
				valu1 = eval(START);
				valu1 += pc;
				relflg = TRUE;
			} else {
				evalerr = TRUE;
				markerr('E');
				break;
			}
			return valu1;

		case VALUE:
			/* Added prec=START to take account of operators at begining of expression */
			/* fixes the precedence issue below */
			while (TRUE) {

				switch (getitem(&valu2, SMALST)) {
				/*  EVAL shall now return the delimiter */
				/*  it makes life so much easier for me */

				case COMMA: /* if (prec != START)  */
					backitem(0, COMMA);
					if (prec == LPREN) {
						evalerr = TRUE;
						markerr('(');
					}
					return (evalerr) ? 0 : valu1;

				case END_LIN:
					backitem(0, END_LIN);

					if (prec == LPREN) {
						evalerr = TRUE;
						markerr('(');
					}
					return (evalerr) ? 0 : valu1;

				case VALUE:
					evalerr = TRUE;
					markerr('E');
					break;

				case OPERATR:
					quitflag = FALSE;
					if (valu2 == '+' && addrmode == INDIRECT) {
						addrmode = AUTOINC;
						return valu1;
					}
					if (valu2 == ')' && addrmode == INDEXED)
						return valu1;

					if (valu2 == '(' && addrmode == SYMBOLIC) {
						addrmode = INDEXED;
						quitflag = TRUE;
						return valu1;
					}
					if (valu2
							== '('|| valu2 == NOT || valu2 == HIGH || valu2 == LOW) {
						evalerr = TRUE;
						markerr('E');
						break;
					}
					if (prec <= getprec(valu2)) {
						/* This is a problem [AC] FIXED ABOVE USING START*/

						backitem(valu2, OPERATR);
						return valu1;
					}

					switch (valu2) {
					case '+':
						valu1 += eval(ADDIT);
						break;

					case '-':
						valu1 -= eval(ADDIT);
						break;

					case '*':
						valu1 *= eval(MULT);
						break;

					case '/':
						valu1 /= eval(MULT);
						break;
					case MOD:
						valu1 %= eval(MULT);
						break;
					case AND:
						valu1 &= eval(LOG1);
						break;
					case OR:
						valu1 |= eval(LOG2);
						break;
					case XOR:
						valu1 ^= eval(LOG2);
						break;
					case '>':
						valu1 = (valu1 > eval(RELAT)) ? 0xffff : 0;
						break;

					case GRTEQ:
						valu1 = (valu1 >= eval(RELAT)) ? 0xffff : 0;
						break;

					case '=':
						valu1 = (valu1 == eval(RELAT)) ? 0xffff : 0;
						break;

					case NOTEQ:
						valu1 = (valu1 != eval(RELAT)) ? 0xffff : 0;
						break;

					case '<':
						valu1 = (valu1 < eval(RELAT)) ? 0xffff : 0;
						break;

					case LESEQ:
						valu1 = (valu1 <= eval(RELAT)) ? 0xffff : 0;
						break;

					case SHL:
						if ((valu2 = eval(MULT)) > 15) {
							evalerr = TRUE;
							markerr('E');
							break;
						}
						valu1 = valu1 << valu2;
						break;

					case SHR:
						if ((valu2 = eval(MULT)) > 15) {
							evalerr = TRUE;
							markerr('E');
							break;
						}
						valu1 = valu1 >> valu2;
						break;

					case ')':
						if (prec == LPREN)
							return valu1;
						else {
							evalerr = TRUE;
							markerr('(');
							break;
						}
					}
					if (quitflag)
						return valu1;
				}
			}
		}
	}
}

/*
 Function to get the precedence of a binary operator.
 Function returns the precedence value.
 */

unsigned char getprec(char operator)

{
	switch (operator) {
	case '*':
	case '/':
	case MOD:
	case SHL:
	case SHR:
		return MULT;

	case '+':
	case '-':
		return ADDIT;

	case '>':
	case '=':
	case '<':
	case GRTEQ:
	case NOTEQ:
	case LESEQ:
		return RELAT;

	case AND:
		return LOG1;

	case OR:
	case XOR:
		return LOG2;

	case HIGH:
	case LOW:
		return UOP3;

	case ')':
		return RPREN;
	}
}

/*
 Function to mark up an error in the line if one isn't
 already marked up.  The error code letter comes in
 through letter.
 */

void markerr(char letter)

{
	if (errcode == ' ') {
		errcode = letter;
		errcount++;
	}
}

