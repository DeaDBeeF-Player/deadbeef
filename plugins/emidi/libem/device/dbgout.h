#ifndef _DBGOUT_H_
#include <windows.h>
static void DEBUG_OUT(const char *format, ... )
{
  static char buf[1024];
  va_list argl;

  va_start(argl,format);
  _vsnprintf(buf,1024,format,argl);
  OutputDebugString(buf);
  va_end(argl);
}
#endif