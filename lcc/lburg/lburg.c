#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "lburg.h"

static char rcsid[] = "lburg.c - faked rcsid";

static char *prefix = "";
static int Tflag = 0;
static int ntnumber = 0;
static Nonterm start = 0;
static Term terms;
static Nonterm nts;
static Rule rules;
static int nrules;
static struct block {
	struct block *link;
} *memlist;			/* list of allocated blocks */

static char *stringf(char *fmt, ...);
static void print(char *fmt, ...);
static void ckreach(Nonterm p);
static void emitclosure(Nonterm nts);
static void emitcost(Tree t, char *v);
static void emitdefs(Nonterm nts, int ntnumber);
static void emitheader(void);
static void emitkids(Rule rules, int nrules);
static void emitnts(Rule rules, int nrules);
static void emitrecalc(char *pre, Term root, Term kid);
static void emitrecord(char *pre, Rule r, char *c, int cost);
static void emitrule(Nonterm nts);
static void emitlabel(Term terms, Nonterm start, int ntnumber);
static void emitstring(Rule rules);
static void emitstruct(Nonterm nts, int ntnumber);
static void emittest(Tree t, char *v, char *suffix);

int main(int argc, char *argv[]) {
	int c, i;
	Nonterm p;
	
	for (i = 1; i < argc; i++)
		if (strcmp(argv[i], "-T") == 0)
			Tflag = 1;
		else if (strncmp(argv[i], "-p", 2) == 0 && argv[i][2])
			prefix = &argv[i][2];
		else if (strncmp(argv[i], "-p", 2) == 0 && i + 1 < argc)
			prefix = argv[++i];
		else if (*argv[i] == '-' && argv[i][1]) {
			yyerror("usage: %s [-T | -p prefix]... [ [ input ] output ] \n",
				argv[0]);
			exit(1);
		} else if (infp == NULL) {
			if (strcmp(argv[i], "-") == 0)
				infp = stdin;
			else if ((infp = fopen(argv[i], "r")) == NULL) {
				yyerror("%s: can't read `%s'\n", argv[0], argv[i]);
				exit(1);
			}
		} else if (outfp == NULL) {
			if (strcmp(argv[i], "-") == 0)
				outfp = stdout;
			if ((outfp = fopen(argv[i], "w")) == NULL) {
				yyerror("%s: can't write `%s'\n", argv[0], argv[i]);
				exit(1);
			}
		}
	if (infp == NULL)
		infp = stdin;
	if (outfp == NULL)
		outfp = stdout;
	yyparse();
	if (start)
		ckreach(start);
	for (p = nts; p; p = p->link) {
		if (p->rules == NULL)
			yyerror("undefined nonterminal `%s'\n", p->name);
		if (!p->reached)
			yyerror("can't reach nonterminal `%s'\n", p->name);
	}
	emitheader();
	emitdefs(nts, ntnumber);
	emitstruct(nts, ntnumber);
	emitnts(rules, nrules);
	emitstring(rules);
	emitrule(nts);
	emitclosure(nts);
	if (start)
		emitlabel(terms, start, ntnumber);
	emitkids(rules, nrules);
	if (!feof(infp))
		while ((c = getc(infp)) != EOF)
			putc(c, outfp);
	while (memlist) {	/* for purify */
		struct block *q = memlist->link;
		free(memlist);
		memlist = q;
	}
	return errcnt > 0;
}

/* alloc - allocate nbytes or issue fatal error */
void *alloc(int nbytes) {
	struct block *p = calloc(1, sizeof *p + nbytes);

	if (p == NULL) {
		yyerror("out of memory\n");
		exit(1);
	}
	p->link = memlist;
	memlist = p;
	return p + 1;
}

/* stringf - format and save a string */
static char *stringf(char *fmt, ...) {
	va_list ap;
	char buf[512];

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);
	return strcpy(alloc(strlen(buf) + 1), buf);
}	

