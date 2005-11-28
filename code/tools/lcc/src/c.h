#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#define NEW(p,a) ((p) = allocate(sizeof *(p), (a)))
#define NEW0(p,a) memset(NEW((p),(a)), 0, sizeof *(p))
#define isaddrop(op) (specific(op)==ADDRG+P || specific(op)==ADDRL+P \
	|| specific(op)==ADDRF+P)

#define	MAXLINE  512
#define	BUFSIZE 4096

#define istypename(t,tsym) (kind[t] == CHAR \
	|| (t == ID && tsym && tsym->sclass == TYPEDEF))
#define sizeop(n) ((n)<<10)
#define generic(op)  ((op)&0x3F0)
#define specific(op) ((op)&0x3FF)
#define opindex(op) (((op)>>4)&0x3F)
#define opkind(op)  ((op)&~0x3F0)
#define opsize(op)  ((op)>>10)
#define optype(op)  ((op)&0xF)
#ifdef __LCC__
#ifndef __STDC__
#define __STDC__
#endif
#endif
#define NELEMS(a) ((int)(sizeof (a)/sizeof ((a)[0])))
#undef roundup
#define roundup(x,n) (((x)+((n)-1))&(~((n)-1)))
#define mkop(op,ty) (specific((op) + ttob(ty)))

#define extend(x,ty) ((x)&(1<<(8*(ty)->size-1)) ? (x)|((~0UL)<<(8*(ty)->size-1)) : (x)&ones(8*(ty)->size))
#define ones(n) ((n)>=8*sizeof (unsigned long) ? ~0UL : ~((~0UL)<<(n)))

#define isqual(t)     ((t)->op >= CONST)
#define unqual(t)     (isqual(t) ? (t)->type : (t))

#define isvolatile(t) ((t)->op == VOLATILE \
                    || (t)->op == CONST+VOLATILE)
#define isconst(t)    ((t)->op == CONST \
                    || (t)->op == CONST+VOLATILE)
#define isarray(t)    (unqual(t)->op == ARRAY)
#define isstruct(t)   (unqual(t)->op == STRUCT \
                    || unqual(t)->op == UNION)
#define isunion(t)    (unqual(t)->op == UNION)
#define isfunc(t)     (unqual(t)->op == FUNCTION)
#define isptr(t)      (unqual(t)->op == POINTER)
#define ischar(t)     ((t)->size == 1 && isint(t))
#define isint(t)      (unqual(t)->op == INT \
                    || unqual(t)->op == UNSIGNED)
#define isfloat(t)    (unqual(t)->op == FLOAT)
#define isarith(t)    (unqual(t)->op <= UNSIGNED)
#define isunsigned(t) (unqual(t)->op == UNSIGNED)
#define isscalar(t)   (unqual(t)->op <= POINTER \
                    || unqual(t)->op == ENUM)
#define isenum(t)     (unqual(t)->op == ENUM)
#define fieldsize(p)  (p)->bitsize
#define fieldright(p) ((p)->lsb - 1)
#define fieldleft(p)  (8*(p)->type->size - \
                        fieldsize(p) - fieldright(p))
#define fieldmask(p)  (~(~(unsigned)0<<fieldsize(p)))
typedef struct node *Node;

typedef struct list *List;

typedef struct code *Code;

typedef struct swtch *Swtch;

typedef struct symbol *Symbol;

typedef struct coord {
	char *file;
	unsigned x, y;
} Coordinate;
typedef struct table *Table;

typedef union value {
	long i;
	unsigned long u;
	double d;
	void *p;
	void (*g)(void);
} Value;
typedef struct tree *Tree;

typedef struct type *Type;

typedef struct field *Field;

typedef struct {
	unsigned printed:1;
	unsigned marked;
	unsigned short typeno;
	void *xt;
} Xtype;

