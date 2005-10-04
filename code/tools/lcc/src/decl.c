#include "c.h"


#define add(x,n) (x > inttype->u.sym->u.limits.max.i-(n) ? (overflow=1,x) : x+(n))
#define chkoverflow(x,n) ((void)add(x,n))
#define bits2bytes(n) (((n) + 7)/8)
static int regcount;

static List autos, registers;
Symbol cfunc;		/* current function */
Symbol retv;		/* return value location for structs */

static void checkref(Symbol, void *);
static Symbol dclglobal(int, char *, Type, Coordinate *);
static Symbol dcllocal(int, char *, Type, Coordinate *);
static Symbol dclparam(int, char *, Type, Coordinate *);
static Type dclr(Type, char **, Symbol **, int);
static Type dclr1(char **, Symbol **, int);
static void decl(Symbol (*)(int, char *, Type, Coordinate *));
extern void doconst(Symbol, void *);
static void doglobal(Symbol, void *);
static void doextern(Symbol, void *);
static void exitparams(Symbol []);
static void fields(Type);
static void funcdefn(int, char *, Type, Symbol [], Coordinate);
static void initglobal(Symbol, int);
static void oldparam(Symbol, void *);
static Symbol *parameters(Type);
static Type specifier(int *);
static Type structdcl(int);
static Type tnode(int, Type);
void program(void) {
	int n;
	
	level = GLOBAL;
	for (n = 0; t != EOI; n++)
		if (kind[t] == CHAR || kind[t] == STATIC
		|| t == ID || t == '*' || t == '(') {
			decl(dclglobal);
			deallocate(STMT);
			if (!(glevel >= 3 || xref))
			deallocate(FUNC);
		} else if (t == ';') {
			warning("empty declaration\n");
			t = gettok();
		} else {
			error("unrecognized declaration\n");
			t = gettok();
		}
	if (n == 0)
		warning("empty input file\n");
}
static Type specifier(int *sclass) {
	int cls, cons, sign, size, type, vol;
	Type ty = NULL;

	cls = vol = cons = sign = size = type = 0;
	if (sclass == NULL)
		cls = AUTO;
	for (;;) {
		int *p, tt = t;
		switch (t) {
		case AUTO:
		case REGISTER: if (level <= GLOBAL && cls == 0)
		               	error("invalid use of `%k'\n", t);
		               p = &cls;  t = gettok();      break;
		case STATIC: case EXTERN:
		case TYPEDEF:  p = &cls;  t = gettok();      break;
		case CONST:    p = &cons; t = gettok();      break;
		case VOLATILE: p = &vol;  t = gettok();      break;
		case SIGNED:
		case UNSIGNED: p = &sign; t = gettok();      break;
		case LONG:     if (size == LONG) {
		                       size = 0;
		                       tt = LONG+LONG;
		               }
		               p = &size; t = gettok();      break;
		case SHORT:    p = &size; t = gettok();      break;
		case VOID: case CHAR: case INT: case FLOAT:
		case DOUBLE:   p = &type; ty = tsym->type;
		                          t = gettok();      break;
		case ENUM:     p = &type; ty = enumdcl();    break;
		case STRUCT:
		case UNION:    p = &type; ty = structdcl(t); break;
		case ID:
			if (istypename(t, tsym) && type == 0
			&& sign == 0 && size == 0) {
				use(tsym, src);
				ty = tsym->type;
				if (isqual(ty)
				&& ty->size != ty->type->size) {
					ty = unqual(ty);
					if (isconst(tsym->type))
						ty = qual(CONST, ty);
					if (isvolatile(tsym->type))
						ty = qual(VOLATILE, ty);
					tsym->type = ty;
				}
				p = &type;
				t = gettok();
			} else
				p = NULL;
			break;
		default: p = NULL;
		}
		if (p == NULL)
			break;
		if (*p)
			error("invalid use of `%k'\n", tt);
		*p = tt;
	}
	if (sclass)
		*sclass = cls;
	if (type == 0) {
		type = INT;
		ty = inttype;
	}
	if ((size == SHORT     && type != INT)
	||  (size == LONG+LONG && type != INT)
	||  (size == LONG      && type != INT && type != DOUBLE)
	||  (sign && type != INT && type != CHAR))
		error("invalid type specification\n");
	if (type == CHAR && sign)
		ty = sign == UNSIGNED ? unsignedchar : signedchar;
	else if (size == SHORT)
		ty = sign == UNSIGNED ? unsignedshort : shorttype;
	else if (size == LONG && type == DOUBLE)
		ty = longdouble;
	else if (size == LONG+LONG) {
		ty = sign == UNSIGNED ? unsignedlonglong : longlong;
		if (Aflag >= 1)
			warning("`%t' is a non-ANSI type\n", ty);
	} else if (size == LONG)
		ty = sign == UNSIGNED ? unsignedlong : longtype;
	else if (sign == UNSIGNED && type == INT)
		ty = unsignedtype;
	if (cons == CONST)
		ty = qual(CONST, ty);
	if (vol  == VOLATILE)
		ty = qual(VOLATILE, ty);
	return ty;
}
static void decl(Symbol (*dcl)(int, char *, Type, Coordinate *)) {
	int sclass;
	Type ty, ty1;
	static char stop[] = { CHAR, STATIC, ID, 0 };

	ty = specifier(&sclass);
	if (t == ID || t == '*' || t == '(' || t == '[') {
		char *id;
		Coordinate pos;
		id = NULL;
		pos = src;
		if (level == GLOBAL) {
			Symbol *params = NULL;
			ty1 = dclr(ty, &id, &params, 0);
			if (params && id && isfunc(ty1)
			    && (t == '{' || istypename(t, tsym)
			    || (kind[t] == STATIC && t != TYPEDEF))) {
				if (sclass == TYPEDEF) {
					error("invalid use of `typedef'\n");
					sclass = EXTERN;
				}
				if (ty1->u.f.oldstyle)
					exitscope();
				funcdefn(sclass, id, ty1, params, pos);
				return;
			} else if (params)
				exitparams(params);
		} else
			ty1 = dclr(ty, &id, NULL, 0);
		for (;;) {
			if (Aflag >= 1 && !hasproto(ty1))
				warning("missing prototype\n");
			if (id == NULL)
				error("missing identifier\n");
			else if (sclass == TYPEDEF)
				{
					Symbol p = lookup(id, identifiers);
					if (p && p->scope == level)
						error("redeclaration of `%s'\n", id);
					p = install(id, &identifiers, level,
						level < LOCAL ? PERM : FUNC);
					p->type = ty1;
					p->sclass = TYPEDEF;
					p->src = pos;
				}
			else
				(void)(*dcl)(sclass, id, ty1, &pos);
			if (t != ',')
				break;
			t = gettok();
			id = NULL;
			pos = src;
			ty1 = dclr(ty, &id, NULL, 0);
		}
	} else if (ty == NULL
	|| !(isenum(ty) ||
	     (isstruct(ty) && (*unqual(ty)->u.sym->name < '1' || *unqual(ty)->u.sym->name > '9'))))
		error("empty declaration\n");
	test(';', stop);
}
static Symbol dclglobal(int sclass, char *id, Type ty, Coordinate *pos) {
	Symbol p;

	if (sclass == 0)
		sclass = AUTO;
	else if (sclass != EXTERN && sclass != STATIC) {
		error("invalid storage class `%k' for `%t %s'\n",
			sclass, ty, id);
		sclass = AUTO;
	}
	p = lookup(id, identifiers);
	if (p && p->scope == GLOBAL) {
		if (p->sclass != TYPEDEF && eqtype(ty, p->type, 1))
			ty = compose(ty, p->type);
		else
			error("redeclaration of `%s' previously declared at %w\n", p->name, &p->src);

		if (!isfunc(ty) && p->defined && t == '=')
			error("redefinition of `%s' previously defined at %w\n", p->name, &p->src);

		if ((p->sclass == EXTERN && sclass == STATIC)
		||  (p->sclass == STATIC && sclass == AUTO)
		||  (p->sclass == AUTO   && sclass == STATIC))
			warning("inconsistent linkage for `%s' previously declared at %w\n", p->name, &p->src);

	}
	if (p == NULL || p->scope != GLOBAL) {
		Symbol q = lookup(id, externals);
		if (q) {
			if (sclass == STATIC || !eqtype(ty, q->type, 1))
				warning("declaration of `%s' does not match previous declaration at %w\n", id, &q->src);

			p = relocate(id, externals, globals);
			p->sclass = sclass;
		} else {
			p = install(id, &globals, GLOBAL, PERM);
			p->sclass = sclass;
			(*IR->defsymbol)(p);
		}
		if (p->sclass != STATIC) {
			static int nglobals;
			nglobals++;
			if (Aflag >= 2 && nglobals == 512)
				warning("more than 511 external identifiers\n");
		}
	} else if (p->sclass == EXTERN)
		p->sclass = sclass;
	p->type = ty;
	p->src = *pos;
	if (t == '=' && isfunc(p->type)) {
		error("illegal initialization for `%s'\n", p->name);
		t = gettok();
		initializer(p->type, 0);
	} else if (t == '=') {
		initglobal(p, 0);
		if (glevel > 0 && IR->stabsym) {
			(*IR->stabsym)(p); swtoseg(p->u.seg); }
	} else if (p->sclass == STATIC && !isfunc(p->type)
	&& p->type->size == 0)
		error("undefined size for `%t %s'\n", p->type, p->name);
	return p;
}
static void initglobal(Symbol p, int flag) {
	Type ty;

	if (t == '=' || flag) {
		if (p->sclass == STATIC) {
			for (ty = p->type; isarray(ty); ty = ty->type)
				;
			defglobal(p, isconst(ty) ? LIT : DATA);
		} else
			defglobal(p, DATA);
		if (t == '=')
			t = gettok();
		ty = initializer(p->type, 0);
		if (isarray(p->type) && p->type->size == 0)
			p->type = ty;
		if (p->sclass == EXTERN)
			p->sclass = AUTO;
	}
}
void defglobal(Symbol p, int seg) {
	p->u.seg = seg;
	swtoseg(p->u.seg);
	if (p->sclass != STATIC)
		(*IR->export)(p);
	(*IR->global)(p);
	p->defined = 1;
}

