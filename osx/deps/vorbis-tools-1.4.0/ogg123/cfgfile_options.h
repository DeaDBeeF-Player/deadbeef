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

 last mod: $Id: cfgfile_options.h 16825 2010-01-27 04:14:08Z xiphmont $

 ********************************************************************/

#ifndef __CFGFILE_OPTIONS_H__
#define __CFGFILE_OPTIONS_H__

#include <stdio.h>

typedef enum {
  opt_type_none = 0,
  opt_type_bool,
  opt_type_char,
  opt_type_string,
  opt_type_int,
  opt_type_float,
  opt_type_double
} file_option_type_t;

typedef enum {
  parse_ok = 0,
  parse_syserr,
  parse_keynotfound,
  parse_nokey,
  parse_badvalue,
  parse_badtype
} parse_code_t;

typedef struct file_option_t {
  char found;
  const char *name;
  const char *desc;
  file_option_type_t type;
  void *ptr;
  void *dfl;
} file_option_t;

void file_options_init (file_option_t opts[]);
void file_options_describe (file_option_t opts[], FILE *outfile);

parse_code_t parse_line (file_option_t opts[], char *line);
parse_code_t parse_config_file (file_option_t opts[], const char *filename);
const char *parse_error_string (parse_code_t pcode);
void parse_std_configs (file_option_t opts[]);

#endif /* __OPTIONS_H__ */
