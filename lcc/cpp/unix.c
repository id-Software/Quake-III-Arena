#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "cpp.h"

extern	int getopt(int, char *const *, const char *);
extern	char	*optarg, rcsid[];
extern	int	optind;
int	verbose;
int	Mflag;	/* only print active include files */
char	*objname; /* "src.$O: " */
int	Cplusplus = 1;

void
setup(int argc, char **argv)
{
	int c, fd, i;
	char *fp, *dp;
	Tokenrow tr;
	extern void setup_kwtab(void);

	setup_kwtab();
	while ((c = getopt(argc, argv, "MNOVv+I:D:U:F:lg")) != -1)
		switch (c) {
		case 'N':
			for (i=0; i<NINCLUDE; i++)
				if (includelist[i].always==1)
					includelist[i].deleted = 1;
			break;
		case 'I':
			for (i=NINCLUDE-2; i>=0; i--) {
				if (includelist[i].file==NULL) {
					includelist[i].always = 1;
					includelist[i].file = optarg;
					break;
				}
			}
			if (i<0)
				error(FATAL, "Too many -I directives");
			break;
		case 'D':
		case 'U':
			setsource("<cmdarg>", -1, optarg);
			maketokenrow(3, &tr);
			gettokens(&tr, 1);
			doadefine(&tr, c);
			unsetsource();
			break;
		case 'M':
			Mflag++;
			break;
		case 'v':
			fprintf(stderr, "%s %s\n", argv[0], rcsid);
			break;
		case 'V':
			verbose++;
			break;
		case '+':
			Cplusplus++;
			break;
		default:
			break;
		}
	dp = ".";
	fp = "<stdin>";
	fd = 0;
	if (optind<argc) {
		if ((fp = strrchr(argv[optind], '/')) != NULL) {
			int len = fp - argv[optind];
			dp = (char*)newstring((uchar*)argv[optind], len+1, 0);
			dp[len] = '\0';
		}
		fp = (char*)newstring((uchar*)argv[optind], strlen(argv[optind]), 0);
		if ((fd = open(fp, 0)) <= 0)
			error(FATAL, "Can't open input file %s", fp);
	}
	if (optind+1<argc) {
		int fdo = creat(argv[optind+1], 0666);
		if (fdo<0)
			error(FATAL, "Can't open output file %s", argv[optind+1]);
		dup2(fdo, 1);
	}
	if(Mflag)
		setobjname(fp);
	includelist[NINCLUDE-1].always = 0;
	includelist[NINCLUDE-1].file = dp;
	setsource(fp, fd, NULL);
}



/* memmove is defined here because some vendors don't provide it at
   all and others do a terrible job (like calling malloc) */
void *
memmove(void *dp, const void *sp, size_t n)
{
	unsigned char *cdp, *csp;

	if (n<=0)
		return 0;
	cdp = dp;
	csp = (unsigned char *)sp;
	if (cdp < csp) {
		do {
			*cdp++ = *csp++;
		} while (--n);
	} else {
		cdp += n;
		csp += n;
		do {
			*--cdp = *--csp;
		} while (--n);
	}
	return 0;
}
