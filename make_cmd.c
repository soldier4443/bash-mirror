/* make_cmd.c -- Functions for making instances of the various
   parser constructs. */

/* Copyright (C) 1989 Free Software Foundation, Inc.

This file is part of GNU Bash, the Bourne Again SHell.

Bash is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 1, or (at your option) any later
version.

Bash is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with Bash; see the file COPYING.  If not, write to the Free Software
Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. */

#include "config.h"

#include <stdio.h>
#include "bashtypes.h"
#include <sys/file.h>
#include "filecntl.h"
#include "bashansi.h"
#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include "command.h"
#include "general.h"
#include "error.h"
#include "flags.h"
#include "make_cmd.h"
#include "variables.h"
#include "subst.h"
#include "input.h"
#include "externs.h"

#if defined (JOB_CONTROL)
#include "jobs.h"
#endif

extern int line_number, current_command_line_count;
extern int disallow_filename_globbing;

WORD_DESC *
make_bare_word (string)
     char *string;
{
  WORD_DESC *temp;

  temp = (WORD_DESC *)xmalloc (sizeof (WORD_DESC));
  if (*string)
    temp->word = savestring (string);
  else
    {
      temp->word = xmalloc (1);
      temp->word[0] = '\0';
    }

  temp->flags = 0;
  return (temp);
}

WORD_DESC *
make_word_flags (w, string)
     WORD_DESC *w;
     char *string;
{
  register char *s;

  for (s = string; *s; s++)
    switch (*s)
      {
	case '$':
	  w->flags |= W_HASDOLLAR;
	  break;
	case '\\':
	  break;	/* continue the loop */
	case '\'':
	case '`':
	case '"':
	  w->flags |= W_QUOTED;
	  break;
      }
  return (w);
}

WORD_DESC *
make_word (string)
     char *string;
{
  WORD_DESC *temp;

  temp = make_bare_word (string);
  return (make_word_flags (temp, string));
}

WORD_DESC *
make_word_from_token (token)
     int token;
{
  char tokenizer[2];

  tokenizer[0] = token;
  tokenizer[1] = '\0';

  return (make_word (tokenizer));
}

WORD_LIST *
make_word_list (word, link)
     WORD_DESC *word;
     WORD_LIST *link;
{
  WORD_LIST *temp;

  temp = (WORD_LIST *)xmalloc (sizeof (WORD_LIST));
  temp->word = word;
  temp->next = link;
  return (temp);
}

WORD_LIST *
add_string_to_list (string, list)
     char *string;
     WORD_LIST *list;
{
  WORD_LIST *temp;

  temp = (WORD_LIST *)xmalloc (sizeof (WORD_LIST));
  temp->word = make_word (string);
  temp->next = list;
  return (temp);
}

COMMAND *
make_command (type, pointer)
     enum command_type type;
     SIMPLE_COM *pointer;
{
  COMMAND *temp;

  temp = (COMMAND *)xmalloc (sizeof (COMMAND));
  temp->type = type;
  temp->value.Simple = pointer;
  temp->value.Simple->flags = temp->flags = 0;
  temp->redirects = (REDIRECT *)NULL;
  return (temp);
}

COMMAND *
command_connect (com1, com2, connector)
     COMMAND *com1, *com2;
     int connector;
{
  CONNECTION *temp;

  temp = (CONNECTION *)xmalloc (sizeof (CONNECTION));
  temp->connector = connector;
  temp->first = com1;
  temp->second = com2;
  return (make_command (cm_connection, (SIMPLE_COM *)temp));
}

static COMMAND *
make_for_or_select (type, name, map_list, action)
     enum command_type type;
     WORD_DESC *name;
     WORD_LIST *map_list;
     COMMAND *action;
{
  FOR_COM *temp;

  temp = (FOR_COM *)xmalloc (sizeof (FOR_COM));
  temp->flags = 0;
  temp->name = name;
  temp->map_list = map_list;
  temp->action = action;
  return (make_command (type, (SIMPLE_COM *)temp));
}

