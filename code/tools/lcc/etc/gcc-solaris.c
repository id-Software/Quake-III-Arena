/* Solaris 10 sparc */

#include <string.h>

/*
TTimo - 10-18-2001
our binaries are named q3lcc q3rcc and q3cpp
removed hardcoded paths
removed __linux__ preprocessor define (confuses the preprocessor, we are doing bytecode!)
*/


#ifndef LCCDIR
#define LCCDIR ""
#endif
#ifndef GCCDIR
#define GCCDIR "/usr/sfw/bin/"
#endif
#ifndef GCCLIB
#define GCCLIB "/usr/sfw/lib/gcc/sparc-sun-solaris2.10/3.4.3/"
#endif
#define BINEXT ""

char *suffixes[] = { ".c", ".i", ".asm", ".s", ".o", ".out", 0 };
char inputs[256] = "";
char *cpp[] = { LCCDIR "q3cpp" BINEXT,
	"-D__STDC__=1", "-Dsparc", "-D__sparc__", "-Dsun", "-D__sun__", "-Dunix", "-D__sun",
	"$1", "$2", "$3", 0 };
char *include[] = { "-I" LCCDIR "include", "-I" LCCDIR "gcc/include",
	"-I" SYSTEM "include", "-I/usr/include", 0 };
/* char *com[] = { LCCDIR "q3rcc", "-target=bytecode", "$1", "$2", "$3", 0 }; */
char *com[] = { LCCDIR "q3rcc", "-target=sparc/solaris", "$1", "$2", "$3", 0 };
char *as[] = { "/usr/ccs/bin/as", "-o", "$3", "$1", "$2", 0 };
char *ld[] = { "/usr/ccs/bin/ld", "-o", "$3", "$1",
        GCCLIB "crti.o", GCCLIB "crt1.o",
        GCCLIB "crtbegin.o", "$2", "", "", "-L" LCCDIR, "-llcc",
        "-L" GCCLIB, "-lgcc", "-lm", "-lc", "",
        GCCLIB "crtend.o", GCCLIB "crtn.o", 0 };

extern char *concat(char *, char *);

int option(char *arg) {
	if (strncmp(arg, "-lccdir=", 8) == 0) {
		cpp[0] = concat(&arg[8], "/gcc/cpp");
		include[0] = concat("-I", concat(&arg[8], "/include"));
		ld[10] = concat("-L", &arg[8]);
		com[0] = concat(&arg[8], "/rcc");
	} else if (strcmp(arg, "-p") == 0 || strcmp(arg, "-pg") == 0) {
		ld[8] = "-lgmon";
	} else if (strcmp(arg, "-g") == 0)
		;
	else if (strcmp(arg, "-b") == 0)
		;
	else
		return 0;
	return 1;
}
