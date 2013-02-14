
/*
 *  Copyright © 2003..2013 : Henk van Kampen <henk@mediatronix.com>
 *
 *  This file is part of pBlazBIT.
 *
 *  pBlazBIT is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pBlazBIT is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pBlazBIT.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "pbCRC32tab.h"

#ifdef CRC32
#  undef CRC32
#endif
#define CRC32(c, b) (crctable[((int)(c) ^ (b)) & 0xff] ^ ((c) >> 8))
#define DO1(buf)  crc = CRC32(crc, *buf++)
#define DO2(buf)  DO1(buf); DO1(buf)
#define DO4(buf)  DO2(buf); DO2(buf)
#define DO8(buf)  DO4(buf); DO4(buf)

/* ========================================================================= */
uint32_t crc32( uint32_t crc, uint8_t * buf, int len ) {
	/* Run a set of bytes through the crc shift register.  If buf is a NULL
	   pointer, then initialize the crc shift register contents instead.
	   Return the current crc in either case. */

	if( buf == NULL )
		return 0 ;

	crc = crc ^ 0xffffffffL ;

	while( len >= 8 ) {
		DO8( buf ) ;
		len -= 8 ;
	}

	if( len )
		do {
			DO1( buf ) ;
		} while( --len ) ;
	return crc ^ 0xffffffffL ;     /* (instead of ~c for 64-bit machines) */
}