struct entry {
	union {
		char *name;
		struct term t;
		struct nonterm nt;
	} sym;
	struct entry *link;
} *table[211];
#define HASHSIZE (sizeof table/sizeof table[0])

/* hash - return hash number for str */
static unsigned hash(char *str) {
	unsigned h = 0;

	while (*str)
		h = (h<<1) + *str++;
	return h;
}

/* lookup - lookup symbol name */
static void *lookup(char *name) {
	struct entry *p = table[hash(name)%HASHSIZE];

	for ( ; p; p = p->link)
		if (strcmp(name, p->sym.name) == 0)
			return &p->sym;
	return 0;
}

/* install - install symbol name */
static void *install(char *name) {
	struct entry *p = alloc(sizeof *p);
	int i = hash(name)%HASHSIZE;

	p->sym.name = name;
	p->link = table[i];
	table[i] = p;
	return &p->sym;
}

/* nonterm - create a new terminal id, if necessary */
Nonterm nonterm(char *id) {
	Nonterm p = lookup(id), *q = &nts;

	if (p && p->kind == NONTERM)
		return p;
	if (p && p->kind == TERM)
		yyerror("`%s' is a terminal\n", id);
	p = install(id);
	p->kind = NONTERM;
	p->number = ++ntnumber;
	if (p->number == 1)
		start = p;
	while (*q && (*q)->number < p->number)
		q = &(*q)->link;
	assert(*q == 0 || (*q)->number != p->number);
	p->link = *q;
	*q = p;
	return p;
}

/* term - create a new terminal id with external symbol number esn */
Term term(char *id, int esn) {
	Term p = lookup(id), *q = &terms;

	if (p)
		yyerror("redefinition of terminal `%s'\n", id);
	else
		p = install(id);
	p->kind = TERM;
	p->esn = esn;
	p->arity = -1;
	while (*q && (*q)->esn < p->esn)
		q = &(*q)->link;
	if (*q && (*q)->esn == p->esn)
		yyerror("duplicate external symbol number `%s=%d'\n",
			p->name, p->esn);
	p->link = *q;
	*q = p;
	return p;
}

/* tree - create & initialize a tree node with the given fields */
Tree tree(char *id, Tree left, Tree right) {
	Tree t = alloc(sizeof *t);
	Term p = lookup(id);
	int arity = 0;

	if (left && right)
		arity = 2;
	else if (left)
		arity = 1;
	if (p == NULL && arity > 0) {
		yyerror("undefined terminal `%s'\n", id);
		p = term(id, -1);
	} else if (p == NULL && arity == 0)
		p = (Term)nonterm(id);
	else if (p && p->kind == NONTERM && arity > 0) {
		yyerror("`%s' is a nonterminal\n", id);
		p = term(id, -1);
	}
	if (p->kind == TERM && p->arity == -1)
		p->arity = arity;
	if (p->kind == TERM && arity != p->arity)
		yyerror("inconsistent arity for terminal `%s'\n", id);
	t->op = p;
	t->nterms = p->kind == TERM;
	if ((t->left = left) != NULL)
		t->nterms += left->nterms;
	if ((t->right = right) != NULL)
		t->nterms += right->nterms;
	return t;
}

/* rule - create & initialize a rule with the given fields */
Rule rule(char *id, Tree pattern, char *template, char *code) {
	Rule r = alloc(sizeof *r), *q;
	Term p = pattern->op;
	char *end;

	r->lhs = nonterm(id);
	r->packed = ++r->lhs->lhscount;
	for (q = &r->lhs->rules; *q; q = &(*q)->decode)
		;
	*q = r;
	r->pattern = pattern;
	r->ern = ++nrules;
	r->template = template;
	r->code = code;
	r->cost = strtol(code, &end, 10);
	if (*end) {
		r->cost = -1;
		r->code = stringf("(%s)", code);
	}
	if (p->kind == TERM) {
		for (q = &p->rules; *q; q = &(*q)->next)
			;
		*q = r;
	} else if (pattern->left == NULL && pattern->right == NULL) {
		Nonterm p = pattern->op;
		r->chain = p->chain;
	        p->chain = r;
		if (r->cost == -1)
			yyerror("illegal nonconstant cost `%s'\n", code);
	}
	for (q = &rules; *q; q = &(*q)->link)
		;
	r->link = *q;
	*q = r;
	return r;
}

