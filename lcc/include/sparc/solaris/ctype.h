#ifndef __CTYPE
#define __CTYPE

extern int isalnum(int);
extern int isalpha(int);
extern int iscntrl(int);
extern int isdigit(int);
extern int isgraph(int);
extern int islower(int);
extern int isprint(int);
extern int ispunct(int);
extern int isspace(int);
extern int isupper(int);
extern int isxdigit(int);
extern int tolower(int);
extern int toupper(int);

#define	__U	01
#define	__L	02
#define	__N	04
#define	__S	010
#define	__P	020
#define	__C	040
#define	_U	01
#define	_L	02
#define	_N	04
#define	_S	010
#define	_P	020
#define	_C	040
#define	_B	0100
#define	_X	0200

extern unsigned char __ctype[];
#define	isalpha(c)	((__ctype + 1)[c] & (_U | _L))
#define	isupper(c)	((__ctype + 1)[c] & _U)
#define	islower(c)	((__ctype + 1)[c] & _L)
#define	isdigit(c)	((__ctype + 1)[c] & _N)
#define	isxdigit(c)	((__ctype + 1)[c] & _X)
#define	isalnum(c)	((__ctype + 1)[c] & (_U | _L | _N))
#define	isspace(c)	((__ctype + 1)[c] & _S)
#define	ispunct(c)	((__ctype + 1)[c] & _P)
#define	isprint(c)	((__ctype + 1)[c] & (_P | _U | _L | _N | _B))
#define	isgraph(c)	((__ctype + 1)[c] & (_P | _U | _L | _N))
#define	iscntrl(c)	((__ctype + 1)[c] & _C)

#endif /* __CTYPE */
