#include <stdarg.h>

struct node { int a[4]; } x = {1,2,3,4};

print(char *fmt, ...);

main() {
	print("test 1\n");
	print("test %s\n", "2");
	print("test %d%c", 3, '\n');
	print("%s%s %w%c", "te", "st", 4, '\n');
	print("%s%s %f%c", "te", "st", 5.0, '\n');
	print("%b %b %b %b %b %b\n", x, x, x, x, x, x);
	return 0;
}

print(char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	for (; *fmt; fmt++)
		if (*fmt == '%')
			switch (*++fmt) {
			case 'b': {
				struct node x = va_arg(ap, struct node);
				printf("{%d %d %d %d}", x.a[0], x.a[1], x.a[2], x.a[3]);
				break;
				}
			case 'c':
				printf("%c", va_arg(ap, char));
				break;
			case 'd':
				printf("%d", va_arg(ap, int));
				break;
			case 'w':
				printf("%x", va_arg(ap, short));
				break;
			case 's':
				printf("%s", va_arg(ap, char *));
				break;
			case 'f':
				printf("%f", va_arg(ap, double));
				break;
			default:
				printf("%c", *fmt);
				break;
			}
		 else
			printf("%c", *fmt);
	va_end(ap);
}
