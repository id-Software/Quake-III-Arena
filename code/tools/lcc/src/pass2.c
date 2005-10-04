#include "c.h"
#include "rcc.h"
#if WIN32
#include <fcntl.h>
#include <io.h>
#endif


Interface *IR = NULL;
int Aflag;		/* >= 0 if -A specified */
int Pflag;		/* != 0 if -P specified */
int glevel;		/* == [0-9] if -g[0-9] specified */
int xref;		/* != 0 for cross-reference data */
Symbol YYnull;		/* _YYnull  symbol if -n or -nvalidate specified */
Symbol YYcheck;		/* _YYcheck symbol if -nvalidate,check specified */

static int verbose = 1;
#define VERBOSE(n,arg) (verbose >= n ? (void)(arg):(void)0)
static int nuids;
static rcc_item_ty *items;
static void **itemmap;

static void *uid2type(int uid) {
	assert(uid >= 0 && uid < nuids);
	if (itemmap[uid] == NULL) {
		Type ty;
		rcc_type_ty type = (void *)items[uid];
		assert(items[uid]);
		assert(items[uid]->uid == uid);
		assert(items[uid]->kind == rcc_Type_enum);
		type = items[uid]->v.rcc_Type.type;
		assert(type);
		switch (type->kind) {
		case rcc_INT_enum:
			ty = btot(INT, type->size);
			assert(ty->align == type->align);
			break;
		case rcc_UNSIGNED_enum:
			ty = btot(UNSIGNED, type->size);
			assert(ty->align == type->align);
			break;
		case rcc_FLOAT_enum:
			ty = btot(FLOAT, type->size);
			assert(ty->align == type->align);
			break;
		case rcc_VOID_enum:
			ty = voidtype;
			break;
		case rcc_POINTER_enum:
			ty = ptr(uid2type(type->v.rcc_POINTER.type));
			break;
		case rcc_ARRAY_enum:
			ty = uid2type(type->v.rcc_ARRAY.type);
			assert(ty->size > 0);
			ty = array(ty, type->size/ty->size, 0);
			break;
		case rcc_CONST_enum:
			ty = qual(CONST, uid2type(type->v.rcc_CONST.type));
			break;
		case rcc_VOLATILE_enum:
			ty = qual(VOLATILE, uid2type(type->v.rcc_VOLATILE.type));
			break;
		case rcc_ENUM_enum: {
			int i, n = Seq_length(type->v.rcc_ENUM.ids);
			ty = newstruct(ENUM, string(type->v.rcc_ENUM.tag));
			ty->type = inttype;
			ty->size = ty->type->size;
			ty->align = ty->type->align;
			ty->u.sym->u.idlist = newarray(n + 1, sizeof *ty->u.sym->u.idlist, PERM);
			for (i = 0; i < n; i++) {
				rcc_enum__ty e = Seq_remlo(type->v.rcc_ENUM.ids);
				Symbol p = install(e->id, &identifiers, GLOBAL, PERM);
				p->type = ty;
				p->sclass = ENUM;
				p->u.value = e->value;
				ty->u.sym->u.idlist[i] = p;
				free(e);
			}
			ty->u.sym->u.idlist[i] = NULL;
			Seq_free(&type->v.rcc_ENUM.ids);
			break;
			}
		case rcc_STRUCT_enum: case rcc_UNION_enum: {
			int i, n;
			Field *tail;
			list_ty fields;
			if (type->kind == rcc_STRUCT_enum) {
				ty = newstruct(STRUCT, string(type->v.rcc_STRUCT.tag));
				fields = type->v.rcc_STRUCT.fields;
			} else {
				ty = newstruct(UNION, string(type->v.rcc_UNION.tag));
				fields = type->v.rcc_UNION.fields;
			}
			itemmap[uid] = ty;	/* recursive types */
			ty->size = type->size;
			ty->align = type->align;
			tail = &ty->u.sym->u.s.flist;
			n = Seq_length(fields);
			for (i = 0; i < n; i++) {
				rcc_field_ty field = Seq_remlo(fields);
				NEW0(*tail, PERM);
				(*tail)->name = (char *)field->id;
				(*tail)->type = uid2type(field->type);
				(*tail)->offset = field->offset;
				(*tail)->bitsize = field->bitsize;
				(*tail)->lsb = field->lsb;
				if (isconst((*tail)->type))
					ty->u.sym->u.s.cfields = 1;
				if (isvolatile((*tail)->type))
					ty->u.sym->u.s.vfields = 1;
				tail = &(*tail)->link;
				free(field);
			}
			Seq_free(&fields);
			break;
			}
		case rcc_FUNCTION_enum: {
			int n = Seq_length(type->v.rcc_FUNCTION.formals);
			if (n > 0) {
				int i;
				Type *proto = newarray(n + 1, sizeof *proto, PERM);
				for (i = 0; i < n; i++) {
					int *formal = Seq_remlo(type->v.rcc_FUNCTION.formals);
					proto[i] = uid2type(*formal);
					free(formal);
				}
				proto[i] = NULL;
				ty = func(uid2type(type->v.rcc_FUNCTION.type), proto, 0);
			} else
				ty = func(uid2type(type->v.rcc_FUNCTION.type), NULL, 1);
			Seq_free(&type->v.rcc_FUNCTION.formals);
			break;
			}
		default: assert(0);
		}
		if (itemmap[uid] == NULL) {
			itemmap[uid] = ty;
			free(type);
			free(items[uid]);
			items[uid] = NULL;
		} else
			assert(itemmap[uid] == ty);
	}
	return itemmap[uid];
}

