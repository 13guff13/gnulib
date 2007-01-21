/* Character set conversion with error handling and autodetection.
   Copyright (C) 2002, 2005, 2007 Free Software Foundation, Inc.
   Written by Bruno Haible.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <config.h>

/* Specification.  */
#include "striconveha.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define SIZEOF(a) (sizeof(a)/sizeof(a[0]))


/* Autodetection list.  */

struct autodetect_alias
{
  struct autodetect_alias *next;
  const char *name;
  const char * const *encodings_to_try;
};

static const char * const autodetect_utf8_try[] =
{
  /* Try UTF-8 first. There are very few ISO-8859-1 inputs that would
     be valid UTF-8, but many UTF-8 inputs are valid ISO-8859-1.  */
  "UTF-8", "ISO-8859-1",
  NULL
};
static const char * const autodetect_jp_try[] =
{
  /* Try 7-bit encoding first. If the input contains bytes >= 0x80,
     it will fail.
     Try EUC-JP next. Short SHIFT_JIS inputs may come out wrong. This
     is unavoidable. People will condemn SHIFT_JIS.
     If we tried SHIFT_JIS first, then some short EUC-JP inputs would
     come out wrong, and people would condemn EUC-JP and Unix, which
     would not be good.
     Finally try SHIFT_JIS.  */
  "ISO-2022-JP-2", "EUC-JP", "SHIFT_JIS",
  NULL
};
static const char * const autodetect_kr_try[] =
{
  /* Try 7-bit encoding first. If the input contains bytes >= 0x80,
     it will fail.
     Finally try EUC-KR.  */
  "ISO-2022-KR", "EUC-KR",
  NULL
};

static struct autodetect_alias autodetect_predefined[] =
{
  { &autodetect_predefined[1], "autodetect_utf8", autodetect_utf8_try },
  { &autodetect_predefined[2], "autodetect_jp",   autodetect_jp_try },
  { NULL,                      "autodetect_kr",   autodetect_kr_try }
};

static struct autodetect_alias *autodetect_list = &autodetect_predefined[0];
static struct autodetect_alias **autodetect_list_end =
  &autodetect_predefined[SIZEOF(autodetect_predefined)-1].next;

int
uniconv_register_autodetect (const char *name,
			     const char * const *try_in_order)
{
  size_t namelen;
  size_t listlen;
  size_t memneed;
  size_t i;
  char *memory;
  struct autodetect_alias *new_alias;
  char *new_name;
  const char **new_try_in_order;

  /* The TRY_IN_ORDER list must not be empty.  */
  if (try_in_order[0] == NULL)
    {
      errno = EINVAL;
      return -1;
    }

  /* We must deep-copy NAME and TRY_IN_ORDER, because they may be allocated
     with dynamic extent.  */
  namelen = strlen (name) + 1;
  memneed = sizeof (struct autodetect_alias) + namelen + sizeof (char *);
  for (i = 0; try_in_order[i] != NULL; i++)
    memneed += sizeof (char *) + strlen (try_in_order[i]) + 1;
  listlen = i;

  memory = (char *) malloc (memneed);
  if (memory != NULL)
    {
      new_alias = (struct autodetect_alias *) memory;
      memory += sizeof (struct autodetect_alias);

      new_try_in_order = (const char **) memory;
      memory += (listlen + 1) * sizeof (char *);

      new_name = (char *) memory;
      memcpy (new_name, name, namelen);
      memory += namelen;

      for (i = 0; i < listlen; i++)
	{
	  size_t len = strlen (try_in_order[i]) + 1;
	  memcpy (memory, try_in_order[i], len);
	  new_try_in_order[i] = (const char *) memory;
	  memory += len;
	}
      new_try_in_order[i] = NULL;

      /* Now insert the new alias.  */
      new_alias->name = new_name;
      new_alias->encodings_to_try = new_try_in_order;
      new_alias->next = NULL;
      /* FIXME: Not multithread-safe.  */
      *autodetect_list_end = new_alias;
      autodetect_list_end = &new_alias->next;
      return 0;
    }
  else
    {
      errno = ENOMEM;
      return -1;
    }
}

int
mem_iconveha (const char *src, size_t srclen,
	      const char *from_codeset, const char *to_codeset,
	      enum iconv_ilseq_handler handler,
	      char **resultp, size_t *lengthp)
{
  int retval = mem_iconveh (src, srclen, from_codeset, to_codeset, handler,
			    resultp, lengthp);
  if (retval >= 0 || errno != EINVAL)
    return retval;
  else
    {
      struct autodetect_alias *alias;

      /* Unsupported from_codeset or to_codeset. Check whether the caller
	 requested autodetection.  */
      for (alias = autodetect_list; alias != NULL; alias = alias->next)
	if (strcmp (from_codeset, alias->name) == 0)
	  {
	    const char * const *encodings = alias->encodings_to_try;

	    do
	      {
		retval = mem_iconveha (src, srclen,
				       from_codeset, to_codeset, handler,
				       resultp, lengthp);
		if (!(retval < 0 && errno == EILSEQ))
		  return retval;
		encodings++;
	      }
	    while (*encodings != NULL);

	    /* Return the last call's result.  */
	    return -1;
	  }

      /* It wasn't an autodetection name.  */
      errno = EINVAL;
      return -1;
    }
}

char *
str_iconveha (const char *src,
	      const char *from_codeset, const char *to_codeset,
	      enum iconv_ilseq_handler handler)
{
  char *result = str_iconveh (src, from_codeset, to_codeset, handler);

  if (result != NULL || errno != EINVAL)
    return result;
  else
    {
      struct autodetect_alias *alias;

      /* Unsupported from_codeset or to_codeset. Check whether the caller
	 requested autodetection.  */
      for (alias = autodetect_list; alias != NULL; alias = alias->next)
	if (strcmp (from_codeset, alias->name) == 0)
	  {
	    const char * const *encodings = alias->encodings_to_try;

	    do
	      {
		result = str_iconveha (src, *encodings, to_codeset, handler);
		if (!(result == NULL && errno == EILSEQ))
		  return result;
		encodings++;
	      }
	    while (*encodings != NULL);

	    /* Return the last call's result.  */
	    return NULL;
	  }

      /* It wasn't an autodetection name.  */
      errno = EINVAL;
      return NULL;
    }
}
