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

#endif /* __CTYPE */