#include "config.h"
typedef struct metrics {
	unsigned char size, align, outofline;
} Metrics;
typedef struct interface {
	Metrics charmetric;
	Metrics shortmetric;
	Metrics intmetric;
	Metrics longmetric;
	Metrics longlongmetric;
	Metrics floatmetric;
	Metrics doublemetric;
	Metrics longdoublemetric;
	Metrics ptrmetric;
	Metrics structmetric;
	unsigned little_endian:1;
	unsigned mulops_calls:1;
	unsigned wants_callb:1;
	unsigned wants_argb:1;
	unsigned left_to_right:1;
	unsigned wants_dag:1;
	unsigned unsigned_char:1;
void (*address)(Symbol p, Symbol q, long n);
void (*blockbeg)(Env *);
void (*blockend)(Env *);
void (*defaddress)(Symbol);
void (*defconst)  (int suffix, int size, Value v);
void (*defstring)(int n, char *s);
void (*defsymbol)(Symbol);
void (*emit)    (Node);
void (*export)(Symbol);
void (*function)(Symbol, Symbol[], Symbol[], int);
Node (*gen)     (Node);
void (*global)(Symbol);
void (*import)(Symbol);
void (*local)(Symbol);
void (*progbeg)(int argc, char *argv[]);
void (*progend)(void);
void (*segment)(int);
void (*space)(int);
void (*stabblock)(int, int, Symbol*);
void (*stabend)  (Coordinate *, Symbol, Coordinate **, Symbol *, Symbol *);
void (*stabfend) (Symbol, int);
void (*stabinit) (char *, int, char *[]);
void (*stabline) (Coordinate *);
void (*stabsym)  (Symbol);
void (*stabtype) (Symbol);
	Xinterface x;
} Interface;
typedef struct binding {
	char *name;
	Interface *ir;
} Binding;

extern Binding bindings[];
extern Interface *IR;
typedef struct {
	List blockentry;
	List blockexit;
	List entry;
	List exit;
	List returns;
	List points;
	List calls;
	List end;
} Events;

enum {
#define xx(a,b,c,d,e,f,g) a=b,
#define yy(a,b,c,d,e,f,g)
#include "token.h"
	LAST
};
struct node {
	short op;
	short count;
 	Symbol syms[3];
	Node kids[2];
	Node link;
	Xnode x;
};
enum {
	F=FLOAT,
	I=INT,
	U=UNSIGNED,
	P=POINTER,
	V=VOID,
	B=STRUCT
};
#define gop(name,value) name=value<<4,
#define op(name,type,sizes)