static Symbol uid2symbol(int uid) {
	assert(uid >= 0 && uid < nuids);
	if (itemmap[uid] == NULL) {
		Symbol p;
		rcc_symbol_ty symbol;
		assert(items[uid]);
		assert(items[uid]->uid == uid);
		assert(items[uid]->kind == rcc_Symbol_enum);
		symbol = items[uid]->v.rcc_Symbol.symbol;
		assert(symbol);
		NEW0(p, PERM);
		p->name = (char *)symbol->id;
		p->scope = symbol->scope;
		p->sclass = symbol->sclass;
		p->type = uid2type(symbol->type);
#define xx(f,n) p->f = symbol->flags>>n;
		xx(structarg,0)
		xx(addressed,1)
		xx(computed,2)
		xx(temporary,3)
		xx(generated,4)
#undef xx
		p->ref = symbol->ref/10000.0;
		assert(p->scope != CONSTANTS && p->scope != LABELS);
		if (p->scope == GLOBAL || p->sclass == STATIC || p->sclass == EXTERN)
			(*IR->defsymbol)(p);
		itemmap[uid] = p;
		free(symbol);
		free(items[uid]);
		items[uid] = NULL;
	}
	return itemmap[uid];
}

#define xx(s) static void do##s(rcc_interface_ty);
xx(Export)
xx(Import)
xx(Global)
xx(Local)
xx(Address)
xx(Segment)
xx(Defaddress)
xx(Deflabel)
xx(Defconst)
xx(Defconstf)
xx(Defstring)
xx(Space)
xx(Function)
xx(Blockbeg)
xx(Blockend)
xx(Forest)
#undef xx
static void (*doX[])(rcc_interface_ty in) = {
#define xx(s) 0,
xx(Export)
xx(Import)
xx(Global)
xx(Local)
xx(Address)
xx(Segment)
xx(Defaddress)
xx(Deflabel)
xx(Defconst)
xx(Defconstf)
xx(Defstring)
xx(Space)
xx(Function)
xx(Blockbeg)
xx(Blockend)
xx(Forest)
	0
#undef xx
};

static void interface(rcc_interface_ty in) {
	assert(in);
	(*doX[in->kind])(in);
	free(in);
}

