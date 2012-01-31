/* SPARCs running Solaris 2.5.1 w/GCC tools
   at CS Dept., Princeton University */

#include <string.h>


#ifndef LCCDIR
#define LCCDIR "/usr/local/lib/lcc/"
#endif
#ifndef GCCDIR
#define GCCDIR "/usr/local/gnu/bin/"
#endif
#ifndef GCCLIB
#define GCCLIB "/usr/local/gnu/lib/gcc-lib/sparc-sun-solaris2.5/2.7.2/"
#endif

char *suffixes[] = { ".c", ".i", ".s", ".o", ".out", 0 };
char inputs[256] = "";
char *cpp[] = { LCCDIR "cpp",
	"-D__STDC__=1", "-Dsparc", "-D__sparc__", "-Dsun", "-D__sun__", "-Dunix",
	"$1", "$2", "$3", 0 };
char *include[] = { "-I" LCCDIR "include", "-I/usr/local/include",
	"-I" GCCLIB "include", "-I/usr/include", 0 };
char *com[] = { LCCDIR "rcc", "-target=sparc/solaris",
	"$1", "$2", "$3", 0 };
char *as[] = { GCCDIR "as", "-f", "-o", "$3", "$1", "$2", 0 };
char *ld[] = { GCCDIR "ld", "-o", "$3", "$1",
	GCCLIB "crti.o", GCCLIB "crt1.o",
	GCCLIB "crtbegin.o", "$2", "", "", "-L" LCCDIR, "-llcc",
	"-L" GCCLIB, "-lgcc", "-lm", "-lc", "",
	GCCLIB "crtend.o", GCCLIB "crtn.o", 0 };

extern char *concat(char *, char *);

int option(char *arg) {
	if (strncmp(arg, "-lccdir=", 8) == 0) {
		cpp[0] = concat(&arg[8], "/cpp");
		include[0] = concat("-I", concat(&arg[8], "/include"));
		ld[10] = concat("-L", &arg[8]);
		com[0] = concat(&arg[8], "/rcc");
	} else if (strcmp(arg, "-g") == 0)
		;
	else if (strcmp(arg, "-pg") == 0) {
		ld[8] = GCCLIB "gmon.o";
	} else if (strcmp(arg, "-b") == 0)
		;
	else
		return 0;
	return 1;
}
