#include <string.h>
#include <stdlib.h>
#include "c.h"
#include "stab.h"


static char *currentfile;       /* current file name */
static int ntypes;

extern Interface sparcIR;

char *stabprefix = "L";

extern char *stabprefix;
extern void stabblock(int, int, Symbol*);
extern void stabend(Coordinate *, Symbol, Coordinate **, Symbol *, Symbol *);
extern void stabfend(Symbol, int);
extern void stabinit(char *, int, char *[]);
extern void stabline(Coordinate *);
extern void stabsym(Symbol);
extern void stabtype(Symbol);

static void asgncode(Type, int);
static void dbxout(Type);
static int dbxtype(Type);
static int emittype(Type, int, int);

/* asgncode - assign type code to ty */
static void asgncode(Type ty, int lev) {
	if (ty->x.marked || ty->x.typeno)
		return;
	ty->x.marked = 1;
	switch (ty->op) {
	case VOLATILE: case CONST: case VOLATILE+CONST:
		asgncode(ty->type, lev);
		ty->x.typeno = ty->type->x.typeno;
		break;
	case POINTER: case FUNCTION: case ARRAY:
		asgncode(ty->type, lev + 1);
		/* fall thru */
	case VOID: case INT: case UNSIGNED: case FLOAT:
		break;
	case STRUCT: case UNION: {
		Field p;
		for (p = fieldlist(ty); p; p = p->link)
			asgncode(p->type, lev + 1);
		/* fall thru */
	case ENUM:
		if (ty->x.typeno == 0)
			ty->x.typeno = ++ntypes;
		if (lev > 0 && (*ty->u.sym->name < '0' || *ty->u.sym->name > '9'))
			dbxout(ty);
		break;
		}
	default:
		assert(0);
	}
}

/* dbxout - output .stabs entry for type ty */
static void dbxout(Type ty) {
	ty = unqual(ty);
	if (!ty->x.printed) {
		int col = 0;
		print(".stabs \""), col += 8;
		if (ty->u.sym && !(isfunc(ty) || isarray(ty) || isptr(ty)))
			print("%s", ty->u.sym->name), col += strlen(ty->u.sym->name);
		print(":%c", isstruct(ty) || isenum(ty) ? 'T' : 't'), col += 2;
		emittype(ty, 0, col);
		print("\",%d,0,0,0\n", N_LSYM);
	}
}

/* dbxtype - emit a stabs entry for type ty, return type code */
static int dbxtype(Type ty) {
	asgncode(ty, 0);
	dbxout(ty);
	return ty->x.typeno;
}

/*
 * emittype - emit ty's type number, emitting its definition if necessary.
 * Returns the output column number after emission; col is the approximate
 * output column before emission and is used to emit continuation lines for long
 * struct, union, and enum types. Continuations are not emitted for other types,
 * even if the definition is long. lev is the depth of calls to emittype.
 */
