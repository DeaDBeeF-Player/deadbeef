/* This program is licensed under the GNU General Public License, 
 * version 2, a copy of which is included with this program.
 *
 * (c) 2000-2002 Michael Smith <msmith@xiph.org>
 * (c) 2001 Ralph Giles <giles@xiph.org>
 *
 * Front end to show how to use vcedit;
 * Of limited usability on its own, but could be useful.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#if HAVE_STAT && HAVE_CHMOD
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "getopt.h"
#include "utf8.h"
#include "i18n.h"

#include "vcedit.h"


/* getopt format struct */
struct option long_options[] = {
	{"list",0,0,'l'},
	{"append",0,0,'a'},
	{"tag",required_argument,0,'t'},
	{"write",0,0,'w'},
	{"help",0,0,'h'},
	{"quiet",0,0,'q'}, /* unused */
	{"version", 0, 0, 'V'},
	{"commentfile",1,0,'c'},
	{"raw", 0,0,'R'},
	{"escapes",0,0,'e'},
	{NULL,0,0,0}
};

/* local parameter storage from parsed options */
typedef struct {
	/* mode and flags */
	int	mode;
	int	raw;
	int	escapes;

	/* file names and handles */
	char	*infilename, *outfilename;
	char	*commentfilename;
	FILE	*in, *out, *com;
	int	tempoutfile;

	/* comments */
	int	commentcount;
	char	**comments;
} param_t;

#define MODE_NONE  0
#define MODE_LIST  1
#define MODE_WRITE 2
#define MODE_APPEND 3

/* prototypes */
void usage(void);
void print_comments(FILE *out, vorbis_comment *vc, int raw, int escapes);
int  add_comment(char *line, vorbis_comment *vc, int raw, int escapes);

char *escape(const char *from, int fromsize);
char *unescape(const char *from, int *tosize);

param_t	*new_param(void);
void free_param(param_t *param);
void parse_options(int argc, char *argv[], param_t *param);
void open_files(param_t *p);
void close_files(param_t *p, int output_written);

char *
read_line (FILE *input)
{
        /* Construct a list of buffers. Each buffer will hold 1024 bytes. If
         * more is required, it is easier to extend the list than to extend
         * a massive buffer. When all the bytes up to a newline have been
         * retrieved, join the buffers together
        **/
        int buffer_count = 0, max_buffer_count = 10, buffer_size = 1024;
        int ii;
        char **buffers = 0, *buffer;

        /* Start with room for 10 buffers */
        buffers = malloc (sizeof (char *) * max_buffer_count);

        while (1)
        {
                char *retval;

                /* Increase the max buffer count in increments of 10 */
                if (buffer_count == max_buffer_count)
                {
                        max_buffer_count = buffer_count + 10;
                        buffers = realloc (buffers, sizeof (char *) * max_buffer_count);
                }

                buffer = malloc (sizeof (char) * (buffer_size + 1));
                retval = fgets (buffer, (buffer_size + 1), input);

                if (retval)
                {
                        buffers[buffer_count] = buffer;
                        buffer_count++;

                        if (retval[strlen (retval) - 1] == '\n')
                        {
                                /* End of the line */
                                break;
                        }
                }

                else
                {
                        /* End of the file */
                        free (buffer);
                        break;
                }
        }

        if (buffer_count == 0)
        {
                /* No more data to read */
                free (buffers);
                return 0;
        }

        /* Create one giant buffer to contain all the retrieved text */
        buffer = malloc (sizeof (char) * (buffer_count * (buffer_size + 1)));

        /* Copy buffer data and free memory */
        for (ii = 0; ii < buffer_count; ii++)
        {
                strncpy (buffer + (ii * buffer_size), buffers[ii], buffer_size);
                free (buffers[ii]);
        }

        free (buffers);
        buffer[buffer_count * (buffer_size + 1) - 1] = 0;
        return buffer;
}

/**********
   main.c

   This is the main function where options are read and written
   you should be able to just read this function and see how
   to call the vcedit routines. Details of how to pack/unpack the
   vorbis_comment structure itself are in the following two routines.
   The rest of the file is ui dressing so make the program minimally
   useful as a command line utility and can generally be ignored.

***********/

