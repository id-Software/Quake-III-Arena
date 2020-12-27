#include	<stdio.h>
#include	<string.h>
#define EPR                 fprintf(stderr,
#define ERR(str, chr)       if(opterr){EPR "%s%c\n", str, chr);}
int     opterr = 1;
int     optind = 1;
int	optopt;
char    *optarg;

int
lcc_getopt (int argc, char *const argv[], const char *opts)
{
	static int sp = 1;
	int c;
	char *cp;

	if (sp == 1) {
		if (optind >= argc ||
		   argv[optind][0] != '-' || argv[optind][1] == '\0')
			return -1;
		else if (strcmp(argv[optind], "--") == 0) {
			optind++;
			return -1;
		}
	}
	optopt = c = argv[optind][sp];
	if (c == ':' || (cp=strchr(opts, c)) == 0) {
		ERR (": illegal option -- ", c);
		if (argv[optind][++sp] == '\0') {
			optind++;
			sp = 1;
		}
		return '?';
	}
	if (*++cp == ':') {
		if (argv[optind][sp+1] != '\0')
			optarg = &argv[optind++][sp+1];
		else if (++optind >= argc) {
			ERR (": option requires an argument -- ", c);
			sp = 1;
			return '?';
		} else
			optarg = argv[optind++];
		sp = 1;
	} else {
		if (argv[optind][++sp] == '\0') {
			sp = 1;
			optind++;
		}
		optarg = 0;
	}
	return c;
}
