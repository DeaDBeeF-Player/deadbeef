/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS BEFORE DISTRIBUTING.                     *
 *                                                                  *
 * THE Ogg123 SOURCE CODE IS (C) COPYRIGHT 2000-2001                *
 * by Stan Seibert <volsung@xiph.org> AND OTHER CONTRIBUTORS        *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 last mod: $Id: cfgfile_options.c 17015 2010-03-24 08:21:29Z xiphmont $

 ********************************************************************/


/* if strcasecmp is giving you problems, switch to strcmp or the appropriate
 * function for your platform / compiler.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h> /* for INT/LONG_MIN/MAX */
#include <errno.h>

#include "cfgfile_options.h"
#include "status.h"
#include "i18n.h"

/* ------------------- Private Functions ---------------------- */

int print_space (FILE *f, int s, int c)
{
  int tmp = 0;
  do {
    fputc (c, f);
    tmp++;
  } while (--s > 0);
  return tmp;
}


int parse_error (parse_code_t pcode, int lineno, const char *filename,
		 char *line)
{
  if (pcode == parse_syserr) {
    if (errno != EEXIST && errno != ENOENT)
      perror (_("System error"));
    return -1;
  } else {
    status_error (_("=== Parse error: %s on line %d of %s (%s)\n"), 
		  parse_error_string(pcode), 
		  lineno, filename, line);
    return 0;
  }
}


/* ------------------- Public Interface ----------------------- */

void file_options_init (file_option_t opts[])
{

  while (opts && opts->name) {

    opts->found = 0;

    if (opts->dfl) {
      switch (opts->type) {
      case opt_type_none:
	/* do nothing */
	break;
	
      case opt_type_char:
	*(char *) opts->ptr = *(char*) opts->dfl;
	break;
	
      case opt_type_string:
	*(char **) opts->ptr = *(char **) opts->dfl;
	break;

      case opt_type_bool:
      case opt_type_int:
	*(int *) opts->ptr = *(int *) opts->dfl;
	break;
	
      case opt_type_float:
	*(float *) opts->ptr = *(float *) opts->dfl;
	break;
	
      case opt_type_double:
	*(double *) opts->ptr = *(double *) opts->dfl;
	break;
      }
    }

    opts++;
  }
}


/* DescribeOptions - describe available options to outfile */
void file_options_describe (file_option_t opts[], FILE *f)
{
  /* name | description | type | default */
  int colWidths[] = {0, 0, 7, 7};
  int totalWidth = 0;
  file_option_t *opt = opts;

  while (opt->name) {
    int len = strlen (opt->name) + 1;
    if (len  > colWidths[0])
      colWidths[0] = len;
    opt++;
  }

  opt = opts;
  while (opt->name) {
    int len = strlen (opt->desc) + 1;
    if (len > colWidths[1])
      colWidths[1] = len;
    opt++;
  }

  /* Column headers */
  /* Name */
  totalWidth += fprintf (f, "%-*s", colWidths[0], _("Name"));

  /* Description */
  totalWidth += fprintf (f, "%-*s", colWidths[1], _("Description"));

  /* Type */
  totalWidth += fprintf (f, "%-*s", colWidths[2], _("Type"));

  /* Default */
  totalWidth += fprintf (f, "%-*s", colWidths[3], _("Default"));

  fputc ('\n', f);

  /* Divider */
  print_space (f, totalWidth, '-');

  fputc ('\n', f);

  opt = opts;
  while (opt->name)
    {
      /* name */
      int w = colWidths[0];
      w -= fprintf (f, "%s", opt->name);
      print_space (f, w, ' ');

      /* description */
      w = colWidths[1];
      w -= fprintf (f, "%s", opt->desc);
      print_space (f, w, ' ');

      /* type */
      w = colWidths[2];
      switch (opt->type) {
      case opt_type_none:
	w -= fprintf (f, _("none"));
	break;
      case opt_type_bool:
	w -= fprintf (f, _("bool"));
	break;
      case opt_type_char:
	w -= fprintf (f, _("char"));
	break;
      case opt_type_string:
	w -= fprintf (f, _("string"));
	break;
      case opt_type_int:
	w -= fprintf (f, _("int"));
	break;
      case opt_type_float:
	w -= fprintf (f, _("float"));
	break;
      case opt_type_double:
	w -= fprintf (f, _("double"));
	break;
      default:
	w -= fprintf (f, _("other"));
      }
      print_space (f, w, ' ');

      /* default */
      if (opt->dfl == NULL)
	fputs (_("(NULL)"), f);
      else {
	switch (opt->type) {
	case opt_type_none:
	  fputs (_("(none)"), f);
	  break;
	case opt_type_char:
	  fputc (*(char *) opt->dfl, f);
	  break;
	case opt_type_string:
	  fputs (*(char **) opt->dfl, f);
	  break;
	case opt_type_bool:
	case opt_type_int:
	  fprintf (f, "%d", *(int *) opt->dfl);
	  break;
	case opt_type_float:
	  fprintf (f, "%f", (double) (*(float *) opt->dfl));
	  break;
	case opt_type_double:
	  fprintf (f, "%f", *(double *) opt->dfl);
	  break;
	}
      }
      fputc ('\n', f);
      opt++;
    }
}


