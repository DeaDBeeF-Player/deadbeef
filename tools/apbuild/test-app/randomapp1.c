#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <fnmatch.h>

int main() {
  char *respath = malloc(PATH_MAX);
  unsigned char c = (unsigned char)"c";
  
  printf("foo\n");
  printf("%d\n", isalpha(c));
  realpath("/some/path", respath);
  fnmatch ("foo", "bar", 0);
  return 0;
}