/* print - formatted output */
static void print(char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	for ( ; *fmt; fmt++)
		if (*fmt == '%')
			switch (*++fmt) {
			case 'd': fprintf(outfp, "%d", va_arg(ap, int)); break;
			case 's': fputs(va_arg(ap, char *), outfp); break;
			case 'P': fprintf(outfp, "%s_", prefix); break;
			case 'T': {
				Tree t = va_arg(ap, Tree);
				print("%S", t->op);
				if (t->left && t->right)
					print("(%T,%T)", t->left, t->right);
				else if (t->left)
					print("(%T)", t->left);
				break;
				}
			case 'R': {
				Rule r = va_arg(ap, Rule);
				print("%S: %T", r->lhs, r->pattern);
				break;
				}
			case 'S': fputs(va_arg(ap, Term)->name, outfp); break;
			case '1': case '2': case '3': case '4': case '5': {
				int n = *fmt - '0';
				while (n-- > 0)
					putc('\t', outfp);
				break;
				}
			default: putc(*fmt, outfp); break;			
			}
		else
			putc(*fmt, outfp);
	va_end(ap);
}

/* reach - mark all nonterminals in tree t as reachable */
static void reach(Tree t) {
	Nonterm p = t->op;

	if (p->kind == NONTERM)
		if (!p->reached)
			ckreach(p);
	if (t->left)
		reach(t->left);
	if (t->right)
		reach(t->right);
}

/* ckreach - mark all nonterminals reachable from p */
static void ckreach(Nonterm p) {
	Rule r;

        p->reached = 1;
	for (r = p->rules; r; r = r->decode)
		reach(r->pattern);
}

/* emitcase - emit one case in function state */
static void emitcase(Term p, int ntnumber) {
	Rule r;

	print("%1case %d: /* %S */\n", p->esn, p);
	switch (p->arity) {
	case 0: case -1:
		break;
	case 1:
		print("%2%Plabel(LEFT_CHILD(a));\n");
		break;
	case 2:
		print("%2%Plabel(LEFT_CHILD(a));\n");
		print("%2%Plabel(RIGHT_CHILD(a));\n");
		break;
	default: assert(0);
	}
	for (r = p->rules; r; r = r->next) {
		char *indent = "\t\t\0";
		switch (p->arity) {
		case 0: case -1:
			print("%2/* %R */\n", r);
			if (r->cost == -1) {
				print("%2c = %s;\n", r->code);
				emitrecord("\t\t", r, "c", 0);
			} else
				emitrecord("\t\t", r, r->code, 0);
			break;
		case 1:
			if (r->pattern->nterms > 1) {
				print("%2if (%1/* %R */\n", r);
				emittest(r->pattern->left, "LEFT_CHILD(a)", " ");
				print("%2) {\n");
				indent = "\t\t\t";
			} else
				print("%2/* %R */\n", r);
			if (r->pattern->nterms == 2 && r->pattern->left
			&&  r->pattern->right == NULL)
				emitrecalc(indent, r->pattern->op, r->pattern->left->op);
			print("%sc = ", indent);
			emitcost(r->pattern->left, "LEFT_CHILD(a)");
			print("%s;\n", r->code);
			emitrecord(indent, r, "c", 0);
			if (indent[2])
				print("%2}\n");
			break;
		case 2:
			if (r->pattern->nterms > 1) {
				print("%2if (%1/* %R */\n", r);
				emittest(r->pattern->left,  "LEFT_CHILD(a)",
					r->pattern->right->nterms ? " && " : " ");
				emittest(r->pattern->right, "RIGHT_CHILD(a)", " ");
				print("%2) {\n");
				indent = "\t\t\t";
			} else
				print("%2/* %R */\n", r);
			print("%sc = ", indent);
			emitcost(r->pattern->left,  "LEFT_CHILD(a)");
			emitcost(r->pattern->right, "RIGHT_CHILD(a)");
			print("%s;\n", r->code);
			emitrecord(indent, r, "c", 0);
			if (indent[2])
				print("%2}\n");
			break;
		default: assert(0);
		}
	}
	print("%2break;\n");
}