static void doExport(rcc_interface_ty in) {
	(*IR->export)(uid2symbol(in->v.rcc_Export.p));
}

static void doImport(rcc_interface_ty in) {
	Symbol p = uid2symbol(in->v.rcc_Export.p);

	(*IR->import)(p);
	p->defined = 1;
}

static void doGlobal(rcc_interface_ty in) {
	Symbol p = uid2symbol(in->v.rcc_Global.p);

	p->u.seg = in->v.rcc_Global.seg;
	(*IR->global)(p);
	p->defined = 1;
}

static void doLocal(rcc_interface_ty in) {
	int uid = in->v.rcc_Local.uid;

	assert(uid >= 0 && uid < nuids);
	assert(items[uid] == NULL);
	items[uid] = rcc_Symbol(uid, in->v.rcc_Local.p);
	if (in->v.rcc_Local.p->scope >= LOCAL)
		addlocal(uid2symbol(uid));
}

static void doAddress(rcc_interface_ty in) {
	int uid = in->v.rcc_Address.uid;
	Symbol p = uid2symbol(in->v.rcc_Address.p);

	assert(uid >= 0 && uid < nuids);
	assert(items[uid] == NULL);
	items[uid] = rcc_Symbol(uid, in->v.rcc_Address.q);
	if (p->scope == GLOBAL || p->sclass == STATIC || p->sclass == EXTERN)
		(*IR->address)(uid2symbol(uid), p, in->v.rcc_Address.n);
	else {
		Code cp = code(Address);
		cp->u.addr.sym = uid2symbol(uid);
		cp->u.addr.base = p;
		cp->u.addr.offset = in->v.rcc_Address.n;
	}
}

static void doSegment(rcc_interface_ty in) {
	(*IR->segment)(in->v.rcc_Segment.seg);
}

static void doDefaddress(rcc_interface_ty in) {
	(*IR->defaddress)(uid2symbol(in->v.rcc_Defaddress.p));
}

static void doDeflabel(rcc_interface_ty in) {
	(*IR->defaddress)(findlabel(in->v.rcc_Deflabel.label));
}

static void doDefconst(rcc_interface_ty in) {
	Value v;

	v.i = in->v.rcc_Defconst.value;
	(*IR->defconst)(in->v.rcc_Defconst.suffix, in->v.rcc_Defconst.size, v);
}

static void doDefconstf(rcc_interface_ty in) {
	Value v;
	unsigned *p = (unsigned *)&v.d;

	p[swap]   = in->v.rcc_Defconstf.value->msb;
	p[1-swap] = in->v.rcc_Defconstf.value->lsb;
	(*IR->defconst)(F, in->v.rcc_Defconstf.size, v);
	free(in->v.rcc_Defconstf.value);
}

static void doDefstring(rcc_interface_ty in) {
	(*IR->defstring)(in->v.rcc_Defstring.s.len, (char *)in->v.rcc_Defstring.s.str);
	free((char *)in->v.rcc_Defstring.s.str);
}

static void doSpace(rcc_interface_ty in) {
	(*IR->space)(in->v.rcc_Space.n);
}