COMMAND *
make_for_command (name, map_list, action)
     WORD_DESC *name;
     WORD_LIST *map_list;
     COMMAND *action;
{
  return (make_for_or_select (cm_for, name, map_list, action));
}

COMMAND *
make_select_command (name, map_list, action)
     WORD_DESC *name;
     WORD_LIST *map_list;
     COMMAND *action;
{
#if defined (SELECT_COMMAND)
  return (make_for_or_select (cm_select, name, map_list, action));
#endif
}

COMMAND *
make_group_command (command)
     COMMAND *command;
{
  GROUP_COM *temp;

  temp = (GROUP_COM *)xmalloc (sizeof (GROUP_COM));
  temp->command = command;
  return (make_command (cm_group, (SIMPLE_COM *)temp));
}

COMMAND *
make_case_command (word, clauses)
     WORD_DESC *word;
     PATTERN_LIST *clauses;
{
  CASE_COM *temp;

  temp = (CASE_COM *)xmalloc (sizeof (CASE_COM));
  temp->flags = 0;
  temp->word = word;
  temp->clauses = REVERSE_LIST (clauses, PATTERN_LIST *);
  return (make_command (cm_case, (SIMPLE_COM *)temp));
}

PATTERN_LIST *
make_pattern_list (patterns, action)
     WORD_LIST *patterns;
     COMMAND *action;
{
  PATTERN_LIST *temp;

  temp = (PATTERN_LIST *)xmalloc (sizeof (PATTERN_LIST));
  temp->patterns = REVERSE_LIST (patterns, WORD_LIST *);
  temp->action = action;
  temp->next = NULL;
  return (temp);
}

COMMAND *
make_if_command (test, true_case, false_case)
     COMMAND *test, *true_case, *false_case;
{
  IF_COM *temp;

  temp = (IF_COM *)xmalloc (sizeof (IF_COM));
  temp->flags = 0;
  temp->test = test;
  temp->true_case = true_case;
  temp->false_case = false_case;
  return (make_command (cm_if, (SIMPLE_COM *)temp));
}

static COMMAND *
make_until_or_while (which, test, action)
     enum command_type which;
     COMMAND *test, *action;
{
  WHILE_COM *temp;

  temp = (WHILE_COM *)xmalloc (sizeof (WHILE_COM));
  temp->flags = 0;
  temp->test = test;
  temp->action = action;
  return (make_command (which, (SIMPLE_COM *)temp));
}

COMMAND *
make_while_command (test, action)
     COMMAND *test, *action;
{
  return (make_until_or_while (cm_while, test, action));
}

COMMAND *
make_until_command (test, action)
     COMMAND *test, *action;
{
  return (make_until_or_while (cm_until, test, action));
}

COMMAND *
make_bare_simple_command ()
{
  COMMAND *command;
  SIMPLE_COM *temp;

  command = (COMMAND *)xmalloc (sizeof (COMMAND));
  command->value.Simple = temp = (SIMPLE_COM *)xmalloc (sizeof (SIMPLE_COM));

  temp->flags = 0;
  temp->line = line_number;
  temp->words = (WORD_LIST *)NULL;
  temp->redirects = (REDIRECT *)NULL;

  command->type = cm_simple;
  command->redirects = (REDIRECT *)NULL;
  command->flags = 0;

  return (command);
}

/* Return a command which is the connection of the word or redirection
   in ELEMENT, and the command * or NULL in COMMAND. */
COMMAND *
make_simple_command (element, command)
     ELEMENT element;
     COMMAND *command;
{
  /* If we are starting from scratch, then make the initial command
     structure.  Also note that we have to fill in all the slots, since
     malloc doesn't return zeroed space. */
  if (!command)
    command = make_bare_simple_command ();

  if (element.word)
    {
      WORD_LIST *tw = (WORD_LIST *)xmalloc (sizeof (WORD_LIST));
      tw->word = element.word;
      tw->next = command->value.Simple->words;
      command->value.Simple->words = tw;
    }
  else
    {
      REDIRECT *r = element.redirect;
      /* Due to the way <> is implemented, there may be more than a single
	 redirection in element.redirect.  We just follow the chain as far
	 as it goes, and hook onto the end. */
      while (r->next)
	r = r->next;
      r->next = command->value.Simple->redirects;
      command->value.Simple->redirects = element.redirect;
    }
  return (command);
}