static Type dclr(Type basety, char **id, Symbol **params, int abstract) {
	Type ty = dclr1(id, params, abstract);

	for ( ; ty; ty = ty->type)
		switch (ty->op) {
		case POINTER:
			basety = ptr(basety);
			break;
		case FUNCTION:
			basety = func(basety, ty->u.f.proto,
				ty->u.f.oldstyle);
			break;
		case ARRAY:
			basety = array(basety, ty->size, 0);
			break;
		case CONST: case VOLATILE:
			basety = qual(ty->op, basety);
			break;
		default: assert(0);
		}
	if (Aflag >= 2 && basety->size > 32767)
		warning("more than 32767 bytes in `%t'\n", basety);
	return basety;
}
static Type tnode(int op, Type type) {
	Type ty;

	NEW0(ty, STMT);
	ty->op = op;
	ty->type = type;
	return ty;
}
static Type dclr1(char **id, Symbol **params, int abstract) {
	Type ty = NULL;

	switch (t) {
	case ID:                if (id)
					*id = token;
				else
					error("extraneous identifier `%s'\n", token);
				t = gettok(); break;
	case '*': t = gettok(); if (t == CONST || t == VOLATILE) {
					Type ty1;
					ty1 = ty = tnode(t, NULL);
					while ((t = gettok()) == CONST || t == VOLATILE)
						ty1 = tnode(t, ty1);
					ty->type = dclr1(id, params, abstract);
					ty = ty1;
				} else
					ty = dclr1(id, params, abstract);
				ty = tnode(POINTER, ty); break;
	case '(': t = gettok(); if (abstract
				&& (t == REGISTER || istypename(t, tsym) || t == ')')) {
					Symbol *args;
					ty = tnode(FUNCTION, ty);
					enterscope();
					if (level > PARAM)
						enterscope();
					args = parameters(ty);
					exitparams(args);
				} else {
					ty = dclr1(id, params, abstract);
					expect(')');
					if (abstract && ty == NULL
					&& (id == NULL || *id == NULL))
						return tnode(FUNCTION, NULL);
				} break;
	case '[': break;
	default:  return ty;
	}
	while (t == '(' || t == '[')
		switch (t) {
		case '(': t = gettok(); { Symbol *args;
					  ty = tnode(FUNCTION, ty);
					  enterscope();
					  if (level > PARAM)
					  	enterscope();
					  args = parameters(ty);
					  if (params && *params == NULL)
					  	*params = args;
					  else
					  	exitparams(args);
 }
		          break;
		case '[': t = gettok(); { int n = 0;
					  if (kind[t] == ID) {
					  	n = intexpr(']', 1);
					  	if (n <= 0) {
					  		error("`%d' is an illegal array size\n", n);
					  		n = 1;
					  	}
					  } else
					  	expect(']');
					  ty = tnode(ARRAY, ty);
					  ty->size = n; } break;
		default: assert(0);
		}
	return ty;
}
static Symbol *parameters(Type fty) {
	List list = NULL;
	Symbol *params;

	if (kind[t] == STATIC || istypename(t, tsym)) {
		int n = 0;
		Type ty1 = NULL;
		for (;;) {
			Type ty;
			int sclass = 0;
			char *id = NULL;
			if (ty1 && t == ELLIPSIS) {
				static struct symbol sentinel;
				if (sentinel.type == NULL) {
					sentinel.type = voidtype;
					sentinel.defined = 1;
				}
				if (ty1 == voidtype)
					error("illegal formal parameter types\n");
				list = append(&sentinel, list);
				t = gettok();
				break;
			}
			if (!istypename(t, tsym) && t != REGISTER)
				error("missing parameter type\n");
			n++;
			ty = dclr(specifier(&sclass), &id, NULL, 1);
			if ( (ty == voidtype && (ty1 || id))
			||  ty1 == voidtype)
				error("illegal formal parameter types\n");
			if (id == NULL)
				id = stringd(n);
			if (ty != voidtype)
				list = append(dclparam(sclass, id, ty, &src), list);
			if (Aflag >= 1 && !hasproto(ty))
				warning("missing prototype\n");
			if (ty1 == NULL)
				ty1 = ty;
			if (t != ',')
				break;
			t = gettok();
		}
		fty->u.f.proto = newarray(length(list) + 1,
			sizeof (Type *), PERM);
		params = ltov(&list, FUNC);
		for (n = 0; params[n]; n++)
			fty->u.f.proto[n] = params[n]->type;
		fty->u.f.proto[n] = NULL;
		fty->u.f.oldstyle = 0;
	} else {
		if (t == ID)
			for (;;) {
				Symbol p;
				if (t != ID) {
					error("expecting an identifier\n");
					break;
				}
				p = dclparam(0, token, inttype, &src);
				p->defined = 0;
				list = append(p, list);
				t = gettok();
				if (t != ',')
					break;
				t = gettok();
			}
		params = ltov(&list, FUNC);
		fty->u.f.proto = NULL;
		fty->u.f.oldstyle = 1;
	}
	if (t != ')') {
		static char stop[] = { CHAR, STATIC, IF, ')', 0 };
		expect(')');
		skipto('{', stop);
	}
	if (t == ')')
		t = gettok();
	return params;
}
static void exitparams(Symbol params[]) {
	assert(params);
	if (params[0] && !params[0]->defined)
		error("extraneous old-style parameter list\n");
	if (level > PARAM)
		exitscope();
	exitscope();
}

