/* Mini Allegro - File and compression routines */
/* Ripped from Allegro WIP */

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <io.h>
#include "minalleg.h"


#ifndef _USE_LFN
#define _USE_LFN  0
#endif


#if (!defined S_IRUSR) && (!defined SCAN_DEPEND)
   #define S_IRUSR   S_IREAD
   #define S_IWUSR   S_IWRITE
#endif



#define N            4096           /* 4k buffers for LZ compression */
#define F            18             /* upper limit for LZ match length */
#define THRESHOLD    2              /* LZ encode string into pos and length
				       if match size is greater than this */


typedef struct PACK_DATA            /* stuff for doing LZ compression */
{
   int state;                       /* where have we got to in the pack? */
   int i, c, len, r, s;
   int last_match_length, code_buf_ptr;
   unsigned char mask;
   char code_buf[17];
   int match_position;
   int match_length;
   int lson[N+1];                   /* left children, */
   int rson[N+257];                 /* right children, */
   int dad[N+1];                    /* and parents, = binary search trees */
   unsigned char text_buf[N+F-1];   /* ring buffer, with F-1 extra bytes
				       for string comparison */
} PACK_DATA;


typedef struct UNPACK_DATA          /* for reading LZ files */
{
   int state;                       /* where have we got to? */
   int i, j, k, r, c;
   int flags;
   unsigned char text_buf[N+F-1];   /* ring buffer, with F-1 extra bytes
				       for string comparison */
} UNPACK_DATA;


static int refill_buffer(PACKFILE *f);
static int flush_buffer(PACKFILE *f, int last);
static void pack_inittree(PACK_DATA *dat);
static void pack_insertnode(int r, PACK_DATA *dat);
static void pack_deletenode(int p, PACK_DATA *dat);
static int pack_write(PACKFILE *file, PACK_DATA *dat, int size, unsigned char *buf, int last);
static int pack_read(PACKFILE *file, UNPACK_DATA *dat, int s, unsigned char *buf);

static char the_password[256] = "";

int _packfile_filesize = 0;
int _packfile_datasize = 0;

int _packfile_type = 0;


#define FA_DAT_FLAGS  (FA_RDONLY | FA_ARCH)


int pack_getc(PACKFILE *f)
{
   f->buf_size--;
   if (f->buf_size > 0)
      return *(f->buf_pos++);
   else
      return _sort_out_getc(f);
}


int pack_putc(int c, PACKFILE *f)
{
   f->buf_size++;
   if (f->buf_size >= F_BUF_SIZE)
      return _sort_out_putc(c, f);
   else
      return (*(f->buf_pos++) = c);
}


/* fix_filename_case:
 *  Converts filename to upper case.
 */
char *fix_filename_case(char *filename)
{
   return filename;
}



/* fix_filename_slashes:
 *  Converts '/' or '\' to system specific path separator.
 */
char *fix_filename_slashes(char *filename)
{
   int pos, c;

   for (pos=0; c = filename[pos]; pos++) {
      if ((c == '/') || (c == '\\'))
		  filename[pos] = OTHER_PATH_SEPARATOR;
   }

   return filename;
}



/* fix_filename_path:
 *  Canonicalizes path.
 */
char *fix_filename_path(char *dest, char *path, int size)
{
   int saved_errno = errno;
   char buf[512], buf2[512];
   char *p;
   int pos = 0;
   int drive = -1;
   int c1, i;

   memset(buf, 0, sizeof(buf));

   #if (DEVICE_SEPARATOR != 0) && (DEVICE_SEPARATOR != '\0')

      /* check whether we have a drive letter */
      c1 = tolower(path[0]);
      if ((c1 >= 'a') && (c1 <= 'z')) {
	 int c2 = path[1];
	 if (c2 == DEVICE_SEPARATOR) {
	    drive = c1 - 'a';
	    path += 2;
	 }
      }

      /* if not, use the current drive */
      if (drive < 0)
	 drive = _al_getdrive();

	  buf[pos] = drive + 'a';      pos++;
	  buf[pos] = DEVICE_SEPARATOR; pos++;
   #endif

   /* if the path is relative, make it absolute */
   if ((path[0] != '/') && (path[0] != OTHER_PATH_SEPARATOR) && (path[0] != '#')) {
      _al_getdcwd(drive, buf2, sizeof(buf2) - 1);
      put_backslash(buf2);

      p = buf2;
      if ((tolower(p[0]) >= 'a') && (tolower(p[0]) <= 'z') && (p[1] == DEVICE_SEPARATOR))
	 p += 2;

	  memcpy(buf + pos, p, sizeof(buf) - pos);
      pos = strlen(buf);
   }

   /* add our path, and clean it up a bit */
   memcpy(buf + pos, path, sizeof(buf) - pos);

   fix_filename_case(buf);
   fix_filename_slashes(buf);

   /* remove duplicate slashes */
   buf2[0] = OTHER_PATH_SEPARATOR;
   buf2[1] = OTHER_PATH_SEPARATOR;
   buf2[2] = 0;
   pos = 2;

   while ((p = strstr(buf, buf2)) != NULL)
      memmove(p, p + 1, strlen(p));

   /* remove /./ patterns */
   buf2[0] = OTHER_PATH_SEPARATOR;
   buf2[1] = '.';
   buf2[2] = OTHER_PATH_SEPARATOR;
   buf2[3] = 0;
   pos = 3;

   while ((p = strstr(buf, buf2)) != NULL) {
      memmove(p, p + 1, strlen(p));
      memmove(p, p + 1, strlen(p));
   }

   /* collapse /../ patterns */
   buf2[0] = OTHER_PATH_SEPARATOR;
   buf2[1] = '.';
   buf2[2] = '.';
   buf2[3] = OTHER_PATH_SEPARATOR;
   buf2[4] = 0;
   pos = 4;

   while ((p = strstr(buf, buf2)) != NULL) {
      for (i = 0; buf + i < p; i++)
		  ;

      while (--i > 0) {
	 c1 = buf[i];

	 if (c1 == OTHER_PATH_SEPARATOR)
	    break;

	 if (c1 == DEVICE_SEPARATOR) {
	    i++;
	    break;
	 }
      }

      if (i < 0)
	 i = 0;

      p += strlen(buf2);
      memmove(buf+i+1, p, strlen(p) + 1);
   }

   /* all done! */
   memcpy(dest, buf, MIN(size, (int)strlen(buf)));

   errno = saved_errno;

   return dest;
}



/* replace_filename:
 *  Replaces filename in path with different one.
 *  It does not append '/' to the path.
 */
char *replace_filename(char *dest, char *path, char *filename, int size)
{
   char tmp[512];
   int pos, c;

   pos = strlen(path);

   while (pos>0) {
      c = path[pos - 1];
      if ((c == '/') || (c == OTHER_PATH_SEPARATOR) || (c == DEVICE_SEPARATOR) || (c == '#'))
	 break;
      pos--;
   }
   memset(tmp, 0, sizeof(tmp));
   memcpy(tmp, path, MIN(sizeof(tmp), pos));
   memcpy(tmp + MIN(sizeof(tmp), pos), filename, MIN(strlen(filename), sizeof(tmp) - pos));

   memcpy(dest, tmp, MIN((int)strlen(tmp), size));

   return dest;
}



