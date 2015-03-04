/*
 *                         sc68 - FILE stream
 *         Copyright (C) 2001-2003 Benjamin Gerard <ben@sashipa.com>
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

#include <config68.h>

/* define this if you don't want FILE support. */
#ifndef ISTREAM_NO_FILE

#include <stdio.h>
#include <string.h>

#include "file68/istream68_file.h"
#include "file68/istream68_def.h"
#include "file68/alloc68.h"

/** istream file structure. */
typedef struct {
  istream_t istream; /**< istream function. */
  FILE *f;           /**< FILE handle.      */

  /** Open modes. */
  int mode;

  /* MUST BE at the end of the structure because supplemental bytes will
   * be allocated to store filename.
   */
  char name[1];       /**< filename. */

} istream_file_t;

static const char * isf_name(istream_t * istream)
{
  istream_file_t * isf = (istream_file_t *)istream;

  return (!isf || !isf->name[0])
    ? 0
    : isf->name;
}

static int isf_open(istream_t * istream)
{
  int imode;
  char mode[8];
  istream_file_t * isf = (istream_file_t *)istream;

  if (!isf || !isf->name[0]) {
    return -1;
  }

/*   SC68os_pdebug("istream_file::open(%s,%c%c)\n",isf->name, */
/* 		isf->mode.read?'R':'r', */
/* 		isf->mode.write?'W':'w'); */

  imode = 0;
  if (ISTREAM_IS_OPEN_READ(isf->mode)) {
    mode[imode++] = 'r';
  }
  if (ISTREAM_IS_OPEN_WRITE(isf->mode)) {
    mode[imode] = !imode ? 'w' : '+';
    ++imode;
  }
  if (!imode) {
    /* No open mode.. */
/*     SC68os_pdebug("istream_file::open(%s) : no open mode\n",isf->name); */
    return -1;
  }
  mode[imode++] = 'b';
  mode[imode] = 0;

/*   SC68os_pdebug("istream_file::open(%s,%s)\n",isf->name,mode); */
  isf->f = fopen(isf->name, mode);
  return isf->f ? 0 : -1; 
}

static int isf_close(istream_t * istream)
{
  istream_file_t * isf = (istream_file_t *)istream;
  int err;

  if (!isf) {
    return -1;
  }
  err = isf->f ? fclose(isf->f) : -1;
  isf->f = 0;
  return err;
}

static int isf_read(istream_t * istream, void * data, int n)
{
  istream_file_t * isf = (istream_file_t *)istream;

  return (!isf || !isf->f)
    ? -1
    : fread(data, 1, n, isf->f);
}

static int isf_write(istream_t * istream, const void * data, int n)
{
  istream_file_t * isf = (istream_file_t *)istream;

  return (!isf || !isf->f)
    ? -1
    : fwrite(data, 1, n, isf->f);
}


/* We could have store the length value at opening, but this way it handles
 * dynamic changes of file size.
 */
static int isf_length(istream_t * istream)
{
  istream_file_t * isf = (istream_file_t *)istream;
  int pos,len;

  if (!isf || !isf->f) {
    return -1;
  }
  /* save current position. */
  len = ftell(isf->f);
  if (len != -1) {
    pos = len;
    /* seek t end of file */
    len = fseek(isf->f, 0, SEEK_END);
    if (len != -1) {
      /* get end of file position (AKA file length) */
      len = ftell(isf->f);
      /* restore saved position. ( $$$ no error check here ) */
      fseek(isf->f, pos, SEEK_SET);
    }
  }
  return len;
}

static int isf_tell(istream_t * istream)
{
  istream_file_t * isf = (istream_file_t *)istream;

  return (!isf || !isf->f)
    ? -1 
    : ftell(isf->f);
}

static int isf_seek(istream_t * istream, int offset)
{
  istream_file_t * isf = (istream_file_t *)istream;

  return (!isf || !isf->f)
    ? -1
    : fseek(isf->f, offset, SEEK_CUR);
}

static void isf_destroy(istream_t * istream)
{
  SC68free(istream);
}

static const istream_t istream_file = {
  isf_name,
  isf_open, isf_close,
  isf_read, isf_write,
  isf_length, isf_tell, isf_seek, isf_seek,
  isf_destroy
};

istream_t * istream_file_create(const char * fname, int mode)
{
  istream_file_t *isf;
  int len;

  if (!fname || !fname[0]) {
    return 0;
  }

  /* Don't need 0, because 1 byte already allocated in the
   * istream_file_t::fname.
   */
  len = strlen(fname);
  isf = SC68alloc(sizeof(istream_file_t) + len);
  if (!isf) {
    return 0;
  }

  /* Copy istream functions. */
  memcpy(&isf->istream, &istream_file, sizeof(istream_file));
  /* Clean file handle. */
  isf->f = 0;
  isf->mode = mode & (ISTREAM_OPEN_READ|ISTREAM_OPEN_WRITE);
  
  /* Copy filename. */
  /* $$$ May be later, we should add a check for relative path and add
   * CWD ... */
  strcpy(isf->name, fname);
  return &isf->istream;
}

#else /* #ifndef ISTREAM_NO_FILE */

/* istream file must not be include in this package. Anyway the creation
 * still exist but it always returns error.
 */

istream_t * istream_file_create(const char * fname, int mode)
{
  return 0;
}

#endif /* #ifndef ISTREAM_NO_FILE */
