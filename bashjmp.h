/* bashjmp.h -- wrapper for setjmp.h with necessary bash definitions. */

#ifndef _BASHJMP_H_
#define _BASHJMP_H_

#include <setjmp.h>

/* This *must* be included *after* config.h */

#if defined (HAVE_POSIX_SIGSETJMP)
#  define procenv_t	sigjmp_buf
#  undef setjmp
#  define setjmp(x)	sigsetjmp((x), 1)
#  undef longjmp
#  define longjmp(x, n)	siglongjmp((x), (n))
#else
#  define procenv_t	jmp_buf
#endif

extern procenv_t	top_level;
extern procenv_t	subshell_top_level;
extern procenv_t	return_catch;	/* used by `return' builtin */

#define SHFUNC_RETURN()	longjmp (return_catch, 1)

#define COPY_PROCENV(old, save) \
	xbcopy ((char *)old, (char *)save, sizeof (procenv_t));

/* Values for the second argument to longjmp/siglongjmp. */
#define NOT_JUMPED 0		/* Not returning from a longjmp. */
#define FORCE_EOF 1		/* We want to stop parsing. */
#define DISCARD 2		/* Discard current command. */
#define EXITPROG 3		/* Unconditionally exit the program now. */

#endif /* _BASHJMP_H_ */
