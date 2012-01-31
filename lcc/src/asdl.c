#include "c.h"
#include "rcc.h"
#if WIN32
#include <fcntl.h>
#include <io.h>
#endif


static list_ty interfaces;
static rcc_program_ty pickle;

char *string(const char *str) {
	return (char *)Atom_string(str);
}

char *stringd(long n) {
	return (char *)Atom_int(n);
}

char *stringn(const char *str, int len) {
	return (char *)Atom_new(str, len);
}

static void put(rcc_interface_ty node) {
	Seq_addhi(interfaces, node);
}

static int typeuid(Type ty) {
	rcc_type_ty type;

	assert(ty);
	if (ty->x.typeno != 0)
		return ty->x.typeno;
	ty->x.typeno = pickle->nuids++;
	switch (ty->op) {
#define xx(op) case op: type = rcc_##op(ty->size, ty->align); break
	xx(INT);
	xx(UNSIGNED);
	xx(FLOAT);
	xx(VOID);
#undef xx
#define xx(op) case op: type = rcc_##op(ty->size, ty->align, typeuid(ty->type)); break
	xx(POINTER);
	xx(ARRAY);
	xx(CONST);
	xx(VOLATILE);
#undef xx
	case CONST+VOLATILE:
		type = rcc_CONST(ty->size, ty->align, typeuid(ty->type));
		break;
	case ENUM: {
		list_ty ids = Seq_new(0);
		int i;
		for (i = 0; ty->u.sym->u.idlist[i] != NULL; i++)
			Seq_addhi(ids, rcc_enum_(ty->u.sym->u.idlist[i]->name,
				ty->u.sym->u.idlist[i]->u.value));
		assert(i > 0);
		type = rcc_ENUM(ty->size, ty->align, ty->u.sym->name, ids);
		break;
		}
	case STRUCT: case UNION: {
		list_ty fields = Seq_new(0);
		Field p = fieldlist(ty);
		for ( ; p != NULL; p = p->link)
			Seq_addhi(fields, rcc_field(p->name, typeuid(p->type), p->offset, p->bitsize, p->lsb));
		if (ty->op == STRUCT)
			type = rcc_STRUCT(ty->size, ty->align, ty->u.sym->name, fields);
		else
			type = rcc_UNION (ty->size, ty->align, ty->u.sym->name, fields);
		break;
		}
	case FUNCTION: {
		list_ty formals = Seq_new(0);
		if (ty->u.f.proto != NULL && ty->u.f.proto[0] != NULL) {
			int i;
			for (i = 0; ty->u.f.proto[i] != NULL; i++)
				Seq_addhi(formals, to_generic_int(typeuid(ty->u.f.proto[i])));
		} else if (ty->u.f.proto != NULL && ty->u.f.proto[0] == NULL)
			Seq_addhi(formals, to_generic_int(typeuid(voidtype)));
		type = rcc_FUNCTION(ty->size, ty->align, typeuid(ty->type), formals);
		break;
		}
	default: assert(0);
	}
	Seq_addhi(pickle->items, rcc_Type(ty->x.typeno, type));
	return ty->x.typeno;
}

static int symboluid(Symbol p) {
	assert(p);
	assert(p->scope != CONSTANTS && p->scope != LABELS);
	if (p->x.offset == 0)
		p->x.offset = pickle->nuids++;
	return p->x.offset;
}

static rcc_symbol_ty mk_symbol(Symbol p) {
	int flags = 0, ref = 10000*p->ref;

	if (p->ref > 0 && ref == 0)
		ref++;
#define xx(f,n) flags |= p->f<<n;
	xx(structarg,0)
	xx(addressed,1)
	xx(computed,2)
	xx(temporary,3)
	xx(generated,4)
#undef xx
	return rcc_symbol(p->name, typeuid(p->type), p->scope, p->sclass, ref, flags);
}

static rcc_real_ty mk_real(int size, Value v) {
	unsigned *p = (unsigned *)&v.d;
	return rcc_real(p[swap], p[1-swap]);
}

static void asdl_segment(int n) {
	static int cseg;

	if (cseg != n)
		put(rcc_Segment(cseg = n));
}

static void asdl_address(Symbol q, Symbol p, long n) {
	assert(q->x.offset == 0);
	put(rcc_Address(symboluid(q), mk_symbol(q), symboluid(p), n));
}

static void asdl_blockbeg(Env *e) {
	put(rcc_Blockbeg());
}

static void asdl_blockend(Env *e) {
	put(rcc_Blockend());
}