enum { gop(CNST,1)
       	op(CNST,F,fdx)
       	op(CNST,I,csilh)
       	op(CNST,P,p)
       	op(CNST,U,csilh)
       gop(ARG,2)
       	op(ARG,B,-)
       	op(ARG,F,fdx)
       	op(ARG,I,ilh)
       	op(ARG,P,p)
       	op(ARG,U,ilh)
       gop(ASGN,3)
       	op(ASGN,B,-)
       	op(ASGN,F,fdx)
       	op(ASGN,I,csilh)
       	op(ASGN,P,p)
       	op(ASGN,U,csilh)
       gop(INDIR,4)
       	op(INDIR,B,-)
       	op(INDIR,F,fdx)
       	op(INDIR,I,csilh)
       	op(INDIR,P,p)
       	op(INDIR,U,csilh)
       gop(CVF,7)
       	op(CVF,F,fdx)
       	op(CVF,I,ilh)
       gop(CVI,8)
       	op(CVI,F,fdx)
       	op(CVI,I,csilh)
       	op(CVI,U,csilhp)
       gop(CVP,9)
       	op(CVP,U,p)
       gop(CVU,11)
       	op(CVU,I,csilh)
       	op(CVU,P,p)
       	op(CVU,U,csilh)
       gop(NEG,12)
       	op(NEG,F,fdx)
       	op(NEG,I,ilh)
       gop(CALL,13)
       	op(CALL,B,-)
       	op(CALL,F,fdx)
       	op(CALL,I,ilh)
       	op(CALL,P,p)
       	op(CALL,U,ilh)
       	op(CALL,V,-)
       gop(RET,15)
       	op(RET,F,fdx)
       	op(RET,I,ilh)
       	op(RET,P,p)
       	op(RET,U,ilh)
       	op(RET,V,-)
       gop(ADDRG,16)
       	op(ADDRG,P,p)
       gop(ADDRF,17)
       	op(ADDRF,P,p)
       gop(ADDRL,18)
       	op(ADDRL,P,p)
       gop(ADD,19)
       	op(ADD,F,fdx)
       	op(ADD,I,ilh)
       	op(ADD,P,p)
       	op(ADD,U,ilhp)
       gop(SUB,20)
       	op(SUB,F,fdx)
       	op(SUB,I,ilh)
       	op(SUB,P,p)
       	op(SUB,U,ilhp)
       gop(LSH,21)
       	op(LSH,I,ilh)
       	op(LSH,U,ilh)
       gop(MOD,22)
       	op(MOD,I,ilh)
       	op(MOD,U,ilh)
       gop(RSH,23)
       	op(RSH,I,ilh)
       	op(RSH,U,ilh)
       gop(BAND,24)
       	op(BAND,I,ilh)
       	op(BAND,U,ilh)
       gop(BCOM,25)
       	op(BCOM,I,ilh)
       	op(BCOM,U,ilh)
       gop(BOR,26)
       	op(BOR,I,ilh)
       	op(BOR,U,ilh)
       gop(BXOR,27)
       	op(BXOR,I,ilh)
       	op(BXOR,U,ilh)
       gop(DIV,28)
       	op(DIV,F,fdx)
       	op(DIV,I,ilh)
       	op(DIV,U,ilh)
       gop(MUL,29)
       	op(MUL,F,fdx)
       	op(MUL,I,ilh)
       	op(MUL,U,ilh)
       gop(EQ,30)
       	op(EQ,F,fdx)
       	op(EQ,I,ilh)
       	op(EQ,U,ilhp)
       gop(GE,31)
       	op(GE,F,fdx)
       	op(GE,I,ilh)
       	op(GE,U,ilhp)
       gop(GT,32)
       	op(GT,F,fdx)
       	op(GT,I,ilh)
       	op(GT,U,ilhp)
       gop(LE,33)
       	op(LE,F,fdx)
       	op(LE,I,ilh)
       	op(LE,U,ilhp)
       gop(LT,34)
       	op(LT,F,fdx)
       	op(LT,I,ilh)
       	op(LT,U,ilhp)
       gop(NE,35)
       	op(NE,F,fdx)
       	op(NE,I,ilh)
       	op(NE,U,ilhp)
       gop(JUMP,36)
       	op(JUMP,V,-)
       gop(LABEL,37)
       	op(LABEL,V,-)
       gop(LOAD,14)
       	op(LOAD,B,-)
       	op(LOAD,F,fdx)
       	op(LOAD,I,csilh)
       	op(LOAD,P,p)
       	op(LOAD,U,csilhp) LASTOP };

#undef gop
#undef op
enum { CODE=1, BSS, DATA, LIT };
enum { PERM=0, FUNC, STMT };
struct list {
	void *x;
	List link;
};

struct code {
	enum { Blockbeg, Blockend, Local, Address, Defpoint,
	       Label,    Start,    Gen,   Jump,    Switch
	} kind;
	Code prev, next;
	union {
		struct {
			int level;
			Symbol *locals;
			Table identifiers, types;
			Env x;
		} block;
		Code begin;
		Symbol var;

		struct {
			Symbol sym;
			Symbol base;
			long offset;
		} addr;
		struct {
			Coordinate src;
			int point;
		} point; 
		Node forest;
		struct {
			Symbol sym;
			Symbol table;
			Symbol deflab;
			int size;
			long *values;
			Symbol *labels;
		} swtch;

	} u;
};
struct swtch {
	Symbol sym;
	int lab;
	Symbol deflab;
	int ncases;
	int size;
	long *values;
	Symbol *labels;
};
struct symbol {
	char *name;
	int scope;
	Coordinate src;
	Symbol up;
	List uses;
	int sclass;
	unsigned structarg:1;

	unsigned addressed:1;
	unsigned computed:1;
	unsigned temporary:1;
	unsigned generated:1;
	unsigned defined:1;
	Type type;
	float ref;
	union {
		struct {
			int label;
			Symbol equatedto;
		} l;
		struct {
			unsigned cfields:1;
			unsigned vfields:1;
			Table ftab;		/* omit */
			Field flist;
		} s;
		int value;
		Symbol *idlist;
		struct {
			Value min, max;
		} limits;
		struct {
			Value v;
			Symbol loc;
		} c;
		struct {
			Coordinate pt;
			int label;
			int ncalls;
			Symbol *callee;
		} f;
		int seg;
		Symbol alias;
		struct {
			Node cse;
			int replace;
			Symbol next;
		} t;
	} u;
	Xsymbol x;
};
enum { CONSTANTS=1, LABELS, GLOBAL, PARAM, LOCAL };
struct tree {
	int op;
	Type type;
	Tree kids[2];
	Node node;
	union {
		Value v;
		Symbol sym;

