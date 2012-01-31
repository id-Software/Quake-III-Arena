#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* bprint [ -c | -Idir... | -f | -b | -n ] [ file... ]
 * annotate listings of files with prof.out data
 */


#define NDIRS (sizeof dirs/sizeof dirs[0] - 1)
#define NEW(p,a) ((p) = alloc(sizeof *(p)))
#define newarray(m,n,a) alloc((m)*(n))
#define NELEMS(a) ((int)(sizeof (a)/sizeof ((a)[0])))

#define MAXTOKEN 64

struct count {			/* count data: */
	int x, y;			/* source coordinate */
	int count;			/* associated execution count */
};

char *progname;
int number;
char *dirs[20];
int fcount;

struct file {			/* per-file prof.out data: */
	struct file *link;		/* link to next file */
	char *name;			/* file name */
	int size;			/* size of counts[] */
	int count;			/* counts[0..count-1] hold valid data */
	struct count *counts;		/* count data */
	struct func {			/* function data: */
		struct func *link;		/* link to next function */
		char *name;			/* function name */
		struct count count;		/* total number of calls */
		struct caller {		/* caller data: */
			struct caller *link;	/* link to next caller */
			char *name;		/* caller's name */
			char *file;		/* call site: file, x, y */
			int x, y;
			int count;		/* number of calls from this site */
		} *callers;
	} *funcs;			/* list of functions */
} *filelist;
FILE *fp;

extern int process(char *);
extern int findfunc(char *, char *);
extern int findcount(char *, int, int);

void *alloc(unsigned);
char *string(char *);
int process(char *);
void emitdata(char *);
void printfile(struct file *, int);
void printfuncs(struct file *, int);

/* alloc - allocate n bytes or die */
void *alloc(unsigned n) {
	void *new = malloc(n);

	assert(new);
	return new;
}

/* emitdata - write prof.out data to file */
void emitdata(char *file) {
	FILE *fp;

	if (fp = fopen(file, "w")) {
		struct file *p;
		for (p = filelist; p; p = p->link) {
			int i;
			struct func *q;
			struct caller *r;
			fprintf(fp, "1\n%s\n", p->name);
			for (i = 0, q = p->funcs; q; i++, q = q->link)
				if (r = q->callers)
					for (i--; r; r = r->link)
						i++;
			fprintf(fp, "%d\n", i);
			for (q = p->funcs; q; q = q->link)
				if (q->count.count == 0 || !q->callers)
					fprintf(fp, "%s 1 %d %d %d ? ? 0 0\n", q->name, q->count.x,
						q->count.y, q->count.count);
				else
					for (r = q->callers; r; r = r->link)
						fprintf(fp, "%s 1 %d %d %d %s %s %d %d\n", q->name, q->count.x,
							q->count.y, r->count, r->name, r->file, r->x, r->y);
			fprintf(fp, "%d\n", p->count);
			for (i = 0; i < p->count; i++)
				fprintf(fp, "1 %d %d %d\n", p->counts[i].x,
					p->counts[i].y, p->counts[i].count);
		}
		fclose(fp);
	} else
		fprintf(stderr, "%s: can't create `%s'\n", progname, file);
}

/* openfile - open name for reading, searching -I directories */
FILE *openfile(char *name) {
	int i;
	FILE *fp;

	if (*name != '/')	
		for (i = 0; dirs[i]; i++) {
			char buf[200];
			sprintf(buf, "%s/%s", dirs[i], name);
			if (fp = fopen(buf, "r"))
				return fp;
		}
	return fopen(name, "r");
}

/* printfile - print annotated listing for p */
void printfile(struct file *p, int nf) {
	int lineno;
	FILE *fp;
	char *s, buf[512];
	struct count *u = p->counts, *r, *uend;

	if (u == 0 || p->count <= 0)
		return;
	uend = &p->counts[p->count];
	if ((fp = openfile(p->name)) == NULL) {
		fprintf(stderr, "%s: can't open `%s'\n", progname, p->name);
		return;
	}
	if (nf)
		printf("%s%s:\n\n", nf == 1 ? "" : "\f", p->name);
	for (lineno = 1; fgets(buf, sizeof buf, fp); lineno++) {
		if (number)
			printf("%d\t", lineno);
		while (u < uend && u->y < lineno)
			u++;
		for (s = buf; *s; ) {
			char *t = s + 1;
			while (u < uend && u->y == lineno && u->x < s - buf)
				u++;
			if (isalnum(*s) || *s == '_')
				while (isalnum(*t) || *t == '_')
					t++;
			while (u < uend && u->y == lineno && u->x < t - buf) {
				printf("<%d>", u->count);
				for (r = u++; u < uend && u->x == r->x && u->y == r->y && u->count == r->count; u++)
					;
			}
			while (s < t)
				putchar(*s++);
		}
		if (*s)
			printf("%s", s);
	}
	fclose(fp);
}

