/* pBlazRanLib.c - ranlib for picoblaze psm archives

   Copyright (C) 2008-2014 Borut Razem, borut dot razem at siol dot net
                           Maarten Brock, sourceforge dot brock at dse dot nl

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "dbuf_string.h"
#include "lkar.h"
#include "pbTypes.h"
#include "pbDirectives.h"
#include "pbLib.h"

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#define NELEM(x)  (sizeof (x) / sizeof (*x))


static int verbose = 0;
static int list = 0;
static int print_index = 0;

static char *
get_member_name_by_offset (FILE * fp, long offset)
{
  struct ar_hdr hdr;
  char *name;

  fseek (fp, offset, SEEK_SET);
  return (ar_get_header (&hdr, fp, &name) != 0) ? name : NULL;
}

struct symbol_s
  {
    const char *name;
    size_t offset;
    struct symbol_s *next;
  };

struct symbol_s *symlist, *lastsym;
unsigned int offset, first_member_offset;

int
add_symbol (const char *sym, void *param)
{
  struct symbol_s *s;

  if ((s = (struct symbol_s *) malloc (sizeof (struct symbol_s))) == NULL)
    return 0;

  if (verbose)
    printf ("%s\n", sym);

  s->name = strdup (sym);
  s->offset = offset - first_member_offset;
  s->next = NULL;

  if (NULL == symlist)
    {
      lastsym = symlist = s;
    }
  else
    {
      lastsym->next = s;
      lastsym = s;
    }

  return 0;
}

int
is_psm (FILE * libfp)
{
  /*
   * No magic cookie for .psm file
   */
  return 1;
}

int
enum_symbols (FILE * fp, long size, int (*func) (const char *sym, void *param), void *param)
{
  long end;
  struct dbuf_s buf;
  struct dbuf_s symname;

  assert (func != NULL);

  dbuf_init (&buf, 512);
  dbuf_init (&symname, 32);

  end = (size >= 0) ? ftell (fp) + size : -1;

  /*
   * Read in the object file.  Look for lines that
   * begin with "S" and end with "D".  These are
   * symbol table definitions.  If we find one, see
   * if it is our symbol.  Make sure we only read in
   * our object file and don't go into the next one.
   */

  while (end < 0 || ftell (fp) < end)
    {
      const char *p;

      dbuf_set_length (&buf, 0);
      if (dbuf_getline (&buf, fp) == 0)
        break;

      p = dbuf_c_str (&buf);

      /* Skip whitespace */
      while (isspace (*p))
        p++;

      /* Skip local symbols or directives */
      if ('.' == p[0])
        continue;

      /*
       * Skip everything that's not a symbol.
       */
      if (isalpha (*p) || '_' == *p)
        {
          bool found = false;

          dbuf_set_length (&symname, 0);

          while (isalnum (*p) || '_' == *p)
            dbuf_append_char (&symname, *p++);

          if (':' == *p) /* a label */
            {
              if (':' == *++p) /* a global label */
                found = true;
            }
          else /* no label */
            {
              size_t i;

              /* Skip whitespace */
              while (isspace (*p))
                p++;

              for (found = false, i = 0; !found && i < sizeof(directives) / sizeof(symbol_t); i++)
                {
                  if (0 == strncmp(p, directives[i].text, strlen(directives[i].text)))
                    {
                      switch (directives[i].subtype)
                        {
                        case stEQU:
                        case stBYTE:
                        case stWORD_BE:
                        case stWORD_LE:
                        case stLONG_BE:
                        case stLONG_LE:
                        case stBUFFER:
                        case stTEXT:
                        case stDS:
                        case stDSIN:
                        case stDSOUT:
                        case stDSIO:
                        case stDSROM:
                        case stDSRAM:
                          /* It's a symbol */
                          found = true;
                          break;
                        default:
						  break;
                        }
                    }
                }
            }

          /* If it's an actual symbol, record it */
          if (found)
            if (func != NULL)
              if ((*func) (dbuf_c_str (&symname), NULL))
                return 1;
        }
    }

  dbuf_destroy (&buf);
  dbuf_destroy (&symname);

  return 0;
}

