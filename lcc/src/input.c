#include "c.h"


static void pragma(void);
static void resynch(void);

static int bsize;
static unsigned char buffer[MAXLINE+1 + BUFSIZE+1];
unsigned char *cp;	/* current input character */
char *file;		/* current input file name */
char *firstfile;	/* first input file */
unsigned char *limit;	/* points to last character + 1 */
char *line;		/* current line */
int lineno;		/* line number of current line */

void nextline(void) {
	do {
		if (cp >= limit) {
			fillbuf();
			if (cp >= limit)
				cp = limit;
			if (cp == limit)
				return;
		} else {
			lineno++;
			for (line = (char *)cp; *cp==' ' || *cp=='\t'; cp++)
				;
			if (*cp == '#') {
				resynch();
				nextline();
			}
		}
	} while (*cp == '\n' && cp == limit);
}
void fillbuf(void) {
	if (bsize == 0)
		return;
	if (cp >= limit)
		cp = &buffer[MAXLINE+1];
	else
		{
			int n = limit - cp;
			unsigned char *s = &buffer[MAXLINE+1] - n;
			assert(s >= buffer);
			line = (char *)s - ((char *)cp - line);
			while (cp < limit)
				*s++ = *cp++;
			cp = &buffer[MAXLINE+1] - n;
		}
	if (feof(stdin))
		bsize = 0;
	else
		bsize = fread(&buffer[MAXLINE+1], 1, BUFSIZE, stdin);
	if (bsize < 0) {
		error("read error\n");
		exit(EXIT_FAILURE);
	}
	limit = &buffer[MAXLINE+1+bsize];
	*limit = '\n';
}
void input_init(int argc, char *argv[]) {
	static int inited;

	if (inited)
		return;
	inited = 1;
	main_init(argc, argv);
	limit = cp = &buffer[MAXLINE+1];
	bsize = -1;
	lineno = 0;
	file = NULL;
	fillbuf();
	if (cp >= limit)
		cp = limit;
	nextline();
}

/* pragma - handle #pragma ref id... */
static void pragma(void) {
	if ((t = gettok()) == ID && strcmp(token, "ref") == 0)
		for (;;) {
			while (*cp == ' ' || *cp == '\t')
				cp++;
			if (*cp == '\n' || *cp == 0)
				break;
			if ((t = gettok()) == ID && tsym) {
				tsym->ref++;
				use(tsym, src);
			}	
		}
}

/* resynch - set line number/file name in # n [ "file" ] and #pragma ... */
static void resynch(void) {
	for (cp++; *cp == ' ' || *cp == '\t'; )
		cp++;
	if (limit - cp < MAXLINE)
		fillbuf();
	if (strncmp((char *)cp, "pragma", 6) == 0) {
		cp += 6;
		pragma();
	} else if (*cp >= '0' && *cp <= '9') {
	line:	for (lineno = 0; *cp >= '0' && *cp <= '9'; )
			lineno = 10*lineno + *cp++ - '0';
		lineno--;
		while (*cp == ' ' || *cp == '\t')
			cp++;
		if (*cp == '"') {
			file = (char *)++cp;
			while (*cp && *cp != '"' && *cp != '\n')
				cp++;
			file = stringn(file, (char *)cp - file);
			if (*cp == '\n')
				warning("missing \" in preprocessor line\n");
			if (firstfile == 0)
				firstfile = file;
		}
	} else if (strncmp((char *)cp, "line", 4) == 0) {
		for (cp += 4; *cp == ' ' || *cp == '\t'; )
			cp++;
		if (*cp >= '0' && *cp <= '9')
			goto line;
		if (Aflag >= 2)
			warning("unrecognized control line\n");
	} else if (Aflag >= 2 && *cp != '\n')
		warning("unrecognized control line\n");
	while (*cp)
		if (*cp++ == '\n')
			if (cp == limit + 1)
				nextline();
			else
				break;
}

