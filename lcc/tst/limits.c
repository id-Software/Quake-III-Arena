#include <limits.h>

main() {
	printf("UCHAR_MAX:	%08x=%d\n", UCHAR_MAX, UCHAR_MAX);
	printf("USHRT_MAX:	%08x=%d\n", USHRT_MAX, USHRT_MAX);
	printf("UINT_MAX:	%08x=%d\n", UINT_MAX, UINT_MAX);
	printf("ULONG_MAX:	%08lx=%ld\n", ULONG_MAX, ULONG_MAX);
	printf("CHAR_MAX:	%08x=%d\n", CHAR_MAX, CHAR_MAX);
	printf("SCHAR_MAX:	%08x=%d\n", SCHAR_MAX, SCHAR_MAX);
	printf("SHRT_MAX:	%08x=%d\n", SHRT_MAX, SHRT_MAX);
	printf("INT_MAX:	%08x=%d\n", INT_MAX, INT_MAX);
	printf("LONG_MAX:	%08lx=%ld\n", LONG_MAX, LONG_MAX);
	printf("CHAR_MIN:	%08x=%d\n", CHAR_MIN, CHAR_MIN);
	printf("SCHAR_MIN:	%08x=%d\n", SCHAR_MIN, SCHAR_MIN);
	printf("SHRT_MIN:	%08x=%d\n", SHRT_MIN, SHRT_MIN);
	printf("INT_MIN:	%08x=%d\n", INT_MIN, INT_MIN);
	printf("LONG_MIN:	%08lx=%ld\n", LONG_MIN, LONG_MIN);
	return 0;
}
