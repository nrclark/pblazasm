/*
 *  Copyright © 2010 : Maarten Brock <sourceforge.brock AT dse.nl>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "lkar.h"
#include "pbLib.h"

/**
 * pBlazASM library loader
 * @file pbLib.c
 */

typedef struct
{
  char * name;            // symbol name
  const char * archive;   // archive filename
  char * filename;        // object filename
  long offset;            // object offset in archive
  int size;               // object filesize
  int seq;                // sequence of symbol insertion
} symbol;

static int nSymbols = 0;
static symbol * libSymbols = NULL;
static int seq = 0;

int
is_ar (FILE * libfp)
{
  char buf[SARMAG];
  int ret;

  if (!(ret = fread (buf, 1, sizeof (buf), libfp) == sizeof (buf) && memcmp (buf, ARMAG, SARMAG) == 0))
    rewind (libfp);

  return ret;
}

static char *sym_tab;
static int sym_tab_size;

char *
get_member_name (char *name, size_t *p_size, int allocate, FILE * libfp)
{
  *p_size = 0;

  if (0 == memcmp (name, "#1/", 3))
    {
      char *p;
      size_t len = strtoul (&name [3], &p, 10);
      if (p > &name [3])
        {
          /* BSD appends real file name to the file header */
          if (p_size != NULL)
            *p_size = len;

          if (allocate)
            {
              char *n = (char *) malloc (len);
              if (fread (n, 1, len, libfp) != len)
                {
                  /* not an ar archive or broken ar archive */
                  return NULL;
                }
              else
                return n;
            }
          else
            {
              /* just advance the file pointer */
              fseek (libfp, len, SEEK_CUR);
              return NULL;
            }
        }
      else
        {
          /* not an ar archive or broken ar archive */
          return NULL;
        }
    }
  else if (allocate)
    {
      if (name[0] == '/')
        {
          if (NULL != sym_tab)
            {
              char *p;

              int name_offset = strtol (++name, &p, 0);
              if (p != name && name_offset < sym_tab_size)
                {
                  int len = p - name + 1;
                  while (len < AR_NAME_LEN && name[len++] == ' ')
                    ;
                  if (len == AR_NAME_LEN)
                    {
                      char *n;

                      /* long name: get it from the symbol table */
                      name = &sym_tab[name_offset];
                      for (p = name; *p != '/' && *p != '\n'; ++p)
                        assert (p < &sym_tab[sym_tab_size]);

                      if (p[0] != '/' || p[1] != '\n')
                        while (*++p != '\n')
                          assert (p < &sym_tab[sym_tab_size]);

                      n = (char *) malloc (p - name + 1);
                      memcpy (n, name, p - name);
                      n[p - name] = '\0';
                      return n;
                    }
                }
            }
        }
      else
        {
          char *p = strrchr (name, '/');

          if (NULL != p)
            {
              int len = p - name;
              while (name[++len] == ' ')
                ;
              if (len == AR_NAME_LEN)
                {
                  char *n = (char *) malloc (p - name + 1);
                  memcpy (n, name, p - name);
                  n[p - name] = '\0';
                  return n;
                }
            }
          else
            {
              /* BSD formed member name:
                 trim trailing spaces */
              char *n;

              p = name + AR_NAME_LEN;
              while (*--p == ' ' && p >= name)
                ;
              ++p;
              n = (char *) malloc (p - name + 1);
              memcpy (n, name, p - name);
              n[p - name] = '\0';
              return n;
            }
        }

      /* bad formed member name:
       just return it */

      return strdup (name);
    }
  else
    return NULL;
}

size_t
ar_get_header (struct ar_hdr *hdr, FILE * libfp, char **p_obj_name)
{
  char header[ARHDR_LEN];
  char buf[AR_DATE_LEN + 1];
  char *obj_name;
  size_t size;

  if (fread (header, 1, sizeof (header), libfp) != sizeof (header)
      || memcmp (header + AR_FMAG_OFFSET, ARFMAG, AR_FMAG_LEN) != 0)
    {
      /* not an ar archive */
      return 0;
    }

  memcpy (hdr->ar_name, &header[AR_NAME_OFFSET], AR_NAME_LEN);
  hdr->ar_name[AR_NAME_LEN] = '\0';

  memcpy (buf, &header[AR_DATE_OFFSET], AR_DATE_LEN);
  buf[AR_DATE_LEN] = '\0';
  hdr->ar_date = strtol (buf, NULL, 0);

  memcpy (buf, &header[AR_UID_OFFSET], AR_GID_LEN);
  buf[AR_GID_LEN] = '\0';
  hdr->ar_uid = (uid_t) strtol (buf, NULL, 0);

  memcpy (buf, &header[AR_GID_OFFSET], AR_DATE_LEN);
  buf[AR_DATE_LEN] = '\0';
  hdr->ar_gid = (gid_t) strtol (buf, NULL, 0);

  memcpy (buf, &header[AR_MODE_OFFSET], AR_MODE_LEN);
  buf[AR_MODE_LEN] = '\0';
  hdr->ar_mode = (mode_t) strtoul (buf, NULL, 0);

  memcpy (buf, &header[AR_SIZE_OFFSET], AR_SIZE_LEN);
  buf[AR_SIZE_LEN] = '\0';
  hdr->ar_size = strtol (buf, NULL, 0);

  if (NULL == (obj_name = get_member_name (hdr->ar_name, &size, p_obj_name != NULL, libfp)) && p_obj_name != NULL)
    {
      /* Malformed archive */
      return 0;
    }

  if (p_obj_name != NULL)
    *p_obj_name = obj_name;

  /* treat BSD appended real file name as a part of the header */
  hdr->ar_size -= size;

  return size + ARHDR_LEN;
}

