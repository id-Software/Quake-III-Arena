/* C compiler: prof.out input

prof.out format:
#files
    name
    ... (#files-1 times)
#functions
    name file# x y count caller file x y 
    ... (#functions-1 times)
#points
    file# x y count
    ... (#points-1 times)
*/
#include "c.h"


struct count {			/* count data: */
	int x, y;			/* source coordinate */
	int count;			/* associated execution count */
};

#define MAXTOKEN 64

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
			if (c[k].y > y || (c[k].y == y && c[k].x > x))
				u = k - 1;
			else if (c[k].y < y || (c[k].y == y && c[k].x < x))
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
