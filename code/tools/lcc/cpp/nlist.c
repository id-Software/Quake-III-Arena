#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpp.h"

extern	char	*optarg;
extern	int	optind;
extern	int	verbose;
extern	int	Cplusplus;
Nlist	*kwdefined;
char	wd[128];

#define	NLSIZE	128

static Nlist	*nlist[NLSIZE];

struct	kwtab {
	char	*kw;
	int	val;
	int	flag;
} kwtab[] = {
	{"if",		KIF,		ISKW},
	{"ifdef",	KIFDEF,		ISKW},
	{"ifndef",	KIFNDEF,	ISKW},
	{"elif",		KELIF,		ISKW},
	{"else",		KELSE,		ISKW},
	{"endif",	KENDIF,		ISKW},
	{"include",	KINCLUDE,	ISKW},
	{"define",	KDEFINE,	ISKW},
	{"undef",	KUNDEF,		ISKW},
	{"line",		KLINE,		ISKW},
	{"error",	KERROR,		ISKW},
	{"pragma",	KPRAGMA,	ISKW},
	{"eval",		KEVAL,		ISKW},
	{"defined",	KDEFINED,	ISDEFINED+ISUNCHANGE},
	{"__LINE__",	KLINENO,	ISMAC+ISUNCHANGE},
	{"__FILE__",	KFILE,		ISMAC+ISUNCHANGE},
	{"__DATE__",	KDATE,		ISMAC+ISUNCHANGE},
	{"__TIME__",	KTIME,		ISMAC+ISUNCHANGE},
	{"__STDC__",	KSTDC,		ISUNCHANGE},
	{NULL}
};

unsigned long	namebit[077+1];
Nlist 	*np;

void
setup_kwtab(void)
{
	struct kwtab *kp;
	Nlist *np;
	Token t;
	static Token deftoken[1] = {{ NAME, 0, 0, 0, 7, (uchar*)"defined" }};
	static Tokenrow deftr = { deftoken, deftoken, deftoken+1, 1 };

	for (kp=kwtab; kp->kw; kp++) {
		t.t = (uchar*)kp->kw;
		t.len = strlen(kp->kw);
		np = lookup(&t, 1);
		np->flag = kp->flag;
		np->val = kp->val;
		if (np->val == KDEFINED) {
			kwdefined = np;
			np->val = NAME;
			np->vp = &deftr;
			np->ap = 0;
		}
	}
}

Nlist *
lookup(Token *tp, int install)
{
	unsigned int h;
	Nlist *np;
	uchar *cp, *cpe;

	h = 0;
	for (cp=tp->t, cpe=cp+tp->len; cp<cpe; )
		h += *cp++;
	h %= NLSIZE;
	np = nlist[h];
	while (np) {
		if (*tp->t==*np->name && tp->len==np->len 
		 && strncmp((char*)tp->t, (char*)np->name, tp->len)==0)
			return np;
		np = np->next;
	}
	if (install) {
		np = new(Nlist);
		np->vp = NULL;
		np->ap = NULL;
		np->flag = 0;
		np->val = 0;
		np->len = tp->len;
		np->name = newstring(tp->t, tp->len, 0);
		np->next = nlist[h];
		nlist[h] = np;
		quickset(tp->t[0], tp->len>1? tp->t[1]:0);
		return np;
	}
	return NULL;
}
