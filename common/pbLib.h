/*
 *  Copyright © 2014 : Maarten Brock <sourceforge.brock AT dse.nl>
 *  $Id:$
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

/**
 * pBlazASM library loader
 * @file pbLib.h
 */

#include "lkar.h"

typedef struct {
    const char * filename ;
    const char * object ;
    int offset ;
    int length ;
} source_t ;

int is_ar (FILE * libfp);
char * get_member_name (char *name, size_t *p_size, int allocate, FILE * libfp);
size_t ar_get_header (struct ar_hdr *hdr, FILE * libfp, char **p_obj_name);
int load_symbol_table (FILE * fp, const char * archive);
int add_lib_obj (const char * name, source_t * libobj, int verbose);