static void doFunction(rcc_interface_ty in) {
	int i, n;
	Symbol *caller, *callee;

	/*
	 Initialize:
	  define the function symbol,
	  initialize callee and caller arrays.
	*/
	cfunc = uid2symbol(in->v.rcc_Function.f);
	labels = table(NULL, LABELS);
	enterscope();
	n = Seq_length(in->v.rcc_Function.caller);
	caller = newarray(n + 1, sizeof *caller, FUNC);
	for (i = 0; i < n; i++) {
		int *uid = Seq_remlo(in->v.rcc_Function.caller);
		caller[i] = uid2symbol(*uid);
		free(uid);
	}
	caller[i] = NULL;
	Seq_free(&in->v.rcc_Function.caller);
	callee = newarray(n + 1, sizeof *callee, FUNC);
	for (i = 0; i < n; i++) {
		int *uid = Seq_remlo(in->v.rcc_Function.callee);
		callee[i] = uid2symbol(*uid);
		free(uid);
	}
	callee[i] = NULL;
	Seq_free(&in->v.rcc_Function.callee);
	cfunc->u.f.callee = callee;
	cfunc->defined = 1;
	/*
	 Initialize the code list,
	  traverse the interfaces inside the function;
	  each call appends code list entries.
	*/
	codelist = &codehead;
	codelist->next = NULL;
	n = Seq_length(in->v.rcc_Function.codelist);
	for (i = 0; i < n; i++)
		interface(Seq_remlo(in->v.rcc_Function.codelist));
	Seq_free(&in->v.rcc_Function.codelist);
	/*
	 Call the back end,
	 Wrap-up.
	*/
	exitscope();
	(*IR->function)(cfunc, caller, callee, in->v.rcc_Function.ncalls);
	cfunc = NULL;
	labels = NULL;
}

static struct block {
	Code begin;
	struct block *prev;
} *blockstack = NULL;

static void doBlockbeg(rcc_interface_ty in) {
	struct block *b;
	Code cp = code(Blockbeg);

	enterscope();
	cp->u.block.level = level;
	cp->u.block.locals = newarray(1, sizeof *cp->u.block.locals, FUNC);
	cp->u.block.locals[0] = NULL;
	cp->u.block.identifiers = NULL;
	cp->u.block.types = NULL;
	NEW(b, FUNC);
	b->begin = cp;
	b->prev = blockstack;
	blockstack = b;
}

static void doBlockend(rcc_interface_ty in) {
	assert(blockstack);
	code(Blockend)->u.begin = blockstack->begin;
	blockstack = blockstack->prev;
	exitscope();
}

static Node visit(rcc_node_ty node) {
	int op;
	Node left = NULL, right = NULL, p = NULL;
	Symbol sym = NULL;

	switch (node->kind) {
#define T(x) rcc_##x##_enum
	case T(CSE): {
		Symbol q = uid2symbol(node->v.rcc_CSE.uid);
		assert(q->temporary);
		q->u.t.cse = p = visit(node->v.rcc_CSE.node);
		break;
		}
	case T(CNST): {
		Value v;
		v.i = node->v.rcc_CNST.value;
		sym = constant(btot(node->suffix, node->size), v);
		op = CNST;
		break;
		}
	case T(CNSTF): {
		Value v;
		unsigned *p = (unsigned *)&v.d;
		p[swap]   = node->v.rcc_CNSTF.value->msb;
		p[1-swap] = node->v.rcc_CNSTF.value->lsb;
		sym = constant(btot(node->suffix, node->size), v);
		free(node->v.rcc_CNSTF.value);
		op = CNST;
		break;
		}
	case T(ARG):
		p = newnode(ARG + node->suffix + sizeop(node->size),
			visit(node->v.rcc_ARG.left), NULL,
			intconst(node->v.rcc_ARG.len));
		p->syms[1] = intconst(node->v.rcc_ARG.align);
		break;
	case T(ASGN):
		p = newnode(ASGN + node->suffix + sizeop(node->size),
			visit(node->v.rcc_ASGN.left), visit(node->v.rcc_ASGN.right),
			intconst(node->v.rcc_ASGN.len));
		p->syms[1] = intconst(node->v.rcc_ASGN.align);
		break;
	case T(CVT):
		op = node->v.rcc_CVT.op;
		left = visit(node->v.rcc_CVT.left);
		sym = intconst(node->v.rcc_CVT.fromsize);
		break;
	case T(CALL):
		op = CALL;
		left = visit(node->v.rcc_CALL.left);
		NEW0(sym, FUNC);
		sym->type = uid2type(node->v.rcc_CALL.type);
		break;
	case T(CALLB):
		op = CALL;
		left  = visit(node->v.rcc_CALLB.left);
		right = visit(node->v.rcc_CALLB.right);
		NEW0(sym, FUNC);
		sym->type = uid2type(node->v.rcc_CALLB.type);
		break;
	case T(RET):
		op = RET;
		break;
	case T(ADDRG):
		op = ADDRG;
		sym = uid2symbol(node->v.rcc_ADDRG.uid);
		break;
	case T(ADDRL):
		op = ADDRL;
		sym = uid2symbol(node->v.rcc_ADDRG.uid);
		break;
	case T(ADDRF):
		op = ADDRF;
		sym = uid2symbol(node->v.rcc_ADDRG.uid);
		break;
	case T(Unary):
		op = node->v.rcc_Unary.op;
		left = visit(node->v.rcc_Unary.left);
		break;
	case T(Binary):
		op = node->v.rcc_Binary.op;
		left  = visit(node->v.rcc_Binary.left);
		right = visit(node->v.rcc_Binary.right);
		break;
	case T(Compare):
		op = node->v.rcc_Compare.op;
		left  = visit(node->v.rcc_Compare.left);
		right = visit(node->v.rcc_Compare.right);
		sym = findlabel(node->v.rcc_Compare.label);
		break;
	case T(LABEL):
		op = LABEL;
		sym = findlabel(node->v.rcc_LABEL.label);
		break;
	case T(BRANCH):
		op = JUMP;
		left = newnode(ADDRG+P+sizeop(voidptype->size), NULL, NULL, findlabel(node->v.rcc_BRANCH.label));
		break;
#undef T
	default: assert(0);
	}
	if (p == NULL)
		p = newnode(op + node->suffix + sizeop(node->size), left, right, sym);
	free(node);
	return p;
}