parse_code_t parse_line (file_option_t opts[], char *line)
{
  char *equals, *value = "";
  file_option_t *opt;
  int len;

  /* skip leading whitespace */
  while (line[0] == ' ')
    line++;

  /* remove comments */
  equals = strchr (line, '#');
  if (equals)
    *equals = '\0';

  /* return if only whitespace on line */
  if (!line[0] || line[0] == '#')
    return parse_ok;

  /* check for an '=' and set to \0 */
  equals = strchr (line, '=');
  if (equals) {
    value = equals + 1;
    *equals = '\0';
  }

  /* cut trailing whitespace from key (line = key now) */
  while ((equals = strrchr(line, ' ')))
    *equals = '\0';

  /* remove this if you want a zero-length key */
  if (strlen(line) == 0)
    return parse_nokey;

  if (value) {
    /* cut leading whitespace from value */
    while (*value == ' ')
      value++;

    /* cut trailing whitespace from value */
    len = strlen (value);
    while (len > 0 && value[len-1] == ' ') {
      len--;
      value[len] = '\0';
    }
  }

  /* now key is in line and value is in value. Search for a matching option. */
  opt = opts;
  while (opt->name) {
    if (!strcasecmp (opt->name, line)) {
      long tmpl;
      char *endptr;

      /* found the key. now set the value. */
      switch (opt->type) {
      case opt_type_none:
	if (value != NULL || strlen(value) > 0)
	  return parse_badvalue;
	opt->found++;
	break;

      case opt_type_bool:
	if (!value || *value == '\0')
	  return parse_badvalue;
	
	/* Maybe this is a numeric bool */
	tmpl = strtol (value, &endptr, 0);

	if ( !strncasecmp(value, "y", 1)
	     || !strcasecmp(value, "true")
	     || (*endptr == '\0' && tmpl) )
	  *(int *) opt->ptr = 1;
	else if ( !strncasecmp(value, "n", 1)
		  || !strcasecmp(value, "false")
		  || (*endptr == '\0' && !tmpl) )
	  *(int *) opt->ptr = 0;
	else
	  return parse_badvalue;
	break;

      case opt_type_char:
	if (strlen(value) != 1)
	  return parse_badvalue;
	opt->found++;
	*(char *) opt->ptr = value[0];
	break;

      case opt_type_string:
	opt->found++;
	if (*(char **)opt->ptr) free(*(char **)opt->ptr);
	*(char **) opt->ptr = strdup (value);
	break;

      case opt_type_int:
	if (!value || *value == '\0')
	  return parse_badvalue;
	errno = 0;
	tmpl = strtol (value, &endptr, 0);
	if (((tmpl == LONG_MIN || tmpl == LONG_MAX) && errno == ERANGE)
	    || (*endptr != '\0'))
	  return parse_badvalue;
	if ((tmpl > INT_MAX) || (tmpl < INT_MIN))
	  return parse_badvalue;
	opt->found++;
	*(int *) opt->ptr = tmpl;
	break;
	
      case opt_type_float:
	if (!value || *value == '\0')
	  return parse_badvalue;
	opt->found++;
	*(float *) opt->ptr = atof (value);
	break;

      case opt_type_double:
	if (!value || *value == '\0')
	  return parse_badvalue;
	opt->found++;
	*(double *) opt->ptr = atof (value);
	break;

      default:
	return parse_badtype;
      }
      return parse_ok;
    }
    opt++;
  }
  return parse_keynotfound;
}


parse_code_t parse_config_file (file_option_t opts[], const char *filename)
{
  unsigned int len=80;
  char *line = malloc(len);
  int readoffset, thischar, lineno;
  FILE *file;
  parse_code_t pcode;
  char empty[] = "";

  if (!line) {
      parse_error(parse_syserr, 0, empty, empty);
      return parse_syserr;
  }

  file = fopen (filename, "r");
  if (!file) {
      parse_error (parse_syserr, 0, empty, empty);
      free (line);
      return parse_syserr;
  }

  lineno = 0;
  while (!feof (file)) {

    lineno++;
    readoffset = 0;
    memset (line, 0, len);

    while ((thischar = fgetc(file)) != EOF) {

      if (readoffset + 1 > len) {
	len *= 2;
	line = realloc (line, len);
	if (!line)
	  {
	    parse_error(parse_syserr, 0, empty, empty);
	    fclose (file);
	    return parse_syserr;
	  }
      }

      if (thischar == '\n') {
	line[readoffset] = '\0';
	break;
      }
      else
	line[readoffset] = (unsigned char) thischar;
      readoffset++;

    }

    pcode = parse_line (opts, line);

    if (pcode != parse_ok)
      if (!parse_error(pcode, lineno, filename, line)) {
	free (line);
	return pcode;
      }

  }

  free (line);
  return parse_ok;
}

/* ParseErr - returns a string corresponding to parse code pcode */
const char *parse_error_string (parse_code_t pcode)
{
  switch (pcode) {
  case parse_ok:
    return _("Success");
  case parse_syserr:
    return strerror(errno);
  case parse_keynotfound:
    return _("Key not found");
  case parse_nokey:
    return _("No key");
  case parse_badvalue:
    return _("Bad value");
  case parse_badtype:
    return _("Bad type in options list");
  default:
    return _("Unknown error");
  }
}


void parse_std_configs (file_option_t opts[])
{
  char filename[FILENAME_MAX];
  char *homedir = getenv("HOME");

  parse_config_file(opts, SYSCONFDIR "/ogg123rc");
  if (homedir && strlen(homedir) < FILENAME_MAX - 10) {
    /* Try ~/.ogg123 */
    strncpy(filename, homedir, FILENAME_MAX);
    strcat(filename, "/.ogg123rc");
    parse_config_file(opts, filename);
  }
}