/* emitclosure - emit the closure functions */
static void emitclosure(Nonterm nts) {
	Nonterm p;

	for (p = nts; p; p = p->link)
		if (p->chain)
			print("static void %Pclosure_%S(NODEPTR_TYPE, int);\n", p);
	print("\n");
	for (p = nts; p; p = p->link)
		if (p->chain) {
			Rule r;
			print("static void %Pclosure_%S(NODEPTR_TYPE a, int c) {\n"
"%1struct %Pstate *p = STATE_LABEL(a);\n", p);
			for (r = p->chain; r; r = r->chain)
				emitrecord("\t", r, "c", r->cost);
			print("}\n\n");
		}
}

/* emitcost - emit cost computation for tree t */
static void emitcost(Tree t, char *v) {
	Nonterm p = t->op;

	if (p->kind == TERM) {
		if (t->left)
			emitcost(t->left,  stringf("LEFT_CHILD(%s)",  v));
		if (t->right)
			emitcost(t->right, stringf("RIGHT_CHILD(%s)", v));
	} else
		print("((struct %Pstate *)(%s->x.state))->cost[%P%S_NT] + ", v, p);
}

/* emitdefs - emit nonterminal defines and data structures */
static void emitdefs(Nonterm nts, int ntnumber) {
	Nonterm p;

	for (p = nts; p; p = p->link)
		print("#define %P%S_NT %d\n", p, p->number);
	print("\n");
	print("static char *%Pntname[] = {\n%10,\n");
	for (p = nts; p; p = p->link)
		print("%1\"%S\",\n", p);
	print("%10\n};\n\n");
}

/* emitheader - emit initial definitions */
static void emitheader(void) {
	time_t timer = time(NULL);

	print("/*\ngenerated at %sby %s\n*/\n", ctime(&timer), rcsid);
	print("static void %Pkids(NODEPTR_TYPE, int, NODEPTR_TYPE[]);\n");
	print("static void %Plabel(NODEPTR_TYPE);\n");
	print("static int %Prule(void*, int);\n\n");
}

/* computekids - compute paths to kids in tree t */
static char *computekids(Tree t, char *v, char *bp, int *ip) {
	Term p = t->op;

	if (p->kind == NONTERM) {
		sprintf(bp, "\t\tkids[%d] = %s;\n", (*ip)++, v);
		bp += strlen(bp);
	} else if (p->arity > 0) {
		bp = computekids(t->left, stringf("LEFT_CHILD(%s)", v), bp, ip);
		if (p->arity == 2)
			bp = computekids(t->right, stringf("RIGHT_CHILD(%s)", v), bp, ip);
	}
	return bp;
}

/* emitkids - emit _kids */
static void emitkids(Rule rules, int nrules) {
	int i;
	Rule r, *rc = alloc((nrules + 1 + 1)*sizeof *rc);
	char **str  = alloc((nrules + 1 + 1)*sizeof *str);

	for (i = 0, r = rules; r; r = r->link) {
		int j = 0;
		char buf[1024], *bp = buf;
		*computekids(r->pattern, "p", bp, &j) = 0;
		for (j = 0; str[j] && strcmp(str[j], buf); j++)
			;
		if (str[j] == NULL)
			str[j] = strcpy(alloc(strlen(buf) + 1), buf);
		r->kids = rc[j];
		rc[j] = r;
	}
	print("static void %Pkids(NODEPTR_TYPE p, int eruleno, NODEPTR_TYPE kids[]) {\n"
"%1if (!p)\n%2fatal(\"%Pkids\", \"Null tree\\n\", 0);\n"
"%1if (!kids)\n%2fatal(\"%Pkids\", \"Null kids\\n\", 0);\n"
"%1switch (eruleno) {\n");
	for (i = 0; (r = rc[i]) != NULL; i++) {
		for ( ; r; r = r->kids)
			print("%1case %d: /* %R */\n", r->ern, r);
		print("%s%2break;\n", str[i]);
	}
	print("%1default:\n%2fatal(\"%Pkids\", \"Bad rule number %%d\\n\", eruleno);\n%1}\n}\n\n");
}

