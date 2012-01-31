#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "c.h"
#include "rcc.h"
#if WIN32
#include <fcntl.h>
#include <io.h>
#endif


static void do_int(int x) {
	printf("%d", x);
}

static void do_scope(int x) {
#define xx(c) if (x == c) { printf(#c); return; }
	xx(CONSTANTS)
	xx(LABELS)
	xx(GLOBAL)
	xx(PARAM)
	xx(LOCAL)
#undef xx
	if (x > LOCAL)
		printf("LOCAL+%d", x-LOCAL);
	else
		do_int(x);
}

static void do_sclass(int x) {
#define xx(c) if (x == c) { printf(#c); return; }
	xx(REGISTER)
	xx(AUTO)
	xx(EXTERN)
	xx(STATIC)
	xx(TYPEDEF)
#undef xx
	do_int(x);
}

static void do_flags(int x) {
	char *bar = "";
#define xx(f,n) if ((x>>n)&1) { printf("%s" #f, bar); bar = "|"; }
	xx(structarg,0)
	xx(addressed,1)
	xx(computed,2)
	xx(temporary,3)
	xx(generated,4)
#undef xx
	if (*bar == '\0')
		do_int(x);
}

static void do_seg(int x) {
#define xx(s) if (x == s) { printf(#s); return; }
	xx(CODE)
	xx(BSS)
	xx(DATA)
	xx(LIT)
#undef xx
	do_int(x);
}

#define xx(ptr,field,type) do { printf("<li>" #field " = "); do_##type(ptr->field); printf("</li>\n"); } while (0)

static void do_op(int x) {
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
	};
	int op = opindex(x);
	if (op < 1 || op >= sizeof opnames/sizeof opnames[0])
		printf("%d", x);
	else
		printf("%s", opnames[op]);
}

static void do_define_uid(int x) {
	printf("<strong id=uid%d>%d</strong>", x, x);
}

static void do_define_label(int x) {
	printf("<strong id=ll%d>%d</strong>", x, x);
}

static void do_uid(int x) {
	printf("<a href='#%d'>%d</a>", x, x, x);
}

static void do_label(int x) {
	printf("<a href='#L%d'>%d</a>", x, x, x);
}

static int nextid;

static void do_list(list_ty x, void do_one(void *), char *type, char *listhtml, char *separator) {
	int count = Seq_length(x);

	if (count == 0)
		printf("<em>empty %s list</em>\n", type);
	else {
		int i;
		printf("<em>%s list</em>", type);
		if (listhtml != NULL)
			printf("<%s>\n", listhtml);
		for (i = 0; i < count; i++) {
			if (listhtml != NULL)
				printf("<li>");
			printf(separator);
			do_one(Seq_get(x, i));
			if (listhtml != NULL)
				printf("</li>\n");
		}
		if (listhtml != NULL)
			printf("</%s>\n", listhtml);
	}
}

static void do_uid_list(list_ty x) {
	int i, count = Seq_length(x);

	if (count == 0)
		printf("<em>empty int list</em>\n");
	else {
		int i;
		printf("<em>int list</em>");
		for (i= 0; i < count; i++) {
			printf(" ");
			do_uid(*(int *)Seq_get(x, i));
		}
	}
}

static void do_identifier(const char *x) {
	printf("%s", x);
}

static void do_real(rcc_real_ty x) {
	double d;
	unsigned *p = (unsigned *)&d;
	static union { int x; char endian; } little = { 1 };

	p[1-little.endian] = x->msb;
	p[little.endian] = x->lsb;
	printf("(%#X,%#X) = %g", x->msb, x->lsb, d);
}

static void do_suffix(int x) {
	static char suffixes[] = "0F234IUPVB";

	if (x < 0 || x >= (sizeof suffixes/sizeof suffixes[0]) - 1)
		printf("%d", x);
	else
		printf("%c", suffixes[x]);
}

static void do_enum(void *x) {
	rcc_enum__ty e = x;

	do_identifier(e->id);
	printf("=");
	do_int(e->value);
}

static void do_enum_list(list_ty x) {
	do_list(x, do_enum, "enum", NULL, " ");
}

static void do_field(void *x) {
	rcc_field_ty f = x;

	printf("<em>field</em><ul>\n");
	xx(f,id,identifier);
	xx(f,type,uid);
	xx(f,offset,int);
	xx(f,bitsize,int);
	xx(f,lsb,int);
	printf("</ul>\n");
}

static void do_field_list(list_ty x) {
	do_list(x, do_field, "field", "ol", "");
}

