/*
 *  Copyright © 2003..2012 : Henk van Kampen <henk@mediatronix.com>
 *
 *  This file is part of pBlazPCR.
 *
 *  pBlazPCR is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pBlazPCR is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pBlazPCR.  If not, see <http://www.gnu.org/licenses/>.
 */

#define MAXMEM 4096
#define MAXSCR 256

typedef struct _instr {
    uint32_t addr ;
    uint32_t code ;
    bool blank ;
    bool breadcrum ;
    bool label ;
} INST_t ;

bool writeVHD6 ( const char * strPSMfile, INST_t * Code, uint32_t * Data ) ;