static int
process_symbol_table (struct ar_hdr *hdr, FILE *fp)
{
  long pos = ftell (fp);

  if (print_index)
    {
      char *buf, *po, *ps;
      int i;
      long nsym;

      printf ("Archive index:\n");

      buf = (char *) malloc (hdr->ar_size);

      if (fread (buf, 1, hdr->ar_size, fp) != hdr->ar_size)
        {
          free (buf);
          return 0;
        }

      nsym = sgetl (buf);

      po = buf + 4;
      ps = po + nsym * 4;

      for (i = 0; i < nsym; ++i)
        {
          char *obj;

          offset = sgetl (po);
          po += 4;

          if (NULL == (obj = get_member_name_by_offset (fp, offset))) /* member name */
            return 0;

          printf ("%s in %s", ps, obj);
          if (verbose)
            printf (" at 0x%04x\n", offset);
          else
            putchar ('\n');
          free (obj);

          ps += strlen(ps) + 1;

        }
      free (buf);

      fseek (fp, pos, SEEK_SET);

      putchar ('\n');
    }

  /* skip the symbol table */
  fseek (fp, pos + hdr->ar_size + (hdr->ar_size & 1), SEEK_SET);

  return 1;
}

static int
process_bsd_symbol_table (struct ar_hdr *hdr, FILE *fp)
{
  long pos = ftell (fp);

  if (print_index)
    {
      char *buf, *po, *ps;
      int i;
      long tablesize;
      long nsym;

      printf ("Archive index:\n");

      buf = (char *) malloc (hdr->ar_size);

      if (fread (buf, 1, hdr->ar_size, fp) != hdr->ar_size)
        {
          free (buf);
          return 0;
        }

      tablesize = sgetl (buf);
      nsym = tablesize / 8;

      po = buf + 4;

      ps = po + tablesize + 4;

      for (i = 0; i < nsym; ++i)
        {
          char *obj;
          long sym;

          sym = sgetl (po);
          po += 4;
          offset = sgetl (po);
          po += 4;

          printf ("%s in ", ps + sym);

          if (NULL == (obj = get_member_name_by_offset (fp, offset))) /* member name */
            return 0;

          printf ("%s\n", obj);
          free (obj);
        }
      free (buf);
      putchar ('\n');
    }

  /* skip the symbol table */
  fseek (fp, pos + hdr->ar_size + (hdr->ar_size & 1), SEEK_SET);

  return 1;
}

int
get_symbols (FILE * fp)
{
  struct ar_hdr hdr;
  size_t hdr_len;
  char *name;

  if (!is_ar (fp) || !(hdr_len = ar_get_header (&hdr, fp, &name)))
    {
      free (name);

      return 0;
    }

  if (AR_IS_SYMBOL_TABLE (name))
    {
      free (name);

      if (!process_symbol_table (&hdr, fp))
        return 0;

      if (feof (fp))
        return 1;
      else if (!(hdr_len = ar_get_header (&hdr, fp, (verbose || list) ? &name : NULL)))
        return 0;
    }
  else if (AR_IS_BSD_SYMBOL_TABLE (name))
    {
      free (name);

      if (!process_bsd_symbol_table (&hdr, fp))
        return 0;

      if (feof (fp))
        return 1;
      else if (!(hdr_len = ar_get_header (&hdr, fp, (verbose || list) ? &name : NULL)))
        return 0;
    }
  else if (!verbose && !list)
    free (name);

  first_member_offset = ftell (fp) - hdr_len;

  /* walk trough all archive members */
  do
    {
      if (is_psm (fp))
        {
          if (verbose || list)
            {
              printf ("%s%s\n", name, verbose ? ":" : "");
              free (name);
            }

          if (!list)
            {
              long mdule_offset = ftell (fp);

              offset = mdule_offset - hdr_len;

              enum_symbols (fp, hdr.ar_size, add_symbol, NULL);

              fseek (fp, mdule_offset + hdr.ar_size + (hdr.ar_size & 1), SEEK_SET);
            }

          if (verbose)
            putchar ('\n');
        }
      else
        {
          if (verbose || list)
            {
              fprintf (stderr, "pBlazRanLib: %s: File format not recognized\n", name);
              free (name);
            }

          /* skip if the member is not a .psm format */
          fseek (fp, hdr.ar_size + (hdr.ar_size & 1), SEEK_CUR);
        }
    }
  while ((hdr_len = ar_get_header (&hdr, fp, (verbose || list) ? &name : NULL)));

  return feof (fp) ? 1 : 0;
}

