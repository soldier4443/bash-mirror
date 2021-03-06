/* xmalloc.c -- safe versions of malloc and realloc */

/* Copyright (C) 1991 Free Software Foundation, Inc.

   This file is part of GNU Readline, a library for reading lines
   of text with interactive input and history editing.

   Readline is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 1, or (at your option) any
   later version.

   Readline is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Readline; see the file COPYING.  If not, write to the Free
   Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. */

#if defined (HAVE_CONFIG_H)
#include <config.h>
#endif

#include <sys/types.h>
#include <stdio.h>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#if defined (HAVE_STDLIB_H)
#  include <stdlib.h>
#else
#  include "ansi_stdlib.h"
#endif /* HAVE_STDLIB_H */

#include "error.h"

#if !defined (PTR_T)
#  if defined (__STDC__)
#    define PTR_T void *
#  else
#    define PTR_T char *
#  endif /* !__STDC__ */
#endif /* !PTR_T */

#if !defined (SBRK_DECLARED)
extern char *sbrk();
#endif

static PTR_T lbreak;
static int brkfound;
static size_t allocated;

/* **************************************************************** */
/*								    */
/*		   Memory Allocation and Deallocation.		    */
/*								    */
/* **************************************************************** */

/* Return a pointer to free()able block of memory large enough
   to hold BYTES number of bytes.  If the memory cannot be allocated,
   print an error message and abort. */
char *
xmalloc (bytes)
     size_t bytes;
{
  char *temp;

  temp = (char *)malloc (bytes);

  if (temp == 0)
    {
      if (brkfound == 0)
	{
	  lbreak = (PTR_T)sbrk (0);
	  brkfound++;
	}
      allocated = (char *)sbrk (0) - (char *)lbreak;
      fatal_error ("xmalloc: cannot allocate %lu bytes (%lu bytes allocated)", (unsigned long)bytes, (unsigned long)allocated);
    }

  return (temp);
}

char *
xrealloc (pointer, bytes)
     PTR_T pointer;
     size_t bytes;
{
  char *temp;

  temp = pointer ? (char *)realloc (pointer, bytes) : (char *)malloc (bytes);

  if (temp == 0)
    {
      if (brkfound == 0)
	{
	  lbreak = (PTR_T)sbrk (0);
	  brkfound++;
	}
      allocated = (char *)sbrk (0) - (char *)lbreak;
      fatal_error ("xrealloc: cannot reallocate %lu bytes (%lu bytes allocated)", (unsigned long)bytes, (unsigned long)allocated);
    }

  return (temp);
}

/* Use this as the function to call when adding unwind protects so we
   don't need to know what free() returns. */
void
xfree (string)
     char *string;
{
  if (string)
    free (string);
}
