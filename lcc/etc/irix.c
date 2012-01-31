/* SGI big endian MIPSes running IRIX 5.2 at CS Dept., Princeton University */

#include <string.h>


#ifndef LCCDIR
#define LCCDIR "/usr/local/lib/lcc/"
#endif

char *suffixes[] = { ".c", ".i", ".s", ".o", ".out", 0 };
char inputs[256] = "";
char *cpp[] = { LCCDIR "cpp", "-D__STDC__=1",
	"-DLANGUAGE_C",
	"-DMIPSEB",
	"-DSYSTYPE_SVR4",
	"-D_CFE",
	"-D_LANGUAGE_C",
	"-D_MIPSEB",
	"-D_MIPS_FPSET=16",
	"-D_MIPS_ISA=_MIPS_ISA_MIPS1",
	"-D_MIPS_SIM=_MIPS_SIM_ABI32",
	"-D_MIPS_SZINT=32",
	"-D_MIPS_SZLONG=32",
	"-D_MIPS_SZPTR=32",
	"-D_SGI_SOURCE",
	"-D_SVR4_SOURCE",
	"-D_SYSTYPE_SVR4",
	"-D__host_mips",
	"-D__mips=1",
	"-D__sgi",
	"-D__unix",
	"-Dhost_mips",
	"-Dmips",
	"-Dsgi",
	"-Dunix",
	"$1", "$2", "$3", 0 };
char *com[] =  { LCCDIR "rcc", "-target=mips/irix", "$1", "$2", "$3", "", 0 };
char *include[] = { "-I" LCCDIR "include", "-I/usr/local/include",
	"-I/usr/include", 0 };
char *as[] = { "/usr/bin/as", "-o", "$3", "$1", "-nocpp", "-KPIC", "$2", 0 };
char *ld[] = { "/usr/bin/ld", "-require_dynamic_link", "_rld_new_interface",
	"-elf", "-_SYSTYPE_SVR4", "-Wx,-G", "0", "-g0", "-KPIC", "-dont_warn_unused",
	"-o", "$3", "/usr/lib/crt1.o", "-L/usr/local/lib",
	"$1", "$2", "", "-L" LCCDIR, "-llcc", "-lc", "-lm", "/usr/lib/crtn.o", 0
};

extern char *concat(char *, char *);

int option(char *arg) {
	if (strncmp(arg, "-lccdir=", 8) == 0) {
		cpp[0] = concat(&arg[8], "/cpp");
		include[0] = concat("-I", concat(&arg[8], "/include"));
		com[0] = concat(&arg[8], "/rcc");
		ld[17] = concat("-L", &arg[8]);
	} else if (strcmp(arg, "-g") == 0)
		;
	else if (strcmp(arg, "-p") == 0)
		ld[12] = "/usr/lib/mcrt1.o";
	else if (strcmp(arg, "-b") == 0)
		;
	else
		return 0;
	return 1;
}
