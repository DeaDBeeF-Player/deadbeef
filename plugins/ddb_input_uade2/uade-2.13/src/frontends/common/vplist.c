#include <stdlib.h>
#include <string.h>

#include "vplist.h"


#define VPLIST_BASIC_LENGTH 5


static void shrink_vplist(struct vplist *v, size_t newsize)
{
  size_t ncopied = v->tail - v->head;
  void **newl;
  if (newsize >= v->allocated) {
    fprintf(stderr, "vplist not shrinked.\n");
    return;
  }
  memmove(v->l, &v->l[v->head], ncopied * sizeof(v->l[0]));
  v->head = 0;
  v->tail = ncopied;
  v->allocated = newsize;
  if ((newl = realloc(v->l, v->allocated * sizeof(v->l[0]))) == NULL) {
    fprintf(stderr, "Not enough memory for shrinking vplist.\n");
    exit(-1);
  }
  v->l = newl;
}


void vplist_grow(struct vplist *v)
{
  size_t newsize = v->allocated * 2;
  void **newl;
  if (newsize == 0)
    newsize = VPLIST_BASIC_LENGTH;
  newl = realloc(v->l, newsize * sizeof(v->l[0]));
  if (newl == NULL) {
    fprintf(stderr, "Not enough memory for growing vplist.\n");
    exit(-1);
  }
  v->l = newl;
  v->allocated = newsize;
}


struct vplist *vplist_create(size_t initial_length)
{
  struct vplist *v;
  if ((v = calloc(1, sizeof(*v))) == NULL) {
    fprintf(stderr, "No memory for vplist.\n");
    exit(-1);
  }
  if (initial_length == 0)
    initial_length = VPLIST_BASIC_LENGTH;
  v->allocated = initial_length;
  if ((v->l = malloc(v->allocated * sizeof(v->l[0]))) == NULL) {
    fprintf(stderr, "Can not create a vplist.\n");
    exit(-1);
  }
  return v;
}


void vplist_flush(struct vplist *v)
{
  v->head = v->tail = 0;
  if (v->allocated >= (2 * VPLIST_BASIC_LENGTH))
    shrink_vplist(v, VPLIST_BASIC_LENGTH);
}


void vplist_free(struct vplist *v)
{
  free(v->l);
  memset(v, 0, sizeof(*v));
  free(v);
}


void *vplist_pop_head(struct vplist *v)
{
  void *item;

  if (v->head == v->tail) {
    fprintf(stderr, "Error: can not pop head from an empty vplist.\n");
    exit(-1);
  }

  item = v->l[v->head++];

  /* If 3/4 of a list is unused, free half the list */
  if (v->allocated >= VPLIST_BASIC_LENGTH && v->head >= ((v->allocated / 4) * 3))
    shrink_vplist(v, v->allocated / 2);

  return item;
}


void *vplist_pop_tail(struct vplist *v)
{
  void *item;

  if (v->head == v->tail) {
    fprintf(stderr, "Error: can not pop tail from an empty vplist.\n");
    exit(-1);
  }

  item = v->l[v->tail--];

  /* If 3/4 of a list is unused, free half the list */
  if (v->allocated >= VPLIST_BASIC_LENGTH && v->tail < (v->allocated / 4))
    shrink_vplist(v, v->allocated / 2);

  return item;
}
