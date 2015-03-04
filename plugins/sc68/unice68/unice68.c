/*
 *                 unice68 - ice depacker (native version)
 *                    Copyright (C) 2003 Benjamin Gerard
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define ICEVERSION  240
#include "unice68/unice68.h"

static int verbose = 0;

static void message(const char *format, ...)
{
  va_list list;

  if (!verbose) {
    return;
  }
  va_start(list, format);
  vfprintf(stderr,format,list);
  va_end(list);
}

static int usage(void)
{
  int ice_version = unice68_ice_version();
  printf("\n"
         "unice68: Depacker ICE! v%d.%02d (native).\n"
         "         (C) 2003 Benjamin Gerard <ben@sashipa.com>\n"
         "\n"
         "Usage: unice68 [OPTIONS] <source> <destination>\n"
	 "\n"
	 "Use '-' for as source/destination for respectively stdin/stdout.\n"
         "\n"
         "OPTIONS:\n"
         "\n"
         "    -v      Verbose.\n"
         "    -q      Quiet.\n"
         "    --help  This message and exit.\n"
         "\n",ice_version/100, ice_version%100);
  return 1;
}


int main(int na, char **a)
{
  int csize, dsize;
  void * buffer = 0, * depack = 0;
  int err = 255;
  FILE * in = 0;
  FILE * out = 0;
  char header[12];
  const char * fin=0, *fout=0;
  int i;

  for (i=1; i<na; ++i) {
    if (a[i][0] == '-') {
      if (!strcmp(a[i],"--help")) {
	return usage();
      } else if (!strcmp(a[i],"-v")) {
	verbose = 1;
      } else if (!strcmp(a[i],"-q")) {
	verbose = 0;
      } else {
	fprintf(stderr, "Invalid parameter [%s]\n", a[i]);
	return 255;
      }
    } else if (!fin) {
      fin = a[i];
    } else if (!fout) {
      fout = a[i];
    } else {
      fprintf(stderr, "Too many parameters [%s]\n", a[i]);
      return 255;
    }
  }

  if (!fin) {
    fin = "stdin";
    in = stdin;
  } else {
    in = fopen(fin,"rb");
  }
  if (!in) {
    perror(fin);
    goto error;
  }

  err = (int)fread (header, 1, sizeof(header), in);
  if (err == -1) {
    perror(fin);
    goto error;
  }

  if (err < sizeof(header)) {
    fprintf(stderr, "not enought byte to be a valid ice file.\n");
    goto error;
  }
  csize = 0;
  dsize = unice68_get_depacked_size(header, &csize);
  if (dsize == -1) {
    fprintf(stderr, "not a valid ice file : missing magic.\n");
    goto error;
  }
  if (csize < 0 || dsize < 0) {
    fprintf(stderr, "weird sizes: %d/%d.\n", csize,dsize);
    goto error;
  }

  message("ICE! compressed:%d uncompressed:%d ratio:%d%%\n",
	  csize,dsize,(csize+50)*100/dsize);

  buffer = malloc(csize + sizeof(header));
  if (!buffer) {
    perror(fin);
    goto error;
  }

  memcpy(buffer, header, sizeof(header));
  err = (int) fread((char *)buffer + sizeof(header) , 1, csize, in);

  if (err != csize) {
    if (err == -1) {
      perror(fin);
      goto error;
    } else {
      fprintf(stderr, "read error get %d bytes on %d requested\n",
	      err, csize);
      err = 2;
      goto error;
    }
  }

  depack = malloc(dsize);
  if (!depack) {
    perror(fin);
    err = 255;
    goto error;
  }

  err = unice68_depacker(depack, buffer);

  if (!err) {
    if (!fout) {
      fout = "stdout";
      out = stdout;
    } else {
      out = fopen(fout,"wb");
    }
    if (!out) {
      perror(fout);
      goto error;
    }
    err = (int) fwrite(depack, 1, dsize, out);
    if (err != dsize) {
      err = -1;
      perror(fout);
      goto error;
    }
    err = 0;
  }

 error:
  if (in && in != stdin) {
    fclose(in);
  }
  if (out && out != stdout) {
    fclose(out);
  }
  if (buffer) {
    free(buffer);
  }
  if (depack) {
    free(depack);
  }
  return err;
}