static void asdl_defaddress(Symbol p) {
	if (p->scope == LABELS)
		put(rcc_Deflabel(p->u.l.label));
	else
		put(rcc_Defaddress(symboluid(p)));
}

static void asdl_defconst(int suffix, int size, Value v) {
	switch (suffix) {
	case I: put(rcc_Defconst(suffix, size, v.i)); return;
	case U: put(rcc_Defconst(suffix, size, v.u)); return;
	case P: put(rcc_Defconst(suffix, size, (unsigned long)v.p)); return;	/* FIXME */
	case F: put(rcc_Defconstf(size, mk_real(size, v))); return;
	assert(0);
	}
}

static void asdl_defstring(int len, char *str) {
	put(rcc_Defstring(Text_box(stringn(str, len), len)));
}

static void asdl_defsymbol(Symbol p) {
	if (p->scope >= GLOBAL)
		symboluid(p);
}

static Symbol temps;

static rcc_node_ty visit(Node p) {
	Symbol q;
	rcc_node_ty left = NULL, right = NULL;
	int suffix = optype(p->op), size = opsize(p->op);

	assert(p);
	for (q = temps; q; q = q->u.t.next)
		if (q->u.t.cse == p) {
			q->u.t.cse = NULL;
			return rcc_CSE(0, 0, symboluid(q), visit(p));
		}
	if (p->kids[0] != NULL)
		left = visit(p->kids[0]);
	if (p->kids[1] != NULL)
		right = visit(p->kids[1]);
	switch (specific(p->op)) {
	case CNST+F:
		assert(p->syms[0]);
		return rcc_CNSTF(suffix, size, mk_real(size, p->syms[0]->u.c.v));
	case CALL+B:
		assert(p->syms[0]);
		assert(p->syms[0]->type);
		return rcc_CALLB(suffix, size, left, right, typeuid(p->syms[0]->type));
	case RET+V:
		return rcc_RET(suffix, size);
	case LABEL+V:
		assert(p->syms[0]);
		return rcc_LABEL(suffix, size, p->syms[0]->u.l.label);
	}
	switch (generic(p->op)) {
	case CNST:
		assert(p->syms[0]);
		return rcc_CNST(suffix, size, p->syms[0]->u.c.v.i);	/* FIXME */
	case ARG:
		assert(p->syms[0]);
		return rcc_ARG(suffix, size, left, p->syms[0]->u.c.v.i, p->syms[1]->u.c.v.i);
	case ASGN:
		assert(p->syms[0]);
		assert(p->syms[1]);
		return rcc_ASGN(suffix, size, left, right, p->syms[0]->u.c.v.i, p->syms[1]->u.c.v.i);
	case CVF: case CVI: case CVP: case CVU:
		assert(p->syms[0]);
		return rcc_CVT(suffix, size, generic(p->op), left, p->syms[0]->u.c.v.i);
	case CALL:
		assert(p->syms[0]);
		assert(p->syms[0]->type);
		return rcc_CALL(suffix, size, left, typeuid(p->syms[0]->type));
#define xx(op) case op: return rcc_##op(suffix, size, symboluid(p->syms[0]))
	xx(ADDRG);
	xx(ADDRF);
#undef xx
	case ADDRL:
		if (!p->syms[0]->defined)
			(*IR->local)(p->syms[0]);
		p->syms[0]->defined = 1;
		return rcc_ADDRL(suffix, size, symboluid(p->syms[0]));
	case JUMP:
		if (p->syms[0] != NULL)
			return rcc_BRANCH(suffix, size, p->syms[0]->u.l.label);
		return rcc_Unary(suffix, size, generic(p->op), left);
	case INDIR: case RET: case NEG: case BCOM: 
		return rcc_Unary(suffix, size, generic(p->op), left);
	case BOR: case BAND: case BXOR: case RSH: case LSH:
	case ADD: case SUB: case DIV: case MUL: case MOD:
		return rcc_Binary(suffix, size, generic(p->op), left, right);
	case EQ: case NE: case GT: case GE: case LE: case LT:
		assert(p->syms[0]);
		return rcc_Compare(suffix, size, generic(p->op), left, right, p->syms[0]->u.l.label);
	}
	assert(0);
	return NULL;
}

static void asdl_emit(Node p) {}

static void asdl_local(Symbol p) {
	assert(p->x.offset == 0);
	put(rcc_Local(symboluid(p), mk_symbol(p)));
	if (p->temporary && p->u.t.cse) {
		p->u.t.next = temps;
		temps = p;
	}
}

static Symbol pending = NULL;

