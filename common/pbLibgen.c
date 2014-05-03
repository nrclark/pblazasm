/*
 *  Copyright � 2003..2013 : Henk van Kampen <henk@mediatronix.com>
 *
 *	This file is part of pBlazASM.
 *
 *  pBlazMRG is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pBlazMRG is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pBlazASM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>

#include "dbuf_string.h"
//!
// operating system dependent filename processing functions
//
const char * basename( const char * path ) {
    const char * slash ;

    slash = strrchr( path, '/' ) ;
    if ( slash ) {
        return slash + 1 ;
    } else {
        slash = strrchr( path, '\\' ) ;
        if ( slash ) {
            return slash + 1 ;
        } else {
            return path ;
        }
    }
}

char * filename( const char * path ) {
    char * name, * dot ;

    name = strdup( basename( path ) ) ;
    dot = strrchr( name, '.' ) ;
    if (dot != NULL)
        *dot = 0 ;
    return name ;
}

char * dirname( const char * path ) {
    char * name ;
    const char * slash ;
    ptrdiff_t length ;

    slash = strrchr( path, '/' ) ;
    if ( slash ) {
        length = slash - path + 1 ;
    } else {
        slash = strrchr( path, '\\' ) ;
        if ( slash ) {
            length = slash - path + 1 ;
        } else {
#ifdef _WIN32
            path = ".\\" ;
#else
            path = "./" ;
#endif
            length = 2 ;
        }
    }
    name = strdup( path ) ;
    name[ length ] = 0 ;
    return name ;
}

/**
 * construct new filename based on base_name and replace extension
*/
char * construct_filename( const char * base_name, const char * ext ) {
	char * pfile ;
	char * ppath ;
	struct dbuf_s dbuf ;
	char * ret ;

	pfile = filename( base_name ) ;
	ppath = dirname( base_name ) ;
	dbuf_init( &dbuf, 128 ) ;
	dbuf_append_str( &dbuf, ppath ) ;
	dbuf_append_str( &dbuf, pfile ) ;
	dbuf_append_str( &dbuf, ext ) ;
	ret = dbuf_detach( &dbuf );
	free( ppath ) ;
	free( pfile ) ;
	return ret ;
}

/**
 * duplicate new filename based on base_name and add extension if necessary
*/
char * duplicate_filename( const char * base_name, const char * defext ) {
	if ( strrchr( base_name, '.' ) == NULL )
		return construct_filename( base_name, defext ) ;
	else
		return strdup( base_name ) ;
}