void
do_ranlib (const char *archive)
{
  FILE *infp;

  if (NULL == (infp = fopen (archive, "rb")))
    {
      fprintf (stderr, "pBlazRanLib: %s: ", archive);
      perror (NULL);
      exit (1);
    }

  if (!get_symbols (infp))
    {
      fprintf (stderr, "pBlazRanLib: %s: Malformed archive\n", archive);
      fclose (infp);
      exit (1);
    }
  else if (!list && !print_index)
    {
      FILE *outfp;
      struct symbol_s *symp;
      char buf[4];
      int str_length;
      int pad;
      int nsym;
      int symtab_size;
      char tmpfile[] = "arXXXXXX";
      struct stat stat_buf;
      int can_stat;

#ifdef _WIN32
      if (NULL == _mktemp (tmpfile) || NULL == (outfp = fopen (tmpfile, "wb")))
        {
          fclose (infp);
          fprintf (stderr, "pBlazRanLib: %s: ", tmpfile);
          perror (NULL);
          exit (1);
        }
#else
      if ((pad = mkstemp (tmpfile)) < 0)
        {
          fclose (infp);
          fprintf (stderr, "pBlazRanLib: %s: ", tmpfile);
          perror (NULL);
          exit (1);
        }

      if (NULL == (outfp = fdopen (pad, "wb")))
        {
          close (pad);
          fclose (infp);
          perror ("pBlazRanLib");
          exit (1);
        }
#endif

      /* calculate the size of symbol table */
      for (str_length = 0, nsym = 0, symp = symlist; symp; ++nsym, symp = symp->next)
        {
          str_length += strlen (symp->name) + 1;
        }

      symtab_size = 4 + 4 * nsym + str_length;

      fprintf (outfp, ARMAG AR_SYMBOL_TABLE_NAME "%-12d%-6d%-6d%-8d%-10d" ARFMAG, (int) time (NULL), 0, 0, 0, symtab_size);

      if (symtab_size & 1)
        {
          pad = 1;
          ++symtab_size;
        }
      else
        pad = 0;

      symtab_size += SARMAG + ARHDR_LEN;

      sputl (nsym, buf);
      fwrite (buf, 1, sizeof (buf), outfp);

      for (symp = symlist; symp; symp = symp->next)
        {
          sputl (symp->offset + symtab_size, buf);
          fwrite (buf, 1, sizeof (buf), outfp);
        }

      for (symp = symlist; symp; symp = symp->next)
        {
          fputs (symp->name, outfp);
          putc ('\0', outfp);
        }

      if (pad)
        putc ('\n', outfp);

      fseek (infp, first_member_offset, SEEK_SET);

      while (EOF != (pad = getc (infp)))
        putc (pad, outfp);

      fclose (outfp);

      if (0 != fstat(fileno(infp), &stat_buf))
        {
          fprintf (stderr, "pBlazRanLib: can't stat %s: ", archive);
          perror (NULL);
          fclose (infp);
          can_stat = 0;
        }
      else
        can_stat = 1;

      fclose (infp);

      if (0 != remove (archive))
        {
          fprintf (stderr, "pBlazRanLib: can't remove %s: ", archive);
          perror (NULL);
        }
      else if (0 != rename (tmpfile, archive))
        {
          fprintf (stderr, "pBlazRanLib: can't rename %s to %s: ", tmpfile, archive);
          perror (NULL);
        }
      else if (!can_stat || 0 != chmod (archive, stat_buf.st_mode))
        {
          fprintf (stderr, "pBlazRanLib: can't chmod %s: ", archive);
          perror (NULL);
        }
    }
  else
    fclose (infp);
}