int main(int argc, char **argv)
{
	vcedit_state *state;
	vorbis_comment *vc;
	param_t	*param;
	int i;

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	/* initialize the cmdline interface */
	param = new_param();
	parse_options(argc, argv, param);

	/* take care of opening the requested files */
	/* relevent file pointers are returned in the param struct */
	open_files(param);

	/* which mode are we in? */

	if (param->mode == MODE_LIST) {
		
		state = vcedit_new_state();

		if(vcedit_open(state, param->in) < 0)
		{
			fprintf(stderr, _("Failed to open file as Vorbis: %s\n"), 
					vcedit_error(state));
            close_files(param, 0);
            free_param(param);
            vcedit_clear(state);
			return 1;
		}

		/* extract and display the comments */
		vc = vcedit_comments(state);
		print_comments(param->com, vc, param->raw, param->escapes);

		/* done */
		vcedit_clear(state);

		close_files(param, 0);
        free_param(param);
		return 0;		
	}

	if (param->mode == MODE_WRITE || param->mode == MODE_APPEND) {

		state = vcedit_new_state();

		if(vcedit_open(state, param->in) < 0)
		{
			fprintf(stderr, _("Failed to open file as Vorbis: %s\n"), 
					vcedit_error(state));
			close_files(param, 0);
			free_param(param);
			vcedit_clear(state);
			return 1;
		}

		/* grab and clear the exisiting comments */
		vc = vcedit_comments(state);
		if(param->mode != MODE_APPEND) 
		{
			vorbis_comment_clear(vc);
			vorbis_comment_init(vc);
		}

		for(i=0; i < param->commentcount; i++)
		{
			if (add_comment(param->comments[i], vc,
					param->raw, param->escapes) < 0)
				fprintf(stderr, _("Bad comment: \"%s\"\n"), param->comments[i]);
		}

		/* build the replacement structure */
		if(param->commentcount==0)
		{
			char *comment;

			while ((comment = read_line (param->com)))
                        {
                                if (add_comment(comment, vc, param->raw, param->escapes) < 0)
                                {
                                        fprintf (stderr, _("bad comment: \"%s\"\n"),
                                                 comment);
                                }
                                free (comment);
                        }
		}

		/* write out the modified stream */
		if(vcedit_write(state, param->out) < 0)
		{
			fprintf(stderr, _("Failed to write comments to output file: %s\n"), 
					vcedit_error(state));
			close_files(param, 0);
			free_param(param);
			vcedit_clear(state);
			return 1;
		}

		/* done */
		vcedit_clear(state);
		
		close_files(param, 1);
		free_param(param);
		return 0;
	}

	/* should never reach this point */
	fprintf(stderr, _("no action specified\n"));
    free_param(param);
	return 1;
}

/**********

   Print out the comments from the vorbis structure

   this version just dumps the raw strings
   a more elegant version would use vorbis_comment_query()

***********/

void print_comments(FILE *out, vorbis_comment *vc, int raw, int escapes)
{
	int i;
	char *escaped_value, *decoded_value;

	for (i = 0; i < vc->comments; i++) {
		if (escapes) {
			escaped_value = escape(vc->user_comments[i], vc->comment_lengths[i]);
		} else {
			escaped_value = vc->user_comments[i];
		}

		if (!raw && utf8_decode(escaped_value, &decoded_value) >= 0) {
			fprintf(out, "%s\n", decoded_value);
			free(decoded_value);
		} else {
			fprintf(out, "%s\n", escaped_value);
		}

		if (escapes) {
			free(escaped_value);
		}
	}
}

/**********

   Take a line of the form "TAG=value string", parse it, convert the
   value to UTF-8, and add it to the
   Error checking is performed (return 0 if OK, negative on error).

   Note that this assumes a null-terminated string, which may cause
   problems with > 8-bit character sets!

***********/

