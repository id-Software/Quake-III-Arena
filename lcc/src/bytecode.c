#include "c.h"
#define I(f) b_##f


static void I(segment)(int n) {
	static int cseg;

	if (cseg != n)
		switch (cseg = n) {
		case CODE: print("code\n"); return;
		case DATA: print("data\n"); return;
		case BSS:  print("bss\n");  return;
		case LIT:  print("lit\n");  return;
		default: assert(0);
		}
}

static void I(address)(Symbol q, Symbol p, long n) {
	q->x.name = stringf("%s%s%D", p->x.name, n > 0 ? "+" : "", n);
}

static void I(defaddress)(Symbol p) {
	print("address %s\n", p->x.name);
}

static void I(defconst)(int suffix, int size, Value v) {
	switch (suffix) {
	case I:
		if (size > sizeof (int))
			print("byte %d %D\n", size, v.i);
		else
			print("byte %d %d\n", size, v.i);
		return;
	case U:
		if (size > sizeof (unsigned))
			print("byte %d %U\n", size, v.u);
		else
			print("byte %d %u\n", size, v.u);
		return;
	case P: print("byte %d %U\n", size, (unsigned long)v.p); return;
	case F:
		if (size == 4) {
			float f = v.d;
			print("byte 4 %u\n", *(unsigned *)&f);
		} else {
			unsigned *p = (unsigned *)&v.d;
			print("byte 4 %u\n", p[swap]);
			print("byte 4 %u\n", p[1 - swap]);
		}
		return;
	}
	assert(0);
}

static void I(defstring)(int len, char *str) {
	char *s;

	for (s = str; s < str + len; s++)
		print("byte 1 %d\n", (*s)&0377);
}

static void I(defsymbol)(Symbol p) {
	if (p->scope == CONSTANTS)
		switch (optype(ttob(p->type))) {
		case I: p->x.name = stringf("%D", p->u.c.v.i); break;
		case U: p->x.name = stringf("%U", p->u.c.v.u); break;
		case P: p->x.name = stringf("%U", p->u.c.v.p); break;
		case F:
			{	// JDC: added this to get inline floats
				unsigned temp;

				*(float *)&temp = p->u.c.v.d;
				p->x.name = stringf("%U", temp );
			}
			break;// JDC: added this
		default: assert(0);
		}
	else if (p->scope >= LOCAL && p->sclass == STATIC)
		p->x.name = stringf("$%d", genlabel(1));
	else if (p->scope == LABELS || p->generated)
		p->x.name = stringf("$%s", p->name);
	else
		p->x.name = p->name;
}

static void dumptree(Node p) {
	switch (specific(p->op)) {
	case ASGN+B:
		assert(p->kids[0]);
		assert(p->kids[1]);
		assert(p->syms[0]);
		dumptree(p->kids[0]);
		dumptree(p->kids[1]);
		print("%s %d\n", opname(p->op), p->syms[0]->u.c.v.u);
		return;
	case RET+V:
		assert(!p->kids[0]);
		assert(!p->kids[1]);
		print("%s\n", opname(p->op));
		return;
	}
	switch (generic(p->op)) {
	case CNST: case ADDRG: case ADDRF: case ADDRL: case LABEL:
		assert(!p->kids[0]);
		assert(!p->kids[1]);
		assert(p->syms[0] && p->syms[0]->x.name);
		print("%s %s\n", opname(p->op), p->syms[0]->x.name);
		return;
	case CVF: case CVI: case CVP: case CVU:
		assert(p->kids[0]);
		assert(!p->kids[1]);
		assert(p->syms[0]);
		dumptree(p->kids[0]);
		print("%s %d\n", opname(p->op), p->syms[0]->u.c.v.i);
		return;
	case ARG: case BCOM: case NEG: case INDIR: case JUMP: case RET:
		assert(p->kids[0]);
		assert(!p->kids[1]);
		dumptree(p->kids[0]);
		print("%s\n", opname(p->op));
		return;
	case CALL:
		assert(p->kids[0]);
		assert(!p->kids[1]);
		assert(optype(p->op) != B);
		dumptree(p->kids[0]);
		print("%s\n", opname(p->op));
		if ( !p->count ) { printf("pop\n"); };	// JDC
		return;
	case ASGN: case BOR: case BAND: case BXOR: case RSH: case LSH:
	case ADD: case SUB: case DIV: case MUL: case MOD:
		assert(p->kids[0]);
		assert(p->kids[1]);
		dumptree(p->kids[0]);
		dumptree(p->kids[1]);
		print("%s\n", opname(p->op));
		return;
	case EQ: case NE: case GT: case GE: case LE: case LT:
		assert(p->kids[0]);
		assert(p->kids[1]);
		assert(p->syms[0]);
		assert(p->syms[0]->x.name);
		dumptree(p->kids[0]);
		dumptree(p->kids[1]);
		print("%s %s\n", opname(p->op), p->syms[0]->x.name);
		return;
	}
	assert(0);
}

static void I(emit)(Node p) {
	for (; p; p = p->link)
		dumptree(p);
}

static void I(export)(Symbol p) {
	print("export %s\n", p->x.name);
}

