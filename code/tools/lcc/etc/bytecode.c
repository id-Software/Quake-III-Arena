/* quake3 bytecode target */

#include <string.h>
#include <stdio.h>
#include "../../../qcommon/q_platform.h"

#ifdef _WIN32
#define BINEXT ".exe"
#else
#define BINEXT ""
#endif

char *suffixes[] = { ".c", ".i", ".asm", ".o", ".out", 0 };
char inputs[256] = "";
char *cpp[] = { "q3cpp" BINEXT,
	"-D__STDC__=1", "-D__STRICT_ANSI__", "-D__signed__=signed", "-DQ3_VM",
	"$1", "$2", "$3", 0 };
char *include[] = { 0 };
char *com[] = { "q3rcc" BINEXT, "-target=bytecode", "$1", "$2", "$3", 0 };
char *ld[] = { 0 };
char *as[] = { 0 };

extern char *concat(char *, char *);

/*
===============
UpdatePaths

Updates the paths to q3cpp and q3rcc based on
the directory that contains q3lcc
===============
*/
void UpdatePaths( const char *lccBinary )
{
	char basepath[ 1024 ];
	char *p;

	strncpy( basepath, lccBinary, 1024 );
	p = strrchr( basepath, PATH_SEP );

	if( p )
	{
		*( p + 1 ) = '\0';

		cpp[ 0 ] = concat( basepath, "q3cpp" BINEXT );
		com[ 0 ] = concat( basepath, "q3rcc" BINEXT );
	}
}

int option(char *arg) {
	if (strncmp(arg, "-lccdir=", 8) == 0) {
		cpp[0] = concat(&arg[8], "/q3cpp" BINEXT);
		include[0] = concat("-I", concat(&arg[8], "/include"));
		com[0] = concat(&arg[8], "/q3rcc" BINEXT);
	} else if (strcmp(arg, "-p") == 0 || strcmp(arg, "-pg") == 0) {
		fprintf( stderr, "no profiling supported, %s ignored.\n", arg);
	} else if (strcmp(arg, "-b") == 0)
		;
	else if (strcmp(arg, "-g") == 0)
		fprintf( stderr, "no debugging supported, %s ignored.\n", arg);
	else if (strncmp(arg, "-ld=", 4) == 0 || strcmp(arg, "-static") == 0) {
		fprintf( stderr, "no linking supported, %s ignored.\n", arg);
	} else
		return 0;
	return 1;
}