/* emitlabel - emit label function */
static void emitlabel(Term terms, Nonterm start, int ntnumber) {
	int i;
	Term p;

	print("static void %Plabel(NODEPTR_TYPE a) {\n%1int c;\n"
"%1struct %Pstate *p;\n\n"
"%1if (!a)\n%2fatal(\"%Plabel\", \"Null tree\\n\", 0);\n");
	print("%1STATE_LABEL(a) = p = allocate(sizeof *p, FUNC);\n"
"%1p->rule._stmt = 0;\n");
	for (i = 1; i <= ntnumber; i++)
		print("%1p->cost[%d] =\n", i);
	print("%20x7fff;\n%1switch (OP_LABEL(a)) {\n");
	for (p = terms; p; p = p->link)
		emitcase(p, ntnumber);
	print("%1default:\n"
"%2fatal(\"%Plabel\", \"Bad terminal %%d\\n\", OP_LABEL(a));\n%1}\n}\n\n");
}

/* computents - fill in bp with _nts vector for tree t */
static char *computents(Tree t, char *bp) {
	if (t) {
		Nonterm p = t->op;
		if (p->kind == NONTERM) {
			sprintf(bp, "%s_%s_NT, ", prefix, p->name);
			bp += strlen(bp);
		} else
			bp = computents(t->right, computents(t->left,  bp));
	}
	return bp;
}

/* emitnts - emit _nts ragged array */
static void emitnts(Rule rules, int nrules) {
	Rule r;
	int i, j, *nts = alloc((nrules + 1)*sizeof *nts);
	char **str = alloc((nrules + 1)*sizeof *str);

	for (i = 0, r = rules; r; r = r->link) {
		char buf[1024];
		*computents(r->pattern, buf) = 0;
		for (j = 0; str[j] && strcmp(str[j], buf); j++)
			;
		if (str[j] == NULL) {
			print("static short %Pnts_%d[] = { %s0 };\n", j, buf);
			str[j] = strcpy(alloc(strlen(buf) + 1), buf);
		}
		nts[i++] = j;
	}
	print("\nstatic short *%Pnts[] = {\n");
	for (i = j = 0, r = rules; r; r = r->link) {
		for ( ; j < r->ern; j++)
			print("%10,%1/* %d */\n", j);
		print("%1%Pnts_%d,%1/* %d */\n", nts[i++], j++);
	}
	print("};\n\n");
}

/* emitrecalc - emit code that tests for recalculation of INDIR?(VREGP) */
static void emitrecalc(char *pre, Term root, Term kid) {
	if (root->kind == TERM && strncmp(root->name, "INDIR", 5) == 0
	&&   kid->kind == TERM &&  strcmp(kid->name,  "VREGP"   ) == 0) {
		Nonterm p;
		print("%sif (mayrecalc(a)) {\n", pre);
		print("%s%1struct %Pstate *q = a->syms[RX]->u.t.cse->x.state;\n", pre);
		for (p = nts; p; p = p->link) {
			print("%s%1if (q->cost[%P%S_NT] == 0) {\n", pre, p);
			print("%s%2p->cost[%P%S_NT] = 0;\n", pre, p);
			print("%s%2p->rule.%P%S = q->rule.%P%S;\n", pre, p, p);
			print("%s%1}\n", pre);
		}
		print("%s}\n", pre);
	}
}

