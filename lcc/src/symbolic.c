#include <time.h>
#include <ctype.h>
#include "c.h"

#define I(f) s_##f

static Node *tail;
static int off, maxoff, uid = 0, verbose = 0, html = 0;

static const char *yyBEGIN(const char *tag) {
	if (html)
		print("<%s>", tag);
	return tag;
}

static void yyEND(const char *tag) {
	if (html)
		print("</%s>", tag);
	if (isupper(*tag))
		print("\n");
}

#define BEGIN(tag) do { const char *yytag=yyBEGIN(#tag);
#define END yyEND(yytag); } while (0)
#define ITEM BEGIN(li)
#define START BEGIN(LI)
#define ANCHOR(attr,code) do { const char *yytag="a"; if (html) { printf("<a " #attr "=\""); code; print("\">"); }
#define NEWLINE print(html ? "<br>\n" : "\n")

static void emitCoord(Coordinate src) {
	if (src.file && *src.file) {
		ANCHOR(href,print("%s", src.file)); print("%s", src.file); END;
		print(":");
	}
	print("%d.%d", src.y, src.x);
}

static void emitString(int len, const char *s) {
	for ( ; len-- > 0; s++)
		if (*s == '&' && html)
			print("&amp;");
		else if (*s == '<' && html)
			print("&lt;");
		else if (*s == '>' && html)
			print("&lt;");
		else if (*s == '"' || *s == '\\')
			print("\\%c", *s);
		else if (*s >= ' ' && *s < 0177)
			print("%c", *s);
		else
			print("\\%d%d%d", (*s>>6)&3, (*s>>3)&7, *s&7);
}

static void emitSymRef(Symbol p) {
	(*IR->defsymbol)(p);
	ANCHOR(href,print("#%s", p->x.name)); BEGIN(code); print("%s", p->name); END; END;
}

static void emitSymbol(Symbol p) {
	(*IR->defsymbol)(p);
	ANCHOR(name,print("%s", p->x.name)); BEGIN(code); print("%s", p->name); END; END;
	BEGIN(ul);
#define xx(field,code) ITEM; if (!html) print(" "); print(#field "="); code; END
	if (verbose && (src.y || src.x))
		xx(src,emitCoord(p->src));
	xx(type,print("%t", p->type));
	xx(sclass,print("%k", p->sclass));
	switch (p->scope) {
	case CONSTANTS: xx(scope,print("CONSTANTS")); break;
	case LABELS:    xx(scope,print("LABELS"));    break;
	case GLOBAL:    xx(scope,print("GLOBAL"));    break;
	case PARAM:     xx(scope,print("PARAM"));     break;
	case LOCAL:     xx(scope,print("LOCAL"));     break;
	default:
		if (p->scope > LOCAL)
			xx(scope,print("LOCAL+%d", p->scope-LOCAL));
		else
			xx(scope,print("%d", p->scope));
	}
	if (p->scope >= PARAM && p->sclass != STATIC)
		xx(offset,print("%d", p->x.offset));
	xx(ref,print("%f", p->ref));
	if (p->temporary && p->u.t.cse)
		xx(u.t.cse,print("%p", p->u.t.cse));
	END;
#undef xx
}

/* address - initialize q for addressing expression p+n */
static void I(address)(Symbol q, Symbol p, long n) {
	q->name = stringf("%s%s%D", p->name, n > 0 ? "+" : "", n);
	(*IR->defsymbol)(q);
	START; print("address "); emitSymbol(q); END;
}

/* blockbeg - start a block */
static void I(blockbeg)(Env *e) {
	e->offset = off;
	START; print("blockbeg off=%d", off); END;
}

/* blockend - start a block */
static void I(blockend)(Env *e) {
	if (off > maxoff)
		maxoff = off;
	START; print("blockend off=%d", off); END;
	off = e->offset;
}

/* defaddress - initialize an address */
static void I(defaddress)(Symbol p){
	START; print("defaddress "); emitSymRef(p); END;
}

/* defconst - define a constant */
static void I(defconst)(int suffix, int size, Value v) {
	START;
	print("defconst ");
	switch (suffix) {
	case I:
		print("int.%d ", size);
		BEGIN(code);
		if (size > sizeof (int))
			print("%D", v.i);
		else
			print("%d", (int)v.i);
		END;
		break;
	case U:
		print("unsigned.%d ", size);
		BEGIN(code);
		if (size > sizeof (unsigned))
			print("%U", v.u);
		else
			print("%u", (unsigned)v.u);
		END;
		break;
	case P: print("void*.%d ", size); BEGIN(code); print("%p", v.p); END; break;
	case F: print("float.%d ", size); BEGIN(code); print("%g", (double)v.d); END; break;
	default: assert(0);
	}
	END;
}

/* defstring - emit a string constant */
static void I(defstring)(int len, char *s) {
	START; print("defstring ");
	BEGIN(code); print("\""); emitString(len, s); print("\""); END;
	END;
}