static int emittype(Type ty, int lev, int col) {
	int tc = ty->x.typeno;

	if (isconst(ty) || isvolatile(ty)) {
		col = emittype(ty->type, lev, col);
		ty->x.typeno = ty->type->x.typeno;
		ty->x.printed = 1;
		return col;
	}
	if (tc == 0) {
		ty->x.typeno = tc = ++ntypes;
/*              fprint(2,"`%t'=%d\n", ty, tc); */
	}
	print("%d", tc), col += 3;
	if (ty->x.printed)
		return col;
	ty->x.printed = 1;
	switch (ty->op) {
	case VOID:	/* void is defined as itself */
		print("=%d", tc), col += 1+3;
		break;
	case INT:
		if (ty == chartype)	/* plain char is a subrange of itself */
			print("=r%d;%d;%d;", tc, ty->u.sym->u.limits.min.i, ty->u.sym->u.limits.max.i),
				col += 2+3+2*2.408*ty->size+2;
		else			/* other signed ints are subranges of int */
			print("=r1;%D;%D;", ty->u.sym->u.limits.min.i, ty->u.sym->u.limits.max.i),
				col += 4+2*2.408*ty->size+2;
		break;
	case UNSIGNED:
		if (ty == chartype)	/* plain char is a subrange of itself */
			print("=r%d;0;%u;", tc, ty->u.sym->u.limits.max.i),
				col += 2+3+2+2.408*ty->size+1;
		else			/* other signed ints are subranges of int */
			print("=r1;0;%U;", ty->u.sym->u.limits.max.i),
				col += 4+2.408*ty->size+1;
		break;
	case FLOAT:	/* float, double, long double get sizes, not ranges */
		print("=r1;%d;0;", ty->size), col += 4+1+3;
		break;
	case POINTER:
		print("=*"), col += 2;
		col = emittype(ty->type, lev + 1, col);
		break;
	case FUNCTION:
		print("=f"), col += 2;
		col = emittype(ty->type, lev + 1, col);
		break;
	case ARRAY:	/* array includes subscript as an int range */
		if (ty->size && ty->type->size)
			print("=ar1;0;%d;", ty->size/ty->type->size - 1), col += 7+3+1;
		else
			print("=ar1;0;-1;"), col += 10;
		col = emittype(ty->type, lev + 1, col);
		break;
	case STRUCT: case UNION: {
		Field p;
		if (!ty->u.sym->defined) {
			print("=x%c%s:", ty->op == STRUCT ? 's' : 'u', ty->u.sym->name);
			col += 2+1+strlen(ty->u.sym->name)+1;
			break;
		}
		if (lev > 0 && (*ty->u.sym->name < '0' || *ty->u.sym->name > '9')) {
			ty->x.printed = 0;
			break;
		}
		print("=%c%d", ty->op == STRUCT ? 's' : 'u', ty->size), col += 1+1+3;
		for (p = fieldlist(ty); p; p = p->link) {
			if (p->name)
				print("%s:", p->name), col += strlen(p->name)+1;
			else
				print(":"), col += 1;
			col = emittype(p->type, lev + 1, col);
			if (p->lsb)
				print(",%d,%d;", 8*p->offset +
					(IR->little_endian ? fieldright(p) : fieldleft(p)),
					fieldsize(p));
			else
				print(",%d,%d;", 8*p->offset, 8*p->type->size);
			col += 1+3+1+3+1;	/* accounts for ,%d,%d; */
			if (col >= 80 && p->link) {
				print("\\\\\",%d,0,0,0\n.stabs \"", N_LSYM);
				col = 8;
			}
		}
		print(";"), col += 1;
		break;
		}
	case ENUM: {
		Symbol *p;
		if (lev > 0 && (*ty->u.sym->name < '0' || *ty->u.sym->name > '9')) {
			ty->x.printed = 0;
			break;
		}
		print("=e"), col += 2;
		for (p = ty->u.sym->u.idlist; *p; p++) {
			print("%s:%d,", (*p)->name, (*p)->u.value), col += strlen((*p)->name)+3;
			if (col >= 80 && p[1]) {
				print("\\\\\",%d,0,0,0\n.stabs \"", N_LSYM);
				col = 8;
			}
		}
		print(";"), col += 1;
		break;
		}
	default:
		assert(0);
	}
	return col;
}

/* stabblock - output a stab entry for '{' or '}' at level lev */
void stabblock(int brace, int lev, Symbol *p) {
	if (brace == '{')
		while (*p)
			stabsym(*p++);
	if (IR == &sparcIR)
		print(".stabd 0x%x,0,%d\n", brace == '{' ? N_LBRAC : N_RBRAC, lev);
	else {
		int lab = genlabel(1);
		print(".stabn 0x%x,0,%d,%s%d-%s\n", brace == '{' ? N_LBRAC : N_RBRAC, lev,
			stabprefix, lab, cfunc->x.name);
		print("%s%d:\n", stabprefix, lab);
	}
}

