#ifndef __ASSERT
#define __ASSERT

void assert(int);

#endif /* __ASSERT */

#undef assert
#ifdef NDEBUG
#define assert(ignore) ((void)0)
#else
extern void __assert(char *, char *, unsigned);
#define assert(e) ((void)((e)||(__assert(#e, __FILE__, __LINE__),0)))
#endif /* NDEBUG */