/* defsymbol - define a symbol: initialize p->x */
static void I(defsymbol)(Symbol p) {
	if (p->x.name == NULL)
		p->x.name = stringd(++uid);
}

/* emit - emit the dags on list p */
static void I(emit)(Node p){
	ITEM;
	if (!html)
		print(" ");
	for (; p; p = p->x.next) {
		if (p->op == LABEL+V) {
			assert(p->syms[0]);
			ANCHOR(name,print("%s", p->syms[0]->x.name));
			BEGIN(code); print("%s", p->syms[0]->name); END;
			END;
			print(":");
		} else {
			int i;
			if (p->x.listed) {
				BEGIN(strong); print("%d", p->x.inst); END; print("'");
				print(" %s", opname(p->op));
			} else
				print("%d. %s", p->x.inst, opname(p->op));
			if (p->count > 1)
				print(" count=%d", p->count);
			for (i = 0; i < NELEMS(p->kids) && p->kids[i]; i++)
				print(" #%d", p->kids[i]->x.inst);
			if (generic(p->op) == CALL && p->syms[0] && p->syms[0]->type)
				print(" {%t}", p->syms[0]->type);
			else
				for (i = 0; i < NELEMS(p->syms) && p->syms[i]; i++) {
					print(" ");
					if (p->syms[i]->scope == CONSTANTS)
						print(p->syms[i]->name);
					else
						emitSymRef(p->syms[i]);
				}
		}
		NEWLINE;
	}
	END;
}

/* export - announce p as exported */
static void I(export)(Symbol p) {
	START; print("export "); emitSymRef(p); END;
}

/* function - generate code for a function */
static void I(function)(Symbol f, Symbol caller[], Symbol callee[], int ncalls) {
	int i;

	(*IR->defsymbol)(f);
	off = 0;
	for (i = 0; caller[i] && callee[i]; i++) {
		off = roundup(off, caller[i]->type->align);
		caller[i]->x.offset = callee[i]->x.offset = off;
		off += caller[i]->type->size;
	}
	if (!html) {
		print("function ");
		emitSymbol(f);
		print(" ncalls=%d\n", ncalls);
		for (i = 0; caller[i]; i++)
			START; print("caller "); emitSymbol(caller[i]); END;
		for (i = 0; callee[i]; i++)
			START; print("callee "); emitSymbol(callee[i]); END;
	} else {
		START;
		print("function");
		BEGIN(UL);
#define xx(field,code) ITEM; print(#field "="); code; END
		xx(f,emitSymbol(f));
		xx(ncalls,print("%d", ncalls));
		if (caller[0]) {
			ITEM; print("caller"); BEGIN(OL);
			for (i = 0; caller[i]; i++)
				ITEM; emitSymbol(caller[i]); END;
			END; END;
			ITEM; print("callee"); BEGIN(OL);
			for (i = 0; callee[i]; i++)
				ITEM; emitSymbol(callee[i]); END;
			END; END;
		} else {
			xx(caller,BEGIN(em); print("empty"); END);
			xx(callee,BEGIN(em); print("empty"); END);
		}
		END;
		END;
	}
	maxoff = off = 0;
	gencode(caller, callee);
	if (html)
		START; print("emitcode"); BEGIN(ul); emitcode(); END; END;
	else
		emitcode();
	START; print("maxoff=%d", maxoff); END;
#undef xx
}

/* visit - generate code for *p */
static int visit(Node p, int n) {
	if (p && p->x.inst == 0) {
		p->x.inst = ++n;
		n = visit(p->kids[0], n);
		n = visit(p->kids[1], n);
		*tail = p;
		tail = &p->x.next;
	}
	return n;
}

/* gen0 - generate code for the dags on list p */
static Node I(gen)(Node p) {
	int n;
	Node nodelist;

	tail = &nodelist;
	for (n = 0; p; p = p->link) {
		switch (generic(p->op)) {	/* check for valid forest */
		case CALL:
			assert(IR->wants_dag || p->count == 0);
			break;
		case ARG:
		case ASGN: case JUMP: case LABEL: case RET:
		case EQ: case GE: case GT: case LE: case LT: case NE:
			assert(p->count == 0);
			break;
		case INDIR:
			assert(IR->wants_dag && p->count > 0);
			break;
		default:
			assert(0);
		}
		check(p);
		p->x.listed = 1;
		n = visit(p, n);
	}
	*tail = 0;
	return nodelist;
}

/* global - announce a global */
static void I(global)(Symbol p) {
	START; print("global "); emitSymbol(p); END;
}

/* import - import a symbol */
static void I(import)(Symbol p) {
	START; print("import "); emitSymRef(p); END;
}

/* local - local variable */
static void I(local)(Symbol p) {
	if (p->temporary)
		p->name = stringf("t%s", p->name);
	(*IR->defsymbol)(p);
	off = roundup(off, p->type->align);
	p->x.offset = off;
	off += p->type->size;
	START; print(p->temporary ? "temporary " : "local "); emitSymbol(p); END;
}

