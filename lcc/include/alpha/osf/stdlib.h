#ifndef __STDLIB
#define __STDLIB

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0
#define MB_CUR_MAX 1
#define NULL 0
#define RAND_MAX 32767

typedef struct { int quot, rem; } div_t;
typedef struct { long quot, rem; } ldiv_t;

#if !defined(_SIZE_T) && !defined(_SIZE_T_)
#define _SIZE_T
#define _SIZE_T_
typedef unsigned long size_t;
#endif

#if !defined(_WCHAR_T) && !defined(_WCHAR_T_)
#define _WCHAR_T
#define _WCHAR_T_
typedef unsigned char wchar_t;
#endif

extern double atof(const char *);
extern int atoi(const char *);
extern long int atol(const char *);
extern double strtod(const char *, char **);
extern long int strtol(const char *, char **, int);
extern unsigned long int strtoul(const char *, char **, int);
extern int rand(void);
extern void srand(unsigned int);
extern void *calloc(size_t, size_t);
extern void free(void *);
extern void *malloc(size_t);
extern void *realloc(void *, size_t);
extern void abort(void);
extern int atexit(void (*)(void));
extern void exit(int);
extern char *getenv(const char *);
extern int system(const char *);
extern void *bsearch(const void *, const void *, size_t, size_t, int (*)(const void *, const void *));
extern void qsort(void *, size_t, size_t, int (*)(const void *, const void *));
extern int abs(int);
extern div_t div(int, int);
extern long int labs(long int);
extern ldiv_t ldiv(long int, long int);
extern int mblen(const char *, size_t);
extern int mbtowc(wchar_t *, const char *, size_t);
extern int wctomb(char *, wchar_t);
extern size_t mbstowcs(wchar_t *, const char *, size_t);
extern size_t wcstombs(char *, const wchar_t *, size_t);

#endif /* __STDLIB */
