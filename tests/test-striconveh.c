/* Test of character set conversion with error handling.
   Copyright (C) 2007 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2007.  */

#include <config.h>

#include "striconveh.h"

#if HAVE_ICONV
# include <iconv.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZEOF(array) (sizeof (array) / sizeof (array[0]))
#define ASSERT(expr) \
  do									     \
    {									     \
      if (!(expr))							     \
        {								     \
          fprintf (stderr, "%s:%d: assertion failed\n", __FILE__, __LINE__); \
          abort ();							     \
        }								     \
    }									     \
  while (0)

/* Magic number for detecting bounds violations.  */
#define MAGIC 0x1983EFF1

static size_t *
new_offsets (size_t n)
{
  size_t *offsets = (size_t *) malloc ((n + 1) * sizeof (size_t));
  offsets[n] = MAGIC;
  return offsets;
}

int
main ()
{
  static enum iconv_ilseq_handler handlers[] =
    { iconveh_error, iconveh_question_mark, iconveh_escape_sequence };
  size_t h;
  size_t o;
  size_t i;

#if HAVE_ICONV
  /* Assume that iconv() supports at least the encodings ASCII, ISO-8859-1,
     ISO-8859-2, and UTF-8.  */
  iconv_t cd_88591_to_88592 = iconv_open ("ISO-8859-2", "ISO-8859-1");
  iconv_t cd_88592_to_88591 = iconv_open ("ISO-8859-1", "ISO-8859-2");
  iconv_t cd_88591_to_utf8 = iconv_open ("UTF-8", "ISO-8859-1");
  iconv_t cd_utf8_to_88591 = iconv_open ("ISO-8859-1", "UTF-8");
  iconv_t cd_88592_to_utf8 = iconv_open ("UTF-8", "ISO-8859-2");
  iconv_t cd_utf8_to_88592 = iconv_open ("ISO-8859-2", "UTF-8");

  ASSERT (cd_88591_to_utf8 != (iconv_t)(-1));
  ASSERT (cd_utf8_to_88591 != (iconv_t)(-1));
  ASSERT (cd_88592_to_utf8 != (iconv_t)(-1));
  ASSERT (cd_utf8_to_88592 != (iconv_t)(-1));

  /* ------------------------ Test mem_cd_iconveh() ------------------------ */

  /* Test conversion from ISO-8859-2 to ISO-8859-1 with no errors.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
      static const char expected[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
      for (o = 0; o < 2; o++)
	{
	  size_t *offsets = (o ? new_offsets (strlen (input)) : NULL);
	  char *result = NULL;
	  size_t length = 0;
	  int retval = mem_cd_iconveh (input, strlen (input),
				       cd_88592_to_88591,
				       cd_88592_to_utf8, cd_utf8_to_88591,
				       handler,
				       offsets,
				       &result, &length);
	  ASSERT (retval == 0);
	  ASSERT (length == strlen (expected));
	  ASSERT (result != NULL && memcmp (result, expected, strlen (expected)) == 0);
	  if (o)
	    {
	      for (i = 0; i < 37; i++)
		ASSERT (offsets[i] == i);
	      ASSERT (offsets[37] == MAGIC);
	      free (offsets);
	    }
	  free (result);
	}
    }

  /* Test conversion from ISO-8859-2 to ISO-8859-1 with EILSEQ.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "Rafa\263 Maszkowski"; /* Rafał Maszkowski */
      for (o = 0; o < 2; o++)
	{
	  size_t *offsets = (o ? new_offsets (strlen (input)) : NULL);
	  char *result = NULL;
	  size_t length = 0;
	  int retval = mem_cd_iconveh (input, strlen (input),
				       cd_88592_to_88591,
				       cd_88592_to_utf8, cd_utf8_to_88591,
				       handler,
				       offsets,
				       &result, &length);
	  switch (handler)
	    {
	    case iconveh_error:
	      ASSERT (retval == -1 && errno == EILSEQ);
	      ASSERT (result == NULL);
	      if (o)
		free (offsets);
	      break;
	    case iconveh_question_mark:
	      {
		static const char expected[] = "Rafa? Maszkowski";
		ASSERT (retval == 0);
		ASSERT (length == strlen (expected));
		ASSERT (result != NULL && memcmp (result, expected, strlen (expected)) == 0);
		if (o)
		  {
		    for (i = 0; i < 16; i++)
		      ASSERT (offsets[i] == i);
		    ASSERT (offsets[16] == MAGIC);
		    free (offsets);
		  }
		free (result);
	      }
	      break;
	    case iconveh_escape_sequence:
	      {
		static const char expected[] = "Rafa\\u0142 Maszkowski";
		ASSERT (retval == 0);
		ASSERT (length == strlen (expected));
		ASSERT (result != NULL && memcmp (result, expected, strlen (expected)) == 0);
		if (o)
		  {
		    for (i = 0; i < 16; i++)
		      ASSERT (offsets[i] == (i < 5 ? i :
					     i + 5));
		    ASSERT (offsets[16] == MAGIC);
		    free (offsets);
		  }
		free (result);
	      }
	      break;
	    }
	}
    }

  /* Test conversion from ISO-8859-1 to UTF-8 with no errors.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
      static const char expected[] = "\303\204rger mit b\303\266sen B\303\274bchen ohne Augenma\303\237";
      for (o = 0; o < 2; o++)
	{
	  size_t *offsets = (o ? new_offsets (strlen (input)) : NULL);
	  char *result = NULL;
	  size_t length = 0;
	  int retval = mem_cd_iconveh (input, strlen (input),
				       cd_88591_to_utf8,
				       cd_88591_to_utf8, (iconv_t)(-1),
				       handler,
				       offsets,
				       &result, &length);
	  ASSERT (retval == 0);
	  ASSERT (length == strlen (expected));
	  ASSERT (result != NULL && memcmp (result, expected, strlen (expected)) == 0);
	  if (o)
	    {
	      for (i = 0; i < 37; i++)
		ASSERT (offsets[i] == (i < 1 ? i :
				       i < 12 ? i + 1 :
				       i < 18 ? i + 2 :
				       i + 3));
	      ASSERT (offsets[37] == MAGIC);
	      free (offsets);
	    }
	  free (result);
	}
    }

  /* Test conversion from UTF-8 to ISO-8859-1 with no errors.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "\303\204rger mit b\303\266sen B\303\274bchen ohne Augenma\303\237";
      static const char expected[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
      for (o = 0; o < 2; o++)
	{
	  size_t *offsets = (o ? new_offsets (strlen (input)) : NULL);
	  char *result = NULL;
	  size_t length = 0;
	  int retval = mem_cd_iconveh (input, strlen (input),
				       cd_utf8_to_88591,
				       (iconv_t)(-1), cd_utf8_to_88591,
				       handler,
				       offsets,
				       &result, &length);
	  ASSERT (retval == 0);
	  ASSERT (length == strlen (expected));
	  ASSERT (result != NULL && memcmp (result, expected, strlen (expected)) == 0);
	  if (o)
	    {
	      for (i = 0; i < 41; i++)
		ASSERT (offsets[i] == (i < 1 ? i :
				       i == 1 ? (size_t)(-1) :
				       i < 13 ? i - 1 :
				       i == 13 ? (size_t)(-1) :
				       i < 20 ? i - 2 :
				       i == 20 ? (size_t)(-1) :
				       i < 40 ? i - 3 :
				       (size_t)(-1)));
	      ASSERT (offsets[41] == MAGIC);
	      free (offsets);
	    }
	  free (result);
	}
    }

  /* Test conversion from UTF-8 to ISO-8859-1 with EILSEQ.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "Rafa\305\202 Maszkowski"; /* Rafał Maszkowski */
      for (o = 0; o < 2; o++)
	{
	  size_t *offsets = (o ? new_offsets (strlen (input)) : NULL);
	  char *result = NULL;
	  size_t length = 0;
	  int retval = mem_cd_iconveh (input, strlen (input),
				       cd_utf8_to_88591,
				       (iconv_t)(-1), cd_utf8_to_88591,
				       handler,
				       offsets,
				       &result, &length);
	  switch (handler)
	    {
	    case iconveh_error:
	      ASSERT (retval == -1 && errno == EILSEQ);
	      ASSERT (result == NULL);
	      if (o)
		free (offsets);
	      break;
	    case iconveh_question_mark:
	      {
		static const char expected[] = "Rafa? Maszkowski";
		ASSERT (retval == 0);
		ASSERT (length == strlen (expected));
		ASSERT (result != NULL && memcmp (result, expected, strlen (expected)) == 0);
		if (o)
		  {
		    for (i = 0; i < 17; i++)
		      ASSERT (offsets[i] == (i < 5 ? i :
					     i == 5 ? (size_t)(-1) :
					     i - 1));
		    ASSERT (offsets[17] == MAGIC);
		    free (offsets);
		  }
		free (result);
	      }
	      break;
	    case iconveh_escape_sequence:
	      {
		static const char expected[] = "Rafa\\u0142 Maszkowski";
		ASSERT (retval == 0);
		ASSERT (length == strlen (expected));
		ASSERT (result != NULL && memcmp (result, expected, strlen (expected)) == 0);
		if (o)
		  {
		    for (i = 0; i < 17; i++)
		      ASSERT (offsets[i] == (i < 5 ? i :
					     i == 5 ? (size_t)(-1) :
					     i + 4));
		    ASSERT (offsets[17] == MAGIC);
		    free (offsets);
		  }
		free (result);
	      }
	      break;
	    }
	}
    }

  /* Test conversion from UTF-8 to ISO-8859-1 with EINVAL.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "\342";
      for (o = 0; o < 2; o++)
	{
	  size_t *offsets = (o ? new_offsets (strlen (input)) : NULL);
	  char *result = NULL;
	  size_t length = 0;
	  int retval = mem_cd_iconveh (input, strlen (input),
				       cd_utf8_to_88591,
				       (iconv_t)(-1), cd_utf8_to_88591,
				       handler,
				       offsets,
				       &result, &length);
	  ASSERT (retval == 0);
	  ASSERT (length == 0);
	  if (o)
	    {
	      ASSERT (offsets[0] == 0);
	      ASSERT (offsets[1] == MAGIC);
	      free (offsets);
	    }
	  if (result != NULL)
	    free (result);
	}
    }

  /* ------------------------ Test str_cd_iconveh() ------------------------ */

  /* Test conversion from ISO-8859-2 to ISO-8859-1 with no errors.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
      static const char expected[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
      char *result = str_cd_iconveh (input,
				     cd_88592_to_88591,
				     cd_88592_to_utf8, cd_utf8_to_88591,
				     handler);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, expected) == 0);
      free (result);
    }

  /* Test conversion from ISO-8859-2 to ISO-8859-1 with EILSEQ.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "Rafa\263 Maszkowski"; /* Rafał Maszkowski */
      char *result = str_cd_iconveh (input,
				     cd_88592_to_88591,
				     cd_88592_to_utf8, cd_utf8_to_88591,
				     handler);
      switch (handler)
	{
	case iconveh_error:
	  ASSERT (result == NULL && errno == EILSEQ);
	  break;
	case iconveh_question_mark:
	  {
	    static const char expected[] = "Rafa? Maszkowski";
	    ASSERT (result != NULL);
	    ASSERT (strcmp (result, expected) == 0);
	    free (result);
	  }
	  break;
	case iconveh_escape_sequence:
	  {
	    static const char expected[] = "Rafa\\u0142 Maszkowski";
	    ASSERT (result != NULL);
	    ASSERT (strcmp (result, expected) == 0);
	    free (result);
	  }
	  break;
	}
    }

  /* Test conversion from ISO-8859-1 to UTF-8 with no errors.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
      static const char expected[] = "\303\204rger mit b\303\266sen B\303\274bchen ohne Augenma\303\237";
      char *result = str_cd_iconveh (input,
				     cd_88591_to_utf8,
				     cd_88591_to_utf8, (iconv_t)(-1),
				     handler);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, expected) == 0);
      free (result);
    }

  /* Test conversion from UTF-8 to ISO-8859-1 with no errors.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "\303\204rger mit b\303\266sen B\303\274bchen ohne Augenma\303\237";
      static const char expected[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
      char *result = str_cd_iconveh (input,
				     cd_utf8_to_88591,
				     (iconv_t)(-1), cd_utf8_to_88591,
				     handler);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, expected) == 0);
      free (result);
    }

  /* Test conversion from UTF-8 to ISO-8859-1 with EILSEQ.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "Costs: 27 \342\202\254"; /* EURO SIGN */
      char *result = str_cd_iconveh (input,
				     cd_utf8_to_88591,
				     (iconv_t)(-1), cd_utf8_to_88591,
				     handler);
      switch (handler)
	{
	case iconveh_error:
	  ASSERT (result == NULL && errno == EILSEQ);
	  break;
	case iconveh_question_mark:
	  {
	    static const char expected[] = "Costs: 27 ?";
	    ASSERT (result != NULL);
	    ASSERT (strcmp (result, expected) == 0);
	    free (result);
	  }
	  break;
	case iconveh_escape_sequence:
	  {
	    static const char expected[] = "Costs: 27 \\u20AC";
	    ASSERT (result != NULL);
	    ASSERT (strcmp (result, expected) == 0);
	    free (result);
	  }
	  break;
	}
    }

  /* Test conversion from UTF-8 to ISO-8859-1 with EINVAL.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "\342";
      char *result = str_cd_iconveh (input,
				     cd_utf8_to_88591,
				     (iconv_t)(-1), cd_utf8_to_88591,
				     handler);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, "") == 0);
      free (result);
    }

  if (cd_88591_to_88592 != (iconv_t)(-1))
    iconv_close (cd_88591_to_88592);
  if (cd_88592_to_88591 != (iconv_t)(-1))
    iconv_close (cd_88592_to_88591);
  iconv_close (cd_88591_to_utf8);
  iconv_close (cd_utf8_to_88591);
  iconv_close (cd_88592_to_utf8);
  iconv_close (cd_utf8_to_88592);

  /* ------------------------- Test mem_iconveh() ------------------------- */

  /* Test conversion from ISO-8859-2 to ISO-8859-1 with no errors.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
      static const char expected[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
      for (o = 0; o < 2; o++)
	{
	  size_t *offsets = (o ? new_offsets (strlen (input)) : NULL);
	  char *result = NULL;
	  size_t length = 0;
	  int retval = mem_iconveh (input, strlen (input),
				    "ISO-8859-2", "ISO-8859-1",
				    handler,
				    offsets,
				    &result, &length);
	  ASSERT (retval == 0);
	  ASSERT (length == strlen (expected));
	  ASSERT (result != NULL && memcmp (result, expected, strlen (expected)) == 0);
	  if (o)
	    {
	      for (i = 0; i < 37; i++)
		ASSERT (offsets[i] == i);
	      ASSERT (offsets[37] == MAGIC);
	      free (offsets);
	    }
	  free (result);
	}
    }

  /* Test conversion from ISO-8859-2 to ISO-8859-1 with EILSEQ.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "Rafa\263 Maszkowski"; /* Rafał Maszkowski */
      for (o = 0; o < 2; o++)
	{
	  size_t *offsets = (o ? new_offsets (strlen (input)) : NULL);
	  char *result = NULL;
	  size_t length = 0;
	  int retval = mem_iconveh (input, strlen (input),
				    "ISO-8859-2", "ISO-8859-1",
				    handler,
				    offsets,
				    &result, &length);
	  switch (handler)
	    {
	    case iconveh_error:
	      ASSERT (retval == -1 && errno == EILSEQ);
	      ASSERT (result == NULL);
	      if (o)
		free (offsets);
	      break;
	    case iconveh_question_mark:
	      {
		static const char expected[] = "Rafa? Maszkowski";
		ASSERT (retval == 0);
		ASSERT (length == strlen (expected));
		ASSERT (result != NULL && memcmp (result, expected, strlen (expected)) == 0);
		if (o)
		  {
		    for (i = 0; i < 16; i++)
		      ASSERT (offsets[i] == i);
		    ASSERT (offsets[16] == MAGIC);
		    free (offsets);
		  }
		free (result);
	      }
	      break;
	    case iconveh_escape_sequence:
	      {
		static const char expected[] = "Rafa\\u0142 Maszkowski";
		ASSERT (retval == 0);
		ASSERT (length == strlen (expected));
		ASSERT (result != NULL && memcmp (result, expected, strlen (expected)) == 0);
		if (o)
		  {
		    for (i = 0; i < 16; i++)
		      ASSERT (offsets[i] == (i < 5 ? i :
					     i + 5));
		    ASSERT (offsets[16] == MAGIC);
		    free (offsets);
		  }
		free (result);
	      }
	      break;
	    }
	}
    }

  /* Test conversion from ISO-8859-1 to UTF-8 with no errors.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
      static const char expected[] = "\303\204rger mit b\303\266sen B\303\274bchen ohne Augenma\303\237";
      for (o = 0; o < 2; o++)
	{
	  size_t *offsets = (o ? new_offsets (strlen (input)) : NULL);
	  char *result = NULL;
	  size_t length = 0;
	  int retval = mem_iconveh (input, strlen (input),
				    "ISO-8859-1", "UTF-8",
				    handler,
				    offsets,
				    &result, &length);
	  ASSERT (retval == 0);
	  ASSERT (length == strlen (expected));
	  ASSERT (result != NULL && memcmp (result, expected, strlen (expected)) == 0);
	  if (o)
	    {
	      for (i = 0; i < 37; i++)
		ASSERT (offsets[i] == (i < 1 ? i :
				       i < 12 ? i + 1 :
				       i < 18 ? i + 2 :
				       i + 3));
	      ASSERT (offsets[37] == MAGIC);
	      free (offsets);
	    }
	  free (result);
	}
    }

  /* Test conversion from UTF-8 to ISO-8859-1 with no errors.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "\303\204rger mit b\303\266sen B\303\274bchen ohne Augenma\303\237";
      static const char expected[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
      for (o = 0; o < 2; o++)
	{
	  size_t *offsets = (o ? new_offsets (strlen (input)) : NULL);
	  char *result = NULL;
	  size_t length = 0;
	  int retval = mem_iconveh (input, strlen (input),
				    "UTF-8", "ISO-8859-1",
				    handler,
				    offsets,
				    &result, &length);
	  ASSERT (retval == 0);
	  ASSERT (length == strlen (expected));
	  ASSERT (result != NULL && memcmp (result, expected, strlen (expected)) == 0);
	  if (o)
	    {
	      for (i = 0; i < 41; i++)
		ASSERT (offsets[i] == (i < 1 ? i :
				       i == 1 ? (size_t)(-1) :
				       i < 13 ? i - 1 :
				       i == 13 ? (size_t)(-1) :
				       i < 20 ? i - 2 :
				       i == 20 ? (size_t)(-1) :
				       i < 40 ? i - 3 :
				       (size_t)(-1)));
	      ASSERT (offsets[41] == MAGIC);
	      free (offsets);
	    }
	  free (result);
	}
    }

  /* Test conversion from UTF-8 to ISO-8859-1 with EILSEQ.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "Rafa\305\202 Maszkowski"; /* Rafał Maszkowski */
      for (o = 0; o < 2; o++)
	{
	  size_t *offsets = (o ? new_offsets (strlen (input)) : NULL);
	  char *result = NULL;
	  size_t length = 0;
	  int retval = mem_iconveh (input, strlen (input),
				    "UTF-8", "ISO-8859-1",
				    handler,
				    offsets,
				    &result, &length);
	  switch (handler)
	    {
	    case iconveh_error:
	      ASSERT (retval == -1 && errno == EILSEQ);
	      ASSERT (result == NULL);
	      if (o)
		free (offsets);
	      break;
	    case iconveh_question_mark:
	      {
		static const char expected[] = "Rafa? Maszkowski";
		ASSERT (retval == 0);
		ASSERT (length == strlen (expected));
		ASSERT (result != NULL && memcmp (result, expected, strlen (expected)) == 0);
		if (o)
		  {
		    for (i = 0; i < 17; i++)
		      ASSERT (offsets[i] == (i < 5 ? i :
					     i == 5 ? (size_t)(-1) :
					     i - 1));
		    ASSERT (offsets[17] == MAGIC);
		    free (offsets);
		  }
		free (result);
	      }
	      break;
	    case iconveh_escape_sequence:
	      {
		static const char expected[] = "Rafa\\u0142 Maszkowski";
		ASSERT (retval == 0);
		ASSERT (length == strlen (expected));
		ASSERT (result != NULL && memcmp (result, expected, strlen (expected)) == 0);
		if (o)
		  {
		    for (i = 0; i < 17; i++)
		      ASSERT (offsets[i] == (i < 5 ? i :
					     i == 5 ? (size_t)(-1) :
					     i + 4));
		    ASSERT (offsets[17] == MAGIC);
		    free (offsets);
		  }
		free (result);
	      }
	      break;
	    }
	}
    }

  /* Test conversion from UTF-8 to ISO-8859-1 with EINVAL.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "\342";
      for (o = 0; o < 2; o++)
	{
	  size_t *offsets = (o ? new_offsets (strlen (input)) : NULL);
	  char *result = NULL;
	  size_t length = 0;
	  int retval = mem_iconveh (input, strlen (input),
				    "UTF-8", "ISO-8859-1",
				    handler,
				    offsets,
				    &result, &length);
	  ASSERT (retval == 0);
	  ASSERT (length == 0);
	  if (o)
	    {
	      ASSERT (offsets[0] == 0);
	      ASSERT (offsets[1] == MAGIC);
	      free (offsets);
	    }
	  if (result != NULL)
	    free (result);
	}
    }

  /* ------------------------- Test str_iconveh() ------------------------- */

  /* Test conversion from ISO-8859-2 to ISO-8859-1 with no errors.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
      static const char expected[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
      char *result = str_iconveh (input, "ISO-8859-2", "ISO-8859-1", handler);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, expected) == 0);
      free (result);
    }

  /* Test conversion from ISO-8859-2 to ISO-8859-1 with EILSEQ.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "Rafa\263 Maszkowski"; /* Rafał Maszkowski */
      char *result = str_iconveh (input, "ISO-8859-2", "ISO-8859-1", handler);
      switch (handler)
	{
	case iconveh_error:
	  ASSERT (result == NULL && errno == EILSEQ);
	  break;
	case iconveh_question_mark:
	  {
	    static const char expected[] = "Rafa? Maszkowski";
	    ASSERT (result != NULL);
	    ASSERT (strcmp (result, expected) == 0);
	    free (result);
	  }
	  break;
	case iconveh_escape_sequence:
	  {
	    static const char expected[] = "Rafa\\u0142 Maszkowski";
	    ASSERT (result != NULL);
	    ASSERT (strcmp (result, expected) == 0);
	    free (result);
	  }
	  break;
	}
    }

  /* Test conversion from ISO-8859-1 to UTF-8 with no errors.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
      static const char expected[] = "\303\204rger mit b\303\266sen B\303\274bchen ohne Augenma\303\237";
      char *result = str_iconveh (input, "ISO-8859-1", "UTF-8", handler);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, expected) == 0);
      free (result);
    }

  /* Test conversion from UTF-8 to ISO-8859-1 with no errors.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "\303\204rger mit b\303\266sen B\303\274bchen ohne Augenma\303\237";
      static const char expected[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
      char *result = str_iconveh (input, "UTF-8", "ISO-8859-1", handler);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, expected) == 0);
      free (result);
    }

  /* Test conversion from UTF-8 to ISO-8859-1 with EILSEQ.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "Costs: 27 \342\202\254"; /* EURO SIGN */
      char *result = str_iconveh (input, "UTF-8", "ISO-8859-1", handler);
      switch (handler)
	{
	case iconveh_error:
	  ASSERT (result == NULL && errno == EILSEQ);
	  break;
	case iconveh_question_mark:
	  {
	    static const char expected[] = "Costs: 27 ?";
	    ASSERT (result != NULL);
	    ASSERT (strcmp (result, expected) == 0);
	    free (result);
	  }
	  break;
	case iconveh_escape_sequence:
	  {
	    static const char expected[] = "Costs: 27 \\u20AC";
	    ASSERT (result != NULL);
	    ASSERT (strcmp (result, expected) == 0);
	    free (result);
	  }
	  break;
	}
    }

  /* Test conversion from UTF-8 to ISO-8859-1 with EINVAL.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "\342";
      char *result = str_iconveh (input, "UTF-8", "ISO-8859-1", handler);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, "") == 0);
      free (result);
    }

#endif

  return 0;
}