/* replace_extension:
 *  Replaces extension in filename with different one.
 *  It appends '.' if it is not present in the filename.
 */
char *replace_extension(char *dest, char *filename, char *ext, int size)
{
   char tmp[512];
   int pos, end, c;

   pos = end = strlen(filename);

   while (pos>0) {
      c = filename[pos - 1];
      if ((c == '.') || (c == '/') || (c == OTHER_PATH_SEPARATOR) || (c == DEVICE_SEPARATOR) || (c == '#'))
	 break;
      pos--;
   }

   if (filename[pos - 1] == '.')
      end = pos - 1;

   memcpy(tmp, filename, MIN((int)sizeof(tmp), strlen(filename)));
   if (strlen(tmp) < sizeof(tmp)-1) {
      tmp[strlen(tmp)+1] = 0;
      tmp[strlen(tmp)] = '.';
   }
   memcpy(tmp + strlen(tmp), ext, MIN(sizeof(tmp) - (int)strlen(tmp), (int)strlen(ext)));
   memcpy(dest, tmp, MIN(size, sizeof(tmp)));

   return dest;
}



/* append_filename:
 *  Append filename to path, adding separator if necessary.
 */
char *append_filename(char *dest, char *path, char *filename, int size)
{
   char tmp[512];
   int pos, c;

   memcpy(tmp, path, MIN(sizeof(tmp) - 1, strlen(path)));
   tmp[511] = 0;
   pos = strlen(tmp);

   if ((pos > 0) && (tmp[pos] < ((int)sizeof(tmp) - 2))) {
      c = tmp[pos - 1];

      if ((c != '/') && (c != OTHER_PATH_SEPARATOR) && (c != DEVICE_SEPARATOR) && (c != '#')) {
		tmp[pos] = OTHER_PATH_SEPARATOR;
		pos++;
		tmp[pos] = 0;
      }
   }

   memcpy(tmp + strlen(tmp), filename, MIN(sizeof(tmp) - (int)strlen(tmp), (int)strlen(filename)));
   memcpy(dest, tmp, MIN(sizeof(tmp), (int)strlen(tmp)));

   return dest;
}



/* get_filename:
 *  When passed a completely specified file path, this returns a pointer
 *  to the filename portion. Both '\' and '/' are recognized as directory
 *  separators.
 */
char *get_filename(char *path)
{
   int pos, c;

   pos = strlen(path);

   while (pos>0) {
      c = path[pos - 1];
      if ((c == '/') || (c == OTHER_PATH_SEPARATOR) || (c == DEVICE_SEPARATOR) || (c == '#'))
	 break;
      pos--;
   }

   return (char *)path + pos;
}



/* get_extension:
 *  When passed a complete filename (with or without path information)
 *  this returns a pointer to the file extension.
 */
char *get_extension(char *filename)
{
   int pos, c;

   pos = strlen(filename);

   while (pos>0) {
      c = filename[pos - 1];
      if ((c == '.') || (c == '/') || (c == OTHER_PATH_SEPARATOR) || (c == DEVICE_SEPARATOR) || (c == '#'))
	 break;
      pos--;
   }

   if ((pos>0) && (filename[pos-1] == '.'))
      return (char *)filename + pos;

   return (char *)filename + strlen(filename);
}



/* put_backslash:
 *  If the last character of the filename is not a \, /, or #, this routine
 *  will concatenate a \ on to it.
 */
void put_backslash(char *filename)
{
   int c;

   if (*filename) {
      c = filename[strlen(filename)-1];

      if ((c == '/') || (c == OTHER_PATH_SEPARATOR) || (c == DEVICE_SEPARATOR) || (c == '#'))
	 return;
   }

   filename += strlen(filename);
   filename[0] = OTHER_PATH_SEPARATOR;
   filename[1] = 0;
}



/* file_exists:
 *  Checks whether a file matching the given name and attributes exists,
 *  returning non zero if it does. The file attribute may contain any of
 *  the FA_* constants from dir.h. If aret is not null, it will be set 
 *  to the attributes of the matching file. If an error occurs the system 
 *  error code will be stored in errno.
 */
int file_exists(char *filename, int attrib, int *aret)
{
   int a;

   if (!_al_file_isok(filename))
      return 0;

   if (!_al_file_exists(filename, attrib, &a))
      return FALSE;

   if (aret)
      *aret = a;

   return TRUE;
}



/* exists:
 *  Shortcut version of file_exists().
 */
int exists(char *filename)
{
   return file_exists(filename, FA_ARCH | FA_RDONLY, NULL);
}



/* file_size:
 *  Returns the size of a file, in bytes.
 *  If the file does not exist or an error occurs, it will return zero
 *  and store the system error code in errno.
 */
long file_size(char *filename)
{

   if (!_al_file_isok(filename))
      return 0;

   return _al_file_size(filename);
}



/* file_time:
 *  Returns a file time-stamp.
 *  If the file does not exist or an error occurs, it will return zero
 *  and store the system error code in errno.
 */
time_t file_time(char *filename)
{
   if (!_al_file_isok(filename))
      return 0;

   return _al_file_time(filename);
}



/* delete_file:
 *  Removes a file from the disk.
 */
int delete_file(char *filename)
{
   errno = 0;

   if (!_al_file_isok(filename))
      return errno;

   unlink(filename);   

   return errno;
}



/* for_each_file:
 *  Finds all the files on the disk which match the given wildcard
 *  specification and file attributes, and executes callback() once for
 *  each. callback() will be passed three arguments, the first a string
 *  which contains the completed filename, the second being the attributes
 *  of the file, and the third an int which is simply a copy of param (you
 *  can use this for whatever you like). If an error occurs an error code
 *  will be stored in errno, and callback() can cause for_each_file() to
 *  abort by setting errno itself. Returns the number of successful calls
 *  made to callback(). The file attribute may contain any of the FA_* 
 *  flags from dir.h.
 */
int for_each_file(char *name, int attrib, void (*callback)(char *filename, int attrib, int param), int param)
{
   char dta_name[512], buf[512];
   void *dta;
   int dta_attrib;
   int c = 0;

   if (!_al_file_isok(name))
      return 0;

   dta = _al_findfirst(name, attrib, dta_name, &dta_attrib);

   if (!dta)
      return 0;

   do {
      replace_filename(buf, name, dta_name, sizeof(buf));
      (*callback)(buf, dta_attrib, param);
      if (errno != 0)
	 break;
      c++;
   } while (_al_findnext(dta, dta_name, &dta_attrib) == 0);

   _al_findclose(dta);

   errno = 0;
   return c;
}



/* packfile_password:
 *  Sets the password to be used by all future read/write operations.
 */
void packfile_password(char *password)
{
   int i = 0;
   int c;

   if (password) {
      while ((c = *password) != 0) {
		  password++;
	 the_password[i++] = c;
	 if (i >= (int)sizeof(the_password)-1)
	    break;
      }
   }

   the_password[i] = 0;
}



/* encrypt_id:
 *  Helper for encrypting magic numbers, using the current password.
 */
