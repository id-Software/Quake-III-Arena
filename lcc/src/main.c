#include "c.h"

static char rcsid[] = "main.c - faked rcsid";

static void typestab(Symbol, void *);

static void stabline(Coordinate *);
static void stabend(Coordinate *, Symbol, Coordinate **, Symbol *, Symbol *);
Interface *IR = NULL;

int Aflag;		/* >= 0 if -A specified */
int Pflag;		/* != 0 if -P specified */
int glevel;		/* == [0-9] if -g[0-9] specified */
int xref;		/* != 0 for cross-reference data */
Symbol YYnull;		/* _YYnull  symbol if -n or -nvalidate specified */
Symbol YYcheck;		/* _YYcheck symbol if -nvalidate,check specified */

static char *comment;
static Interface stabIR;
static char *currentfile;       /* current file name */
static int currentline;		/* current line number */
static FILE *srcfp;		/* stream for current file, if non-NULL */
static int srcpos;		/* position of srcfp, if srcfp is non-NULL */
int main(int argc, char *argv[]) {
	int i, j;
	for (i = argc - 1; i > 0; i--)
		if (strncmp(argv[i], "-target=", 8) == 0)
			break;
	if (i > 0) {
		char *s = strchr(argv[i], '\\');
		if (s != NULL)
			*s = '/';
		for (j = 0; bindings[j].name && bindings[j].ir; j++)
			if (strcmp(&argv[i][8], bindings[j].name) == 0) {
				IR = bindings[j].ir;
				break;
			}
		if (s != NULL)
			*s = '\\';
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
	init(argc, argv);
	t = gettok();
	(*IR->progbeg)(argc, argv);
	if (glevel && IR->stabinit)
		(*IR->stabinit)(firstfile, argc, argv);
	program();
	if (events.end)
		apply(events.end, NULL, NULL);
	memset(&events, 0, sizeof events);
	if (glevel || xref) {
		Symbol symroot = NULL;
		Coordinate src;
		foreach(types,       GLOBAL, typestab, &symroot);
		foreach(identifiers, GLOBAL, typestab, &symroot);
		src.file = firstfile;
		src.x = 0;
		src.y = lineno;
		if ((glevel > 2 || xref) && IR->stabend)
			(*IR->stabend)(&src, symroot,
				ltov(&loci,    PERM),
				ltov(&symbols, PERM), NULL);
		else if (IR->stabend)
			(*IR->stabend)(&src, NULL, NULL, NULL, NULL);
	}
	finalize();
	(*IR->progend)();
	deallocate(PERM);
	return errcnt > 0;
}
/* main_init - process program arguments */
void main_init(int argc, char *argv[]) {
	char *infile = NULL, *outfile = NULL;
	int i;
	static int inited;

	if (inited)
		return;
	inited = 1;
	type_init(argc, argv);
	for (i = 1; i < argc; i++)
		if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "-g2") == 0)
			glevel = 2;
		else if (strncmp(argv[i], "-g", 2) == 0) {	/* -gn[,x] */
			char *p = strchr(argv[i], ',');
			glevel = atoi(argv[i]+2);
			if (p) {
				comment = p + 1;
				if (glevel == 0)
					glevel = 1;
				if (stabIR.stabline == NULL) {
					stabIR.stabline = IR->stabline;
					stabIR.stabend = IR->stabend;
					IR->stabline = stabline;
					IR->stabend = stabend;
				}
			}	
		} else if (strcmp(argv[i], "-x") == 0)
			xref++;
		else if (strcmp(argv[i], "-A") == 0) {
			++Aflag;
		} else if (strcmp(argv[i], "-P") == 0)
			Pflag++;
		else if (strcmp(argv[i], "-w") == 0)
			wflag++;
		else if (strcmp(argv[i], "-n") == 0) {
			if (!YYnull) {
				YYnull = install(string("_YYnull"), &globals, GLOBAL, PERM);
				YYnull->type = func(voidptype, NULL, 1);
				YYnull->sclass = EXTERN;
				(*IR->defsymbol)(YYnull);
			}
		} else if (strncmp(argv[i], "-n", 2) == 0) {	/* -nvalid[,check] */
			char *p = strchr(argv[i], ',');
			if (p) {
				YYcheck = install(string(p+1), &globals, GLOBAL, PERM);
				YYcheck->type = func(voidptype, NULL, 1);
				YYcheck->sclass = EXTERN;
				(*IR->defsymbol)(YYcheck);
				p = stringn(argv[i]+2, p - (argv[i]+2));
			} else
				p = string(argv[i]+2);
			YYnull = install(p, &globals, GLOBAL, PERM);
			YYnull->type = func(voidptype, NULL, 1);
			YYnull->sclass = EXTERN;
			(*IR->defsymbol)(YYnull);
		} else if (strcmp(argv[i], "-v") == 0)
			fprint(stderr, "%s %s\n", argv[0], rcsid);
		else if (strncmp(argv[i], "-s", 2) == 0)
			density = strtod(&argv[i][2], NULL);
		else if (strncmp(argv[i], "-errout=", 8) == 0) {
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
		} else if (strncmp(argv[i], "-little_endian=", 15) == 0)
			IR->little_endian = argv[i][15] - '0';
		else if (strncmp(argv[i], "-mulops_calls=", 18) == 0)
			IR->mulops_calls = argv[i][18] - '0';
		else if (strncmp(argv[i], "-wants_callb=", 13) == 0)
			IR->wants_callb = argv[i][13] - '0';
		else if (strncmp(argv[i], "-wants_argb=", 12) == 0)
			IR->wants_argb = argv[i][12] - '0';
		else if (strncmp(argv[i], "-left_to_right=", 15) == 0)
			IR->left_to_right = argv[i][15] - '0';
		else if (strncmp(argv[i], "-wants_dag=", 11) == 0)
			IR->wants_dag = argv[i][11] - '0';
		else if (*argv[i] != '-' || strcmp(argv[i], "-") == 0) {
			if (infile == NULL)
				infile = argv[i];
			else if (outfile == NULL)
				outfile = argv[i];
		}

	if (infile != NULL && strcmp(infile, "-") != 0
	&& freopen(infile, "r", stdin) == NULL) {
		fprint(stderr, "%s: can't read `%s'\n", argv[0], infile);
		exit(EXIT_FAILURE);
	}
	if (outfile != NULL && strcmp(outfile, "-") != 0
	&& freopen(outfile, "w", stdout) == NULL) {
		fprint(stderr, "%s: can't write `%s'\n", argv[0], outfile);
		exit(EXIT_FAILURE);
	}
}
/* typestab - emit stab entries for p */
static void typestab(Symbol p, void *cl) {
	if (*(Symbol *)cl == 0 && p->sclass && p->sclass != TYPEDEF)
		*(Symbol *)cl = p;
	if ((p->sclass == TYPEDEF || p->sclass == 0) && IR->stabtype)
		(*IR->stabtype)(p);
}

/* stabline - emit source code for source coordinate *cp */
static void stabline(Coordinate *cp) {
	if (cp->file && cp->file != currentfile) {
		if (srcfp)
			fclose(srcfp);
		currentfile = cp->file;
		srcfp = fopen(currentfile, "r");
		srcpos = 0;
		currentline = 0;
	}
	if (currentline != cp->y && srcfp) {
		char buf[512];
		if (srcpos > cp->y) {
			rewind(srcfp);
			srcpos = 0;
		}
		for ( ; srcpos < cp->y; srcpos++)
			if (fgets(buf, sizeof buf, srcfp) == NULL) {
				fclose(srcfp);
				srcfp = NULL;
				break;
			}
		if (srcfp && srcpos == cp->y)
			print("%s%s", comment, buf);
	}
	currentline = cp->y;
	if (stabIR.stabline)
		(*stabIR.stabline)(cp);
}

static void stabend(Coordinate *cp, Symbol p, Coordinate **cpp, Symbol *sp, Symbol *stab) {
	if (stabIR.stabend)
		(*stabIR.stabend)(cp, p, cpp, sp, stab);
	if (srcfp)
		fclose(srcfp);
}
