/* Formatted output to strings.
   Copyright (C) 1999, 2002, 2006-2007 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
   USA.  */

/* Tell glibc's <stdio.h> to provide a prototype for snprintf().
   This must come before <config.h> because <config.h> may include
   <features.h>, and once <features.h> has been included, it's too late.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE    1
#endif

#include <config.h>
#include <alloca.h>

/* Specification.  */
#include "unistdio.h"

#include "uniconv.h"
#include "unistr.h"
#include "unistdio/u-printf-parse.h"

#define VASNPRINTF ulc_vasnprintf
#define FCHAR_T char
#define DIRECTIVE char_directive
#define DIRECTIVES char_directives
#define FCHAR_T_ONLY_ASCII 1
#define PRINTF_PARSE ulc_printf_parse
#define DCHAR_T char
#define DCHAR_CPY memcpy
#define DCHAR_SET memset
#define DCHAR_MBSNLEN mbsnlen
#define TCHAR_T char
#define DCHAR_IS_TCHAR 1
#include "vasnprintf.c"