static long encrypt_id(long x, int new_format)
{
   long mask = 0;
   int i, pos;

   if (the_password[0]) {
      for (i=0; the_password[i]; i++)
	 mask ^= ((long)the_password[i] << ((i&3) * 8));

      for (i=0, pos=0; i<4; i++) {
	 mask ^= (long)the_password[pos++] << (24-i*8);
	 if (!the_password[pos])
	    pos = 0;
      }

      if (new_format)
	 mask ^= 42;
   }

   return x ^ mask;
}



/* clone_password:
 *  Sets up a local password string for use by this packfile.
 */
static int clone_password(PACKFILE *f)
{
   if (the_password[0]) {
      if ((f->passdata = malloc(strlen(the_password)+1)) == NULL) {
	 errno = ENOMEM;
	 return FALSE;
      }
      strcpy(f->passdata, the_password);
   }
   else
      f->passdata = NULL;

   f->passpos = f->passdata;

   return TRUE;
}



/* pack_fopen:
 *  Opens a file according to mode, which may contain any of the flags:
 *  'r': open file for reading.
 *  'w': open file for writing, overwriting any existing data.
 *  'p': open file in 'packed' mode. Data will be compressed as it is
 *       written to the file, and automatically uncompressed during read
 *       operations. Files created in this mode will produce garbage if
 *       they are read without this flag being set.
 *  '!': open file for writing in normal, unpacked mode, but add the value
 *       F_NOPACK_MAGIC to the start of the file, so that it can be opened
 *       in packed mode and Allegro will automatically detect that the
 *       data does not need to be decompressed.
 *
 *  Instead of these flags, one of the constants F_READ, F_WRITE,
 *  F_READ_PACKED, F_WRITE_PACKED or F_WRITE_NOPACK may be used as the second 
 *  argument to fopen().
 *
 *  On success, fopen() returns a pointer to a file structure, and on error
 *  it returns NULL and stores an error code in errno. An attempt to read a 
 *  normal file in packed mode will cause errno to be set to EDOM.
 */