int  add_comment(char *line, vorbis_comment *vc, int raw, int escapes)
{
	char *mark, *value, *utf8_value, *unescaped_value;
	int unescaped_len;

	/* strip any terminal newline */
	{
		int len = strlen(line);
		if (line[len-1] == '\n') line[len-1] = '\0';
	}

	/* validation: basically, we assume it's a tag
	 * if it has an '=' after one or more valid characters,
	 * as the comment spec requires. For the moment, we
	 * also restrict ourselves to 0-terminated values */

	mark = strchr(line, '=');
	if (mark == NULL) return -1;

	value = line;
	while (value < mark) {
		if(*value < 0x20 || *value > 0x7d || *value == 0x3d) return -1;
		value++;
	}

	/* split the line by turning the '=' in to a null */
	*mark = '\0';	
	value++;

	if (raw) {
		if (!utf8_validate(value)) {
			fprintf(stderr, _("'%s' is not valid UTF-8, cannot add\n"), line);
			return -1;
		}
		utf8_value = value;
	} else {
		/* convert the value from the native charset to UTF-8 */
		if (utf8_encode(value, &utf8_value) < 0) {
			fprintf(stderr,
					_("Couldn't convert comment to UTF-8, cannot add\n"));
			return -1;
		}
	}

	if (escapes) {
		unescaped_value = unescape(utf8_value, &unescaped_len);
		/*
		  NOTE: unescaped_len remains unused; to write comments with embeded
		  \0's one would need to access the vc struct directly -- see
		  vorbis_comment_add() in vorbis/lib/info.c for details, but use mem*
		  instead of str*...
		*/
		if(unescaped_value == NULL) {
			fprintf(stderr,
					_("Couldn't un-escape comment, cannot add\n"));
			if (!raw)
				free(utf8_value);
			return -1;
		}
	} else {
		unescaped_value = utf8_value;
	}

	/* append the comment and return */
	vorbis_comment_add_tag(vc, line, unescaped_value);
	if (escapes)
		free(unescaped_value);
	if (!raw)
		free(utf8_value);
	return 0;
}


/*** Escaping routines. ***/

/**********

   Convert raw comment content to a safely escaped single-line 0-terminated
   string.  The raw comment can contain null bytes and thus requires an
   explicit size argument.  The size argument doesn't include a trailing '\0'
   (the vorbis bitstream doesn't use one).

   Returns the address of a newly allocated string - caller is responsible to
   free it.

***********/

char *escape(const char *from, int fromsize)
{
	/* worst-case allocation, will be trimmed when done */
	char *to = malloc(fromsize * 2 + 1);

	char *s;
	for (s = to; fromsize > 0; fromsize--, from++) {
		switch (*from) {
		case '\n':
			*s++ = '\\';
			*s++ = 'n';
			break;
		case '\r':
			*s++ = '\\';
			*s++ = 'r';
			break;
		case '\0':
			*s++ = '\\';
			*s++ = '0';
			break;
		case '\\':
			*s++ = '\\';
			*s++ = '\\';
			break;
		default:
			/* normal character */
			*s++ = *from;
			break;
		}
	}
	
	*s++ = '\0';
	to = realloc(to, s - to);	/* free unused space */
	return to;
}

/**********

   Convert a safely escaped 0-terminated string to raw comment content.  The
   result can contain null bytes, so the the result's length is written into
   *tosize.  This size doesn't include a trailing '\0' (the vorbis bitstream
   doesn't use one) but we do append it for convenience since
   vorbis_comment_add[_tag]() has a null-terminated interface.

   Returns the address of a newly allocated string - caller is responsible to
   free it.  Returns NULL in case of error (if the input is mal-formed).

***********/

char *unescape(const char *from, int *tosize)
{
	/* worst-case allocation, will be trimmed when done */
	char *to = malloc(strlen(from) + 1);

	char *s;
	for (s = to; *from != '\0'; ) {
		if (*from == '\\') {
			from++;
			switch (*from++) {
			case 'n':
				*s++ = '\n';
				break;
			case 'r':
				*s++ = '\r';
				break;
			case '0':
				*s++ = '\0';
				break;
			case '\\':
				*s++ = '\\';
				break;
			case '\0':
				/* A backslash as the last character of the string is an error. */
				/* FALL-THROUGH */
			default:
				/* We consider any unrecognized escape as an error.  This is
				   good in general and reserves them for future expansion. */
				free(to);
				return NULL;
			}
		} else {
			/* normal character */
			*s++ = *from++;
		}
	}

	*tosize = s - to;			/* excluding '\0' */

	*s++ = '\0';
	to = realloc(to, s - to);	/* free unused space */
	return to;
}


/*** ui-specific routines ***/

/**********

   Print out to usage summary for the cmdline interface (ui)

***********/

/* XXX: -q is unused
  printf (_("  -q, --quiet             Don't display comments while editing\n"));
*/