/* Because we are Bourne compatible, we read the input for this
   << or <<- redirection now, from wherever input is coming from.
   We store the input read into a WORD_DESC.  Replace the text of
   the redirectee.word with the new input text.  If <<- is on,
   then remove leading TABS from each line. */
void
make_here_document (temp)
     REDIRECT *temp;
{
  int kill_leading, redir_len;
  char *redir_word, *document, *full_line;
  int document_index, document_size, delim_unquoted;

  if (temp->instruction != r_deblank_reading_until &&
      temp->instruction != r_reading_until)
    {
      internal_error ("make_here_document: bad instruction type %d", temp->instruction);
      return;
    }

  kill_leading = temp->instruction == r_deblank_reading_until;

  document = (char *)NULL;
  document_index = document_size = 0;

  /* Quote removal is the only expansion performed on the delimiter
     for here documents, making it an extremely special case. */
  redir_word = string_quote_removal (temp->redirectee.filename->word, 0);

  /* redirection_expand will return NULL if the expansion results in
     multiple words or no words.  Check for that here, and just abort
     this here document if it does. */
  if (redir_word)
    redir_len = strlen (redir_word);
  else
    {
      temp->here_doc_eof = xmalloc (1);
      temp->here_doc_eof[0] = '\0';
      goto document_done;
    }

  free (temp->redirectee.filename->word);
  temp->here_doc_eof = redir_word;

  /* Read lines from wherever lines are coming from.
     For each line read, if kill_leading, then kill the
     leading tab characters.
     If the line matches redir_word exactly, then we have
     manufactured the document.  Otherwise, add the line to the
     list of lines in the document. */

  /* If the here-document delimiter was quoted, the lines should
     be read verbatim from the input.  If it was not quoted, we
     need to perform backslash-quoted newline removal. */
  delim_unquoted = (temp->redirectee.filename->flags & W_QUOTED) == 0;
  while (full_line = read_secondary_line (delim_unquoted))
    {
      register char *line;
      int len;

      line = full_line;
      line_number++;

      if (kill_leading && *line)
        {
	  /* Hack:  To be compatible with some Bourne shells, we
	     check the word before stripping the whitespace.  This
	     is a hack, though. */
	  if (STREQN (line, redir_word, redir_len) && line[redir_len] == '\n')
	    goto document_done;

	  while (*line == '\t')
	    line++;
	}

      if (*line == 0)
        continue;

      if (STREQN (line, redir_word, redir_len) && line[redir_len] == '\n')
	goto document_done;

      len = strlen (line);
      if (len + document_index >= document_size)
	{
	  document_size = document_size ? 2 * (document_size + len) : 1000;
	  document = xrealloc (document, document_size);
	}

      /* len is guaranteed to be > 0 because of the check for line
	 being an empty string before the call to strlen. */
      FASTCOPY (line, document + document_index, len);
      document_index += len;
    }

document_done:
  if (document)
    document[document_index] = '\0';
  else
    {
      document = xmalloc (1);
      document[0] = '\0';
    }
  temp->redirectee.filename->word = document;
}

/* Generate a REDIRECT from SOURCE, DEST, and INSTRUCTION.
   INSTRUCTION is the instruction type, SOURCE is a file descriptor,
   and DEST is a file descriptor or a WORD_DESC *. */