PACKFILE *pack_fopen(const char *filename, char *mode)
{
   PACKFILE *f, *f2;
   long header = FALSE;
   int c;

   _packfile_type = 0;

   if (!_al_file_isok(filename))
      return NULL;

   errno = 0;

   if ((f = malloc(sizeof(PACKFILE))) == NULL) {
      errno = ENOMEM;
      return NULL;
   }

   f->buf_pos = f->buf;
   f->flags = 0;
   f->buf_size = 0;
   f->filename = NULL;
   f->passdata = NULL;
   f->passpos = NULL;

   while ((c = *(mode++)) != 0) {
      switch (c) {
	 case 'r': case 'R': f->flags &= ~PACKFILE_FLAG_WRITE; break;
	 case 'w': case 'W': f->flags |= PACKFILE_FLAG_WRITE; break;
	 case 'p': case 'P': f->flags |= PACKFILE_FLAG_PACK; break;
	 case '!': f->flags &= ~PACKFILE_FLAG_PACK; header = TRUE; break;
      }
   }

   if (f->flags & PACKFILE_FLAG_WRITE) {
      if (f->flags & PACKFILE_FLAG_PACK) {
	 /* write a packed file */
	 PACK_DATA *dat = malloc(sizeof(PACK_DATA));

	 if (!dat) {
	    errno = ENOMEM;
	    free(f);
	    return NULL;
	 }

	 if ((f->parent = pack_fopen(filename, F_WRITE)) == NULL) {
	    free(dat);
	    free(f);
	    return NULL;
	 }

	 pack_mputl(encrypt_id(F_PACK_MAGIC, TRUE), f->parent);

	 f->todo = 4;

	 for (c=0; c < N - F; c++)
	    dat->text_buf[c] = 0; 

	 dat->state = 0;

	 f->pack_data = dat;
      }
      else {
	 /* write a 'real' file */
	 f->parent = NULL;
	 f->pack_data = NULL;

	 if (!clone_password(f)) {
	    free(f);
	    return NULL;
	 }
#ifndef ALLEGRO_MPW
	 f->hndl = open(filename, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
#else
	 f->hndl = _al_open(uconvert_toascii(filename, NULL), O_WRONLY | O_BINARY | O_CREAT | O_TRUNC);
#endif
	 if (f->hndl < 0) {
	    if (f->passdata)
	       free(f->passdata);
	    free(f);
	    return NULL;
	 }

	 errno = 0;
	 f->todo = 0;

	 if (header)
	    pack_mputl(encrypt_id(F_NOPACK_MAGIC, TRUE), f);
      }
   }
   else { 
      if (f->flags & PACKFILE_FLAG_PACK) {
	 /* read a packed file */
	 UNPACK_DATA *dat = malloc(sizeof(UNPACK_DATA));

	 if (!dat) {
	    errno = ENOMEM;
	    free(f);
	    return NULL;
	 }

	 if ((f->parent = pack_fopen(filename, F_READ)) == NULL) {
	    free(dat);
	    free(f);
	    return NULL;
	 }

	 header = pack_mgetl(f->parent);

	 if ((f->parent->passpos) &&
	     ((header == encrypt_id(F_PACK_MAGIC, FALSE)) ||
	      (header == encrypt_id(F_NOPACK_MAGIC, FALSE)))) {

	    /* backward compatibility mode */
	    pack_fclose(f->parent);

	    if (!clone_password(f)) {
	       free(dat);
	       free(f);
	       return NULL;
	    }

	    if ((f->parent = pack_fopen(filename, F_READ)) == NULL) {
	       free(dat);
	       free(f);
	       return NULL;
	    }

	    f->parent->flags |= PACKFILE_FLAG_OLD_CRYPT;
	    f->flags |= PACKFILE_FLAG_OLD_CRYPT;

	    pack_mgetl(f->parent);

	    if (header == encrypt_id(F_PACK_MAGIC, FALSE))
	       header = encrypt_id(F_PACK_MAGIC, TRUE);
	    else
	       header = encrypt_id(F_NOPACK_MAGIC, TRUE);
	 }

	 if (header == encrypt_id(F_PACK_MAGIC, TRUE)) {
	    for (c=0; c < N - F; c++)
	       dat->text_buf[c] = 0; 
	    dat->state = 0;
	    f->todo = LONG_MAX;
	    f->pack_data = (char *)dat;
	 }
	 else if (header == encrypt_id(F_NOPACK_MAGIC, TRUE)) {
	    f2 = f->parent;
	    free(dat);
	    free(f);
	    return f2;
	 }
	 else {
	    pack_fclose(f->parent);
	    free(dat);
	    free(f);
	    if (errno == 0)
	       errno = EDOM;
	    return NULL;
	 }
      }
      else {
	 /* read a 'real' file */
	 f->parent = NULL;
	 f->pack_data = NULL;

	 f->todo = _al_file_size(filename);

	 if (errno) {
	    free(f);
	    return NULL;
	 }

	 if (!clone_password(f)) {
	    free(f);
	    return NULL;
	 }

#ifndef ALLEGRO_MPW
	 f->hndl = open(filename, O_RDONLY | O_BINARY, S_IRUSR | S_IWUSR);
#else
	 f->hndl = _al_open(uconvert_toascii(filename, NULL), O_RDONLY | O_BINARY);
#endif

	 if (f->hndl < 0) {
	    if (f->passdata)
	       free(f->passdata);
	    free(f);
	    return NULL;
	 }
      }
   }

   return f;
}



/* pack_fclose:
 *  Closes a file after it has been read or written.
 *  Returns zero on success. On error it returns an error code which is
 *  also stored in errno. This function can fail only when writing to
 *  files: if the file was opened in read mode it will always succeed.
 */
int pack_fclose(PACKFILE *f)
{
   if (f) {
      if (f->flags & PACKFILE_FLAG_WRITE) {
	 if (f->flags & PACKFILE_FLAG_CHUNK)
	    return pack_fclose(pack_fclose_chunk(f));

	 flush_buffer(f, TRUE);
      }

      if (f->passdata)
	 free(f->passdata);

      if (f->pack_data)
	 free(f->pack_data);

      if (f->parent)
	 pack_fclose(f->parent);
      else
	 close(f->hndl);

      free(f);
      return errno;
   }

   return 0;
}



/* pack_fopen_chunk: 
 *  Opens a sub-chunk of the specified file, for reading or writing depending
 *  on the type of the file. The returned file pointer describes the sub
 *  chunk, and replaces the original file, which will no longer be valid.
 *  When writing to a chunk file, data is sent to the original file, but
 *  is prefixed with two length counts (32 bit, big-endian). For uncompressed 
 *  chunks these will both be set to the length of the data in the chunk.
 *  For compressed chunks, created by setting the pack flag, the first will
 *  contain the raw size of the chunk, and the second will be the negative
 *  size of the uncompressed data. When reading chunks, the pack flag is
 *  ignored, and the compression type is detected from the sign of the
 *  second size value. The file structure used to read chunks checks the
 *  chunk size, and will return EOF if you try to read past the end of
 *  the chunk. If you don't read all of the chunk data, when you call
 *  pack_fclose_chunk(), the parent file will advance past the unused data.
 *  When you have finished reading or writing a chunk, you should call
 *  pack_fclose_chunk() to return to your original file.
 */
PACKFILE *pack_fopen_chunk(PACKFILE *f, int pack)
{
   PACKFILE *chunk;
   char *name;
   int c;

   if (f->flags & PACKFILE_FLAG_WRITE) {

      /* write a sub-chunk */ 
      name = tmpnam(NULL);
      chunk = pack_fopen(name, (pack ? F_WRITE_PACKED : F_WRITE_NOPACK));

      if (chunk) {
         chunk->filename = strdup(name);

	 if (pack)
	    chunk->parent->parent = f;
	 else
	    chunk->parent = f;

	 chunk->flags |= PACKFILE_FLAG_CHUNK;
      }
   }
   else {
      /* read a sub-chunk */
      _packfile_filesize = pack_mgetl(f);
      _packfile_datasize = pack_mgetl(f);

      if ((chunk = malloc(sizeof(PACKFILE))) == NULL) {
	 errno = ENOMEM;
	 return NULL;
      }

      chunk->buf_pos = chunk->buf;
      chunk->flags = PACKFILE_FLAG_CHUNK;
      chunk->buf_size = 0;
      chunk->filename = NULL;
      chunk->passdata = NULL;
      chunk->passpos = NULL;
      chunk->parent = f;

      if (f->flags & PACKFILE_FLAG_OLD_CRYPT) {
	 /* backward compatibility mode */
	 if (f->passdata) {
	    if ((chunk->passdata = malloc(strlen(f->passdata)+1)) == NULL) {
	       errno = ENOMEM;
	       free(chunk);
	       return NULL;
	    }
	    strcpy(chunk->passdata, f->passdata);
	    chunk->passpos = chunk->passdata + (long)f->passpos - (long)f->passdata;
	    f->passpos = f->passdata;
	 }
	 chunk->flags |= PACKFILE_FLAG_OLD_CRYPT;
      }

      if (_packfile_datasize < 0) {
	 /* read a packed chunk */
	 UNPACK_DATA *dat = malloc(sizeof(UNPACK_DATA));

	 if (!dat) {
	    errno = ENOMEM;
	    if (chunk->passdata) free(chunk->passdata);
	    free(chunk);
	    return NULL;
	 }

	 for (c=0; c < N - F; c++)
	    dat->text_buf[c] = 0; 

	 dat->state = 0;
	 _packfile_datasize = -_packfile_datasize;
	 chunk->todo = _packfile_datasize;
	 chunk->pack_data = (char *)dat;
	 chunk->flags |= PACKFILE_FLAG_PACK;
      }
      else {
	 /* read an uncompressed chunk */
	 chunk->todo = _packfile_datasize;
	 chunk->pack_data = NULL;
      }
   }

   return chunk;
}



/* pack_fclose_chunk:
 *  Call after reading or writing a sub-chunk. This closes the chunk file,
 *  and returns a pointer to the original file structure (the one you
 *  passed to pack_fopen_chunk()), to allow you to read or write data 
 *  after the chunk.
 */
PACKFILE *pack_fclose_chunk(PACKFILE *f)
{
   PACKFILE *parent = f->parent;
   PACKFILE *tmp;
   char *name = f->filename;
   int header;

   if (f->flags & PACKFILE_FLAG_WRITE) {
      /* finish writing a chunk */
      _packfile_datasize = f->todo + f->buf_size - 4;

      if (f->flags & PACKFILE_FLAG_PACK) {
	 parent = parent->parent;
	 f->parent->parent = NULL;
      }
      else
	 f->parent = NULL;

      f->flags &= ~PACKFILE_FLAG_CHUNK;
      pack_fclose(f);

      tmp = pack_fopen(name, F_READ);
      _packfile_filesize = tmp->todo - 4;

      header = pack_mgetl(tmp);

      pack_mputl(_packfile_filesize, parent);

      if (header == encrypt_id(F_PACK_MAGIC, TRUE))
	 pack_mputl(-_packfile_datasize, parent);
      else
	 pack_mputl(_packfile_datasize, parent);

      while (!pack_feof(tmp))
	 pack_putc(pack_getc(tmp), parent);

      pack_fclose(tmp);

      delete_file(name);
      free(name);
   }
   else {
      /* finish reading a chunk */
      while (f->todo > 0)
	 pack_getc(f);

      if ((f->passpos) && (f->flags & PACKFILE_FLAG_OLD_CRYPT))
	 parent->passpos = parent->passdata + (long)f->passpos - (long)f->passdata;

      if (f->passdata)
	 free(f->passdata);

      if (f->pack_data)
	 free(f->pack_data);

      free(f);
   }

   return parent;
}



/* pack_fseek:
 *  Like the stdio fseek() function, but only supports forward seeks 
 *  relative to the current file position.
 */
int pack_fseek(PACKFILE *f, int offset)
{
   int i;

   if (f->flags & PACKFILE_FLAG_WRITE)
      return -1;

   errno = 0;

   /* skip forward through the buffer */
   if (f->buf_size > 0) {
      i = MIN(offset, f->buf_size);
      f->buf_size -= i;
      f->buf_pos += i;
      offset -= i;
      if ((f->buf_size <= 0) && (f->todo <= 0))
	 f->flags |= PACKFILE_FLAG_EOF;
   }

   /* need to seek some more? */
   if (offset > 0) {
      i = MIN(offset, f->todo);

      if ((f->flags & PACKFILE_FLAG_PACK) || (f->passpos)) {
	 /* for compressed files, we just have to read through the data */
	 while (i > 0) {
	    pack_getc(f);
	    i--;
	 }
      }
      else {
	 if (f->parent) {
	    /* pass the seek request on to the parent file */
	    pack_fseek(f->parent, i);
	 }
	 else {
	    /* do a real seek */
	    lseek(f->hndl, i, SEEK_CUR);
	 }
	 f->todo -= i;
	 if (f->todo <= 0)
	    f->flags |= PACKFILE_FLAG_EOF;
      }
   }

   return errno;
}



/* pack_igetw:
 *  Reads a 16 bit word from a file, using intel byte ordering.
 */
int pack_igetw(PACKFILE *f)
{
   int b1, b2;

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
	 return ((b2 << 8) | b1);

   return EOF;
}



/* pack_igetl:
 *  Reads a 32 bit long from a file, using intel byte ordering.
 */
long pack_igetl(PACKFILE *f)
{
   int b1, b2, b3, b4;

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
	 if ((b3 = pack_getc(f)) != EOF)
	    if ((b4 = pack_getc(f)) != EOF)
	       return (((long)b4 << 24) | ((long)b3 << 16) |
		       ((long)b2 << 8) | (long)b1);

   return EOF;
}



/* pack_iputw:
 *  Writes a 16 bit int to a file, using intel byte ordering.
 */
int pack_iputw(int w, PACKFILE *f)
{
   int b1, b2;

   b1 = (w & 0xFF00) >> 8;
   b2 = w & 0x00FF;

   if (pack_putc(b2,f)==b2)
      if (pack_putc(b1,f)==b1)
	 return w;

   return EOF;
}



/* pack_iputw:
 *  Writes a 32 bit long to a file, using intel byte ordering.
 */
long pack_iputl(long l, PACKFILE *f)
{
   int b1, b2, b3, b4;

   b1 = (int)((l & 0xFF000000L) >> 24);
   b2 = (int)((l & 0x00FF0000L) >> 16);
   b3 = (int)((l & 0x0000FF00L) >> 8);
   b4 = (int)l & 0x00FF;

   if (pack_putc(b4,f)==b4)
      if (pack_putc(b3,f)==b3)
	 if (pack_putc(b2,f)==b2)
	    if (pack_putc(b1,f)==b1)
	       return l;

   return EOF;
}



/* pack_mgetw:
 *  Reads a 16 bit int from a file, using motorola byte-ordering.
 */
int pack_mgetw(PACKFILE *f)
{
   int b1, b2;

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
	 return ((b1 << 8) | b2);

   return EOF;
}



/* pack_mgetl:
 *  Reads a 32 bit long from a file, using motorola byte-ordering.
 */
long pack_mgetl(PACKFILE *f)
{
   int b1, b2, b3, b4;

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
	 if ((b3 = pack_getc(f)) != EOF)
	    if ((b4 = pack_getc(f)) != EOF)
	       return (((long)b1 << 24) | ((long)b2 << 16) |
		       ((long)b3 << 8) | (long)b4);

   return EOF;
}



/* pack_mputw:
 *  Writes a 16 bit int to a file, using motorola byte-ordering.
 */
int pack_mputw(int w, PACKFILE *f)
{
   int b1, b2;

   b1 = (w & 0xFF00) >> 8;
   b2 = w & 0x00FF;

   if (pack_putc(b1,f)==b1)
      if (pack_putc(b2,f)==b2)
	 return w;

   return EOF;
}



/* pack_mputl:
 *  Writes a 32 bit long to a file, using motorola byte-ordering.
 */
long pack_mputl(long l, PACKFILE *f)
{
   int b1, b2, b3, b4;

   b1 = (int)((l & 0xFF000000L) >> 24);
   b2 = (int)((l & 0x00FF0000L) >> 16);
   b3 = (int)((l & 0x0000FF00L) >> 8);
   b4 = (int)l & 0x00FF;

   if (pack_putc(b1,f)==b1)
      if (pack_putc(b2,f)==b2)
	 if (pack_putc(b3,f)==b3)
	    if (pack_putc(b4,f)==b4)
	       return l;

   return EOF;
}



/* pack_fread:
 *  Reads n bytes from f and stores them at memory location p. Returns the 
 *  number of items read, which will be less than n if EOF is reached or an 
 *  error occurs. Error codes are stored in errno.
 */
long pack_fread(void *p, long n, PACKFILE *f)
{
   unsigned char *cp = (unsigned char *)p;
   long c;
   int i;

   for (c=0; c<n; c++) {
      if (--(f->buf_size) > 0)
	 *(cp++) = *(f->buf_pos++);
      else {
	 i = _sort_out_getc(f);
	 if (i == EOF)
	    return c;
	 else
	    *(cp++) = i;
      }
   }

   return n;
}



/* pack_fwrite:
 *  Writes n bytes to the file f from memory location p. Returns the number 
 *  of items written, which will be less than n if an error occurs. Error 
 *  codes are stored in errno.
 */
long pack_fwrite(void *p, long n, PACKFILE *f)
{
   unsigned char *cp = (unsigned char *)p;
   long c;

   for (c=0; c<n; c++) {
      if (++(f->buf_size) >= F_BUF_SIZE) {
	 if (_sort_out_putc(*cp,f) != *cp)
	    return c;
	 cp++;
      }
      else
	 *(f->buf_pos++)=*(cp++);
   }

   return n;
}



/* pack_ungetc:
 *  Puts a character back in the file's input buffer.  Added by gfoot for
 *  use in the fgets function; maybe it should be in the API.  It only works
 *  for characters just fetched by pack_getc.
 */
static void pack_ungetc (int ch, PACKFILE *f)
{
   *(--f->buf_pos) = (unsigned char) ch;
   f->buf_size++;
   f->flags &= ~PACKFILE_FLAG_EOF;
}



/* pack_fgets:
 *  Reads a line from a text file, storing it at location p. Stops when a
 *  linefeed is encountered, or max characters have been read. Returns a
 *  pointer to where it stored the text, or NULL on error. The end of line
 *  is handled by detecting optional '\r' characters optionally followed 
 *  by '\n' characters. This supports CR-LF (DOS/Windows), LF (Unix), and
 *  CR (Mac) formats.
 */
char *pack_fgets(char *p, int max, PACKFILE *f)
{
   char *pmax;
   int c;

   errno = 0;

   pmax = p+max - 1;

   if (pack_feof(f)) {
      if (1 < max) *p = 0;
      return NULL;
   }

   while ((c = pack_getc (f)) != EOF) {

      if (c == '\r' || c == '\n') {
	 /* Technically we should check there's space in the buffer, and if so,
	  * add a \n.  But pack_fgets has never done this. */
	 if (c == '\r') {
	    /* eat the following \n, if any */
	    if ((c = pack_getc (f)) != '\n') pack_ungetc (c, f);
	 }
	 break;
      }

      /* is there room in the buffer? */
      if (1 > pmax - p) {
	 pack_ungetc (c, f);
	 c = '\0';
	 break;
      }

      /* write the character */
      *p = c;
	  p++;
   }

   /* terminate the string */
   *p = 0;

   if (c == '\0' || errno)
      return NULL;

   return p;
}



/* pack_fputs:
 *  Writes a string to a text file, returning zero on success, -1 on error.
 */
int pack_fputs(char *p, PACKFILE *f)
{
   char *s;

   errno = 0;

   s = p;

   while (*s) {
	 if (*s == '\n')
	    pack_putc('\r', f);

      pack_putc(*s, f);
      s++;
   }

   if (errno)
      return -1;
   else
      return 0;
}



/* _sort_out_getc:
 *  Helper function for the pack_getc() macro.
 */
int _sort_out_getc(PACKFILE *f)
{
   if (f->buf_size == 0) {
      if (f->todo <= 0)
	 f->flags |= PACKFILE_FLAG_EOF;
      return *(f->buf_pos++);
   }
   return refill_buffer(f);
}



/* refill_buffer:
 *  Refills the read buffer. The file must have been opened in read mode,
 *  and the buffer must be empty.
 */
static int refill_buffer(PACKFILE *f)
{
   int i, sz, done, offset;

   if ((f->flags & PACKFILE_FLAG_EOF) || (f->todo <= 0)) {
      f->flags |= PACKFILE_FLAG_EOF;
      return EOF;
   }

   if (f->parent) {
      if (f->flags & PACKFILE_FLAG_PACK) {
	 f->buf_size = pack_read(f->parent, (UNPACK_DATA *)f->pack_data, MIN(F_BUF_SIZE, f->todo), f->buf);
      }
      else {
	 f->buf_size = pack_fread(f->buf, MIN(F_BUF_SIZE, f->todo), f->parent);
      } 
      if (f->parent->flags & PACKFILE_FLAG_EOF)
	 f->todo = 0;
      if (f->parent->flags & PACKFILE_FLAG_ERROR)
	 goto err;
   }
   else {
      f->buf_size = MIN(F_BUF_SIZE, f->todo);

      offset = lseek(f->hndl, 0, SEEK_CUR);
      done = 0;

      errno = 0;
      sz = read(f->hndl, f->buf, f->buf_size);

      while (sz+done < f->buf_size) {
	 if ((sz < 0) && ((errno != EINTR) && (errno != EAGAIN)))
	    goto err;

	 if (sz > 0)
	    done += sz;

	 lseek(f->hndl, offset+done, SEEK_SET);
         errno = 0;
	 sz = read(f->hndl, f->buf+done, f->buf_size-done);
      }
      errno = 0;

      if (errno == EINTR)
	 errno = 0;

      if ((f->passpos) && (!(f->flags & PACKFILE_FLAG_OLD_CRYPT))) {
	 for (i=0; i<f->buf_size; i++) {
	    f->buf[i] ^= *(f->passpos++);
	    if (!*f->passpos)
	       f->passpos = f->passdata;
	 }
      }
   }

   f->todo -= f->buf_size;
   f->buf_pos = f->buf;
   f->buf_size--;
   if (f->buf_size <= 0)
      if (f->todo <= 0)
	 f->flags |= PACKFILE_FLAG_EOF;

   return *(f->buf_pos++);

   err:
   errno = EFAULT;
   f->flags |= PACKFILE_FLAG_ERROR;
   return EOF;
}



/* _sort_out_putc:
 *  Helper function for the pack_putc() macro.
 */
int _sort_out_putc(int c, PACKFILE *f)
{
   f->buf_size--;

   if (flush_buffer(f, FALSE))
      return EOF;

   f->buf_size++;
   return (*(f->buf_pos++)=c);
}



/* flush_buffer:
 * flushes a file buffer to the disk. The file must be open in write mode.
 */
static int flush_buffer(PACKFILE *f, int last)
{
   int i, sz, done, offset;

   if (f->buf_size > 0) {
      if (f->flags & PACKFILE_FLAG_PACK) {
	 if (pack_write(f->parent, (PACK_DATA *)f->pack_data, f->buf_size, f->buf, last))
	    goto err;
      }
      else {
	 if ((f->passpos) && (!(f->flags & PACKFILE_FLAG_OLD_CRYPT))) {
	    for (i=0; i<f->buf_size; i++) {
	       f->buf[i] ^= *(f->passpos++);
	       if (!*f->passpos)
		  f->passpos = f->passdata;
	    }
	 }

	 offset = lseek(f->hndl, 0, SEEK_CUR);
	 done = 0;

	 errno = 0;
	 sz = write(f->hndl, f->buf, f->buf_size);

	 while (sz+done < f->buf_size) {
	    if ((sz < 0) && ((errno != EINTR) && (errno != EAGAIN)))
	       goto err;

	    if (sz > 0)
	       done += sz;

	    lseek(f->hndl, offset+done, SEEK_SET);
	    errno = 0;
	    sz = write(f->hndl, f->buf+done, f->buf_size-done);
	 }
	 errno = 0;
      }
      f->todo += f->buf_size;
   }
   f->buf_pos = f->buf;
   f->buf_size = 0;
   return 0;

   err:
   errno = EFAULT;
   f->flags |= PACKFILE_FLAG_ERROR;
   return EOF;
}



/***************************************************
 ************ LZSS compression routines ************
 ***************************************************

   This compression algorithm is based on the ideas of Lempel and Ziv,
   with the modifications suggested by Storer and Szymanski. The algorithm 
   is based on the use of a ring buffer, which initially contains zeros. 
   We read several characters from the file into the buffer, and then 
   search the buffer for the longest string that matches the characters 
   just read, and output the length and position of the match in the buffer.

   With a buffer size of 4096 bytes, the position can be encoded in 12
   bits. If we represent the match length in four bits, the <position,
   length> pair is two bytes long. If the longest match is no more than
   two characters, then we send just one character without encoding, and
   restart the process with the next letter. We must send one extra bit
   each time to tell the decoder whether we are sending a <position,
   length> pair or an unencoded character, and these flags are stored as
   an eight bit mask every eight items.

   This implementation uses binary trees to speed up the search for the
   longest match.

   Original code by Haruhiko Okumura, 4/6/1989.
   12-2-404 Green Heights, 580 Nagasawa, Yokosuka 239, Japan.

   Modified for use in the Allegro filesystem by Shawn Hargreaves.

   Use, distribute, and modify this code freely.
*/



/* pack_inittree:
 *  For i = 0 to N-1, rson[i] and lson[i] will be the right and left 
 *  children of node i. These nodes need not be initialized. Also, dad[i] 
 *  is the parent of node i. These are initialized to N, which stands for 
 *  'not used.' For i = 0 to 255, rson[N+i+1] is the root of the tree for 
 *  strings that begin with character i. These are initialized to N. Note 
 *  there are 256 trees.
 */
static void pack_inittree(PACK_DATA *dat)
{
   int i;

   for (i=N+1; i<=N+256; i++)
      dat->rson[i] = N;

   for (i=0; i<N; i++)
      dat->dad[i] = N;
}



/* pack_insertnode:
 *  Inserts a string of length F, text_buf[r..r+F-1], into one of the trees 
 *  (text_buf[r]'th tree) and returns the longest-match position and length 
 *  via match_position and match_length. If match_length = F, then removes 
 *  the old node in favor of the new one, because the old one will be 
 *  deleted sooner. Note r plays double role, as tree node and position in 
 *  the buffer. 
 */
static void pack_insertnode(int r, PACK_DATA *dat)
{
   int i, p, cmp;
   unsigned char *key;
   unsigned char *text_buf = dat->text_buf;

   cmp = 1;
   key = &text_buf[r];
   p = N + 1 + key[0];
   dat->rson[r] = dat->lson[r] = N;
   dat->match_length = 0;

   for (;;) {

      if (cmp >= 0) {
	 if (dat->rson[p] != N)
	    p = dat->rson[p];
	 else {
	    dat->rson[p] = r;
	    dat->dad[r] = p;
	    return;
	 }
      }
      else {
	 if (dat->lson[p] != N)
	    p = dat->lson[p];
	 else {
	    dat->lson[p] = r;
	    dat->dad[r] = p;
	    return;
	 }
      }

      for (i = 1; i < F; i++)
	 if ((cmp = key[i] - text_buf[p + i]) != 0)
	    break;

      if (i > dat->match_length) {
	 dat->match_position = p;
	 if ((dat->match_length = i) >= F)
	    break;
      }
   }

   dat->dad[r] = dat->dad[p];
   dat->lson[r] = dat->lson[p];
   dat->rson[r] = dat->rson[p];
   dat->dad[dat->lson[p]] = r;
   dat->dad[dat->rson[p]] = r;
   if (dat->rson[dat->dad[p]] == p)
      dat->rson[dat->dad[p]] = r;
   else
      dat->lson[dat->dad[p]] = r;
   dat->dad[p] = N;                 /* remove p */
}



/* pack_deletenode:
 *  Removes a node from a tree.
 */
static void pack_deletenode(int p, PACK_DATA *dat)
{
   int q;

   if (dat->dad[p] == N)
      return;     /* not in tree */

   if (dat->rson[p] == N)
      q = dat->lson[p];
   else
      if (dat->lson[p] == N)
	 q = dat->rson[p];
      else {
	 q = dat->lson[p];
	 if (dat->rson[q] != N) {
	    do {
	       q = dat->rson[q];
	    } while (dat->rson[q] != N);
	    dat->rson[dat->dad[q]] = dat->lson[q];
	    dat->dad[dat->lson[q]] = dat->dad[q];
	    dat->lson[q] = dat->lson[p];
	    dat->dad[dat->lson[p]] = q;
	 }
	 dat->rson[q] = dat->rson[p];
	 dat->dad[dat->rson[p]] = q;
      }

   dat->dad[q] = dat->dad[p];
   if (dat->rson[dat->dad[p]] == p)
      dat->rson[dat->dad[p]] = q;
   else
      dat->lson[dat->dad[p]] = q;

   dat->dad[p] = N;
}



/* pack_write:
 *  Called by flush_buffer(). Packs size bytes from buf, using the pack 
 *  information contained in dat. Returns 0 on success.
 */
static int pack_write(PACKFILE *file, PACK_DATA *dat, int size, unsigned char *buf, int last)
{
   int i = dat->i;
   int c = dat->c;
   int len = dat->len;
   int r = dat->r;
   int s = dat->s;
   int last_match_length = dat->last_match_length;
   int code_buf_ptr = dat->code_buf_ptr;
   unsigned char mask = dat->mask;
   int ret = 0;

   if (dat->state==2)
      goto pos2;
   else
      if (dat->state==1)
	 goto pos1;

   dat->code_buf[0] = 0;
      /* code_buf[1..16] saves eight units of code, and code_buf[0] works
	 as eight flags, "1" representing that the unit is an unencoded
	 letter (1 byte), "0" a position-and-length pair (2 bytes).
	 Thus, eight units require at most 16 bytes of code. */

   code_buf_ptr = mask = 1;

   s = 0;
   r = N - F;
   pack_inittree(dat);

   for (len=0; (len < F) && (size > 0); len++) {
      dat->text_buf[r+len] = *(buf++);
      if (--size == 0) {
	 if (!last) {
	    dat->state = 1;
	    goto getout;
	 }
      }
      pos1:
	 ; 
   }

   if (len == 0)
      goto getout;

   for (i=1; i <= F; i++)
      pack_insertnode(r-i,dat);
	    /* Insert the F strings, each of which begins with one or
	       more 'space' characters. Note the order in which these
	       strings are inserted. This way, degenerate trees will be
	       less likely to occur. */

   pack_insertnode(r,dat);
	    /* Finally, insert the whole string just read. match_length
	       and match_position are set. */

   do {
      if (dat->match_length > len)
	 dat->match_length = len;  /* match_length may be long near the end */

      if (dat->match_length <= THRESHOLD) {
	 dat->match_length = 1;  /* not long enough match: send one byte */
	 dat->code_buf[0] |= mask;    /* 'send one byte' flag */
	 dat->code_buf[code_buf_ptr++] = dat->text_buf[r]; /* send uncoded */
      }
      else {
	 /* send position and length pair. Note match_length > THRESHOLD */
	 dat->code_buf[code_buf_ptr++] = (unsigned char) dat->match_position;
	 dat->code_buf[code_buf_ptr++] = (unsigned char)
				     (((dat->match_position >> 4) & 0xF0) |
				      (dat->match_length - (THRESHOLD + 1)));
      }

      if ((mask <<= 1) == 0) {                  /* shift mask left one bit */
	 if ((file->passpos) && (file->flags & PACKFILE_FLAG_OLD_CRYPT)) {
	    dat->code_buf[0] ^= *file->passpos;
	    file->passpos++;
	    if (!*file->passpos)
	       file->passpos = file->passdata;
	 };

	 for (i=0; i<code_buf_ptr; i++)         /* send at most 8 units of */
	    pack_putc(dat->code_buf[i], file);  /* code together */

	 if (pack_ferror(file)) {
	    ret = EOF;
	    goto getout;
	 }
	 dat->code_buf[0] = 0;
	 code_buf_ptr = mask = 1;
      }

      last_match_length = dat->match_length;

      for (i=0; (i < last_match_length) && (size > 0); i++) {
	 c = *(buf++);
	 if (--size == 0) {
	    if (!last) {
	       dat->state = 2;
	       goto getout;
	    }
	 }
	 pos2:
	 pack_deletenode(s,dat);    /* delete old strings and */
	 dat->text_buf[s] = c;      /* read new bytes */
	 if (s < F-1)
	    dat->text_buf[s+N] = c; /* if the position is near the end of
				       buffer, extend the buffer to make
				       string comparison easier */
	 s = (s+1) & (N-1);
	 r = (r+1) & (N-1);         /* since this is a ring buffer,
				       increment the position modulo N */

	 pack_insertnode(r,dat);    /* register the string in
				       text_buf[r..r+F-1] */
      }

      while (i++ < last_match_length) {   /* after the end of text, */
	 pack_deletenode(s,dat);          /* no need to read, but */
	 s = (s+1) & (N-1);               /* buffer may not be empty */
	 r = (r+1) & (N-1);
	 if (--len)
	    pack_insertnode(r,dat); 
      }

   } while (len > 0);   /* until length of string to be processed is zero */

   if (code_buf_ptr > 1) {         /* send remaining code */
      if ((file->passpos) && (file->flags & PACKFILE_FLAG_OLD_CRYPT)) {
	 dat->code_buf[0] ^= *file->passpos;
	 file->passpos++;
	 if (!*file->passpos)
	    file->passpos = file->passdata;
      };

      for (i=0; i<code_buf_ptr; i++) {
	 pack_putc(dat->code_buf[i], file);
	 if (pack_ferror(file)) {
	    ret = EOF;
	    goto getout;
	 }
      }
   }

   dat->state = 0;

   getout:

   dat->i = i;
   dat->c = c;
   dat->len = len;
   dat->r = r;
   dat->s = s;
   dat->last_match_length = last_match_length;
   dat->code_buf_ptr = code_buf_ptr;
   dat->mask = mask;

   return ret;
}



/* pack_read:
 *  Called by refill_buffer(). Unpacks from dat into buf, until either
 *  EOF is reached or s bytes have been extracted. Returns the number of
 *  bytes added to the buffer
 */
static int pack_read(PACKFILE *file, UNPACK_DATA *dat, int s, unsigned char *buf)
{
   int i = dat->i;
   int j = dat->j;
   int k = dat->k;
   int r = dat->r;
   int c = dat->c;
   unsigned int flags = dat->flags;
   int size = 0;

   if (dat->state==2)
      goto pos2;
   else
      if (dat->state==1)
	 goto pos1;

   r = N-F;
   flags = 0;

   for (;;) {
      if (((flags >>= 1) & 256) == 0) {
	 if ((c = pack_getc(file)) == EOF)
	    break;

	 if ((file->passpos) && (file->flags & PACKFILE_FLAG_OLD_CRYPT)) {
	    c ^= *file->passpos;
	    file->passpos++;
	    if (!*file->passpos)
	       file->passpos = file->passdata;
	 };

	 flags = c | 0xFF00;        /* uses higher byte to count eight */
      }

      if (flags & 1) {
	 if ((c = pack_getc(file)) == EOF)
	    break;
	 dat->text_buf[r++] = c;
	 r &= (N - 1);
	 *(buf++) = c;
	 if (++size >= s) {
	    dat->state = 1;
	    goto getout;
	 }
	 pos1:
	    ; 
      }
      else {
	 if ((i = pack_getc(file)) == EOF)
	    break;
	 if ((j = pack_getc(file)) == EOF)
	    break;
	 i |= ((j & 0xF0) << 4);
	 j = (j & 0x0F) + THRESHOLD;
	 for (k=0; k <= j; k++) {
	    c = dat->text_buf[(i + k) & (N - 1)];
	    dat->text_buf[r++] = c;
	    r &= (N - 1);
	    *(buf++) = c;
	    if (++size >= s) {
	       dat->state = 2;
	       goto getout;
	    }
	    pos2:
	       ; 
	 }
      }
   }

   dat->state = 0;

   getout:

   dat->i = i;
   dat->j = j;
   dat->k = k;
   dat->r = r;
   dat->c = c;
   dat->flags = flags;

   return size;
}




/* _al_file_isok:
 *  Helper function to check if it is safe to access a file on a floppy
 *  drive. This really only applies to the DOS library, so we don't bother
 *  with it.
 */
int _al_file_isok(const char *filename)
{
   return TRUE;
}



/* _al_file_exists:
 *  Checks whether the specified file exists.
 */
int _al_file_exists(const char *filename, int attrib, int *aret)
{
   struct _finddata_t info;
   long handle;

   errno = 0;

   if ((handle = _findfirst(filename, &info)) < 0) {
      return FALSE;
   }

   _findclose(handle);

   if (aret)
      *aret = info.attrib;

   info.attrib &= (FA_HIDDEN | FA_SYSTEM | FA_LABEL | FA_DIREC);

   if ((info.attrib & attrib) != info.attrib)
      return FALSE;

   return TRUE;
}



/* _al_file_size:
 *  Measures the size of the specified file.
 */
long _al_file_size(const char *filename)
{
   struct _finddata_t info;
   long handle;

   errno = 0;

   if ((handle = _findfirst(filename, &info)) < 0) {
      return 0;
   }

   _findclose(handle);

   if (info.attrib & (FA_SYSTEM | FA_LABEL | FA_DIREC))
      return 0;

   return info.size;
}



/* _al_file_time:
 *  Returns the timestamp of the specified file.
 */
time_t _al_file_time(const char *filename)
{
   struct _finddata_t info;
   long handle;

   errno = 0;

   if ((handle = _findfirst(filename, &info)) < 0) {
      return 0;
   }

   _findclose(handle);

   if (info.attrib & (FA_SYSTEM | FA_LABEL | FA_DIREC))
      return 0;

   return info.time_write;
}



/* information structure for use by the directory scanning routines */
typedef struct FFIND_INFO {
   struct _finddata_t info;
   long handle;
   int attrib;
} FFIND_INFO;



/* _al_findfirst:
 *  Initiates a directory search.
 */
void *_al_findfirst(const char *name, int attrib, char *nameret, int *aret)
{
   FFIND_INFO *info;
   int a;

   info = malloc(sizeof(FFIND_INFO));

   if (!info) {
      errno = ENOMEM;
      return NULL;
   }

   info->attrib = attrib;

   errno = 0;

   if ((info->handle = _findfirst(name, &info->info)) < 0) {
      free(info);
      return NULL;
   }

   a = info->info.attrib & (FA_HIDDEN | FA_SYSTEM | FA_LABEL | FA_DIREC);

   if ((a & attrib) != a) {
      if (_al_findnext(info, nameret, aret) != 0) {
	 _findclose(info->handle);
	 free(info);
	 return NULL;
      }
      else
	 return info;
   }

   strcpy(nameret, info->info.name);

   if (aret)
      *aret = info->info.attrib;

   return info;
}



/* _al_findnext:
 *  Retrieves the next file from a directory search.
 */
int _al_findnext(void *dta, char *nameret, int *aret)
{
   FFIND_INFO *info = (FFIND_INFO *) dta;
   int a;

   do {
      if (_findnext(info->handle, &info->info) != 0) {
	 return -1;
      }

      a = info->info.attrib & (FA_HIDDEN | FA_SYSTEM | FA_LABEL | FA_DIREC);

   } while ((a & info->attrib) != a);

   strcpy(nameret, info->info.name);

   if (aret)
      *aret = info->info.attrib;

   return 0;
}



/* _al_findclose:
 *  Cleans up after a directory search.
 */
void _al_findclose(void *dta)
{
   FFIND_INFO *info = (FFIND_INFO *) dta;

   _findclose(info->handle);
   free(info);
}



/* _al_getdrive:
 *  Returns the current drive number (0=A, 1=B, etc).
 */
int _al_getdrive()
{
   return _getdrive() - 1;
}



/* _al_getdcwd:
 *  Returns the current directory on the specified drive.
 */
void _al_getdcwd(int drive, char *buf, int size)
{
   char tmp[256];

   if (_getdcwd(drive+1, tmp, sizeof(tmp)))
      strcpy(buf, tmp);
   else
      buf[0] = 0;
}