static Symbol dclparam(int sclass, char *id, Type ty, Coordinate *pos) {
	Symbol p;

	if (isfunc(ty))
		ty = ptr(ty);
	else if (isarray(ty))
		ty = atop(ty);
	if (sclass == 0)
		sclass = AUTO;
	else if (sclass != REGISTER) {
		error("invalid storage class `%k' for `%t%s\n",
			sclass, ty, stringf(id ? " %s'" : "' parameter", id));
		sclass = AUTO;
	} else if (isvolatile(ty) || isstruct(ty)) {
		warning("register declaration ignored for `%t%s\n",
			ty, stringf(id ? " %s'" : "' parameter", id));
		sclass = AUTO;
	}

	p = lookup(id, identifiers);
	if (p && p->scope == level)
		error("duplicate declaration for `%s' previously declared at %w\n", id, &p->src);

	else
		p = install(id, &identifiers, level, FUNC);
	p->sclass = sclass;
	p->src = *pos;
	p->type = ty;
	p->defined = 1;
	if (t == '=') {
		error("illegal initialization for parameter `%s'\n", id);
		t = gettok();
		(void)expr1(0);
	}
	return p;
}
static Type structdcl(int op) {
	char *tag;
	Type ty;
	Symbol p;
	Coordinate pos;

	t = gettok();
	pos = src;
	if (t == ID) {
		tag = token;
		t = gettok();
	} else
		tag = "";
	if (t == '{') {
		static char stop[] = { IF, ',', 0 };
		ty = newstruct(op, tag);
		ty->u.sym->src = pos;
		ty->u.sym->defined = 1;
		t = gettok();
		if (istypename(t, tsym))
			fields(ty);
		else
			error("invalid %k field declarations\n", op);
		test('}', stop);
	}
	else if (*tag && (p = lookup(tag, types)) != NULL
	&& p->type->op == op) {
		ty = p->type;
		if (t == ';' && p->scope < level)
			ty = newstruct(op, tag);
	}
	else {
		if (*tag == 0)
			error("missing %k tag\n", op);
		ty = newstruct(op, tag);
	}
	if (*tag && xref)
		use(ty->u.sym, pos);
	return ty;
}
static void fields(Type ty) {
	{ int n = 0;
	  while (istypename(t, tsym)) {
	  	static char stop[] = { IF, CHAR, '}', 0 };
	  	Type ty1 = specifier(NULL);
	  	for (;;) {
	  		Field p;
	  		char *id = NULL;
	  		Type fty = dclr(ty1, &id, NULL, 0);
			p = newfield(id, ty, fty);
			if (Aflag >= 1 && !hasproto(p->type))
				warning("missing prototype\n");
			if (t == ':') {
				if (unqual(p->type) != inttype
				&&  unqual(p->type) != unsignedtype) {
					error("`%t' is an illegal bit-field type\n",
						p->type);
					p->type = inttype;
				}
				t = gettok();
				p->bitsize = intexpr(0, 0);
				if (p->bitsize > 8*inttype->size || p->bitsize < 0) {
					error("`%d' is an illegal bit-field size\n",
						p->bitsize);
					p->bitsize = 8*inttype->size;
				} else if (p->bitsize == 0 && id) {
					warning("extraneous 0-width bit field `%t %s' ignored\n", p->type, id);

					p->name = stringd(genlabel(1));
				}
				p->lsb = 1;
			}
			else {
				if (id == NULL)
					error("field name missing\n");
				else if (isfunc(p->type))
					error("`%t' is an illegal field type\n", p->type);
				else if (p->type->size == 0)
					error("undefined size for field `%t %s'\n",
						p->type, id);
			}
			if (isconst(p->type))
				ty->u.sym->u.s.cfields = 1;
			if (isvolatile(p->type))
				ty->u.sym->u.s.vfields = 1;
	  		n++;
	  		if (Aflag >= 2 && n == 128)
	  			warning("more than 127 fields in `%t'\n", ty);
	  		if (t != ',')
	  			break;
	  		t = gettok();
	  	}
	  	test(';', stop);
	  } }
	{ int bits = 0, off = 0, overflow = 0;
	  Field p, *q = &ty->u.sym->u.s.flist;
	  ty->align = IR->structmetric.align;
	  for (p = *q; p; p = p->link) {
	  	int a = p->type->align ? p->type->align : 1;
		if (p->lsb)
			a = unsignedtype->align;
		if (ty->op == UNION)
			off = bits = 0;
		else if (p->bitsize == 0 || bits == 0
		|| bits - 1 + p->bitsize > 8*unsignedtype->size) {
			off = add(off, bits2bytes(bits-1));
			bits = 0;
			chkoverflow(off, a - 1);
			off = roundup(off, a);
		}
		if (a > ty->align)
			ty->align = a;
		p->offset = off;

		if (p->lsb) {
			if (bits == 0)
				bits = 1;
			if (IR->little_endian)
				p->lsb = bits;
			else
				p->lsb = 8*unsignedtype->size - bits + 1
					- p->bitsize + 1;
			bits += p->bitsize;
		} else
			off = add(off, p->type->size);
		if (off + bits2bytes(bits-1) > ty->size)
			ty->size = off + bits2bytes(bits-1);
	  	if (p->name == NULL
	  	|| !('1' <= *p->name && *p->name <= '9')) {
	  		*q = p;
	  		q = &p->link;
	  	}
	  }
	  *q = NULL;
	  chkoverflow(ty->size, ty->align - 1);
	  ty->size = roundup(ty->size, ty->align);
	  if (overflow) {
	  	error("size of `%t' exceeds %d bytes\n", ty, inttype->u.sym->u.limits.max.i);
	  	ty->size = inttype->u.sym->u.limits.max.i&(~(ty->align - 1));
	  } }
}
static void funcdefn(int sclass, char *id, Type ty, Symbol params[], Coordinate pt) {
	int i, n;
	Symbol *callee, *caller, p;
	Type rty = freturn(ty);

	if (isstruct(rty) && rty->size == 0)
		error("illegal use of incomplete type `%t'\n", rty);
	for (n = 0; params[n]; n++)
		;
	if (n > 0 && params[n-1]->name == NULL)
		params[--n] = NULL;
	if (Aflag >= 2 && n > 31)
		warning("more than 31 parameters in function `%s'\n", id);
	if (ty->u.f.oldstyle) {
		if (Aflag >= 1)
			warning("old-style function definition for `%s'\n", id);
		caller = params;
		callee = newarray(n + 1, sizeof *callee, FUNC);
		memcpy(callee, caller, (n+1)*sizeof *callee);
		enterscope();
		assert(level == PARAM);
		while (kind[t] == STATIC || istypename(t, tsym))
			decl(dclparam);
		foreach(identifiers, PARAM, oldparam, callee);

		for (i = 0; (p = callee[i]) != NULL; i++) {
			if (!p->defined)
				callee[i] = dclparam(0, p->name, inttype, &p->src);
			*caller[i] = *p;
			caller[i]->sclass = AUTO;
			caller[i]->type = promote(p->type);
		}
		p = lookup(id, identifiers);
		if (p && p->scope == GLOBAL && isfunc(p->type)
		&& p->type->u.f.proto) {
			Type *proto = p->type->u.f.proto;
			for (i = 0; caller[i] && proto[i]; i++) {
				Type ty = unqual(proto[i]);
				if (eqtype(isenum(ty) ? ty->type : ty,
					unqual(caller[i]->type), 1) == 0)
					break;
				else if (isenum(ty) && !isenum(unqual(caller[i]->type)))
					warning("compatibility of `%t' and `%t' is compiler dependent\n",
						proto[i], caller[i]->type);
			}
			if (proto[i] || caller[i])
				error("conflicting argument declarations for function `%s'\n", id);

		}
		else {
			Type *proto = newarray(n + 1, sizeof *proto, PERM);
			if (Aflag >= 1)
				warning("missing prototype for `%s'\n", id);
			for (i = 0; i < n; i++)
				proto[i] = caller[i]->type;
			proto[i] = NULL;
			ty = func(rty, proto, 1);
		}
	} else {
		callee = params;
		caller = newarray(n + 1, sizeof *caller, FUNC);
		for (i = 0; (p = callee[i]) != NULL && p->name; i++) {
			NEW(caller[i], FUNC);
			*caller[i] = *p;
			if (isint(p->type))
				caller[i]->type = promote(p->type);
			caller[i]->sclass = AUTO;
			if ('1' <= *p->name && *p->name <= '9')
				error("missing name for parameter %d to function `%s'\n", i + 1, id);

		}
		caller[i] = NULL;
	}
	for (i = 0; (p = callee[i]) != NULL; i++)
		if (p->type->size == 0) {
			error("undefined size for parameter `%t %s'\n",
				p->type, p->name);
			caller[i]->type = p->type = inttype;
		}
	if (Aflag >= 2 && sclass != STATIC && strcmp(id, "main") == 0) {
		if (ty->u.f.oldstyle)
			warning("`%t %s()' is a non-ANSI definition\n", rty, id);
		else if (!(rty == inttype
			&& ((n == 0 && callee[0] == NULL)
			||  (n == 2 && callee[0]->type == inttype
			&& isptr(callee[1]->type) && callee[1]->type->type == charptype
			&& !variadic(ty)))))
			warning("`%s' is a non-ANSI definition\n", typestring(ty, id));
	}
	p = lookup(id, identifiers);
	if (p && isfunc(p->type) && p->defined)
		error("redefinition of `%s' previously defined at %w\n",
			p->name, &p->src);
	cfunc = dclglobal(sclass, id, ty, &pt);
	cfunc->u.f.label = genlabel(1);
	cfunc->u.f.callee = callee;
	cfunc->u.f.pt = src;
	cfunc->defined = 1;
	if (xref)
		use(cfunc, cfunc->src);
	if (Pflag)
		printproto(cfunc, cfunc->u.f.callee);
	if (ncalled >= 0)
		ncalled = findfunc(cfunc->name, pt.file);
	labels   = table(NULL, LABELS);
	stmtlabs = table(NULL, LABELS);
	refinc = 1.0;
	regcount = 0;
	codelist = &codehead;
	codelist->next = NULL;
	if (!IR->wants_callb && isstruct(rty))
		retv = genident(AUTO, ptr(rty), PARAM);
	compound(0, NULL, 0);

	{
		Code cp;
		for (cp = codelist; cp->kind < Label; cp = cp->prev)
			;
		if (cp->kind != Jump) {
			if (rty != voidtype) {
				warning("missing return value\n");
				retcode(cnsttree(inttype, 0L));
			} else
				retcode(NULL);
		}
	}
	definelab(cfunc->u.f.label);
	if (events.exit)
		apply(events.exit, cfunc, NULL);
	walk(NULL, 0, 0);
	exitscope();
	assert(level == PARAM);
	foreach(identifiers, level, checkref, NULL);
	if (!IR->wants_callb && isstruct(rty)) {
		Symbol *a;
		a = newarray(n + 2, sizeof *a, FUNC);
		a[0] = retv;
		memcpy(&a[1], callee, (n+1)*sizeof *callee);
		callee = a;
		a = newarray(n + 2, sizeof *a, FUNC);
		NEW(a[0], FUNC);
		*a[0] = *retv;
		memcpy(&a[1], caller, (n+1)*sizeof *callee);
		caller = a;
	}
	if (!IR->wants_argb)
		for (i = 0; caller[i]; i++)
			if (isstruct(caller[i]->type)) {
				caller[i]->type = ptr(caller[i]->type);
				callee[i]->type = ptr(callee[i]->type);
				caller[i]->structarg = callee[i]->structarg = 1;
			}
	if (glevel > 1)	for (i = 0; callee[i]; i++) callee[i]->sclass = AUTO;
	if (cfunc->sclass != STATIC)
		(*IR->export)(cfunc);
	if (glevel && IR->stabsym) {
		swtoseg(CODE); (*IR->stabsym)(cfunc); }
	swtoseg(CODE);
	(*IR->function)(cfunc, caller, callee, cfunc->u.f.ncalls);
	if (glevel && IR->stabfend)
		(*IR->stabfend)(cfunc, lineno);
	foreach(stmtlabs, LABELS, checklab, NULL);
	exitscope();
	expect('}');
	labels = stmtlabs = NULL;
	retv  = NULL;
	cfunc = NULL;
}
static void oldparam(Symbol p, void *cl) {
	int i;
	Symbol *callee = cl;

	for (i = 0; callee[i]; i++)
		if (p->name == callee[i]->name) {
			callee[i] = p;
			return;
		}
	error("declared parameter `%s' is missing\n", p->name);
}
void compound(int loop, struct swtch *swp, int lev) {
	Code cp;
	int nregs;

	walk(NULL, 0, 0);
	cp = code(Blockbeg);
	enterscope();
	assert(level >= LOCAL);
	if (level == LOCAL && events.entry)
		apply(events.entry, cfunc, NULL);
	definept(NULL);
	expect('{');
	autos = registers = NULL;
	if (level == LOCAL && IR->wants_callb
	&& isstruct(freturn(cfunc->type))) {
		retv = genident(AUTO, ptr(freturn(cfunc->type)), level);
		retv->defined = 1;
		retv->ref = 1;
		registers = append(retv, registers);
	}
	while (kind[t] == CHAR || kind[t] == STATIC
	|| (istypename(t, tsym) && getchr() != ':'))
		decl(dcllocal);
	{
		int i;
		Symbol *a = ltov(&autos, STMT);
		nregs = length(registers);
		for (i = 0; a[i]; i++)
			registers = append(a[i], registers);
		cp->u.block.locals = ltov(&registers, FUNC);
	}
	if (events.blockentry)
		apply(events.blockentry, cp->u.block.locals, NULL);
	while (kind[t] == IF || kind[t] == ID)
		statement(loop, swp, lev);
	walk(NULL, 0, 0);
	foreach(identifiers, level, checkref, NULL);
	{
		int i = nregs, j;
		Symbol p;
		for ( ; (p = cp->u.block.locals[i]) != NULL; i++) {
			for (j = i; j > nregs
				&& cp->u.block.locals[j-1]->ref < p->ref; j--)
				cp->u.block.locals[j] = cp->u.block.locals[j-1];
			cp->u.block.locals[j] = p;
		}
	}
	if (events.blockexit)
		apply(events.blockexit, cp->u.block.locals, NULL);
	cp->u.block.level = level;
	cp->u.block.identifiers = identifiers;
	cp->u.block.types = types;
	code(Blockend)->u.begin = cp;
	if (reachable(Gen))
		definept(NULL);
	if (level > LOCAL) {
		exitscope();
		expect('}');
	}
}
static void checkref(Symbol p, void *cl) {
	if (p->scope >= PARAM
	&& (isvolatile(p->type) || isfunc(p->type)))
		p->addressed = 1;
	if (Aflag >= 2 && p->defined && p->ref == 0) {
		if (p->sclass == STATIC)
			warning("static `%t %s' is not referenced\n",
				p->type, p->name);
		else if (p->scope == PARAM)
			warning("parameter `%t %s' is not referenced\n",
				p->type, p->name);
		else if (p->scope >= LOCAL && p->sclass != EXTERN)
			warning("local `%t %s' is not referenced\n",
				p->type, p->name);
	}
	if (p->sclass == AUTO
	&& ((p->scope  == PARAM && regcount == 0)
	 || p->scope  >= LOCAL)
	&& !p->addressed && isscalar(p->type) && p->ref >= 3.0)
		p->sclass = REGISTER;
	if (level == GLOBAL && p->sclass == STATIC && !p->defined
	&& isfunc(p->type) && p->ref)
		error("undefined static `%t %s'\n", p->type, p->name);
	assert(!(level == GLOBAL && p->sclass == STATIC && !p->defined && !isfunc(p->type)));
}
static Symbol dcllocal(int sclass, char *id, Type ty, Coordinate *pos) {
	Symbol p, q;

	if (sclass == 0)
		sclass = isfunc(ty) ? EXTERN : AUTO;
	else if (isfunc(ty) && sclass != EXTERN) {
		error("invalid storage class `%k' for `%t %s'\n",
			sclass, ty, id);
		sclass = EXTERN;
	} else if (sclass == REGISTER
	&& (isvolatile(ty) || isstruct(ty) || isarray(ty))) {
		warning("register declaration ignored for `%t %s'\n",
			ty, id);
		sclass = AUTO;
	}
	q = lookup(id, identifiers);
	if ((q && q->scope >= level)
	||  (q && q->scope == PARAM && level == LOCAL)) {
		if (sclass == EXTERN && q->sclass == EXTERN
		&& eqtype(q->type, ty, 1))
			ty = compose(ty, q->type);
		else
			error("redeclaration of `%s' previously declared at %w\n", q->name, &q->src);
	}

	assert(level >= LOCAL);
	p = install(id, &identifiers, level, sclass == STATIC || sclass == EXTERN ? PERM : FUNC);
	p->type = ty;
	p->sclass = sclass;
	p->src = *pos;
	switch (sclass) {
	case EXTERN:   q = lookup(id, globals);
		       if (q == NULL || q->sclass == TYPEDEF || q->sclass == ENUM) {
		       	q = lookup(id, externals);
		       	if (q == NULL) {
		       		q = install(p->name, &externals, GLOBAL, PERM);
		       		q->type = p->type;
		       		q->sclass = EXTERN;
		       		q->src = src;
		       		(*IR->defsymbol)(q);
		       	}
		       }
		       if (!eqtype(p->type, q->type, 1))
		       	warning("declaration of `%s' does not match previous declaration at %w\n", q->name, &q->src);

		       p->u.alias = q; break;
	case STATIC:   (*IR->defsymbol)(p);
		       initglobal(p, 0);
		       if (!p->defined) {
		       	if (p->type->size > 0) {
		       		defglobal(p, BSS);
		       		(*IR->space)(p->type->size);
		       	} else
		       		error("undefined size for `%t %s'\n",
		       			p->type, p->name);
		       }
		       p->defined = 1; break;
	case REGISTER: registers = append(p, registers);
		       regcount++;
		       p->defined = 1;
 break;
	case AUTO:     autos = append(p, autos);
		       p->defined = 1; break;
	default: assert(0);
	}
	if (t == '=') {
		Tree e;
		if (sclass == EXTERN)
			error("illegal initialization of `extern %s'\n", id);
		t = gettok();
		definept(NULL);
		if (isscalar(p->type)
		||  (isstruct(p->type) && t != '{')) {
			if (t == '{') {
				t = gettok();
				e = expr1(0);
				expect('}');
			} else
				e = expr1(0);
		} else {
			Symbol t1;
			Type ty = p->type, ty1 = ty;
			while (isarray(ty1))
				ty1 = ty1->type;
			if (!isconst(ty) && (!isarray(ty) || !isconst(ty1)))
				ty = qual(CONST, ty);
			t1 = genident(STATIC, ty, GLOBAL);
			initglobal(t1, 1);
			if (isarray(p->type) && p->type->size == 0
			&& t1->type->size > 0)
				p->type = array(p->type->type,
					t1->type->size/t1->type->type->size, 0);
			e = idtree(t1);
		}
		walk(root(asgn(p, e)), 0, 0);
		p->ref = 1;
	}
	if (!isfunc(p->type) && p->defined && p->type->size <= 0)
		error("undefined size for `%t %s'\n", p->type, id);
	return p;
}
void finalize(void) {
	foreach(externals,   GLOBAL,    doextern, NULL);
	foreach(identifiers, GLOBAL,    doglobal, NULL);
	foreach(identifiers, GLOBAL,    checkref, NULL);
	foreach(constants,   CONSTANTS, doconst,  NULL);
}
static void doextern(Symbol p, void *cl) {
	(*IR->import)(p);
}
static void doglobal(Symbol p, void *cl) {
	if (!p->defined && (p->sclass == EXTERN
	|| (isfunc(p->type) && p->sclass == AUTO)))
		(*IR->import)(p);
	else if (!p->defined && !isfunc(p->type)
	&& (p->sclass == AUTO || p->sclass == STATIC)) {
		if (isarray(p->type)
		&& p->type->size == 0 && p->type->type->size > 0)
			p->type = array(p->type->type, 1, 0);
		if (p->type->size > 0) {
			defglobal(p, BSS);
			(*IR->space)(p->type->size);
			if (glevel > 0 && IR->stabsym)
				(*IR->stabsym)(p);
		} else
			error("undefined size for `%t %s'\n",
				p->type, p->name);
		p->defined = 1;
	}
	if (Pflag
	&& !isfunc(p->type)
	&& !p->generated && p->sclass != EXTERN)
		printdecl(p, p->type);
}
void doconst(Symbol p, void *cl) {
	if (p->u.c.loc) {
		assert(p->u.c.loc->u.seg == 0); 
		defglobal(p->u.c.loc, LIT);
		if (isarray(p->type) && p->type->type == widechar) {
			unsigned int *s = p->u.c.v.p;
			int n = p->type->size/widechar->size;
			while (n-- > 0) {
				Value v;
				v.u = *s++;
				(*IR->defconst)(widechar->op, widechar->size, v);
			}
		} else if (isarray(p->type))
			(*IR->defstring)(p->type->size, p->u.c.v.p);
		else
			(*IR->defconst)(p->type->op, p->type->size, p->u.c.v);
		p->u.c.loc = NULL;
	}
}
void checklab(Symbol p, void *cl) {
	if (!p->defined)
		error("undefined label `%s'\n", p->name);
	p->defined = 1;
}

