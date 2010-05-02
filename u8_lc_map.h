/* C code produced by gperf version 3.0.4 */
/* Command-line: gperf -c -t -H u8_lc_hash -N u8_lc_in_word_set u8_lc_map.txt  */
/* Computed positions: -k'1-2' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 1 "u8_lc_map.txt"
struct u8_case_map_t {
    const char *name;
    const char *lower;
};

#define TOTAL_KEYWORDS 49
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 2
#define MIN_HASH_VALUE 0
#define MAX_HASH_VALUE 67
/* maximum key range = 68, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
/*ARGSUSED*/
static unsigned int
u8_lc_hash (str, len)
     register const char *str;
     register unsigned int len;
{
  static unsigned char asso_values[] =
    {
      68, 68, 68, 68, 68, 68, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 68, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 68, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 68, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 68, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 68, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 68, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 68, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 68, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 68, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 68, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 68, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 68, 68, 68, 33, 60,
      68, 68, 28, 23, 18, 13,  8,  3, 62, 68,
      68, 61, 68, 68, 62, 50, 57, 40, 52, 47,
      30, 42, 20, 37, 10, 32,  0, 27, 22, 17,
      12,  7,  2, 61, 56, 51, 46, 41, 36, 31,
      26, 21, 16, 11,  6,  1, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 68, 68, 68, 68, 68,
      68, 68, 68, 68, 68,  5, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 68, 68, 68,  0, 68,
      68, 68, 68, 68, 68, 68, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 68, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 68, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 68, 68, 68, 68, 68,
      68, 68, 68, 68, 68, 68
    };
  return asso_values[(unsigned char)str[1]] + asso_values[(unsigned char)str[0]];
}

#ifdef __GNUC__
__inline
#if defined __GNUC_STDC_INLINE__ || defined __GNUC_GNU_INLINE__
__attribute__ ((__gnu_inline__))
#endif
#endif
struct u8_case_map_t *
u8_lc_in_word_set (str, len)
     register const char *str;
     register unsigned int len;
{
  static struct u8_case_map_t wordlist[] =
    {
#line 35 "u8_lc_map.txt"
      {"\320\234", "м"},
#line 54 "u8_lc_map.txt"
      {"\320\257", "я"},
#line 41 "u8_lc_map.txt"
      {"\320\242", "т"},
      {""}, {""},
#line 12 "u8_lc_map.txt"
      {"\303\234", "ü"},
#line 53 "u8_lc_map.txt"
      {"\320\256", "ю"},
#line 40 "u8_lc_map.txt"
      {"\320\241", "с"},
#line 7 "u8_lc_map.txt"
      {"\303\211", "é"},
      {""},
#line 33 "u8_lc_map.txt"
      {"\320\232", "к"},
#line 52 "u8_lc_map.txt"
      {"\320\255", "э"},
#line 38 "u8_lc_map.txt"
      {"\320\240", "р"},
#line 20 "u8_lc_map.txt"
      {"\303\210", "è"},
      {""},
#line 11 "u8_lc_map.txt"
      {"\303\232", "ú"},
#line 51 "u8_lc_map.txt"
      {"\320\254", "ь"},
#line 39 "u8_lc_map.txt"
      {"\320\237", "п"},
#line 19 "u8_lc_map.txt"
      {"\303\207", "ç"},
      {""},
#line 31 "u8_lc_map.txt"
      {"\320\230", "и"},
#line 50 "u8_lc_map.txt"
      {"\320\253", "ы"},
#line 37 "u8_lc_map.txt"
      {"\320\236", "о"},
#line 16 "u8_lc_map.txt"
      {"\303\206", "æ"},
      {""},
#line 17 "u8_lc_map.txt"
      {"\303\230", "ø"},
#line 49 "u8_lc_map.txt"
      {"\320\252", "ъ"},
#line 36 "u8_lc_map.txt"
      {"\320\235", "н"},
#line 15 "u8_lc_map.txt"
      {"\303\205", "å"},
      {""},
#line 29 "u8_lc_map.txt"
      {"\320\226", "ж"},
#line 48 "u8_lc_map.txt"
      {"\320\251", "щ"},
#line 34 "u8_lc_map.txt"
      {"\320\233", "л"},
#line 13 "u8_lc_map.txt"
      {"\303\204", "ä"},
      {""},
#line 14 "u8_lc_map.txt"
      {"\303\226", "ö"},
#line 47 "u8_lc_map.txt"
      {"\320\250", "ш"},
#line 32 "u8_lc_map.txt"
      {"\320\231", "й"},
#line 18 "u8_lc_map.txt"
      {"\303\200", "à"},
      {""},
#line 25 "u8_lc_map.txt"
      {"\320\223", "г"},
#line 46 "u8_lc_map.txt"
      {"\320\247", "ч"},
#line 30 "u8_lc_map.txt"
      {"\320\227", "з"},
      {""}, {""},
#line 10 "u8_lc_map.txt"
      {"\303\223", "ó"},
#line 45 "u8_lc_map.txt"
      {"\320\246", "ц"},
#line 27 "u8_lc_map.txt"
      {"\320\225", "е"},
      {""}, {""},
#line 23 "u8_lc_map.txt"
      {"\320\221", "б"},
#line 44 "u8_lc_map.txt"
      {"\320\245", "х"},
#line 26 "u8_lc_map.txt"
      {"\320\224", "д"},
      {""}, {""},
#line 9 "u8_lc_map.txt"
      {"\303\221", "ñ"},
#line 43 "u8_lc_map.txt"
      {"\320\244", "ф"},
#line 24 "u8_lc_map.txt"
      {"\320\222", "в"},
      {""}, {""},
#line 28 "u8_lc_map.txt"
      {"\320\201", "ё"},
#line 42 "u8_lc_map.txt"
      {"\320\243", "у"},
#line 22 "u8_lc_map.txt"
      {"\320\220", "а"},
      {""}, {""},
#line 6 "u8_lc_map.txt"
      {"\303\201", "á"},
#line 8 "u8_lc_map.txt"
      {"\303\215", "í"},
#line 21 "u8_lc_map.txt"
      {"\303\212", "ê"}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = u8_lc_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].name;

          if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
            return &wordlist[key];
        }
    }
  return 0;
}
#line 55 "u8_lc_map.txt"

