 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Communication between threads
  *
  * Copyright 1997, 2001 Bernd Schmidt
  */

typedef union {
    int i;
    uae_u32 u32;
    void *pv;
} uae_pt;

/* These currently require the maximum size to be known at initialization
 * time, but it wouldn't be hard to use a "normal" pipe as an extension once the
 * user-level one gets full.
 * We queue up to chunks pieces of data before signalling the other thread to
 * avoid overhead. */

typedef struct {
    uae_sem_t lock;
    uae_sem_t reader_wait;
    uae_sem_t writer_wait;
    uae_pt *data;
    int size, chunks;
    volatile int rdp, wrp;
    volatile int writer_waiting;
    volatile int reader_waiting;
} smp_comm_pipe;

static inline void init_comm_pipe (smp_comm_pipe *p, int size, int chunks)
{
    p->data = (uae_pt *)malloc (size*sizeof (uae_pt));
    p->size = size;
    p->chunks = chunks;
    p->rdp = p->wrp = 0;
    p->reader_waiting = 0;
    p->writer_waiting = 0;
    uae_sem_init (&p->lock, 0, 1);
    uae_sem_init (&p->reader_wait, 0, 0);
    uae_sem_init (&p->writer_wait, 0, 0);
}

static inline void destroy_comm_pipe (smp_comm_pipe *p)
{
    uae_sem_destroy (&p->lock);
    uae_sem_destroy (&p->reader_wait);
    uae_sem_destroy (&p->writer_wait);
}

static inline void maybe_wake_reader (smp_comm_pipe *p, int no_buffer)
{
    if (p->reader_waiting
	&& (no_buffer || ((p->wrp - p->rdp + p->size) % p->size) >= p->chunks))
    {
	p->reader_waiting = 0;
	uae_sem_post (&p->reader_wait);
    }
}

static inline void write_comm_pipe_pt (smp_comm_pipe *p, uae_pt data, int no_buffer)
{
    int nxwrp = (p->wrp + 1) % p->size;

    if (p->reader_waiting) {
	/* No need to do all the locking */
	p->data[p->wrp] = data;
	p->wrp = nxwrp;
	maybe_wake_reader (p, no_buffer);
	return;
    }
    
    uae_sem_wait (&p->lock);
    if (nxwrp == p->rdp) {
	/* Pipe full! */
	p->writer_waiting = 1;
	uae_sem_post (&p->lock);
	/* Note that the reader could get in between here and do a
	 * sem_post on writer_wait before we wait on it. That's harmless.
	 * There's a similar case in read_comm_pipe_int_blocking. */
	uae_sem_wait (&p->writer_wait);
	uae_sem_wait (&p->lock);
    }
    p->data[p->wrp] = data;
    p->wrp = nxwrp;
    maybe_wake_reader (p, no_buffer);
    uae_sem_post (&p->lock);
}

static inline uae_pt read_comm_pipe_pt_blocking (smp_comm_pipe *p)
{
    uae_pt data;

    uae_sem_wait (&p->lock);
    if (p->rdp == p->wrp) {
	p->reader_waiting = 1;
	uae_sem_post (&p->lock);
	uae_sem_wait (&p->reader_wait);
	uae_sem_wait (&p->lock);
    }
    data = p->data[p->rdp];
    p->rdp = (p->rdp + 1) % p->size;

    /* We ignore chunks here. If this is a problem, make the size bigger in the init call. */
    if (p->writer_waiting) {
	p->writer_waiting = 0;
	uae_sem_post (&p->writer_wait);
    }
    uae_sem_post (&p->lock);
    return data;
}

static inline int comm_pipe_has_data (smp_comm_pipe *p)
{
    return p->rdp != p->wrp;
}

static inline int read_comm_pipe_int_blocking (smp_comm_pipe *p)
{
    uae_pt foo = read_comm_pipe_pt_blocking (p);
    return foo.i;
}
static inline uae_u32 read_comm_pipe_u32_blocking (smp_comm_pipe *p)
{
    uae_pt foo = read_comm_pipe_pt_blocking (p);
    return foo.u32;
}

static inline void *read_comm_pipe_pvoid_blocking (smp_comm_pipe *p)
{
    uae_pt foo = read_comm_pipe_pt_blocking (p);
    return foo.pv;
}

static inline void write_comm_pipe_int (smp_comm_pipe *p, int data, int no_buffer)
{
    uae_pt foo;
    foo.i = data;
    write_comm_pipe_pt (p, foo, no_buffer);
}

static inline void write_comm_pipe_u32 (smp_comm_pipe *p, int data, int no_buffer)
{
    uae_pt foo;
    foo.u32 = data;
    write_comm_pipe_pt (p, foo, no_buffer);
}

static inline void write_comm_pipe_pvoid (smp_comm_pipe *p, void *data, int no_buffer)
{
    uae_pt foo;
    foo.pv = data;
    write_comm_pipe_pt (p, foo, no_buffer);
}
