/* DEC ALPHAs running OSF/1 V3.2A (Rev. 17) at Princeton University */

#include <string.h>


#ifndef LCCDIR
#define LCCDIR "/usr/local/lib/lcc/"
#endif

char *suffixes[] = { ".c", ".i", ".s", ".o", ".out", 0 };
char inputs[256] = "";
char *cpp[] = {
	LCCDIR "cpp", "-D__STDC__=1",
	"-DLANGUAGE_C", "-D__LANGUAGE_C__",
 	"-D_unix", "-D__unix__", "-D_osf", "-D__osf__", "-Dunix",
	"-Dalpha", "-D_alpha", "-D__alpha",
	"-D__SYSTYPE_BSD",  "-D_SYSTYPE_BSD",
	"$1", "$2", "$3", 0 };
char *com[] =  { LCCDIR "rcc", "-target=alpha/osf", "$1", "$2", "$3", "", 0 };
char *include[] = { "-I" LCCDIR "include", "-I/usr/local/include",
	"-I/usr/include", 0 };
char *as[] =  { "/bin/as", "-o", "$3", "", "$1", "-nocpp", "$2", 0 };
char *ld[] =  { "/usr/bin/ld", "-o", "$3", "/usr/lib/cmplrs/cc/crt0.o",
	"$1", "$2", "", "", "-L" LCCDIR, "-llcc", "-lm", "-lc", 0 };

extern char *concat(char *, char *);
extern int access(const char *, int);

int option(char *arg) {
	if (strncmp(arg, "-lccdir=", 8) == 0) {
		cpp[0] = concat(&arg[8], "/cpp");
		include[0] = concat("-I", concat(&arg[8], "/include"));
		com[0] = concat(&arg[8], "/rcc");
		ld[8] = concat("-L", &arg[8]);
	} else if (strcmp(arg, "-g4") == 0
	&& access("/u/drh/lib/alpha/rcc", 4) == 0
	&& access("/u/drh/book/cdb/alpha/osf/cdbld", 4) == 0) {
		com[0] = "/u/drh/lib/alpha/rcc";
		com[5] = "-g4";
		ld[0] = "/u/drh/book/cdb/alpha/osf/cdbld";
		ld[1] = "-o";
		ld[2] = "$3";
		ld[3] = "$1";
		ld[4] = "$2";
		ld[5] = 0;
	} else if (strcmp(arg, "-g") == 0)
		return 1;
	else if (strcmp(arg, "-b") == 0)
		;
	else
		return 0;
	return 1;
}