/* stabinit - initialize stab output */
void stabinit(char *file, int argc, char *argv[]) {
	typedef void (*Closure)(Symbol, void *);
	extern char *getcwd(char *, size_t);

	print(".stabs \"lcc4_compiled.\",0x%x,0,0,0\n", N_OPT);
	if (file && *file) {
		char buf[1024], *cwd = getcwd(buf, sizeof buf);
		if (cwd)
			print(".stabs \"%s/\",0x%x,0,3,%stext0\n", cwd, N_SO, stabprefix);
		print(".stabs \"%s\",0x%x,0,3,%stext0\n", file, N_SO, stabprefix);
		(*IR->segment)(CODE);
		print("%stext0:\n", stabprefix, N_SO);
		currentfile = file;
	}
	dbxtype(inttype);
	dbxtype(chartype);
	dbxtype(doubletype);
	dbxtype(floattype);
	dbxtype(longdouble);
	dbxtype(longtype);
	dbxtype(longlong);
	dbxtype(shorttype);
	dbxtype(signedchar);
	dbxtype(unsignedchar);
	dbxtype(unsignedlong);
	dbxtype(unsignedlonglong);
	dbxtype(unsignedshort);
	dbxtype(unsignedtype);
	dbxtype(voidtype);
	foreach(types, GLOBAL, (Closure)stabtype, NULL);
}

/* stabline - emit stab entry for source coordinate *cp */
void stabline(Coordinate *cp) {
	if (cp->file && cp->file != currentfile) {
		int lab = genlabel(1);
		print(".stabs \"%s\",0x%x,0,0,%s%d\n", cp->file, N_SOL, stabprefix, lab);
		print("%s%d:\n", stabprefix, lab);
		currentfile = cp->file;
	}
	if (IR == &sparcIR)
		print(".stabd 0x%x,0,%d\n", N_SLINE, cp->y);
	else {
		int lab = genlabel(1);
		print(".stabn 0x%x,0,%d,%s%d-%s\n", N_SLINE, cp->y,
			stabprefix, lab, cfunc->x.name);
		print("%s%d:\n", stabprefix, lab);
	}
}

/* stabsym - output a stab entry for symbol p */
void stabsym(Symbol p) {
	int code, tc, sz = p->type->size;

	if (p->generated || p->computed)
		return;
	if (isfunc(p->type)) {
		print(".stabs \"%s:%c%d\",%d,0,0,%s\n", p->name,
			p->sclass == STATIC ? 'f' : 'F', dbxtype(freturn(p->type)),
			N_FUN, p->x.name);
		return;
	}
	if (!IR->wants_argb && p->scope == PARAM && p->structarg) {
		assert(isptr(p->type) && isstruct(p->type->type));
		tc = dbxtype(p->type->type);
		sz = p->type->type->size;
	} else
		tc = dbxtype(p->type);
	if (p->sclass == AUTO && p->scope == GLOBAL || p->sclass == EXTERN) {
		print(".stabs \"%s:G", p->name);
		code = N_GSYM;
	} else if (p->sclass == STATIC) {
		print(".stabs \"%s:%c%d\",%d,0,0,%s\n", p->name, p->scope == GLOBAL ? 'S' : 'V',
			tc, p->u.seg == BSS ? N_LCSYM : N_STSYM, p->x.name);
		return;
	} else if (p->sclass == REGISTER) {
		if (p->x.regnode) {
			int r = p->x.regnode->number;
			if (p->x.regnode->set == FREG)
				r += 32;	/* floating point */
				print(".stabs \"%s:%c%d\",%d,0,", p->name,
					p->scope == PARAM ? 'P' : 'r', tc, N_RSYM);
			print("%d,%d\n", sz, r);
		}
		return;
	} else if (p->scope == PARAM) {
		print(".stabs \"%s:p", p->name);
		code = N_PSYM;
	} else if (p->scope >= LOCAL) {
		print(".stabs \"%s:", p->name);
		code = N_LSYM;
	} else
		assert(0);
	print("%d\",%d,0,0,%s\n", tc, code,
		p->scope >= PARAM && p->sclass != EXTERN ? p->x.name : "0");
}

/* stabtype - output a stab entry for type *p */
void stabtype(Symbol p) {
	if (p->type) {
		if (p->sclass == 0)
			dbxtype(p->type);
		else if (p->sclass == TYPEDEF)
			print(".stabs \"%s:t%d\",%d,0,0,0\n", p->name, dbxtype(p->type), N_LSYM);
	}
}

/* stabend - finalize a function */
void stabfend(Symbol p, int lineno) {}

/* stabend - finalize stab output */
void stabend(Coordinate *cp, Symbol p, Coordinate **cpp, Symbol *sp, Symbol *stab) {
	(*IR->segment)(CODE);
	print(".stabs \"\", %d, 0, 0,%setext\n", N_SO, stabprefix);
	print("%setext:\n", stabprefix);
}
