#ifndef __DSA_COMMON_HPP__
#define __DSA_COMMON_HPP__

#if defined (_MSC_VER)
#if defined (_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#endif

namespace dsa {

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef unsigned char  UINT8;
typedef unsigned short UINT16;
typedef unsigned long  UINT32;
typedef signed char  INT8;
typedef signed short INT16;
typedef signed long  INT32;

typedef unsigned int UINT;
typedef signed int INT;

enum RESULT { FAILURE=0, SUCCESS=1 };
// Exception of so-called 'Runtime Errors'.
class RuntimeException { 
  const bool m_fatal;
  const char *m_message;
  const char *m_file;
  const int m_lineno;
public:
  RuntimeException(const char *message, const char *file, int lineno, bool fatal=true) 
    : m_message(message), m_file(file), m_lineno(lineno), m_fatal(fatal) {}
};

} // namespace dsa
#endif // __DSA_COMMON_HPP__