void
do_verbose (void)
{
  verbose = 1;
}

void
print_version (void)
{
  printf ("pBlazRanLib 1.0 $Revision: 5667 $\n");
  exit (0);
}

void
do_list (void)
{
  list = 1;
}

void
print_armap (void)
{
  print_index = 1;
}

void usage (void);

struct opt_s
  {
    char short_opt;
    const char *long_opt;
    void (*optfnc) (void);
    const char *comment;
  }
opts[] =
  {
    { 'v', "verbose", &do_verbose, "Be more verbose about the operation" },
    { 'V', "version", &print_version, "Print version information" },
    { 'h', "help", &usage, "Print this help message" },
    { 't', "list", &do_list, "List the contents of an archive" },
    { 's', "print-armap", &print_armap, "Print the archive index" },
  };

void
usage (void)
{
  int i;

  printf ("Usage: pBlazRanLib [options] archive\n"
    " Generate an index to speed access to archives\n"
    " The options are:\n");

  for (i = 0; i < NELEM (opts); ++i)
    {
      int len = 5;
      if ('\0' != opts[i].short_opt)
        printf ("  -%c ", opts[i].short_opt);
      else
        printf ("     ");

      if (NULL != opts[i].long_opt)
        {
          printf ("--%s ", opts[i].long_opt);
          len += strlen (opts[i].long_opt);
        }

      while (len++ < 30)
        putchar (' ');
      printf ("%s\n", opts[i].comment);
    }

  printf ("pBlazRanLib: supported targets: pBlazASM\n");

  exit (1);
}

int
main (int argc, char *argv[])
{
  char **argp;
  int noopts = 0;
  int narch = 0;

  for (argp = argv + 1; *argp; ++argp)
    {
      if (!noopts && (*argp)[0] == '-')
        {
          int i;

          if ((*argp)[1] == '-')
            {
              if ((*argp)[2] == '\0')
                {
                  /* end of options */
                  noopts = 1;
                  continue;
                }
              else
                {
                  /* long option */
                  for (i = 0; i < NELEM (opts); ++i)
                    {
                      if (0 == strcmp (&(*argp)[2], opts[i].long_opt))
                        {
                          if (NULL != opts[i].optfnc)
                            {
                              (*opts[i].optfnc) ();
                              break;
                            }
                        }
                    }
                  if (i >= NELEM (opts))
                    {
                      fprintf (stderr, "pBlazRanLib: unrecognized option `%s'\n", *argp);
                      usage ();
                    }
                }
            }
          else
            {
              char *optp;

              /* short option */
              for (optp = &(*argp)[1]; *optp != '\0'; ++optp)
                {
                  for (i = 0; i < NELEM (opts); ++i)
                    {
                      if (*optp == opts[i].short_opt)
                        {
                          if (NULL != opts[i].optfnc)
                            {
                              (*opts[i].optfnc) ();
                              break;
                            }
                        }
                    }
                  if (i >= NELEM (opts))
                    {
                      fprintf (stderr, "pBlazRanLib: invalid option -- %c\n", *optp);
                      usage ();
                    }
                }
            }
        }
      else
        {
          /* not an option */
          do_ranlib (*argp);
          ++narch;
        }
    }

  if (!narch)
    usage ();

  return 0;
}