/* printfuncs - summarize data for functions in p */
void printfuncs(struct file *p, int nf) {
	struct func *q;

	if (nf)
		printf("%s:\n", p->name);
	for (q = p->funcs; q; q = q->link)
		if (fcount <= 1 || q->count.count == 0 || !q->callers)
			printf("%d\t%s\n", q->count.count, q->name);
		else {
			struct caller *r;
			for (r = q->callers; r; r = r->link)
				printf("%d\t%s\tfrom %s\tin %s:%d.%d\n", r->count, q->name, r->name,
					r->file, r->y, r->x + 1);
		}
		
}

/* string - save a copy of str, if necessary */
char *string(char *str) {
	static struct string { struct string *link; char str[1]; } *list;
	struct string *p;

	for (p = list; p; p = p->link)
		if (strcmp(p->str, str) == 0)
			return p->str;
	p = (struct string *)alloc(strlen(str) + sizeof *p);
	strcpy(p->str, str);
	p->link = list;
	list = p;
	return p->str;
}
/* acaller - add caller and site (file,x,y) to callee's callers list */
static void acaller(char *caller, char *file, int x, int y, int count, struct func *callee) {
	struct caller *q;

	assert(callee);
	for (q = callee->callers; q && (caller != q->name
		|| file != q->file || x != q->x || y != q->y); q = q->link)
		;
	if (!q) {
		struct caller **r;
		NEW(q, PERM);
		q->name = caller;
		q->file = file;
		q->x = x;
		q->y = y;
		q->count = 0;
		for (r = &callee->callers; *r && (strcmp(q->name, (*r)->name) > 0
			|| strcmp(q->file, (*r)->file) > 0 || q->y > (*r)->y || q->y > (*r)->y); r = &(*r)->link)
			;
		q->link = *r;
		*r = q;
	}
	q->count += count;
}

/* compare - return <0, 0, >0 if a<b, a==b, a>b, resp. */
static int compare(struct count *a, struct count *b) {
	if (a->y == b->y)
		return a->x - b->x;
	return a->y - b->y;
}

/* findfile - return file name's file list entry, or 0 */
static struct file *findfile(char *name) {
	struct file *p;

	for (p = filelist; p; p = p->link)
		if (p->name == name)
			return p;
	return 0;
}

/* afunction - add function name and its data to file's function list */
static struct func *afunction(char *name, char *file, int x, int y, int count) {
	struct file *p = findfile(file);
	struct func *q;

	assert(p);
	for (q = p->funcs; q && name != q->name; q = q->link)
		;
	if (!q) {
		struct func **r;
		NEW(q, PERM);
		q->name = name;
		q->count.x = x;
		q->count.y = y;
		q->count.count = 0;
		q->callers = 0;
		for (r = &p->funcs; *r && compare(&q->count, &(*r)->count) > 0; r = &(*r)->link)
			;
		q->link = *r;
		*r = q;
	}
	q->count.count += count;
	return q;
}

/* apoint - append execution point i to file's data */ 
static void apoint(int i, char *file, int x, int y, int count) {
	struct file *p = findfile(file);

	assert(p);
	if (i >= p->size) {
		int j;
		if (p->size == 0) {
			p->size = i >= 200 ? 2*i : 200;
			p->counts = newarray(p->size, sizeof *p->counts, PERM);
		} else {
			struct count *new;
			p->size = 2*i;
			new = newarray(p->size, sizeof *new, PERM);
			for (j = 0; j < p->count; j++)
				new[j] = p->counts[j];
			p->counts = new;
		}
		for (j = p->count; j < p->size; j++) {
			static struct count z;
			p->counts[j] = z;
		}
	}
	p->counts[i].x = x;
	p->counts[i].y = y;
	p->counts[i].count += count;
	if (i >= p->count)
		p->count = i + 1;
}

/* findcount - return count associated with (file,x,y) or -1 */
int findcount(char *file, int x, int y) {
	static struct file *cursor;

	if (cursor == 0 || cursor->name != file)
		cursor = findfile(file);
	if (cursor) {
		int l, u;
		struct count *c = cursor->counts;
		for (l = 0, u = cursor->count - 1; l <= u; ) {
			int k = (l + u)/2;
			if (c[k].y > y || c[k].y == y && c[k].x > x)
				u = k - 1;
			else if (c[k].y < y || c[k].y == y && c[k].x < x)
				l = k + 1;
			else
				return c[k].count;
		}
	}
	return -1;
}

/* findfunc - return count associated with function name in file or -1 */
int findfunc(char *name, char *file) {
	static struct file *cursor;

	if (cursor == 0 || cursor->name != file)
		cursor = findfile(file);
	if (cursor) {
		struct func *p;
		for (p = cursor->funcs; p; p = p->link)
			if (p->name == name)
				return p->count.count;
	}
	return -1;
}

