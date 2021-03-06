/* tcap.h -- termcap library functions and variables. */

/* Copyright (C) 1996 Free Software Foundation, Inc.

   This file contains the Readline Library (the Library), a set of
   routines for providing Emacs style line input to programs that ask
   for it.

   The Library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   The Library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   The GNU General Public License is often shipped with GNU software, and
   is generally kept in a file called COPYING or LICENSE.  If you do not
   have a copy of the license, write to the Free Software Foundation,
   675 Mass Ave, Cambridge, MA 02139, USA. */

#if !defined (_RLTCAP_H_)
#define _RLTCAP_H_

#if defined (HAVE_CONFIG_H)
#  include "config.h"
#endif

#if defined (HAVE_TERMCAP_H)
#  include <termcap.h>
#else

/* On Solaris2, sys/types.h #includes sys/reg.h, which #defines PC.
   Unfortunately, PC is a global variable used by the termcap library. */
#ifdef PC
#  undef PC
#endif

extern char PC;
extern char *UP, *BC;

extern short ospeed;

extern int tgetent ();
extern int tgetflag ();
extern int tgetnum ();
extern char *tgetstr ();

extern int tputs ();

extern char *tgoto ();

#endif /* HAVE_TERMCAP_H */

#endif /* !_RLTCAP_H_ */