static int
process_symbol_table (char * buf, const char * archive, FILE * fp)
{
  char *po, *ps;
  int i;
  long nsym, offset;
  symbol* newsym;
  struct ar_hdr hdr;

  nsym = sgetl (buf);
  libSymbols = realloc (libSymbols, (nSymbols + nsym) * sizeof (symbol));
  newsym = libSymbols + nSymbols;
  nSymbols += nsym;

  po = buf + 4;
  ps = po + nsym * 4;

  for (i = 0; i < nsym; ++i)
    {
      offset = sgetl (po);
      po += 4;
      newsym->name = strdup (ps);
      ps += strlen(ps) + 1;
      newsym->archive = archive;
      fseek (fp, offset, SEEK_SET);
      if (!ar_get_header (&hdr, fp, &newsym->filename))
        return 0;
      newsym->offset = ftell(fp);
      newsym->size = hdr.ar_size;
      newsym->seq = seq++;
      newsym++;
    }
  return 1;
}

static int
process_bsd_symbol_table (char * buf, const char * archive, FILE * fp)
{
  char *po, *ps;
  int i;
  long tablesize;
  long nsym, offset;
  symbol* newsym;
  struct ar_hdr hdr;

  tablesize = sgetl (buf);
  nsym = tablesize / 8;
  libSymbols = realloc (libSymbols, (nSymbols + nsym) * sizeof (symbol));
  newsym = libSymbols + nSymbols;
  nSymbols += nsym;

  po = buf + 4;
  ps = po + tablesize + 4;

  for (i = 0; i < nsym; ++i)
    {
     long sym;

      sym = sgetl (po);
      po += 4;
      offset = sgetl (po);
      po += 4;
      newsym->name = strdup (ps + sym);
      newsym->archive = archive;
      fseek (fp, offset, SEEK_SET);
      if (!ar_get_header (&hdr, fp, &newsym->filename))
        return 0;
      newsym->offset = ftell(fp);
      newsym->size = hdr.ar_size;
      newsym->seq = seq++;
      newsym++;
    }
  return 1;
}

/**
 * compare_sort
 * compare two symbols for sorting
 */
static int compare_sort( const void *arg1, const void *arg2 )
{
  symbol* sym1 = (symbol*)arg1;
  symbol* sym2 = (symbol*)arg2;
  int ret = strcmp(sym1->name, sym2->name);
  return (ret != 0) ? ret : sym1->seq - sym2->seq;
}

int load_symbol_table (FILE * fp, const char * archive)
{
  struct ar_hdr hdr;
  char *name;
  char *buf;
  int ret = 0;

  if (!is_ar (fp) || !ar_get_header (&hdr, fp, &name))
    {
      free (name);
      return 0;
    }

  buf = (char *) malloc (hdr.ar_size);

  if (fread (buf, 1, hdr.ar_size, fp) != hdr.ar_size)
    {
      free (buf);
      free (name);
      return 0;
    }

  if (AR_IS_SYMBOL_TABLE (name))
    {
      ret = process_symbol_table (buf, archive, fp);
    }
  else if (AR_IS_BSD_SYMBOL_TABLE (name))
    {
      ret = process_bsd_symbol_table (buf, archive, fp);
    }
  free (buf);
  free (name);
  qsort (libSymbols, nSymbols, sizeof (symbol), compare_sort);
  return ret;
}

/**
 * compare_search
 * compare two symbols for searching
 */
static int compare_search( const void *key, const void *item )
{
  char* name = (char*)key;
  symbol* sym = (symbol*)item;
  return strcmp(name, sym->name);
}

int add_lib_obj (const char * name, source_t * libobj, int verbose)
{
  symbol * sym;
  char * object;

  sym = (symbol*)bsearch (name, libSymbols, nSymbols, sizeof (symbol), compare_search);

  if (!sym)
    return 0;

  libobj->filename = sym->archive;
  libobj->object = sym->filename;
  object = malloc (strlen(sym->archive) + strlen(sym->filename) + 2);
  if (object)
    {
      strcpy(object, sym->archive);
      strcat(object, "/");
      strcat(object, sym->filename);
      libobj->object = object;
    }
  libobj->offset = sym->offset;
  libobj->length = sym->size;

  if (verbose)
    printf ("! imported '%s' from '%s' for '%s'\n", sym->filename, sym->archive, sym->name);
  return 1;
}