		Field field;
	} u;
};
enum {
	AND=38<<4,
	NOT=39<<4,
	OR=40<<4,
	COND=41<<4,
	RIGHT=42<<4,
	FIELD=43<<4
};
struct type {
	int op;
	Type type;
	int align;
	int size;
	union {
		Symbol sym;
		struct {
			unsigned oldstyle:1;
			Type *proto;
		} f;
	} u;
	Xtype x;
};
struct field {
	char *name;
	Type type;
	int offset;
	short bitsize;
	short lsb;
	Field link;
};
extern int assignargs;
extern int prunetemps;
extern int nodecount;
extern Symbol cfunc;
extern Symbol retv;
extern Tree (*optree[])(int, Tree, Tree);

extern char kind[];
extern int errcnt;
extern int errlimit;
extern int wflag;
extern Events events;
extern float refinc;

extern unsigned char *cp;
extern unsigned char *limit;
extern char *firstfile;
extern char *file;
extern char *line;
extern int lineno;
extern int t;
extern char *token;
extern Symbol tsym;
extern Coordinate src;
extern int Aflag;
extern int Pflag;
extern Symbol YYnull;
extern Symbol YYcheck;
extern int glevel;
extern int xref;

extern int ncalled;
extern int npoints;

extern int needconst;
extern int explicitCast;
extern struct code codehead;
extern Code codelist;
extern Table stmtlabs;
extern float density;
extern Table constants;
extern Table externals;
extern Table globals;
extern Table identifiers;
extern Table labels;
extern Table types;
extern int level;

extern List loci, symbols;

extern List symbols;

extern int where;
extern Type chartype;
extern Type doubletype;
extern Type floattype;
extern Type inttype;
extern Type longdouble;
extern Type longtype;
extern Type longlong;
extern Type shorttype;
extern Type signedchar;
extern Type unsignedchar;
extern Type unsignedlonglong;
extern Type unsignedlong;
extern Type unsignedshort;
extern Type unsignedtype;
extern Type charptype;
extern Type funcptype;
extern Type voidptype;
extern Type voidtype;
extern Type unsignedptr;
extern Type signedptr;
extern Type widechar;
extern void  *allocate(unsigned long n, unsigned a);
extern void deallocate(unsigned a);
extern void *newarray(unsigned long m, unsigned long n, unsigned a);
extern void walk(Tree e, int tlab, int flab);
extern Node listnodes(Tree e, int tlab, int flab);
extern Node newnode(int op, Node left, Node right, Symbol p);
extern Tree cvtconst(Tree);
extern void printdag(Node, int);
extern void compound(int, Swtch, int);
extern void defglobal(Symbol, int);
extern void finalize(void);
extern void program(void);

extern Tree vcall(Symbol func, Type ty, ...);
extern Tree addrof(Tree);
extern Tree asgn(Symbol, Tree);
extern Tree asgntree(int, Tree, Tree);
extern Type assign(Type, Tree);
extern Tree bittree(int, Tree, Tree);
extern Tree call(Tree, Type, Coordinate);
extern Tree calltree(Tree, Type, Tree, Symbol);
extern Tree condtree(Tree, Tree, Tree);
extern Tree cnsttree(Type, ...);
extern Tree consttree(unsigned int, Type);
extern Tree eqtree(int, Tree, Tree);
extern int iscallb(Tree);
extern Tree shtree(int, Tree, Tree);
extern void typeerror(int, Tree, Tree);

extern void test(int tok, char set[]);
extern void expect(int tok);
extern void skipto(int tok, char set[]);
extern void error(const char *, ...);
extern int fatal(const char *, const char *, int);
extern void warning(const char *, ...);

