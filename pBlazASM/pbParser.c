/*
 *  Copyright © 2003..2008 : Henk van Kampen <henk@mediatronix.com>
 *
 *	This file is part of pBlazASM.
 *
 *  pBlazASM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pBlazASM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pBlazASM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include "pbTypes.h"
#include "pbSymbols.h"
#include "pbLexer.h"
#include "pbErrors.h"

//#ifdef _MSC_VER	//Microsoft Visual C doesn't have strcasemcp, but has stricmp instead
#define strcasecmp	stricmp
//#endif


//! \file
//!  pBlazASM work horse
//!

//! \brief
// assemler states
typedef enum {
	bsINIT, bsLABEL, bsLABELED, bsSYMBOL, bsOPCODE, bsDIRECTIVE, bsOPERAND, bsEND
} build_state ;

static bool bMode = false ; //! KCPSM mode, accepts 'NAMEREG' etc
static bool bCode = true ; // list code
static uint32_t gCode[ 1024 + 256 / 2 ] ; // code space and scratchpad space (in words)
static char * gSource ; // source filename for error printer
static int gLinenr = 0 ; // current line number
static uint32_t gPC = 0 ; // current code counter
static uint32_t gSCR = 2048 ; // current scratchpad counter

static const char * s_errors[] = {
		"<none>", "unexpected tokens", "doubly defined", "undefined", "phasing error", "missing symbol",
		"syntax error", "syntax error in expression", "syntax error in operand", "syntax error in operand",
		"syntax error in operator", "syntax error, register expected", "comma expected", "unexpected characters",
		"expression expected", "code size > 1024", "<not-implemented>", "<internal error>" } ;

static error_t expression( uint32_t * result ) ;

//! \brief
//! handle 'expression' <operator> 'term'
//!
static bool processOperator( unsigned int * result, unsigned int term, symbol_t ** oper ) {
	if ( *oper == NULL ) {
		*result = term ;
		return true ;
	}
	switch ( ( *oper )->subtype ) {
	case stMUL :
		*result *= term ;
		break ;
	case stDIV :
		*result /= term ;
		break ;
	case stADD :
		*result += term ;
		break ;
	case stSUB :
		*result -= term ;
		break ;
	case stIOR :
		*result |= term ;
		break ;
	case stXOR :
		*result ^= term ;
		break ;
	case stAND :
		*result &= term ;
		break ;
	case stSHL :
		*result <<= term ;
		break ;
	case stSHR :
		*result >>= term ;
		break ;
	default :
		return false ;
	}
	*oper = NULL ;
	return true ;
}

// convert escaped char's
static char convert_char( char * * p ) {
	int r = 0 ;
	char * s = *p ;

	if ( *s == '\\' ) { // '\r' or '\013'
		s++ ;
		switch ( *s++ ) { // \a \b \f \n \r \t \v
		case '\'' :
			r = '\'' ;
			break ;
		case '\\' :
			r = '\\' ;
			break ;
		case '"' :
			r = '"' ;
			break ;
		case 'a' :
		case 'A' :
			r = '\a' ;
			break ;
		case 'b' :
		case 'B' :
			r = '\b' ;
			break ;
		case 'f' :
		case 'F' :
			r = '\f' ;
			break ;
		case 'n' :
		case 'N' :
			r = '\n' ;
			break ;
		case 'r' :
		case 'R' :
			r = '\r' ;
			break ;
		case 't' :
		case 'T' :
			r = '\t' ;
			break ;
		case 'v' :
		case 'V' :
			r = '\v' ;
			break ;

		case 'x' :
		case 'X' :
			if ( sscanf( s, "%x", &r ) != 1 )
				return etLEX ;
			while ( isxdigit( *s ) )
				s++ ;
			break ;
		case '0' :
			--s ;
			if ( sscanf( s, "%o", &r ) != 1 )
				return etLEX ;
			while ( isdigit( *s ) && *s != '8' && *s != '9' )
				s++ ;
			break ;
		default :
			return etLEX ;
		}
	} else
		r = *s++ ;
	*p = s ;
	return r ;
}

// convert escaped char's in a string
static char * convert_string( char * s ) {
	char * r = calloc( 1, 256 ), *p ;

	for ( p = r ; *s != '\0' ; )
		*p++ = convert_char( &s ) ;
	*p++ = '\0' ;
	return r ;
}

static error_t term( uint32_t * result ) {
	symbol_t * oper = NULL ;
	const char * p = NULL ;
	symbol_t * h = NULL ;
	error_t e = etNONE ;
	char * s = NULL ;
	uint32_t val ;

	// full expression handling
	if ( tok_current()->type == tNONE )
		return etEXPR ;

	if ( tok_current()->type == tOPERATOR )
		oper = tok_next() ;

	s = tok_current()->text ;
	switch ( tok_current()->type ) {
	case tDEC :
		if ( sscanf( s, "%d", &val ) != 1 )
			return etEXPR ;
		break ;
	case tCHAR :
		val = convert_char( &s ) ;
		break ;
	case tHEX :
		if ( sscanf( s, "%X", &val ) != 1 )
			return etEXPR ;
		break ;
	case tBIN :
		// parse a binary value
		val = 0 ;
		for ( p = s ; *p != 0 ; p++ ) {
			val <<= 1 ;
			if ( *p == '1' )
				val |= 1 ;
		}
		break ;
	case tIDENT :
		h = find_symbol( s, false ) ;
		if ( h == NULL )
			return etUNDEF ;
		val = h->value & 0xFFFF ;
		if ( h->type != tVALUE && h->type != tLABEL )
			return etVALUE ;
		break ;
	case tLPAREN :
		tok_next() ;
		e = expression( &val ) ;
		if ( e != etNONE )
			return e ;
		if ( tok_current()->type != tRPAREN )
			return etEXPR ;
		break ;
	default :
		return etEXPR ;
	}
	tok_next() ;
	if ( oper != NULL ) {
		switch ( oper->subtype ) {
		case stSUB :
			*result = -val ;
			break ;
		case stTILDA :
			*result = ~val ;
			break ;
		default :
			return etOPERATOR ;
		}
	} else
		*result = val ;
	return etNONE ;
}

//! \brief
//! eat expression
//!
static error_t expression( uint32_t * result ) {
	symbol_t * h = NULL ;
	char * s = NULL ;
	symbol_t * oper = NULL ;
	error_t e = etNONE ;
	uint32_t val ;

	*result = 0 ;

	// crippled expression handling
	while ( bMode && tok_current()->type != tNONE ) {
		switch ( tok_current()->type ) {
		case tLPAREN :
			tok_next() ;
			e = expression( &val ) ;
			if ( e != etNONE )
				return e ;
			if ( !processOperator( result, val, &oper ) )
				return etOPERATOR ;
		case tRPAREN :
			tok_next() ;
			return etNONE ;
		case tCOMMA :
			return etNONE ;
		case tIDENT :
			s = tok_current()->text ;
			h = find_symbol( s, false ) ;
			if ( h != NULL ) {
				val = h->value ;
			} else if ( sscanf( s, "%X", &val ) != 1 )
				return etEXPR ;
			tok_next() ;
			*result = val ;
			return etNONE ;
		default :
			return etEXPR ;
		}
	}

	if ( tok_current()->type == tNONE ) {
		return etEMPTY ;
	}

	// full expression handling
	while ( tok_current()->type != tNONE ) {
		switch ( tok_current()->type ) {
		case tLPAREN :
			tok_next() ;
			e = expression( &val ) ;
			if ( e != etNONE )
				return e ;
			if ( tok_current()->type == tRPAREN ) {
				tok_next() ;
			} else
				return etEXPR ;
			break ;
		case tOPERATOR :
		case tDEC :
		case tCHAR :
		case tHEX :
		case tBIN :
		case tIDENT :
			e = term( &val ) ;
			if ( e != etNONE )
				return e ;
			break ;
		default :
			return etNONE ;
		}
		if ( oper != NULL ) {
			if ( !processOperator( result, val, &oper ) )
				return etOPERATOR ;
			oper = NULL ;
		} else
			*result = val ;
		if ( tok_current()->type == tOPERATOR )
			oper = tok_next() ;
		else
			break ;
	}
	return etNONE ;
}

//! \brief destreg
//! eat destination register
//!
static bool destreg( uint32_t * result ) {
	symbol_t * h ;

	if ( result == NULL )
		return false ;
	*result = 0 ;

	h = find_symbol( tok_current()->text, false ) ;
	if ( h != NULL && h->type == tREGISTER ) {
		tok_next() ;
		*result = h->value << 8 ;
		return true ;
	}
	return false ;
}

//! \brief srcreg
//! eat source register
//!
static bool srcreg( uint32_t * result ) {
	symbol_t * h ;
	bool bpar = false ;
	bool retval = true ;
	symbol_t * back = tok_current() ;

	if ( result == NULL )
		return false ;
	*result = 0 ;

	if ( tok_current()->type == tLPAREN ) {
		bpar = true ;
		tok_next() ;
	}

	h = find_symbol( tok_current()->text, false ) ;
	if ( h == NULL || h->type != tREGISTER ) {
		retval = false ;
		goto finally ;
	}
	*result = h->value << 4 ;

	tok_next() ;
	if ( bpar ) {
		if ( tok_current()->type == tRPAREN )
			tok_next() ;
		else {
			retval = false ;
			goto finally ;
		}
	}

	finally: {
		if ( !retval )
			tok_back( back ) ;
		return retval ;
	}
}

// eat comma
static bool comma( void ) {
	if ( tok_current()->type == tCOMMA ) {
		tok_next() ;
		return true ;
	} else
		return false ;
}

// eat condition
static bool condition( uint32_t * result ) {
	symbol_t * h ;

	if ( result == NULL )
		return false ;
	*result = 0 ;

	if ( tok_current()->type != tNONE ) {
		h = find_symbol( tok_current()->text, true ) ;
		if ( h != NULL && h->type == tCONDITION ) {
			tok_next() ;
			*result = h->value ;
			return true ;
		}
	}
	return false ;
}

// eat enable/disable
static bool enadis( uint32_t * result ) {
	symbol_t * h ;

	if ( result == NULL )
		return false ;
	*result = 0 ;

	h = find_symbol( tok_current()->text, true ) ;
	if ( h != NULL && h->type == tOPCODE && h->subtype == stINTI ) {
		tok_next() ;
		*result = h->value & 1 ;
		return true ;
	}
	return false ;
}

static error_t build( void ) {
	build_state state = bsINIT ;
	symbol_t * symtok = NULL ;
	symbol_t * h = NULL ;
	symbol_t * r = NULL ;
	uint32_t result = 0 ;
	error_t e = etNONE ;

	// process statement
	for ( tok_first(), state = bsINIT ; state != bsEND ; tok_next() ) {
		switch ( tok_current()->type ) {
		case tNONE :
			// empty line?
			if ( state != bsINIT && state != bsLABELED )
				return etSYNTAX ;
			state = bsEND ;
			break ;

			// opcode or symbol definition
		case tIDENT :
			// opcode or directive?
			h = find_symbol( tok_current()->text, false ) ;
			if ( h != NULL ) {
				switch ( h->type ) {
				case tOPCODE :
					if ( state != bsINIT && state != bsLABELED )
						return etSYNTAX ;
					gPC += 1 ;
					state = bsEND ; // we know enough for now
					break ;

				case tDIRECTIVE :
					switch ( h->subtype ) {

					// ORG
					case stADDRESS :
					case stORG :
						if ( state != bsINIT )
							return etSYNTAX ;
						tok_next() ;
						if ( ( e = expression( &result ) ) == etNONE ) {
							if ( result >= 1024 ) // within code range
								return etRANGE ;
							gPC = result ;
						} else
							return e ;
						state = bsEND ;
						break ;

						// _SRC
					case stSCRATCHPAD :
						if ( state != bsINIT )
							return etSYNTAX ;
						tok_next() ;
						if ( ( e = expression( &result ) ) == etNONE ) {
							if ( result >= 1024 + 128 ) // within code + scratchpad range
								return etRANGE ;
							gSCR = result * 2 ;
						} else
							return e ;
						state = bsEND ;
						break ;

						// EQU
					case stEQU :
						if ( state != bsSYMBOL )
							return etSYNTAX ;
						tok_next() ;
						if ( symtok != NULL ) {
							// register clone?
							r = find_symbol( tok_current()->text, false ) ;
							if ( r != NULL && r->type == tREGISTER ) {
								if ( !add_symbol( tREGISTER, stCLONE, symtok->text, r->value ) )
									return etDOUBLE ;
								else {
									state = bsEND ;
									break ;
								}
							} else if ( ( e = expression( &result ) ) == etNONE ) {
								// normal expression?
								if ( !add_symbol( tVALUE, stINT, symtok->text, result ) )
									return etDOUBLE ;
							} else
								return e ;
						} else
							return etMISSING ;
						state = bsEND ;
						break ;

					case stCONSTANT :
						if ( state != bsINIT )
							return etSYNTAX ;
						tok_next() ;
						symtok = tok_next() ;
						if ( symtok->type != tIDENT )
							return etSYNTAX ;
						if ( !comma() )
							return etCOMMA ;
						// normal expression?
						if ( ( e = expression( &result ) ) == etNONE ) {
							if ( !add_symbol( tVALUE, stINT, symtok->text, result ) )
								return etDOUBLE ;
						} else
							return e ;
						state = bsEND ;
						break ;

					case stNAMEREG :
						if ( state != bsINIT )
							return etSYNTAX ;
						tok_next() ;
						symtok = tok_next() ;
						if ( symtok->type != tIDENT )
							return etSYNTAX ;
						r = find_symbol( symtok->text, true ) ;
						if ( r == NULL || r->type != tREGISTER )
							return etREGISTER ;
						if ( !comma() )
							return etCOMMA ;
						if ( tok_current()->type == tIDENT ) {
							if ( !add_symbol( tREGISTER, stCLONE, tok_current()->text, r->value ) )
								return etDOUBLE ;
						} else
							return etSYNTAX ;
						state = bsEND ;
						break ;

					case stSFR :
						// DS, pBlazIDE support
					case stDS :
					case stDSIN :
					case stDSOUT :
					case stDSIO :
					case stDSRAM :
					case stDSROM :
						if ( state != bsSYMBOL )
							return etSYNTAX ;
						tok_next() ;
						if ( symtok != NULL ) {
							if ( ( e = expression( &result ) ) == etNONE ) {
								if ( !add_symbol( tVALUE, stINT, symtok->text, result ) )
									return etDOUBLE ;
							} else
								return e ;
						} else
							return etMISSING ;
						state = bsEND ;
						break ;

						// _BYT
					case stBYTE :
						if ( state != bsINIT && state != bsSYMBOL )
							return etSYNTAX ;
						tok_next() ;
						if ( symtok && !add_symbol( tVALUE, stINT, symtok->text, ( gSCR ) & 0xFF ) )
							return etDOUBLE ;
						do {
							if ( ( e = expression( &result ) ) != etNONE ) {
								if ( e == etEMPTY )
									break ;	// allow an empty expression list for generating a symbol only
								else
									return e ;
							}
							gSCR += 1 ;
						} while ( comma() ) ;
						state = bsEND ;
						break ;

						// _BUFFER
					case stBUFFER :
						if ( state != bsINIT && state != bsSYMBOL )
							return etSYNTAX ;
						tok_next() ;
						if ( symtok && !add_symbol( tVALUE, stINT, symtok->text, ( gSCR ) & 0xFF ) )
							return etDOUBLE ;
						if ( ( e = expression( &result ) ) == etNONE ) {
							if ( result < 256 )
								gSCR += result ;
							else
								return etRANGE ;
						} else
							return e ;
						state = bsEND ;
						break ;

						// _TXT
					case stTEXT :
						if ( state != bsINIT && state != bsSYMBOL )
							return etSYNTAX ;
						tok_next() ;
						if ( symtok && !add_symbol( tVALUE, stINT, symtok->text, ( gSCR ) & 0xFF ) )
							return etDOUBLE ;
						if ( tok_current()->type == tSTRING ) {
							char * dup = convert_string( tok_current()->text ) ;
							gSCR += strlen( dup ) + 1 ;
							free( dup ) ;
						} else
							return etEXPR ;
						state = bsEND ;
						break ;

					case stVHDL :
					case stHEX :
					case stMEM :
					case stCOE :
						if ( state != bsINIT )
							return etSYNTAX ;
						state = bsEND ;
						break ;

					default :
						return etSYNTAX ;
					}
					break ;
				default :
					return etDOUBLE ;
				}
			} else if ( state == bsINIT ) {
				// not known yet, label or symbol definition?
				symtok = tok_current() ;
				state = bsSYMBOL ;
			}
			break ;

		case tCOLON :
			if ( state != bsSYMBOL )
				return etSYNTAX ;
			if ( !add_symbol( tLABEL, stNONE, symtok->text, gPC ) )
				return etDOUBLE ;
			state = bsLABELED ;
			break ;

		default :
			return etSYNTAX ;
		}
	}
	return etNONE ;
}

static error_t assemble( uint32_t * addr, uint32_t * code ) {
	build_state state = bsINIT ;
	symbol_t * h = NULL ;
	uint32_t result = 0 ;
	uint32_t operand1 = 0 ;
	uint32_t operand2 = 0 ;
	uint32_t opcode = 0 ;
	uint32_t oPC = 0 ;
	error_t e = etNONE ;

	*addr = 0xFFFFFFFF ;
	*code = 0xFFFFFFFF ;

	// process statement
	for ( tok_first(), state = bsINIT ; state != bsEND ; ) {
		switch ( tok_current()->type ) {
		case tNONE :
			// empty line?
			if ( state != bsINIT && state != bsLABELED )
				return etSYNTAX ;
			state = bsEND ;
			break ;

		case tIDENT :
			h = find_symbol( tok_current()->text, false ) ;
			if ( h != NULL ) {
				switch ( h->type ) {
				// opcodes
				case tOPCODE :
					if ( state != bsINIT && state != bsLABELED )
						return etSYNTAX ;
					tok_next() ;
					opcode = 0xFFFFFFFF ;
					operand1 = 0 ;
					operand2 = 0 ;
					oPC = gPC ;
					gPC += 1 ;

					switch ( h->subtype ) {
					case stMOVE :
						if ( !destreg( &operand1 ) )
							return etREGISTER ;
						if ( !comma() )
							return etCOMMA ;
						if ( !srcreg( &operand2 ) ) {
							if ( ( e = expression( &operand2 ) ) != etNONE )
								return e ;
							opcode = h->value | operand1 | ( operand2 & 0xFF ) ;
						} else
							opcode = h->value | operand1 | ( operand2 & 0xFF ) | 0x01000 ;
						break ;

					case stCJMP :
						if ( condition( &operand1 ) ) {
							if ( !comma() )
								return etCOMMA ;
						}
						if ( ( e = expression( &operand2 ) ) != etNONE )
							return e ;
						opcode = h->value | operand1 | ( operand2 & 0x3FF ) ;
						break ;

					case stCRET :
						condition( &operand1 ) ;
						opcode = h->value | operand1 ;
						break ;

					case stINT :
						opcode = h->value ;
						break ;

					case stINTI :
						if ( !bMode )
							return etNOTIMPL ;
						opcode = h->value ;
						if ( tok_current()->type != tIDENT || strcasecmp( tok_current()->text, "INTERRUPT" ) != 0 )
							return etMISSING ;
						tok_next() ;
						break ;

					case stINTE :
						opcode = h->value ;
						if ( enadis( &operand1 ) )
							opcode = ( h->value & 0xFFFFE ) | operand1 ;
						break ;

					case stIO :
						if ( !destreg( &operand1 ) )
							return etREGISTER ;
						if ( !comma() )
							return etCOMMA ;
						if ( !srcreg( &operand2 ) ) {
							if ( ( e = expression( &operand2 ) ) != etNONE )
								return e ;
							opcode = h->value | operand1 | ( operand2 & 0xFF ) ;
						} else
							opcode = h->value | operand1 | ( operand2 & 0xFF ) | 0x01000 ;
						break ;

					case stSHIFT :
						if ( !destreg( &operand1 ) )
							return etREGISTER ;
						opcode = h->value | operand1 ;
						break ;

					case stINST :
						if ( ( e = expression( &opcode ) ) != etNONE )
							return e ;
						break ;

					default :
						return etNOTIMPL ;
					}
					if ( opcode == 0xFFFFFFFF )
						return etINTERNAL ;
					if ( oPC < 1024 ) {
						gCode[ oPC ] = opcode ;
						*addr = oPC ;
						*code = opcode ;
					} else
						return etRANGE ;
					state = bsEND ;
					break ;

					// directives
				case tDIRECTIVE :
					tok_next() ;
					switch ( h->subtype ) {

					case stADDRESS :
						if ( !bMode )
							return etNOTIMPL ;
					case stORG :
						if ( state != bsINIT )
							return etSYNTAX ;
						if ( ( e = expression( &result ) ) == etNONE ) {
							if ( result >= 1024 )
								return etRANGE ;
							gPC = result ;
							*addr = gPC ;
						} else
							return e ;
						break ;

					case stSCRATCHPAD :
						if ( state != bsINIT )
							return etSYNTAX ;
						if ( ( e = expression( &result ) ) == etNONE ) {
							// within code range
							if ( result >= 1024 + 256 / 2 )
								return etRANGE ;
							gSCR = result * 2 ;
							*addr = gSCR ;
						} else
							return e ;
						break ;

					case stEQU :
						if ( state != bsSYMBOL )
							return etSYNTAX ;
						// NO-OP, just eat tokens in an orderly way
						if ( destreg( &result ) ) {
						} else if ( ( e = expression( &result ) ) == etNONE ) {
						} else
							return e ;
						break ;

					case stSFR :
					case stDS :
					case stDSIN :
					case stDSOUT :
					case stDSIO :
					case stDSRAM :
					case stDSROM :
						if ( state != bsSYMBOL )
							return etSYNTAX ;
						// NO-OP, just eat tokens in an orderly way
						do {
							if ( ( e = expression( &result ) ) == etNONE ) {
							} else
								return e ;
						} while ( comma() ) ;
						break ;

					case stBYTE :
						if ( state != bsINIT && state != bsSYMBOL )
							return etSYNTAX ;
						*addr = gSCR ;
						do {
							if ( ( e = expression( &result ) ) == etNONE ) {
								if ( ( gSCR & 1 ) == 0 )
									gCode[ gSCR / 2 ] = ( gCode[ gSCR / 2 ] & 0x0FF00 ) | ( result & 0xFF ) ;
								else
									gCode[ gSCR / 2 ] = ( gCode[ gSCR / 2 ] & 0x000FF ) | ( ( result & 0xFF ) << 8 ) ;
							} else if ( e == etEMPTY ) {
								// allow an empty expression list for generating a symbol only
								*code = 0xFFFFFFFF ;
								break ;
							} else
								return e ;
							// only show the first 2 bytes as a 'uint18'
							if ( ( gSCR & 0xFFFE ) == ( *addr & 0xFFFE ) )
								*code = gCode[ gSCR / 2 ] ;
							gSCR += 1 ;
						} while ( comma() ) ;
						break ;

					case stBUFFER :
						if ( state != bsINIT && state != bsSYMBOL )
							return etSYNTAX ;
						if ( ( e = expression( &result ) ) == etNONE ) {
							if ( result < 256 ) {
								*addr = gSCR ;
								gSCR += result ;
								*code = 0xFFFF0000 | result ;
							} else
								return etRANGE ;
						} else
							return e ;
						break ;

						// _TXT
					case stTEXT :
						if ( state != bsINIT && state != bsSYMBOL )
							return etSYNTAX ;
						if ( tok_current()->type == tSTRING ) {
							char * dup = convert_string( tok_current()->text ) ;
							int i = 0 ;

							*addr = gSCR ;
							for ( i = 0 ; i < strlen( dup ) + 1 ; i += 1 ) {
								if ( ( gSCR & 1 ) == 0 )
									gCode[ gSCR / 2 ] = ( gCode[ gSCR / 2 ] & 0x0FF00 ) | ( dup[ i ] & 0xFF ) ;
								else
									gCode[ gSCR / 2 ] = ( gCode[ gSCR / 2 ] & 0x000FF ) | ( ( dup[ i ] & 0xFF ) << 8 ) ;
								// only show the first 2 bytes as a 'uint18'
								if ( ( gSCR & 0xFFFE ) == ( *addr & 0xFFFE ) )
									*code = gCode[ gSCR / 2 ] ;
								gSCR += 1 ;
							}
							free( dup ) ;
						} else
							return etEXPR ;
						tok_next() ;
						break ;

					case stVHDL :
					case stHEX :
					case stMEM :
					case stCOE :
						if ( state != bsINIT )
							return etSYNTAX ;
						//						if ( bKCPSM_mode )
						return etNOTIMPL ;
						break ;

					case stCONSTANT :
						if ( state != bsINIT )
							return etSYNTAX ;
						tok_next() ;
						if ( !comma() )
							return etCOMMA ;
						// normal expression?
						if ( ( e = expression( &result ) ) == etNONE ) {
							*code = result ;
						} else
							return e ;
						break ;

					case stNAMEREG :
						if ( state != bsINIT )
							return etSYNTAX ;
						if ( !bMode )
							return etNOTIMPL ;
						tok_next() ;
						tok_next() ;
						tok_next() ;
						break ;

					default :
						return etSYNTAX ;
					}
					state = bsEND ;
					break ;

					// labels
				case tLABEL :
					if ( state != bsINIT )
						return etSYNTAX ;
					if ( h->value != gPC )
						return etPHASING ;
					tok_next()->type = tLABEL ; // just for formatting
					*addr = h->value ;
					state = bsLABEL ;
					break ;

					// equated values
				case tVALUE :
				case tREGISTER :
					if ( state != bsINIT )
						return etSYNTAX ;
					tok_next()->subtype = stEQU ; // just for formatting
					*code = h->value & 0xFFFF ;
					state = bsSYMBOL ;
					break ;

				default :
					return etSYNTAX ;
				}
			} else
				return etUNDEF ;
			break ;

		case tCOLON :
			// if we have a potential label, we need a ':'
			if ( state != bsLABEL )
				return etSYNTAX ;
			tok_next() ;
			state = bsLABELED ;
			break ;

		default :
			return etSYNTAX ;
		}
	}
	// only comment may follow
	if ( tok_current()->type != tNONE )
		return etEND ;
	return etNONE ;
}

// error printer
static bool error( const error_t e ) {
	if ( e != etNONE ) {
		fprintf( stdout, "%s:%d: %s\n", gSource, gLinenr, s_errors[ e ] ) ;
		return false ;
	} else
		return true ;
}

// dump code in mem file format
static void dump_code( FILE * f ) {
	int h = 0 ;
	bool b_addr = true ;

	for ( h = 0 ; h < 1024 + 256 / 2 ; h += 1 ) {
		if ( gCode[ h ] == 0xFFFC0000 )
			b_addr = true ;
		else {
			if ( b_addr ) {
				fprintf( f, "@%08X\n", h ) ;
				b_addr = false ;
			}
			fprintf( f, "%05X\n", gCode[ h ] ) ;
		}
	}
}

// format list file
static void print_line( FILE * f, error_t e, uint32_t addr, uint32_t code ) {
	int n = 0 ;
	char * s = NULL ;

	tok_first() ;

	if ( e != etNONE )
		fprintf( f, "?? %s:\n", s_errors[ e ] ) ;

	if ( bCode ) {
		// address info
		if ( addr != 0xFFFFFFFF ) {
			n += fprintf( f, "%03X ", addr ) ;
		} else
			n += fprintf( f, "%4s", "" ) ;

		// code info
		if ( code != 0xFFFFFFFF ) {
			if ( addr != 0xFFFFFFFF ) {
				if ( code <= 0xFFFFF )
					n += fprintf( f, "%05X ", code ) ;
				else
					n += fprintf( f, "  %02X  ", code & 0xFF ) ;
			} else if ( code > 0xFF )
				n += fprintf( f, "%04X  ", code ) ;
			else
				n += fprintf( f, "  %02X  ", code & 0xFF ) ;
		} else
			n += fprintf( f, "%6s", "" ) ;
	}

	if ( tok_current()->type == tLABEL ) {
		// labels in the margin
		n += fprintf( f, "%*s", -16, tok_next()->text ) ;
		n += fprintf( f, "%s", tok_next()->text ) ;
	} else if ( tok_current()->subtype == stEQU ) {
		// print EQUates in the label margin
		n += fprintf( f, "%*s", -( 16 + 1 ), tok_next()->text ) ;
	} else if ( tok_current()->type != tNONE )
		// else print a blank margin
		n += fprintf( f, "%*s", 16 + 1, "" ) ;

	// opcode
	if ( tok_current()->type != tNONE && tok_current()->text != NULL ) {
		for ( s = tok_current()->text ; s != NULL && isalpha( *s ) ; s++ )
			*s = toupper( *s ) ;
		n += fprintf( f, " %*s", -6, tok_next()->text ) ;
	}

	// operand
	for ( ; tok_current()->type != tNONE ; tok_next() ) {
		if ( tok_current()->text != NULL ) {
			if ( tok_current()->type != tCOMMA )
				n += fprintf( f, " " ) ;
			switch ( tok_current()->type ) {
			case tHEX :
				n += fprintf( f, "0x%s", tok_current()->text ) ;
				break ;
			case tBIN :
				n += fprintf( f, "0b%s", tok_current()->text ) ;
				break ;
			case tCHAR :
				n += fprintf( f, "'%s'", tok_current()->text ) ;
				break ;
			case tSTRING :
				n += fprintf( f, "\"%s\"", tok_current()->text ) ;
				break ;
			default :
				n += fprintf( f, "%s", tok_current()->text ) ;
				break ;
			}
		}
	}

	// comment
	if ( tok_current()->type == tNONE && tok_current()->subtype == stCOMMENT ) {
		if ( tok_current()->text != NULL ) {
			if ( n <= 10 )
				// at the start
				fprintf( f, "%s", tok_current()->text ) ;
			else if ( n < 60 ) {
				// at column 60
				fprintf( f, "%*s", 60 - n, "" ) ;
				fprintf( f, "%s", tok_current()->text ) ;
			} else
				// after the rest
				fprintf( f, " %s", tok_current()->text ) ;
		}
	}
	fprintf( f, "\n" ) ;
}

// main entry for the 2-pass assembler
bool assembler( char * sourcefilename, char * codefilename, char * listfilename, bool mode, bool listcode ) {
	FILE * fsrc = NULL ;
	FILE * fmem = NULL ;
	FILE * flist = NULL ;
	char line[ 256 ] ;
	error_t e = etNONE ;
	int h = 0 ;
	bool result = true ;

	uint32_t addr, code ;

	// open source file
	fsrc = fopen( sourcefilename, "r" ) ;
	if ( fsrc == NULL ) {
		fprintf( stderr, "? unable to open source file '%s'", sourcefilename ) ;
		return false ;
	}
	gSource = sourcefilename ;

	// set up symbol table with keywords
	init_symbol() ;

	// pass 1, add symbols from source
	for ( gLinenr = 1, gPC = 0, gSCR = 2048, bMode = mode ; fgets( line, sizeof( line ), fsrc ) != NULL ; gLinenr += 1 ) {
		if ( lex( line, mode ) ) {
			result &= error( build() ) ;
			tok_free() ;
		} else
			result &= error( etLEX ) ;
	}

	// rewind
	fseek( fsrc, 0, SEEK_SET ) ;

	// clear code
	for ( h = 0 ; h < 1024 + 256 / 2 ; h += 1 )
		gCode[ h ] = 0xFFFC0000 ;

	// pass 2, build code and scratchpad
	if ( strlen( listfilename ) > 0 ) {
		flist = fopen( listfilename, "w" ) ;
		if ( flist == NULL ) {
			fprintf( stderr, "? unable to create LST file '%s'", listfilename ) ;
			result = false ;
		}
	}
	bMode = mode ;
	bCode = listcode ;
	for ( gLinenr = 1, gPC = 0, gSCR = 2048 ; fgets( line, sizeof( line ), fsrc ) != NULL ; gLinenr += 1 ) {
		if ( lex( line, mode ) ) {
			result &= error( e = assemble( &addr, &code ) ) ;
			if ( flist != NULL )
				print_line( flist, e, addr, code ) ;
		} else {
			result &= error( etLEX ) ;
			if ( flist != NULL )
				print_line( flist, etLEX, addr, code ) ;
		}
		tok_free() ;
	}
	if ( flist != NULL )
		fclose( flist ) ;

	// dump code and scratch pad
	if ( strlen( codefilename ) > 0 ) {
		fmem = fopen( codefilename, "w" ) ;
		if ( fmem == NULL ) {
			fprintf( stderr, "? unable to create MEM file '%s'", codefilename ) ;
			result = false ;
		} else {
			dump_code( fmem ) ;
			fclose( fmem ) ;
		}
	}

	free_symbol() ;
	fclose( fsrc ) ;

	return result ;
}