static void doForest(rcc_interface_ty in) {
	Node *tail = &code(Gen)->u.forest;
	int i, n = Seq_length(in->v.rcc_Forest.nodes);

	for (i = 0; i < n; i++) {
		*tail = visit(Seq_remlo(in->v.rcc_Forest.nodes));
		assert(*tail);
		tail = &(*tail)->link;
	}
	*tail = NULL;
	Seq_free(&in->v.rcc_Forest.nodes);
}

int main(int argc, char *argv[]) {
	int i, version;
	float stamp = (assert(strstr(rcsid, ",v")), strtod(strstr(rcsid, ",v")+2, NULL))
;
	char *infile = NULL, *outfile = NULL;
	rcc_program_ty pickle;

	for (i = 1; i < argc; i++)
		if (*argv[i] != '-' || strcmp(argv[i], "-") == 0) {
			if (infile == NULL)
				infile = argv[i];
			else if (outfile == NULL)
				outfile = argv[i];
		}
	if (infile != NULL && strcmp(infile, "-") != 0
	&& freopen(infile, "rb", stdin) == NULL) {
		fprint(stderr, "%s: can't read `%s'\n", argv[0], infile);
		exit(EXIT_FAILURE);
	}
#if WIN32
	else
		_setmode(_fileno(stdin), _O_BINARY);
#endif
	if (outfile != NULL && strcmp(outfile, "-") != 0
	&& freopen(outfile, "w", stdout) == NULL) {
		fprint(stderr, "%s: can't write `%s'\n", argv[0], outfile);
		exit(EXIT_FAILURE);
	}
	version = read_int(stdin);
	assert(version/100 == (int)stamp);
	pickle = rcc_read_program(stdin);
	argc = pickle->argc;
	argv = newarray(argc + 1, sizeof *argv, PERM);
	{
		for (i = 0; i < argc; i++) {
			string_ty *arg = Seq_remlo(pickle->argv);
			argv[i] = (char *)arg->str;
			free(arg);
		}
		argv[i] = NULL;
		assert(i == argc);
		Seq_free(&pickle->argv);
	}
	for (i = argc - 1; i > 0; i--)
		if (strncmp(argv[i], "-target=", 8) == 0)
			break;
	if (i > 0) {
		int j;
		for (j = 0; bindings[j].name && bindings[j].ir; j++)
			if (strcmp(&argv[i][8], bindings[j].name) == 0) {
				IR = bindings[j].ir;
				break;
			}
	}
	if (!IR) {
		fprint(stderr, "%s: unknown target", argv[0]);
		if (i > 0)
			fprint(stderr, " `%s'", &argv[i][8]);
		fprint(stderr, "; must specify one of\n");
		for (i = 0; bindings[i].name; i++)
			fprint(stderr, "\t-target=%s\n", bindings[i].name);
		exit(EXIT_FAILURE);
	}
	IR->wants_dag = 0;	/* pickle's hold trees */
	init(argc, argv);
	genlabel(pickle->nlabels);
	level = GLOBAL;
	{
		int i, count;
		nuids = pickle->nuids;
		items = newarray(nuids, sizeof *items, PERM);
		itemmap = newarray(nuids, sizeof *items, PERM);
		for (i = 0; i < nuids; i++) {
			itemmap[i] = NULL;
			items[i] = NULL;
		}
		(*IR->progbeg)(argc, argv);
		count = Seq_length(pickle->items);
		for (i = 0; i < count; i++) {
			rcc_item_ty item = Seq_remlo(pickle->items);
			int uid = item->uid;
			assert(uid >= 0 && uid < nuids);
			assert(items[uid] == NULL);
			items[uid] = item;
		}
		Seq_free(&pickle->items);
#define xx(s) assert(rcc_##s##_enum < sizeof doX/sizeof doX[0] && doX[rcc_##s##_enum]==0); \
	      doX[rcc_##s##_enum] = do##s;
	      xx(Export)
	      xx(Import)
	      xx(Global)
	      xx(Local)
	      xx(Address)
	      xx(Segment)
	      xx(Defaddress)
	      xx(Deflabel)
	      xx(Defconst)
	      xx(Defconstf)
	      xx(Defstring)
	      xx(Space)
	      xx(Function)
	      xx(Blockbeg)
	      xx(Blockend)
	      xx(Forest)
#undef xx
		count = Seq_length(pickle->interfaces);
		for (i = 0; i < count; i++)
			interface(Seq_remlo(pickle->interfaces));
		Seq_free(&pickle->interfaces);
		free(pickle);
		(*IR->progend)();
	}
	deallocate(PERM);
	return errcnt > 0;
}

/* main_init - process program arguments */
void main_init(int argc, char *argv[]) {
	int i;
	static int inited;

	if (inited)
		return;
	inited = 1;
	for (i = 1; i < argc; i++)
		if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "-g2") == 0)
			glevel = 2;
		else if (strcmp(argv[i], "-w") == 0)
			wflag++;
		else if (strcmp(argv[i], "-v") == 0) {
			fprint(stderr, "%s %s\n", argv[0], rcsid);
			verbose++;
		} else if (strncmp(argv[i], "-errout=", 8) == 0) {
			FILE *f = fopen(argv[i]+8, "w");
			if (f == NULL) {
				fprint(stderr, "%s: can't write errors to `%s'\n", argv[0], argv[i]+8);
				exit(EXIT_FAILURE);
			}
			fclose(f);
			f = freopen(argv[i]+8, "w", stderr);
			assert(f);
		} else if (strncmp(argv[i], "-e", 2) == 0) {
			int x;
			if ((x = strtol(&argv[i][2], NULL, 0)) > 0)
				errlimit = x;
		}
}

void init(int argc, char *argv[]) {
	{extern void main_init(int, char *[]); main_init(argc, argv);}
	{extern void prof_init(int, char *[]); prof_init(argc, argv);}
	{extern void trace_init(int, char *[]); trace_init(argc, argv);}
	{extern void type_init(int, char *[]); type_init(argc, argv);}
	{extern void x86linux_init(int, char *[]); x86linux_init(argc, argv);}
}