typedef void (*Apply)(void *, void *, void *);
extern void attach(Apply, void *, List *);
extern void apply(List event, void *arg1, void *arg2);
extern Tree retype(Tree p, Type ty);
extern Tree rightkid(Tree p);
extern int hascall(Tree p);
extern Type binary(Type, Type);
extern Tree cast(Tree, Type);
extern Tree cond(Tree);
extern Tree expr0(int);
extern Tree expr(int);
extern Tree expr1(int);
extern Tree field(Tree, const char *);
extern char *funcname(Tree);
extern Tree idtree(Symbol);
extern Tree incr(int, Tree, Tree);
extern Tree lvalue(Tree);
extern Tree nullcall(Type, Symbol, Tree, Tree);
extern Tree pointer(Tree);
extern Tree rvalue(Tree);
extern Tree value(Tree);

extern void defpointer(Symbol);
extern Type initializer(Type, int);
extern void swtoseg(int);

extern void input_init(int, char *[]);
extern void fillbuf(void);
extern void nextline(void);

extern int getchr(void);
extern int gettok(void);

extern void emitcode(void);
extern void gencode (Symbol[], Symbol[]);
extern void fprint(FILE *f, const char *fmt, ...);
extern char *stringf(const char *, ...);
extern void check(Node);
extern void print(const char *, ...);

extern List append(void *x, List list);
extern int  length(List list);
extern void *ltov (List *list, unsigned a);
extern void init(int, char *[]);

extern Type typename(void);
extern void checklab(Symbol p, void *cl);
extern Type enumdcl(void);
extern void main_init(int, char *[]);
extern int main(int, char *[]);

extern void vfprint(FILE *, char *, const char *, va_list);

extern int process(char *);
extern int findfunc(char *, char *);
extern int findcount(char *, int, int);

extern Tree constexpr(int);
extern int intexpr(int, int);
extern Tree simplify(int, Type, Tree, Tree);
extern int ispow2(unsigned long u);

extern int reachable(int);

extern void addlocal(Symbol);
extern void branch(int);
extern Code code(int);
extern void definelab(int);
extern void definept(Coordinate *);
extern void equatelab(Symbol, Symbol);
extern Node jump(int);
extern void retcode(Tree);
extern void statement(int, Swtch, int);
extern void swcode(Swtch, int *, int, int);
extern void swgen(Swtch);

extern char * string(const char *str);
extern char *stringn(const char *str, int len);
extern char *stringd(long n);
extern Symbol relocate(const char *name, Table src, Table dst);
extern void use(Symbol p, Coordinate src);
extern void locus(Table tp, Coordinate *cp);
extern Symbol allsymbols(Table);

extern Symbol constant(Type, Value);
extern void enterscope(void);
extern void exitscope(void);
extern Symbol findlabel(int);
extern Symbol findtype(Type);
extern void foreach(Table, int, void (*)(Symbol, void *), void *);
extern Symbol genident(int, Type, int);
extern int genlabel(int);
extern Symbol install(const char *, Table *, int, int);
extern Symbol intconst(int);
extern Symbol lookup(const char *, Table);
extern Symbol mkstr(char *);
extern Symbol mksymbol(int, const char *, Type);
extern Symbol newtemp(int, int, int);
extern Table table(Table, int);
extern Symbol temporary(int, Type);
extern char *vtoa(Type, Value);

extern int nodeid(Tree);
extern char *opname(int);
extern int *printed(int);
extern void printtree(Tree, int);
extern Tree root(Tree);
extern Tree texpr(Tree (*)(int), int, int);
extern Tree tree(int, Type, Tree, Tree);

extern void type_init(int, char *[]);

extern Type signedint(Type);

extern int hasproto(Type);
extern void outtype(Type, FILE *);
extern void printdecl (Symbol p, Type ty);
extern void printproto(Symbol p, Symbol args[]);
extern char *typestring(Type ty, char *id);
extern Field fieldref(const char *name, Type ty);
extern Type array(Type, int, int);
extern Type atop(Type);
extern Type btot(int, int);
extern Type compose(Type, Type);
extern Type deref(Type);
extern int eqtype(Type, Type, int);
extern Field fieldlist(Type);
extern Type freturn(Type);
extern Type ftype(Type, Type);
extern Type func(Type, Type *, int);
extern Field newfield(char *, Type, Type);
extern Type newstruct(int, char *);
extern void printtype(Type, int);
extern Type promote(Type);
extern Type ptr(Type);
extern Type qual(int, Type);
extern void rmtypes(int);
extern int ttob(Type);
extern int variadic(Type);