Type enumdcl(void) {
	char *tag;
	Type ty;
	Symbol p = {0};
	Coordinate pos;

	t = gettok();
	pos = src;
	if (t == ID) {
		tag = token;
		t = gettok();
	} else
		tag = "";
	if (t == '{') {
		static char follow[] = { IF, 0 };
		int n = 0;
		long k = -1;
		List idlist = 0;
		ty = newstruct(ENUM, tag);
		t = gettok();
		if (t != ID)
			error("expecting an enumerator identifier\n");
		while (t == ID) {
			char *id = token;
			Coordinate s;
			if (tsym && tsym->scope == level)
				error("redeclaration of `%s' previously declared at %w\n",
					token, &tsym->src);
			s = src;
			t = gettok();
			if (t == '=') {
				t = gettok();
				k = intexpr(0, 0);
			} else {
				if (k == inttype->u.sym->u.limits.max.i)
					error("overflow in value for enumeration constant `%s'\n", id);
				k++;
			}
			p = install(id, &identifiers, level,  level < LOCAL ? PERM : FUNC);
			p->src = s;
			p->type = ty;
			p->sclass = ENUM;
			p->u.value = k;
			idlist = append(p, idlist);
			n++;
			if (Aflag >= 2 && n == 128)
				warning("more than 127 enumeration constants in `%t'\n", ty);
			if (t != ',')
				break;
			t = gettok();
			if (Aflag >= 2 && t == '}')
				warning("non-ANSI trailing comma in enumerator list\n");
		}
		test('}', follow);
		ty->type = inttype;
		ty->size = ty->type->size;
		ty->align = ty->type->align;
		ty->u.sym->u.idlist = ltov(&idlist, PERM);
		ty->u.sym->defined = 1;
	} else if ((p = lookup(tag, types)) != NULL && p->type->op == ENUM) {
		ty = p->type;
		if (t == ';')
			error("empty declaration\n");
	} else {
		error("unknown enumeration `%s'\n",  tag);
		ty = newstruct(ENUM, tag);
		ty->type = inttype;
	}
	if (*tag && xref)
		use(p, pos);
	return ty;
}

Type typename(void) {
	Type ty = specifier(NULL);

	if (t == '*' || t == '(' || t == '[') {
		ty = dclr(ty, NULL, NULL, 1);
		if (Aflag >= 1 && !hasproto(ty))
			warning("missing prototype\n");
	}
	return ty;
}

