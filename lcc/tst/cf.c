/* cf - print character frequencies */
float f[128];

main(argc, argv)
int argc;
char *argv[];
{
	int i, c, nc;
	float cutoff, atof();

	if (argc <= 1)
		cutoff = 0.0;
	else
		cutoff = atof(argv[1])/100;
	for (i = 0; i <= 127; )
		f[i++] = 0.0;
	nc = 0;
	while ((c = getchar()) != -1) {
		f[c] += 1;
		nc++;
	}
	printf("char\tfreq\n");
	for (i = 0; i <= 127; ++i)
		if (f[i] && f[i]/nc >= cutoff) {
			if (i <= ' ')
				printf("%03o", i);
			else
				printf("%c", i);
			printf("\t%.1f\n", 100*f[i]/nc);
		}
	return 0;
}