REDIRECT *
make_redirection (source, instruction, dest_and_filename)
     int source;
     enum r_instruction instruction;
     REDIRECTEE dest_and_filename;
{
  REDIRECT *temp = (REDIRECT *)xmalloc (sizeof (REDIRECT));

  /* First do the common cases. */
  temp->redirector = source;
  temp->redirectee = dest_and_filename;
  temp->instruction = instruction;
  temp->flags = 0;
  temp->next = (REDIRECT *)NULL;

  switch (instruction)
    {

    case r_output_direction:	/* >foo */
    case r_output_force:	/* >| foo */
      temp->flags = O_TRUNC | O_WRONLY | O_CREAT;
      break;

    case r_input_direction:	/* <foo */
    case r_inputa_direction:	/* foo & makes this. */
      temp->flags = O_RDONLY;
      break;

    case r_appending_to:	/* >>foo */
      temp->flags = O_APPEND | O_WRONLY | O_CREAT;
      break;

    case r_deblank_reading_until: /* <<-foo */
    case r_reading_until:	/* << foo */
      break;

    case r_close_this:			/* <&- */
    case r_duplicating_input:		/* 1<&2 */
    case r_duplicating_output:		/* 1>&2 */
    case r_duplicating_input_word:	/* 1<&$foo */
    case r_duplicating_output_word:	/* 1>&$foo */
      break;

    case r_err_and_out:		/* command &>filename */
      temp->flags = O_TRUNC | O_WRONLY | O_CREAT;
      break;

    case r_input_output:
      temp->flags = O_RDWR | O_CREAT;
      break;

    default:
      programming_error ("make_redirection: redirection instruction `%d' out of range", instruction);
      abort ();
      break;
    }
  return (temp);
}

COMMAND *
make_function_def (name, command, lineno, lstart)
     WORD_DESC *name;
     COMMAND *command;
     int lineno, lstart;
{
  FUNCTION_DEF *temp;

  temp = (FUNCTION_DEF *)xmalloc (sizeof (FUNCTION_DEF));
  temp->command = command;
  temp->name = name;
  temp->line = lineno;
  temp->ignore = 0;
  command->line = lstart;
  return (make_command (cm_function_def, (SIMPLE_COM *)temp));
}

/* Reverse the word list and redirection list in the simple command
   has just been parsed.  It seems simpler to do this here the one
   time then by any other method that I can think of. */
COMMAND *
clean_simple_command (command)
     COMMAND *command;
{
  if (command->type != cm_simple)
    programming_error ("clean_simple_command: bad command type `%d'", command->type);
  else
    {
      command->value.Simple->words =
	REVERSE_LIST (command->value.Simple->words, WORD_LIST *);
      command->value.Simple->redirects =
	REVERSE_LIST (command->value.Simple->redirects, REDIRECT *);
    }

  return (command);
}

/* The Yacc grammar productions have a problem, in that they take a
   list followed by an ampersand (`&') and do a simple command connection,
   making the entire list effectively asynchronous, instead of just
   the last command.  This means that when the list is executed, all
   the commands have stdin set to /dev/null when job control is not
   active, instead of just the last.  This is wrong, and needs fixing
   up.  This function takes the `&' and applies it to the last command
   in the list.  This is done only for lists connected by `;'; it makes
   `;' bind `tighter' than `&'. */
COMMAND *
connect_async_list (command, command2, connector)
     COMMAND *command, *command2;
     int connector;
{
  COMMAND *t, *t1, *t2;

  t1 = command;
  t = command->value.Connection->second;

  if (!t || (command->flags & CMD_WANT_SUBSHELL) ||
      command->value.Connection->connector != ';')
    {
      t = command_connect (command, command2, connector);
      return t;
    }

  /* This is just defensive programming.  The Yacc precedence rules
     will generally hand this function a command where t points directly
     to the command we want (e.g. given a ; b ; c ; d &, t1 will point
     to the `a ; b ; c' list and t will be the `d').  We only want to do
     this if the list is not being executed as a unit in the background
     with `( ... )', so we have to check for CMD_WANT_SUBSHELL.  That's
     the only way to tell. */
  while (((t->flags & CMD_WANT_SUBSHELL) == 0) && t->type == cm_connection &&
	 t->value.Connection->connector == ';')
    {
      t1 = t;
      t = t->value.Connection->second;
    }
  /* Now we have t pointing to the last command in the list, and
     t1->value.Connection->second == t. */
  t2 = command_connect (t, command2, connector);
  t1->value.Connection->second = t2;
  return command;
}