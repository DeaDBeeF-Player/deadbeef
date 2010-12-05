 /*
  * UAE - The Un*x Amiga Emulator
  *
  * m68k -> i386 compiler
  *
  * (c) 1995 Bernd Schmidt
  */

typedef uaecptr (*code_execfunc)(void);

struct code_page {
    struct code_page *next;
    uae_u32 allocmask;
};

struct hash_block {
    struct hash_block *lru_next, *lru_prev;
    struct hash_entry *he_first;

    struct code_page *cpage;
    int alloclen;
    uae_u32 page_allocmask;
    char *compile_start;

    int nrefs;

    int translated:1;
    int untranslatable:1;
    int allocfailed:1;
};

struct hash_entry {
    code_execfunc execute; /* For the sake of the stubs in X86.S */
    struct hash_entry *next,*prev;
    struct hash_entry *next_same_block, *lru_next, *lru_prev;
    struct hash_block *block;

    uaecptr addr;
    uae_u32 matchword;
    int ncalls:8;
    int locked:1;
    int cacheflush:1;
};

extern int nr_bbs_start;
extern uae_u8 nr_bbs_to_run;
extern code_execfunc exec_me;

#ifdef USE_COMPILER
static inline void run_compiled_code(void)
{

    /*if (regs.spcflags == SPCFLAG_EXEC && may_run_compiled) {*/
	while (regs.spcflags == SPCFLAG_EXEC) {
	    uaecptr newpc;
	    regs.spcflags = 0;
	    /*		newpc = (*exec_me)();*/
	    __asm__ __volatile__ ("pushl %%ebp; call *%1; popl %%ebp" : "=a" (newpc) : "r" (exec_me) :
				  "%eax", "%edx", "%ecx", "%ebx",
				  "%edi", "%esi", "memory", "cc");
	    if (nr_bbs_to_run == 0) {
		struct hash_entry *h = (struct hash_entry *)newpc;
		regs.spcflags = SPCFLAG_EXEC;
		exec_me = h->execute;
		regs.pc = h->addr;
		regs.pc_p = regs.pc_oldp = get_real_address(h->addr);
		nr_bbs_to_run = nr_bbs_start;
	    } else
		m68k_setpc_fast(newpc);
	    do_cycles();
	}
/*} else */
	regs.spcflags &= ~SPCFLAG_EXEC;
}

extern void compiler_init(void);
extern void possible_loadseg(void);

extern void m68k_do_rts(void);
extern void m68k_do_bsr(uaecptr, uae_s32);
extern void m68k_do_jsr(uaecptr, uaecptr);
extern void compiler_flush_jsr_stack(void);

#else

#define run_compiled_code() do { ; } while (0)
#define compiler_init() do { ; } while (0)
#define possible_loadseg() do { ; } while (0)
#define compiler_flush_jsr_stack() do { ; } while (0)

static inline void m68k_do_rts(void)
{
    m68k_setpc(get_long(m68k_areg(regs, 7)));
    m68k_areg(regs, 7) += 4;
}

static inline void m68k_do_bsr(uaecptr oldpc, uae_s32 offset)
{
    m68k_areg(regs, 7) -= 4;
    put_long(m68k_areg(regs, 7), oldpc);
    m68k_incpc(offset);
}

static inline void m68k_do_jsr(uaecptr oldpc, uaecptr dest)
{
    m68k_areg(regs, 7) -= 4;
    put_long(m68k_areg(regs, 7), oldpc);
    m68k_setpc(dest);
}

#endif