/* getd - read a nonnegative number */
static int getd(void) {
	int c, n = 0;

	while ((c = getc(fp)) != EOF && (c == ' ' || c == '\n' || c == '\t'))
		;
	if (c >= '0' && c <= '9') {
		do
			n = 10*n + (c - '0');
		while ((c = getc(fp)) >= '0' && c <= '9');
		return n;
	}
	return -1;
}

/* getstr - read a string */
static char *getstr(void) {
	int c;
	char buf[MAXTOKEN], *s = buf;

	while ((c = getc(fp)) != EOF && c != ' ' && c != '\n' && c != '\t')
		if (s - buf < (int)sizeof buf - 2)
			*s++ = c;
	*s = 0;
	return s == buf ? (char *)0 : string(buf);
}

/* gather - read prof.out data from fd */
static int gather(void) {
	int i, nfiles, nfuncs, npoints;
	char *files[64];

	if ((nfiles = getd()) < 0)
		return 0;
	assert(nfiles < NELEMS(files));
	for (i = 0; i < nfiles; i++) {
		if ((files[i] = getstr()) == 0)
			return -1;
		if (!findfile(files[i])) {
			struct file *new;
			NEW(new, PERM);
			new->name = files[i];
			new->size = new->count = 0;
			new->counts = 0;
			new->funcs = 0;
			new->link = filelist;
			filelist = new;
		}
	}
	if ((nfuncs = getd()) < 0)
		return -1;
	for (i = 0; i < nfuncs; i++) {
		struct func *q;
		char *name, *file;
		int f, x, y, count;
		if ((name = getstr()) == 0 || (f = getd()) <= 0
		|| (x = getd()) < 0 || (y = getd()) < 0 || (count = getd()) < 0)
			return -1;
		q = afunction(name, files[f-1], x, y, count);
		if ((name = getstr()) == 0 || (file = getstr()) == 0
		|| (x = getd()) < 0 || (y = getd()) < 0)
			return -1;
		if (*name != '?')
			acaller(name, file, x, y, count, q);
	}
	if ((npoints = getd()) < 0)
		return -1;
	for (i = 0; i < npoints; i++) {
		int f, x, y, count;
		if ((f = getd()) < 0 || (x = getd()) < 0 || (y = getd()) < 0
		|| (count = getd()) < 0)
			return -1;
		if (f)
			apoint(i, files[f-1], x, y, count);
	}
	return 1;
}

/* process - read prof.out data from file */
int process(char *file) {
	int more;

	if ((fp = fopen(file, "r")) != NULL) {
		struct file *p;
		while ((more = gather()) > 0)
			;
		fclose(fp);
		if (more < 0)
			return more;
		for (p = filelist; p; p = p->link)
			qsort(p->counts, p->count, sizeof *p->counts,
				(int (*)(const void *, const void *))
				compare);
		
		return 1;
	}
	return 0;
}
int main(int argc, char *argv[]) {
	int i;
	struct file *p;
	void (*f)(struct file *, int) = printfile;

	progname = argv[0];
	if ((i = process("prof.out")) <= 0) {
		fprintf(stderr, "%s: can't %s `%s'\n", progname,
			i == 0 ? "open" : "interpret", "prof.out");
		exit(1);
	}
	for (i = 1; i < argc && *argv[i] == '-'; i++)
		if (strcmp(argv[i], "-c") == 0) {
			emitdata("prof.out"); 
			exit(0);
		} else if (strcmp(argv[i], "-b") == 0)
			f = printfile;
		else if (strcmp(argv[i], "-f") == 0) {
			fcount++;
			f = printfuncs;
		} else if (strcmp(argv[i], "-n") == 0)
			number++;
		else if (strncmp(argv[i], "-I", 2) == 0) {
			int j;
			for (j = 0; j < NDIRS && dirs[j]; j++)
				;
			if (j < NDIRS)
				dirs[j] = &argv[i][2];
			else
				fprintf(stderr, "%s: too many -I options\n", progname);
		} else {
			fprintf(stderr, "usage: %s [ -c | -b | -n | -f | -Idir... ] [ file... ]\n", progname);
			exit(1);
		}
	for (p = filelist; p; p = p->link)
		qsort(p->counts, p->count, sizeof *p->counts,
			(int (*)(const void *, const void *))compare);
	if (i < argc) {
		int nf = i < argc - 1 ? 1 : 0;
		for ( ; i < argc; i++, nf ? nf++ : 0)
			if (p = findfile(string(argv[i])))
				(*f)(p, nf);
			else
				fprintf(stderr, "%s: no data for `%s'\n", progname, argv[i]);
	} else {
		int nf = filelist && filelist->link ? 1 : 0;
		for (p = filelist; p; p = p->link, nf ? nf++ : 0)
			(*f)(p, nf);
	}
	return 0;
}

