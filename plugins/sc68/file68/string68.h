/**
 * @ingroup   file68_devel
 * @file      file68/string68.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      2003/08/11
 * @brief     String functions
 *
 * $Id: string68.h,v 2.0 2003/08/21 04:58:35 benjihan Exp $
 *
 * Common string operations.
 *
 */

#ifndef _STRING_68_H_
#define _STRING_68_H_

#ifdef __cplusplus
extern "C" {
#endif

/** Compare two string (case insensitive).
 *
 *    The SC68strcmp() function compares the two strings a and b,
 *    ignoring the case of the characters. It returns an integer less than,
 *    equal to, or greater than zero if a is found, respectively, to be less
 *    than, to match, or be greater than b.
 *
 *  @param  a  First string to compare
 *  @param  b  String to compare with
 *
 *  @return  Integer result of the two string compare. The difference
 *           between last tested characters of a and b.
 *  @retval  0   a and b are equal
 *  @retval  <0  a is less than b
 *  @retval  >0  a is greater than b
 */
int SC68strcmp(const char *a, const char *b);

/** Concatenate two strings.
 *
 *    The SC68strcat() function appends the b string to the a string
 *    overwriting the 0 character at the end of dest, and then adds a
 *    terminating 0 character. The strings may not overlap. Destination
 *    string has a maximum size of l characters. On overflow, the trailing 0
 *    is omitted.
 *
 *  @param  a  Destination string
 *  @param  b  String to append.
 *  @param  l  Destination maximum size (including trailing 0)
 *
 *  @return  Destination string
 *  @retval  a
 */
char * SC68strcat(char * a, const char * b, int l);

/** Duplicate a string.
 *
 *    The SC68strdup() function returns a pointer to a new string which is a
 *    duplicate of the string s. Memory for the new string is obtained with
 *    SC68alloc(), and can be freed with SC68free().
 *
 *  @param  s  String to duplicate.
 *
 *  @return  Duplicated string
 *  @return  0 error
 */
char * SC68strdup(const char * s);

/** Concat two strings in a duplicate buffer.
 *
 *    The SC68strcatdup() function returns a pointer to a new string which is a
 *    duplicate of the string a+b. Memory for the new string is obtained with
 *    SC68alloc(), and can be freed with SC68free(). If either a or b is null
 *    the function does not failed but replace it by an empty string. If both
 *    a and b are null the function returns 0.
 *
 *  @param   a  Left string.
 *  @param   b  right string.
 *
 *  @return  Duplicated string a+b
 *  @return  0 error
 */
char * SC68strcatdup(const char * a, const char * b);

/** Make a track and time infornmation string.
 *
 *    The SC68time_str() function formats a string with track time info.
 *    The general format is "TK MN:SS" where:
 *     - TK is track_num or "--" if track_num < 0 or track_num > 99
 *     - MN:SS is time minutes and seconds or "--:--" if seconds < 0
 *
 *  @param  buffer     Destination buffer (0 use default static buffer).
 *  @param  track_num  Track number from 00 to 99, minus values disable.
 *  @param  seconds    Time to display in seconds [00..5999], other values
 *                     disable.
 *
 * @return  Pointer to result formatted string in a static buffer.
 *
 * @warning  The function returns a static buffer. Do try to modify it.
 * @warning  Not thread safe.
 *
 * @see SC68utils_make_big_playing_time()
 */
char * SC68time_str(char * buffer, int track_num, int seconds);

/** Convert time (in second) to string.
 *
 *    The SC68long_time_str() function converts a time in
 *    seconds to days, hours, minutes and second string. Day and hour unit
 *    are removed if they are null (not signifiant). The output string looks
 *    like : [D days, ][H h, ] MN' SS"
 *
 *  @param  buffer  Destination buffer (0 use default static buffer).
 *  @param  time    Time in second to convert to string.
 *
 *  @return  pointer to result time string (given buffer or static buffer).
 *
 * @warning  Not thread safe when using static buffer.
 *
 * @see SC68utils_make_track_time_info()
 */
char * SC68long_time_str(char * buffer, int time);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _STRING_68_H_ */