/* progbeg - beginning of program */
static void I(progbeg)(int argc, char *argv[]) {
	int i;

	for (i = 1; i < argc; i++)
		if (strcmp(argv[i], "-v") == 0)
			verbose++;
		else if (strcmp(argv[i], "-html") == 0)
			html++;
	if (html) {
		print("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n");
		print("<html>");
		BEGIN(head);
		if (firstfile && *firstfile)
			BEGIN(title); emitString(strlen(firstfile), firstfile);	END;
		print("<link rev=made href=\"mailto:drh@microsoft.com\">\n");
		END;
		print("<body>\n");
		if (firstfile && *firstfile)
			BEGIN(h1); emitString(strlen(firstfile), firstfile); END;
		BEGIN(P); BEGIN(em);
		print("Links lead from uses of identifiers and labels to their definitions.");
		END; END;
		print("<ul>\n");
		START;
		print("progbeg");
		BEGIN(ol);
		for (i = 1; i < argc; i++) {
			ITEM;
			BEGIN(code); print("\""); emitString(strlen(argv[i]), argv[i]); print("\""); END;
			END;
		}
		END;
		END;
	}
}

/* progend - end of program */
static void I(progend)(void) {
	START; print("progend"); END;
	if (html) {
		time_t t;
		print("</ul>\n");
		time(&t);
		print("<hr><address>%s</address>\n", ctime(&t));
		print("</body></html>\n");
	}
}

/* segment - switch to segment s */
static void I(segment)(int s) {
	START; print("segment %s", &"text\0bss\0.data\0lit\0.sym\0."[5*s-5]); END;
}

/* space - initialize n bytes of space */
static void I(space)(int n) {
	START; print("space %d", n); END;
}

static void I(stabblock)(int brace, int lev, Symbol *p) {}

/* stabend - finalize stab output */
static void I(stabend)(Coordinate *cp, Symbol p, Coordinate **cpp, Symbol *sp, Symbol *stab) {
	int i;

	if (p)
		emitSymRef(p);
	print("\n");
	if (cpp && sp)
		for (i = 0; cpp[i] && sp[i]; i++) {
			print("%w.%d: ", cpp[i], cpp[i]->x);
			emitSymRef(sp[i]);
			print("\n");
		}
}

static void I(stabfend)(Symbol p, int lineno) {}
static void I(stabinit)(char *file, int argc, char *argv[]) {}

/* stabline - emit line number information for source coordinate *cp */
static void I(stabline)(Coordinate *cp) {
	if (cp->file)
		print("%s:", cp->file);
	print("%d.%d:\n", cp->y, cp->x);
}

static void I(stabsym)(Symbol p) {}
static void I(stabtype)(Symbol p) {}

Interface symbolicIR = {
	1, 1, 0,	/* char */
	2, 2, 0,	/* short */
	4, 4, 0,	/* int */
	4, 4, 0,	/* long */
	4, 4, 0,	/* long long */
	4, 4, 1,	/* float */
	8, 8, 1,	/* double */
	8, 8, 1,	/* long double */
	4, 4, 0,	/* T* */
	0, 4, 0,	/* struct */
	0,		/* little_endian */
	0,		/* mulops_calls */
	0,		/* wants_callb */
	1,		/* wants_argb */
	1,		/* left_to_right */
	1,		/* wants_dag */
	0,		/* unsigned_char */
	I(address),
	I(blockbeg),
	I(blockend),
	I(defaddress),
	I(defconst),
	I(defstring),
	I(defsymbol),
	I(emit),
	I(export),
	I(function),
	I(gen),
	I(global),
	I(import),
	I(local),
	I(progbeg),
	I(progend),
	I(segment),
	I(space),
	I(stabblock),
	I(stabend),
	I(stabfend),
	I(stabinit),
	I(stabline),
	I(stabsym),
	I(stabtype)
};

Interface symbolic64IR = {
	1, 1, 0,	/* char */
	2, 2, 0,	/* short */
	4, 4, 0,	/* int */
	8, 8, 0,	/* long */
	8, 8, 0,	/* long long */
	4, 4, 1,	/* float */
	8, 8, 1,	/* double */
	8, 8, 1,	/* long double */
	8, 8, 0,	/* T* */
	0, 1, 0,	/* struct */
	1,		/* little_endian */
	0,		/* mulops_calls */
	0,		/* wants_callb */
	1,		/* wants_argb */
	1,		/* left_to_right */
	1,		/* wants_dag */
	0,		/* unsigned_char */
	I(address),
	I(blockbeg),
	I(blockend),
	I(defaddress),
	I(defconst),
	I(defstring),
	I(defsymbol),
	I(emit),
	I(export),
	I(function),
	I(gen),
	I(global),
	I(import),
	I(local),
	I(progbeg),
	I(progend),
	I(segment),
	I(space),
	I(stabblock),
	I(stabend),
	I(stabfend),
	I(stabinit),
	I(stabline),
	I(stabsym),
	I(stabtype)
};