/* emitrecord - emit code that tests for a winning match of rule r */
static void emitrecord(char *pre, Rule r, char *c, int cost) {
	if (Tflag)
		print("%s%Ptrace(a, %d, %s + %d, p->cost[%P%S_NT]);\n",
			pre, r->ern, c, cost, r->lhs);
	print("%sif (", pre);
	print("%s + %d < p->cost[%P%S_NT]) {\n"
"%s%1p->cost[%P%S_NT] = %s + %d;\n%s%1p->rule.%P%S = %d;\n",
		c, cost, r->lhs, pre, r->lhs, c, cost, pre, r->lhs,
		r->packed);
	if (r->lhs->chain)
		print("%s%1%Pclosure_%S(a, %s + %d);\n", pre, r->lhs, c, cost);
	print("%s}\n", pre);
}

/* emitrule - emit decoding vectors and _rule */
static void emitrule(Nonterm nts) {
	Nonterm p;

	for (p = nts; p; p = p->link) {
		Rule r;
		print("static short %Pdecode_%S[] = {\n%10,\n", p);
		for (r = p->rules; r; r = r->decode)
			print("%1%d,\n", r->ern);
		print("};\n\n");
	}
	print("static int %Prule(void *state, int goalnt) {\n"
"%1if (goalnt < 1 || goalnt > %d)\n%2fatal(\"%Prule\", \"Bad goal nonterminal %%d\\n\", goalnt);\n"
"%1if (!state)\n%2return 0;\n%1switch (goalnt) {\n", ntnumber);
	for (p = nts; p; p = p->link)
		print("%1case %P%S_NT:"
"%1return %Pdecode_%S[((struct %Pstate *)state)->rule.%P%S];\n", p, p, p);
	print("%1default:\n%2fatal(\"%Prule\", \"Bad goal nonterminal %%d\\n\", goalnt);\n%2return 0;\n%1}\n}\n\n");
}

/* emitstring - emit arrays of templates, instruction flags, and rules */
static void emitstring(Rule rules) {
	Rule r;

	print("static char *%Ptemplates[] = {\n");
	print("/* 0 */%10,\n");
	for (r = rules; r; r = r->link)
		print("/* %d */%1\"%s\",%1/* %R */\n", r->ern, r->template, r);
	print("};\n");
	print("\nstatic char %Pisinstruction[] = {\n");
	print("/* 0 */%10,\n");
	for (r = rules; r; r = r->link) {
		int len = strlen(r->template);
		print("/* %d */%1%d,%1/* %s */\n", r->ern,
			len >= 2 && r->template[len-2] == '\\' && r->template[len-1] == 'n',
			r->template);
	}
	print("};\n");
	print("\nstatic char *%Pstring[] = {\n");
	print("/* 0 */%10,\n");
	for (r = rules; r; r = r->link)
		print("/* %d */%1\"%R\",\n", r->ern, r);
	print("};\n\n");
}

/* emitstruct - emit the definition of the state structure */
static void emitstruct(Nonterm nts, int ntnumber) {
	print("struct %Pstate {\n%1short cost[%d];\n%1struct {\n", ntnumber + 1);
	for ( ; nts; nts = nts->link) {
		int n = 1, m = nts->lhscount;
		while ((m >>= 1) != 0)
			n++;		
		print("%2unsigned int %P%S:%d;\n", nts, n);
	}
	print("%1} rule;\n};\n\n");
}

/* emittest - emit clause for testing a match */
static void emittest(Tree t, char *v, char *suffix) {
	Term p = t->op;

	if (p->kind == TERM) {
		print("%3%s->op == %d%s/* %S */\n", v, p->esn,
			t->nterms > 1 ? " && " : suffix, p);
		if (t->left)
			emittest(t->left, stringf("LEFT_CHILD(%s)",  v),
				t->right && t->right->nterms ? " && " : suffix);
		if (t->right)
			emittest(t->right, stringf("RIGHT_CHILD(%s)", v), suffix);
	}
}