static void dopending(Symbol p) {
	if (pending != NULL) {
		int uid = symboluid(pending);
		rcc_symbol_ty symbol = mk_symbol(pending);
		Seq_addhi(pickle->items, rcc_Symbol(uid, symbol));
	}
	pending = p;
}


static void asdl_export(Symbol p) {
	put(rcc_Export(symboluid(p)));
}

static void asdl_function(Symbol f, Symbol caller[], Symbol callee[], int ncalls) {
	list_ty codelist = Seq_new(0), save, calleelist = Seq_new(0), callerlist = Seq_new(0);
	int i;

	dopending(f);
	for (i = 0; caller[i] != NULL; i++) {
		asdl_local(caller[i]);
		Seq_addhi(callerlist, to_generic_int(symboluid(caller[i])));
	}
	for (i = 0; callee[i] != NULL; i++) {
		asdl_local(callee[i]);
		Seq_addhi(calleelist, to_generic_int(symboluid(callee[i])));
	}
	save = interfaces;
	interfaces = codelist;
	gencode(caller, callee);
	asdl_segment(CODE);
	emitcode();
	interfaces = save;
	put(rcc_Function(symboluid(f), callerlist, calleelist, ncalls, codelist));
}

static Node asdl_gen(Node p) {
	Node q;
	list_ty forest = Seq_new(0);

	for (q = p; p != NULL; p = p->link)
		if (specific(p->op) == JUMP+V && specific(p->kids[0]->op) == ADDRG+P
		&& p->kids[0]->syms[0]->scope == LABELS) {
			p->syms[0] = p->kids[0]->syms[0];
			p->kids[0] = NULL;
		}
	for (p = q; p != NULL; p = p->link)
		Seq_addhi(forest, visit(p));
	put(rcc_Forest(forest));
	temps = NULL;
	return q;
}

static void asdl_global(Symbol p) {
	dopending(p);
	put(rcc_Global(symboluid(p), p->u.seg));
}

static void asdl_import(Symbol p) {
	dopending(p);
	put(rcc_Import(symboluid(p)));
}

static void asdl_progbeg(int argc, char *argv[]) {
	int i;

#if WIN32
	_setmode(_fileno(stdout), _O_BINARY);
#endif
	pickle = rcc_program(1, 0, Seq_new(0), Seq_new(0), argc, Seq_new(0));
	for (i = 0; i < argc; i++)
		Seq_addhi(pickle->argv, to_generic_string(Text_box(argv[i], strlen(argv[i]) + 1)));
	interfaces = pickle->interfaces;
}

static int checkuid(list_ty list) {
	int i, n = 0, count = Seq_length(list);

	for (i = 0; i < count; i++) {
		rcc_interface_ty in = Seq_get(list, i);
		if (in->kind == rcc_Local_enum
		||  in->kind == rcc_Address_enum)
			n++;
		else if (in->kind == rcc_Function_enum)
			n += checkuid(in->v.rcc_Function.codelist);
	}
	return n;
}

static void asdl_progend(void) {
	dopending(NULL);
	{
		int n = checkuid(pickle->interfaces) + Seq_length(pickle->items);
		if (n != pickle->nuids - 1)
			fprintf(stderr, "?bogus uid count: have %d should have %d\n",
				n, pickle->nuids-1);
	}
	pickle->nlabels = genlabel(0);
	write_int((int)(100*(assert(strstr(rcsid, ",v")), strtod(strstr(rcsid, ",v")+2, NULL))
), stdout);
	rcc_write_program(pickle, stdout);
}
static void asdl_space(int n) {
	put(rcc_Space(n));
}

void asdl_init(int argc, char *argv[]) {
	int i;
	static int inited;

	if (inited)
		return;
	inited = 1;
	for (i = 1; i < argc; i++)
		if (strcmp(argv[i], "-asdl") == 0) {
#define xx(f) IR->f = asdl_##f
		xx(address);
		xx(blockbeg);
		xx(blockend);
		xx(defaddress);
		xx(defconst);
		xx(defstring);
		xx(defsymbol);
		xx(emit);
		xx(export);
		xx(function);
		xx(gen);
		xx(global);
		xx(import);
		xx(local);
		xx(progbeg);
		xx(progend);
		xx(segment);
		xx(space);
#undef xx
#define xx(f) IR->f = 0
		xx(stabblock);
		xx(stabend);
		xx(stabfend);
		xx(stabinit);
		xx(stabline);
		xx(stabsym);
		xx(stabtype);
#undef xx
		IR->wants_dag = 0;
		prunetemps = 0;	/* pass2 prunes useless temps */
		assignargs = 0;	/* pass2 generates caller to callee assignments */
		}
}
