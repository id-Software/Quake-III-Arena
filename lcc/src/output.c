#include "c.h"


static char *outs(const char *str, FILE *f, char *bp) {
	if (f)
		fputs(str, f);
	else
		while (*bp = *str++)
			bp++;
	return bp;
}

static char *outd(long n, FILE *f, char *bp) {
	unsigned long m;
	char buf[25], *s = buf + sizeof buf;

	*--s = '\0';
	if (n < 0)
		m = -n;
	else
		m = n;
	do
		*--s = m%10 + '0';
	while ((m /= 10) != 0);
	if (n < 0)
		*--s = '-';
	return outs(s, f, bp);
}

static char *outu(unsigned long n, int base, FILE *f, char *bp) {
	char buf[25], *s = buf + sizeof buf;

	*--s = '\0';
	do
		*--s = "0123456789abcdef"[n%base];
	while ((n /= base) != 0);
	return outs(s, f, bp);
}
void print(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vfprint(stdout, NULL, fmt, ap);
	va_end(ap);
}
/* fprint - formatted output to  f */
void fprint(FILE *f, const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vfprint(f, NULL, fmt, ap);
	va_end(ap);
}

/* stringf - formatted output to a saved string */
char *stringf(const char *fmt, ...) {
	char buf[1024];
	va_list ap;

	va_start(ap, fmt);
	vfprint(NULL, buf, fmt, ap);
	va_end(ap);
	return string(buf);
}

/* vfprint - formatted output to f or string bp */
void vfprint(FILE *f, char *bp, const char *fmt, va_list ap) {
	for (; *fmt; fmt++)
		if (*fmt == '%')
			switch (*++fmt) {
			case 'd': bp = outd(va_arg(ap, int), f, bp); break;
			case 'D': bp = outd(va_arg(ap, long), f, bp); break;
			case 'U': bp = outu(va_arg(ap, unsigned long), 10, f, bp); break;
			case 'u': bp = outu(va_arg(ap, unsigned), 10, f, bp); break;
			case 'o': bp = outu(va_arg(ap, unsigned), 8, f, bp); break;
			case 'X': bp = outu(va_arg(ap, unsigned long), 16, f, bp); break;
			case 'x': bp = outu(va_arg(ap, unsigned), 16, f, bp); break;
			case 'f': case 'e':
			case 'g': {
				  	static char format[] = "%f";
				  	char buf[128];
				  	format[1] = *fmt;
				  	sprintf(buf, format, va_arg(ap, double));
				  	bp = outs(buf, f, bp);
				  }
; break;
			case 's': bp = outs(va_arg(ap, char *), f, bp); break;
			case 'p': {
				void *p = va_arg(ap, void *);
				if (p)
					bp = outs("0x", f, bp);
				bp = outu((unsigned long)p, 16, f, bp);
				break;
				  }
			case 'c': if (f) fputc(va_arg(ap, int), f); else *bp++ = va_arg(ap, int); break;
			case 'S': { char *s = va_arg(ap, char *);
				    int n = va_arg(ap, int);
				    if (s)
				    	for ( ; n-- > 0; s++)
				    		if (f) (void)putc(*s, f); else *bp++ = *s;
 } break;
			case 'k': { int t = va_arg(ap, int);
				    static char *tokens[] = {
#define xx(a,b,c,d,e,f,g) g,
#define yy(a,b,c,d,e,f,g) g,
#include "token.h"
				    };
				    assert(tokens[t&0177]);
				    bp = outs(tokens[t&0177], f, bp);
 } break;
			case 't': { Type ty = va_arg(ap, Type);
				    assert(f);
				    outtype(ty ? ty : voidtype, f);
 } break;
			case 'w': { Coordinate *p = va_arg(ap, Coordinate *);
				    if (p->file && *p->file) {
				    	bp = outs(p->file, f, bp);
				    	bp = outs(":", f, bp);
				    }
				    bp = outd(p->y, f, bp);
 } break;
			case 'I': { int n = va_arg(ap, int);
				    while (--n >= 0)
				    	if (f) (void)putc(' ', f); else *bp++ = ' ';
 } break;
			default:  if (f) (void)putc(*fmt, f); else *bp++ = *fmt; break;
			}
		else if (f)
			(void)putc(*fmt, f);
		else
			*bp++ = *fmt;
	if (!f)
		*bp = '\0';
}
