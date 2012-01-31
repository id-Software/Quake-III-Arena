#include "c.h"


static void printtoken(void);
int errcnt   = 0;
int errlimit = 20;
char kind[] = {
#define xx(a,b,c,d,e,f,g) f,
#define yy(a,b,c,d,e,f,g) f,
#include "token.h"
};
int wflag;		/* != 0 to suppress warning messages */

void test(int tok, char set[]) {
	if (t == tok)
		t = gettok();
	else {
		expect(tok);
		skipto(tok, set);
		if (t == tok)
			t = gettok();
	}
}
void expect(int tok) {
	if (t == tok)
		t = gettok();
	else {
		error("syntax error; found");
		printtoken();
		fprint(stderr, " expecting `%k'\n", tok);
	}
}
void error(const char *fmt, ...) {
	va_list ap;

	if (errcnt++ >= errlimit) {
		errcnt = -1;
		error("too many errors\n");
		exit(1);
	}
	va_start(ap, fmt);
	if (firstfile != file && firstfile && *firstfile)
		fprint(stderr, "%s: ", firstfile);
	fprint(stderr, "%w: ", &src);
	vfprint(stderr, NULL, fmt, ap);
	va_end(ap);
}

void skipto(int tok, char set[]) {
	int n;
	char *s;

	assert(set);
	for (n = 0; t != EOI && t != tok; t = gettok()) {
		for (s = set; *s && kind[t] != *s; s++)
			;
		if (kind[t] == *s)
			break;
		if (n++ == 0)
			error("skipping");
		if (n <= 8)
			printtoken();
		else if (n == 9)
			fprint(stderr, " ...");
	}
	if (n > 8) {
		fprint(stderr, " up to");
		printtoken();
	}
	if (n > 0)
		fprint(stderr, "\n");
}
/* fatal - issue fatal error message and exit */
int fatal(const char *name, const char *fmt, int n) {
	print("\n");
	errcnt = -1;
	error("compiler error in %s--", name);
	fprint(stderr, fmt, n);
	exit(EXIT_FAILURE);
	return 0;
}

/* printtoken - print current token preceeded by a space */
static void printtoken(void) {
	switch (t) {
	case ID: fprint(stderr, " `%s'", token); break;
	case ICON:
		fprint(stderr, " `%s'", vtoa(tsym->type, tsym->u.c.v));
		break;
	case SCON: {
		int i, n;
		if (ischar(tsym->type->type)) {
			char *s = tsym->u.c.v.p;
			n = tsym->type->size;
			fprint(stderr, " \"");
			for (i = 0; i < 20 && i < n && *s; s++, i++)
				if (*s < ' ' || *s >= 0177)
					fprint(stderr, "\\%o", *s);
				else
					fprint(stderr, "%c", *s);
		} else {	/* wchar_t string */
			unsigned int *s = tsym->u.c.v.p;
			assert(tsym->type->type->size == widechar->size);
			n = tsym->type->size/widechar->size;
			fprint(stderr, " L\"");
			for (i = 0; i < 20 && i < n && *s; s++, i++)
				if (*s < ' ' || *s >= 0177)
					fprint(stderr, "\\x%x", *s);
				else
					fprint(stderr, "%c", *s);
		}
		if (i < n)
			fprint(stderr, " ...");
		else
			fprint(stderr, "\"");
		break;
		}
	case FCON:
		fprint(stderr, " `%S'", token, (char*)cp - token);
		break;
	case '`': case '\'': fprint(stderr, " \"%k\"", t); break;
	default: fprint(stderr, " `%k'", t);
	}
}

/* warning - issue warning error message */
void warning(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	if (wflag == 0) {
		errcnt--;
		error("warning: ");
		vfprint(stderr, NULL, fmt, ap);
	}
	va_end(ap);
}
