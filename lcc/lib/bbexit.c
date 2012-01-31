#include <stdio.h>
#include <stdlib.h>


struct callsite {
	char *file, *name;
	union coordinate {
		struct { unsigned int index:6,x:10,y:16; } be;
		struct { unsigned int y:16,x:10,index:6; } le;
		unsigned int coord;
	} u;
} *_caller;

static struct _bbdata {
	struct _bbdata *link;
	unsigned npoints, *counts;
	union coordinate *coords;
	char **files;
	struct func {
		struct func *link;
		struct caller {
			struct caller *link;
			struct callsite *caller;
			unsigned count;
		} *callers;
		char *name;
		union coordinate src;
	} *funcs;
} tail, *_bblist = &tail;

static void unpack(unsigned int coord, int *index, int *x, int *y) {
	static union { int x; char endian; } little = { 1 };
	union coordinate u;

	u.coord = coord;
	if (little.endian) {
		*index = u.le.index;
		*x = u.le.x;
		*y = u.le.y;
	} else {
		*index = u.be.index;
		*x = u.be.x;
		*y = u.be.y;
	}
}				

static void profout(struct _bbdata *p, FILE *fp) {
	int i, index, x, y;
	struct func *f;
	struct caller *q;

	for (i = 0; p->files[i]; i++)
		;
	fprintf(fp, "%d\n", i);
	for (i = 0; p->files[i]; i++)
		fprintf(fp, "%s\n", p->files[i]);
	for (i = 0, f = p->funcs; f; i++, f = f->link)
		if (q = f->callers)
			for (i--; q; q = q->link)
				i++;
	fprintf(fp, "%d\n", i);
	for (f = p->funcs; f; f = f->link) {
		int n = 0;
		for (q = f->callers; q; n += q->count, q = q->link) {
			unpack(f->src.coord, &index, &x, &y);
			fprintf(fp, "%s %d %d %d %d", f->name, index, x, y, q->count);
			if (q->caller) {
				unpack(q->caller->u.coord, &index, &x, &y);
				fprintf(fp, " %s %s %d %d\n", q->caller->name, q->caller->file, x, y);
			} else
				fprintf(fp, " ? ? 0 0\n");
		}
		if (n == 0) {
			unpack(f->src.coord, &index, &x, &y);
			fprintf(fp, "%s %d %d %d 0 ? ? 0 0\n", f->name, index, x, y);
		}
	}		
	fprintf(fp, "%d\n", p->npoints);
	for (i = 0; i < p->npoints; i++) {
		unpack(p->coords[i].coord, &index, &x, &y);
		fprintf(fp, "%d %d %d %d\n", index, x, y, p->counts[i]);
	}
}

static void bbexit(void) {
	FILE *fp;

	if (_bblist != &tail && (fp = fopen("prof.out", "a"))) {
		for ( ; _bblist != &tail; _bblist = _bblist->link)
			profout(_bblist, fp);
		fclose(fp);
	}
}

void _epilogue(struct func *callee) {
	_caller = 0;
}

void _prologue(struct func *callee, struct _bbdata *yylink) {
	static struct caller callers[4096];
	static int next;
	struct caller *p;

	if (!yylink->link) {
		yylink->link = _bblist;
		_bblist = yylink;
		if (next == 0)
			atexit(bbexit);
	}
	for (p = callee->callers; p; p = p->link)
		if (p->caller == _caller) {
			p->count++;
			break;
		}
	if (!p && next < sizeof callers/sizeof callers[0]) {
		p = &callers[next++];
		p->caller = _caller;
		p->count = 1;
		p->link = callee->callers;
		callee->callers = p;
	}
	_caller = 0;
}
