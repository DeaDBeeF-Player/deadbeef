#ifndef _SHD_VPLIST_H_
#define _SHD_VPLIST_H_

#include <stdio.h>
#include <assert.h>


struct vplist {
  size_t head;
  size_t tail;
  size_t allocated;
  void **l;
};


struct vplist *vplist_create(size_t initial_length);
void vplist_flush(struct vplist *v);
void vplist_free(struct vplist *v);
void vplist_grow(struct vplist *v);
void *vplist_pop_head(struct vplist *v);
void *vplist_pop_tail(struct vplist *v);


static inline void vplist_append(struct vplist *v, void *item)
{
  if (v->tail == v->allocated)
    vplist_grow(v);
  v->l[v->tail++] = item;
}


static inline void *vplist_get(struct vplist *v, size_t i)
{
  assert(i < (v->tail - v->head));
  return v->l[v->head + i];
}


static inline size_t vplist_len(struct vplist *v)
{
  return v->tail - v->head;
}

#endif