void usage(void)
{

  printf (_("vorbiscomment from %s %s\n"
            " by the Xiph.Org Foundation (http://www.xiph.org/)\n\n"), PACKAGE, VERSION);

  printf (_("List or edit comments in Ogg Vorbis files.\n"));
  printf ("\n");

  printf (_("Usage: \n"
	    "  vorbiscomment [-Vh]\n"
	    "  vorbiscomment [-lRe] inputfile\n"
	    "  vorbiscomment <-a|-w> [-Re] [-c file] [-t tag] inputfile [outputfile]\n"));
  printf ("\n");

  printf (_("Listing options\n"));
  printf (_("  -l, --list              List the comments (default if no options are given)\n"));
  printf ("\n");

  printf (_("Editing options\n"));
  printf (_("  -a, --append            Append comments\n"));
  printf (_("  -t \"name=value\", --tag \"name=value\"\n"
            "                          Specify a comment tag on the commandline\n"));
  printf (_("  -w, --write             Write comments, replacing the existing ones\n"));
  printf ("\n");

  printf (_("Miscellaneous options\n"));
  printf (_("  -c file, --commentfile file\n"
            "                          When listing, write comments to the specified file.\n"
            "                          When editing, read comments from the specified file.\n"));
  printf (_("  -R, --raw               Read and write comments in UTF-8\n"));
  printf (_("  -e, --escapes           Use \\n-style escapes to allow multiline comments.\n"));
  printf ("\n");

  printf (_("  -h, --help              Display this help\n"));
  printf (_("  -V, --version           Output version information and exit\n"));
  printf ("\n");

  printf (_("If no output file is specified, vorbiscomment will modify the input file. This\n"
            "is handled via temporary file, such that the input file is not modified if any\n"
            "errors are encountered during processing.\n"));
  printf ("\n");

  printf (_("vorbiscomment handles comments in the format \"name=value\", one per line. By\n"
            "default, comments are written to stdout when listing, and read from stdin when\n"
            "editing. Alternatively, a file can be specified with the -c option, or tags\n"
            "can be given on the commandline with -t \"name=value\". Use of either -c or -t\n"
            "disables reading from stdin.\n"));
  printf ("\n");

  printf (_("Examples:\n"
            "  vorbiscomment -a in.ogg -c comments.txt\n"
            "  vorbiscomment -a in.ogg -t \"ARTIST=Some Guy\" -t \"TITLE=A Title\"\n"));
  printf ("\n");

  printf (_("NOTE: Raw mode (--raw, -R) will read and write comments in UTF-8 rather than\n"
	    "converting to the user's character set, which is useful in scripts. However,\n"
	    "this is not sufficient for general round-tripping of comments in all cases,\n"
	    "since comments can contain newlines. To handle that, use escaping (-e,\n"
	    "--escape).\n"));
}

void free_param(param_t *param) {
    free(param->infilename);
    free(param->outfilename);
    free(param);
}

/**********

   allocate and initialize a the parameter struct

***********/

param_t *new_param(void)
{
	param_t *param = (param_t *)malloc(sizeof(param_t));

	/* mode and flags */
	param->mode = MODE_LIST;
	param->raw = 0;
	param->escapes = 0;

	/* filenames */
	param->infilename  = NULL;
	param->outfilename = NULL;
	param->commentfilename = "-";	/* default */

	/* file pointers */
	param->in = param->out = NULL;
	param->com = NULL;
	param->tempoutfile=0;

	/* comments */
	param->commentcount=0;
	param->comments=NULL;

	return param;
}

/**********
   parse_options()

   This function takes care of parsing the command line options
   with getopt() and fills out the param struct with the mode,
   flags, and filenames.

***********/

void parse_options(int argc, char *argv[], param_t *param)
{
	int ret;
	int option_index = 1;

	setlocale(LC_ALL, "");

	while ((ret = getopt_long(argc, argv, "alwhqVc:t:Re",
			long_options, &option_index)) != -1) {
		switch (ret) {
			case 0:
				fprintf(stderr, _("Internal error parsing command options\n"));
				exit(1);
				break;
			case 'l':
				param->mode = MODE_LIST;
				break;
			case 'R':
				param->raw = 1;
				break;
			case 'e':
				param->escapes = 1;
				break;
			case 'w':
				param->mode = MODE_WRITE;
				break;
			case 'a':
				param->mode = MODE_APPEND;
				break;
			case 'V':
				fprintf(stderr, _("vorbiscomment from vorbis-tools " VERSION "\n"));
				exit(0);
				break;
			case 'h':
				usage();
				exit(0);
				break;
			case 'q':
				/* set quiet flag: unused */
				break;
			case 'c':
				param->commentfilename = strdup(optarg);
				break;
			case 't':
				param->comments = realloc(param->comments, 
						(param->commentcount+1)*sizeof(char *));
				param->comments[param->commentcount++] = strdup(optarg);
				break;
			default:
				usage();
				exit(1);
		}
	}

	/* remaining bits must be the filenames */
	if((param->mode == MODE_LIST && (argc-optind) != 1) ||
	   ((param->mode == MODE_WRITE || param->mode == MODE_APPEND) &&
	   ((argc-optind) < 1 || (argc-optind) > 2))) {
			usage();
			exit(1);
	}

	param->infilename = strdup(argv[optind]);
	if (param->mode == MODE_WRITE || param->mode == MODE_APPEND)
	{
		if(argc-optind == 1)
		{
			param->tempoutfile = 1;
			param->outfilename = malloc(strlen(param->infilename)+8);
			strcpy(param->outfilename, param->infilename);
			strcat(param->outfilename, ".vctemp");
		}
		else
			param->outfilename = strdup(argv[optind+1]);
	}
}