static void do_symbol(rcc_symbol_ty x) {
	printf("<em>symbol</em><ul>\n");
	xx(x,id,identifier);
	xx(x,type,uid);
	xx(x,scope,scope);
	xx(x,sclass,sclass);
	xx(x,ref,int);
	xx(x,flags,flags);
	printf("</ul>\n");
}

#define caselabel(kind) case rcc_##kind##_enum: \
	printf("<strong>" #kind "</strong> : <em>%s</em>", typename); \
	printf("<ul>\n"); attributes
#define yy(kind,field,type) xx((&x->v.rcc_##kind),field,type)

static void do_type(rcc_type_ty x) {
#define attributes xx(x,size,int); xx(x,align,int)
	switch (x->kind) {
	static char *typename = "type";
	caselabel(INT); break;
	caselabel(UNSIGNED); break;
	caselabel(FLOAT); break;
	caselabel(VOID); break;
	caselabel(POINTER);
		yy(POINTER,type,uid);
		break;
	caselabel(ENUM);
		yy(ENUM,tag,identifier);
		yy(ENUM,ids,enum_list);
		break;
	caselabel(STRUCT);
		yy(STRUCT,tag,identifier);
		yy(STRUCT,fields,field_list);
		break;
	caselabel(UNION);
		yy(UNION,tag,identifier);
		yy(UNION,fields,field_list);
		break;
	caselabel(ARRAY);
		yy(ARRAY,type,uid);
		break;
	caselabel(FUNCTION);
		yy(FUNCTION,type,uid);
		yy(FUNCTION,formals,uid_list);
		break;
	caselabel(CONST);
		yy(CONST,type,uid);
		break;
	caselabel(VOLATILE);
		yy(VOLATILE,type,uid);
		break;
	default: assert(0);
	}
#undef attributes
	printf("</ul>\n");
}

static void do_item(rcc_item_ty x) {
	printf("<a name='%d'>", x->uid);
#define attributes xx(x,uid,define_uid)
	printf("</a>");
	switch (x->kind) {
	static char *typename = "item";
	caselabel(Symbol);
		yy(Symbol,symbol,symbol);
		break;
	caselabel(Type);
		yy(Type,type,type);
		break;
	default: assert(0);
	}
#undef attributes
	printf("</ul>\n");
}

static void do_item_list(list_ty x) {
	int count = Seq_length(x);

	if (count == 0)
		printf("<em>empty item list</em>\n");
	else {
		int i;
		printf("<em>item list</em>");
		printf("<ol>\n");
		for (i = 0; i < count; i++) {
			rcc_item_ty item = Seq_get(x, i);
			printf("<li value=%d>", item->uid);
			do_item(item);
			printf("</li>\n");
		}
		printf("</ol>\n");
	}
}

static void do_string(string_ty x) {
	printf("%d,<code>'%s'</code>", x.len, x.str);
}

static void do_generic_string(void *x) {
	do_string(*(string_ty *)x);
}

static void do_string_list(list_ty x) {
	do_list(x, do_generic_string, "string", "ol", "");
}

static void do_node(void *y) {
	rcc_node_ty x = y;

	if (x->kind == rcc_LABEL_enum)
		printf("<a name='L%d'></a>", x->v.rcc_LABEL.label);
#define attributes xx(x,suffix,suffix); xx(x,size,int)
	switch (x->kind) {
	static char *typename = "node";
	caselabel(CNST);
		yy(CNST,value,int);
		break;
	caselabel(CNSTF);
		yy(CNSTF,value,real);
		break;
	caselabel(ARG);
		yy(ARG,left,node);
		yy(ARG,len,int);
		yy(ARG,align,int);
		break;
	caselabel(ASGN);
		yy(ASGN,left,node);
		yy(ASGN,right,node);
		yy(ASGN,len,int);
		yy(ASGN,align,int);
		break;
	caselabel(CVT);
		yy(CVT,op,op);
		yy(CVT,left,node);
		yy(CVT,fromsize,int);
		break;
	caselabel(CALL);
		yy(CALL,left,node);
		yy(CALL,type,uid);
		break;
	caselabel(CALLB);
		yy(CALLB,left,node);
		yy(CALLB,right,node);
		yy(CALLB,type,uid);
		break;
	caselabel(RET);	break;
	caselabel(ADDRG);
		yy(ADDRG,uid,uid);
		break;
	caselabel(ADDRL);
		yy(ADDRL,uid,uid);
		break;
	caselabel(ADDRF);
		yy(ADDRF,uid,uid);
		break;
	caselabel(Unary);
		yy(Unary,op,op);
		yy(Unary,left,node);
		break;
	caselabel(Binary);
		yy(Binary,op,op);
		yy(Binary,left,node);
		yy(Binary,right,node);
		break;
	caselabel(Compare);
		yy(Compare,op,op);
		yy(Compare,left,node);
		yy(Compare,right,node);
		yy(Compare,label,label);
		break;
	caselabel(LABEL);
		yy(LABEL,label,define_label);
		break;
	caselabel(BRANCH);
		yy(BRANCH,label,label);
		break;
	caselabel(CSE);
		yy(CSE,uid,uid);
		yy(CSE,node,node);
		break;
	default: assert(0);
	}
#undef attributes
	printf("</ul>");
}

static void do_node_list(list_ty x) {
	do_list(x, do_node, "node", "ol", "");
}

static void do_interface(void *);

static void do_interface_list(list_ty x) {
	do_list(x, do_interface, "interface", "ol", "");
}

static void do_interface(void *y) {
	rcc_interface_ty x = y;

	if (x->kind == rcc_Address_enum)
		printf("<a name='%d'></a>", x->v.rcc_Address.uid);
	else if (x->kind == rcc_Local_enum)
		printf("<a name='%d'></a>", x->v.rcc_Local.uid);
#define attributes
	switch (x->kind) {
	static char *typename = "interface";
	caselabel(Export);
		yy(Export,p,uid);
		break;
	caselabel(Import);
		yy(Import,p,uid);
		break;
	caselabel(Global);
		yy(Global,p,uid);
		yy(Global,seg,seg);
		break;
	caselabel(Local);
		yy(Local,uid,define_uid);
		yy(Local,p,symbol);
		break;
	caselabel(Address);
		yy(Address,uid,define_uid);
		yy(Address,q,symbol);
		yy(Address,p,uid);
		yy(Address,n,int);
		break;
	caselabel(Segment);
		yy(Segment,seg,seg);
		break;
	caselabel(Defaddress);
		yy(Defaddress,p,uid);
		break;
	caselabel(Deflabel);
		yy(Deflabel,label,label);
		break;
	caselabel(Defconst);
		yy(Defconst,suffix,suffix);
		yy(Defconst,size,int);
		yy(Defconst,value,int);
		break;
	caselabel(Defconstf);
		yy(Defconstf,size,int);
		yy(Defconstf,value,real);
		break;
	caselabel(Defstring);
		yy(Defstring,s,string);
		break;
	caselabel(Space);
		yy(Space,n,int);
		break;
	caselabel(Function);
		yy(Function,f,uid);
		yy(Function,caller,uid_list);
		yy(Function,callee,uid_list);
		yy(Function,ncalls,int);
		yy(Function,codelist,interface_list);
		break;
	caselabel(Forest);
		yy(Forest,nodes,node_list);
		break;
	case rcc_Blockbeg_enum: printf("<strong>Blockbeg</strong> : <em>%s</em>", typename); return;
	case rcc_Blockend_enum: printf("<strong>Blockend</strong> : <em>%s</em>", typename); return;
	default: assert(0);
	}
#undef attributes
	printf("</ul>\n");
}

static void do_program(rcc_program_ty x) {
	printf("<ul>\n");
	xx(x,nuids,int);
	xx(x,nlabels,int);
	xx(x,items,item_list);
	xx(x,interfaces,interface_list);
	xx(x,argc,int);
	xx(x,argv,string_list);
	printf("</ul>\n");
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
		fprintf(stderr, "%s: can't read `%s'\n", argv[0], infile);
		exit(EXIT_FAILURE);
	}
	if (infile == NULL || strcmp(infile, "-") == 0)
		infile = "Standard Input";
#if WIN32
	else
		_setmode(_fileno(stdin), _O_BINARY);
#endif
	if (outfile != NULL && strcmp(outfile, "-") != 0
	&& freopen(outfile, "w", stdout) == NULL) {
		fprintf(stderr, "%s: can't write `%s'\n", argv[0], outfile);
		exit(EXIT_FAILURE);
	}
	version = read_int(stdin);
	assert(version/100 == (int)stamp);
	pickle = rcc_read_program(stdin);
	printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\"\n");
	printf("<html><head><title>%s</title>\n"
	"<link rev=made href=\"mailto:drh@microsoft.com\">\n"
	"</head><body>\n<h1>%s</h1>\n",	infile, infile);
	printf("<p>version = %d.%d</p>", version/100, version%100);
	do_program(pickle);
	{
		time_t t;
		time(&t);
		printf("<hr><address>%s</address>\n", ctime(&t));
	}
	printf("</body></html>\n");
	return EXIT_SUCCESS;
}