static void I(function)(Symbol f, Symbol caller[], Symbol callee[], int ncalls) {
	int i;

	(*IR->segment)(CODE);
	offset = 0;
	for (i = 0; caller[i] && callee[i]; i++) {
		offset = roundup(offset, caller[i]->type->align);
		caller[i]->x.name = callee[i]->x.name = stringf("%d", offset);
		caller[i]->x.offset = callee[i]->x.offset = offset;
		offset += caller[i]->type->size;
	}
	maxargoffset = maxoffset = argoffset = offset = 0;
	gencode(caller, callee);
	print("proc %s %d %d\n", f->x.name, maxoffset, maxargoffset);
	emitcode();
	print("endproc %s %d %d\n", f->x.name, maxoffset, maxargoffset);

}

static void gen02(Node p) {
	assert(p);
	if (generic(p->op) == ARG) {
		assert(p->syms[0]);
		argoffset += (p->syms[0]->u.c.v.i < 4 ? 4 : p->syms[0]->u.c.v.i);
	} else if (generic(p->op) == CALL) {
		maxargoffset = (argoffset > maxargoffset ? argoffset : maxargoffset);
		argoffset = 0;
	}
}

static void gen01(Node p) {
	if (p) {
		gen01(p->kids[0]);
		gen01(p->kids[1]);
		gen02(p);
	}
}

static Node I(gen)(Node p) {
	Node q;

	assert(p);
	for (q = p; q; q = q->link)
		gen01(q);
	return p;
}

static void I(global)(Symbol p) {
	print("align %d\n", p->type->align > 4 ? 4 : p->type->align);
	print("LABELV %s\n", p->x.name);
}

static void I(import)(Symbol p) {
	print("import %s\n", p->x.name);
}

static void I(local)(Symbol p) {
	offset = roundup(offset, p->type->align);
	p->x.name = stringf("%d", offset);
	p->x.offset = offset;
	offset += p->type->size;
}

static void I(progbeg)(int argc, char *argv[]) {}

static void I(progend)(void) {}

static void I(space)(int n) {
	print("skip %d\n", n);
}

//========================================================

// JDC: hacked up to get interleaved source lines in asm code
static char	*sourceFile;
static char *sourcePtr;
static int sourceLine;

static int filelength( FILE *f ) {
	int		pos;
	int		end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

static void LoadSourceFile( const char *filename ) {
	FILE	*f;
	int		length;

	f = fopen( filename, "r" );
	if ( !f ) {
		print( ";couldn't open %s\n", filename );
		sourceFile = NULL;
		return;
	}
	length = filelength( f );
	sourceFile = malloc( length + 1 );
	if ( sourceFile ) {
		fread( sourceFile, length, 1, f );
		sourceFile[length] = 0;
	}

	fclose( f );
	sourceLine = 1;
	sourcePtr = sourceFile;
}

static void PrintToSourceLine( int line ) {
	int		c;

	if ( !sourceFile ) {
		return;
	}
	while ( sourceLine <= line ) {
		int		i;

		for ( i = 0 ; sourcePtr[i] && sourcePtr[i] != '\n' ; i++ ) {
		}
		c = sourcePtr[i];
		if ( c == '\n' ) {
			sourcePtr[i] = 0;
		}
		print( ";%d:%s\n", sourceLine, sourcePtr );
		if ( c == 0 ) {
			sourcePtr += i;	// end of file
		} else {
			sourcePtr += i+1;
		}
		sourceLine++;
	}
}

static void I(stabline)(Coordinate *cp) {
	static char *prevfile;
	static int prevline;

	if (cp->file && (prevfile == NULL || strcmp(prevfile, cp->file) != 0)) {
		print("file \"%s\"\n", prevfile = cp->file);
		prevline = 0;
		if ( sourceFile ) {
			free( sourceFile );
			sourceFile = NULL;
		}
		// load the new source file
		LoadSourceFile( cp->file );
	}
	if (cp->y != prevline) {
		print("line %d\n", prevline = cp->y);
		PrintToSourceLine( cp->y );
	}
}

//========================================================

#define b_blockbeg blockbeg
#define b_blockend blockend

Interface bytecodeIR = {
	1, 1, 0,	/* char */
	2, 2, 0,	/* short */
	4, 4, 0,	/* int */
	4, 4, 0,	/* long */
	4, 4, 0,	/* long long */
	4, 4, 0,	/* float */				// JDC: use inline floats
	4, 4, 0,	/* double */			// JDC: don't ever emit 8 byte double code
	4, 4, 0,	/* long double */		// JDC: don't ever emit 8 byte double code
	4, 4, 0,	/* T* */
	0, 4, 0,	/* struct */
	0,		/* little_endian */
	0,		/* mulops_calls */
	0,		/* wants_callb */
	0,		/* wants_argb */
	1,		/* left_to_right */
	0,		/* wants_dag */
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
	0,		/* I(stabblock) */
	0,		/* I(stabend) */
	0,		/* I(stabfend) */
	0,		/* I(stabinit) */
	I(stabline),
	0,		/* I(stabsym) */
	0,		/* I(stabtype) */
};