/**********
   open_files()

   This function takes care of opening the appropriate files
   based on the mode and filenames in the param structure.
   A filename of '-' is interpreted as stdin/out.

   The idea is just to hide the tedious checking so main()
   is easier to follow as an example.

***********/

void open_files(param_t *p)
{
	/* for all modes, open the input file */

	if (strncmp(p->infilename,"-",2) == 0) {
		p->in = stdin;
	} else {
		p->in = fopen(p->infilename, "rb");
	}
	if (p->in == NULL) {
		fprintf(stderr,
			_("Error opening input file '%s'.\n"),
			p->infilename);
		exit(1);
	}

	if (p->mode == MODE_WRITE || p->mode == MODE_APPEND) { 

		/* open output for write mode */
        if(!strcmp(p->infilename, p->outfilename)) {
            fprintf(stderr, _("Input filename may not be the same as output filename\n"));
            exit(1);
        }

		if (strncmp(p->outfilename,"-",2) == 0) {
			p->out = stdout;
		} else {
			p->out = fopen(p->outfilename, "wb");
		}
		if(p->out == NULL) {
			fprintf(stderr,
				_("Error opening output file '%s'.\n"),
				p->outfilename);
			exit(1);
		}

		/* commentfile is input */
		
		if ((p->commentfilename == NULL) ||
				(strncmp(p->commentfilename,"-",2) == 0)) {
			p->com = stdin;
		} else {
			p->com = fopen(p->commentfilename, "r");
		}
		if (p->com == NULL) {
			fprintf(stderr,
				_("Error opening comment file '%s'.\n"),
				p->commentfilename);
			exit(1);
		}

	} else {

		/* in list mode, commentfile is output */

		if ((p->commentfilename == NULL) ||
				(strncmp(p->commentfilename,"-",2) == 0)) {
			p->com = stdout;
		} else {
			p->com = fopen(p->commentfilename, "w");
		}
		if (p->com == NULL) {
			fprintf(stderr,
				_("Error opening comment file '%s'\n"),
				p->commentfilename);
			exit(1);
		}
	}

	/* all done */
}

/**********
   close_files()

   Do some quick clean-up.

***********/

void close_files(param_t *p, int output_written)
{
  if (p->in != NULL && p->in != stdin) fclose(p->in);
  if (p->out != NULL && p->out != stdout) fclose(p->out);
  if (p->com != NULL && p->com != stdout && p->com != stdin) fclose(p->com);

  if(p->tempoutfile) {
#if HAVE_STAT && HAVE_CHMOD
    struct stat st;
    stat (p->infilename, &st);
#endif

    if(output_written) {
      /* Some platforms fail to rename a file if the new name already 
       * exists, so we need to remove, then rename. How stupid.
       */
      if(rename(p->outfilename, p->infilename)) {
        if(remove(p->infilename))
          fprintf(stderr, _("Error removing old file %s\n"), p->infilename);
        else if(rename(p->outfilename, p->infilename)) 
          fprintf(stderr, _("Error renaming %s to %s\n"), p->outfilename, 
                  p->infilename);
      } else {
#if HAVE_STAT && HAVE_CHMOD
        chmod (p->infilename, st.st_mode);
#endif
      }
    }
    else {
      if(remove(p->outfilename)) {
        fprintf(stderr, _("Error removing erroneous temporary file %s\n"), 
                    p->outfilename);
      }
    }
  }
}
