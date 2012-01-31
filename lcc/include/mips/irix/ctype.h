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
#define	__B	0100
#define	__X	0200

extern unsigned char _ctype[];
#define	isalnum(c)	((_ctype+1)[c]&(__U|__L|__N))
#define	isalpha(c)	((_ctype+1)[c]&(__U|__L))
#define	iscntrl(c)	((_ctype+1)[c]&__C)
#define	isdigit(c)	((_ctype+1)[c]&__N)
#define	isgraph(c)	((_ctype+1)[c]&(__P|__U|__L|__N))
#define	islower(c)	((_ctype+1)[c]&__L)
#define	isprint(c)	((_ctype+1)[c]&(__P|__U|__L|__N|__B))
#define	ispunct(c)	((_ctype+1)[c]&__P)
#define	isspace(c)	((_ctype+1)[c]&__S)
#define	isupper(c)	((_ctype+1)[c]&__U)
#define	isxdigit(c)	((_ctype+1)[c]&__X)

#endif /* __CTYPE */
