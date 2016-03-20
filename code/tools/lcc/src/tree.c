#include "c.h"


int where = STMT;
static int warn;
static int nid = 1;		/* identifies trees & nodes in debugging output */
static struct nodeid {
	int printed;
	Tree node;
} ids[500];			/* if ids[i].node == p, then p's id is i */

static void printtree1(Tree, int, int);

Tree tree(int op, Type type, Tree left, Tree right) {
	Tree p;

	NEW0(p, where);
	p->op = op;
	p->type = type;
	p->kids[0] = left;
	p->kids[1] = right;
	return p;
}

Tree texpr(Tree (*f)(int), int tok, int a) {
	int save = where;
	Tree p;

	where = a;
	p = (*f)(tok);
	where = save;
	return p;
}
static Tree root1(Tree p) {
	if (p == NULL)
		return p;
	if (p->type == voidtype)
		warn++;
	switch (generic(p->op)) {
	case COND: {
		Tree q = p->kids[1];
		assert(q && q->op == RIGHT);
		if (p->u.sym && q->kids[0] && generic(q->kids[0]->op) == ASGN)
			q->kids[0] = root1(q->kids[0]->kids[1]);
		else
			q->kids[0] = root1(q->kids[0]);
		if (p->u.sym && q->kids[1] && generic(q->kids[1]->op) == ASGN)
			q->kids[1] = root1(q->kids[1]->kids[1]);
		else
			q->kids[1] = root1(q->kids[1]);
		p->u.sym = 0;
		if (q->kids[0] == 0 && q->kids[1] == 0)
			p = root1(p->kids[0]);
		}
		break;
	case AND: case OR:
		if ((p->kids[1] = root1(p->kids[1])) == 0)
			p = root1(p->kids[0]);
		break;
	case NOT:
		if (warn++ == 0)
			warning("expression with no effect elided\n");
		return root1(p->kids[0]);
	case RIGHT:
		if (p->kids[1] == 0)
			return root1(p->kids[0]);
		if (p->kids[0] && p->kids[0]->op == CALL+B
		&&  p->kids[1] && p->kids[1]->op == INDIR+B)
			/* avoid premature release of the CALL+B temporary */
			return p->kids[0];
		if (p->kids[0] && p->kids[0]->op == RIGHT
		&&  p->kids[1] == p->kids[0]->kids[0])
			/* de-construct e++ construction */
			return p->kids[0]->kids[1];
		p = tree(RIGHT, p->type, root1(p->kids[0]), root1(p->kids[1]));
		return p->kids[0] || p->kids[1] ? p : (Tree)0;
	case EQ:  case NE:  case GT:   case GE:  case LE:  case LT: 
	case ADD: case SUB: case MUL:  case DIV: case MOD:
	case LSH: case RSH: case BAND: case BOR: case BXOR:
		if (warn++ == 0)
			warning("expression with no effect elided\n");
		p = tree(RIGHT, p->type, root1(p->kids[0]), root1(p->kids[1]));
		return p->kids[0] || p->kids[1] ? p : (Tree)0;
	case INDIR:
		if (p->type->size == 0 && unqual(p->type) != voidtype)
			warning("reference to `%t' elided\n", p->type);
		if (isptr(p->kids[0]->type) && isvolatile(p->kids[0]->type->type))
			warning("reference to `volatile %t' elided\n", p->type);
		/* fall thru */
	case CVI: case CVF: case CVU: case CVP:
	case NEG: case BCOM: case FIELD:
		if (warn++ == 0)
			warning("expression with no effect elided\n");
		return root1(p->kids[0]);
	case ADDRL: case ADDRG: case ADDRF: case CNST:
		if (needconst)
			return p;
		if (warn++ == 0)
			warning("expression with no effect elided\n");
		return NULL;
	case ARG: case ASGN: case CALL: case JUMP: case LABEL:
		break;
	default: assert(0);
	}
	return p;
}

Tree root(Tree p) {
	warn = 0;
	return root1(p);
}

char *opname(int op) {
	static char *opnames[] = {
	"",
	"CNST",
	"ARG",
	"ASGN",
	"INDIR",
	"CVC",
	"CVD",
	"CVF",
	"CVI",
	"CVP",
	"CVS",
	"CVU",
	"NEG",
	"CALL",
	"*LOAD*",
	"RET",
	"ADDRG",
	"ADDRF",
	"ADDRL",
	"ADD",
	"SUB",
	"LSH",
	"MOD",
	"RSH",
	"BAND",
	"BCOM",
	"BOR",
	"BXOR",
	"DIV",
	"MUL",
	"EQ",
	"GE",
	"GT",
	"LE",
	"LT",
	"NE",
	"JUMP",
	"LABEL",
	"AND",
	"NOT",
	"OR",
	"COND",
	"RIGHT",
	"FIELD"
	}, *suffixes[] = {
		"0", "F", "D", "C", "S", "I", "U", "P", "V", "B",
		"10","11","12","13","14","15"
	};

	if (generic(op) >= AND && generic(op) <= FIELD && opsize(op) == 0)
		return opnames[opindex(op)];
	return stringf("%s%s%s",
		opindex(op) > 0 && opindex(op) < NELEMS(opnames) ?
			opnames[opindex(op)] : stringd(opindex(op)),
		suffixes[optype(op)], opsize(op) > 0 ? stringd(opsize(op)) : "");
}

int nodeid(Tree p) {
	int i = 1;

	ids[nid].node = p;
	while (ids[i].node != p)
		i++;
	if (i == nid)
		ids[nid++].printed = 0;
	return i;
}

/* printed - return pointer to ids[id].printed */
int *printed(int id) {
	if (id)
		return &ids[id].printed;
	nid = 1;
	return 0;
}

/* printtree - print tree p on fd */
void printtree(Tree p, int fd) {
	(void)printed(0);
	printtree1(p, fd, 1);
}

/* printtree1 - recursively print tree p */
static void printtree1(Tree p, int fd, int lev) {
	FILE *f = fd == 1 ? stdout : stderr;
	int i;
	static char blanks[] = "                                                   ";

	if (p == 0 || *printed(i = nodeid(p)))
		return;
	fprint(f, "#%d%S%S", i, blanks, i < 10 ? 2 : i < 100 ? 1 : 0, blanks, lev);
	fprint(f, "%s %t", opname(p->op), p->type);
	*printed(i) = 1;
	for (i = 0; i < NELEMS(p->kids); i++)
		if (p->kids[i])
			fprint(f, " #%d", nodeid(p->kids[i]));
	if (p->op == FIELD && p->u.field)
		fprint(f, " %s %d..%d", p->u.field->name,
			fieldsize(p->u.field) + fieldright(p->u.field), fieldright(p->u.field));
	else if (generic(p->op) == CNST)
		fprint(f, " %s", vtoa(p->type, p->u.v));
	else if (p->u.sym)
		fprint(f, " %s", p->u.sym->name);
	if (p->node)
		fprint(f, " node=%p", p->node);
	fprint(f, "\n");
	for (i = 0; i < NELEMS(p->kids); i++)
		printtree1(p->kids[i], fd, lev + 1);
}
