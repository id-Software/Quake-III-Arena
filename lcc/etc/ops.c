#include "c.h"

/* ops [ {csilhfdxp}=n ]...
 * prints lcc dag operator set for a given set of type sizes.
 */


static char list[] = { 'c', 's', 'i', 'l', 'h', 'f', 'd', 'x', 'p', 0 };
static int sizes[] = {  1,   2,   4,   4,   8,   4,   8,  16,   8 };

static int doop(int op, int type, const char *sz, const char *opname) {
	int count = 0;
	static int last;

	if (op == LOAD)
		return 0;
	if (last != 0 && last != op)
		printf("\n");
	last = op;
	if (type == B || type == V) {
		printf(" %s=%d", opname, op + type);
		count++;
	} else {
		int i, done = 0;
		const char *s;
		for (i = 0; sz[i] != '\0' && (s = strchr(list, sz[i])) != NULL; i++) {
			int n = sizes[s-list];
			if ((done&(1<<n)) == 0) {
				printf(" %s%d=%d", opname, n, op + type + sizeop(n));
				count++;
			}
			done |= 1<<n;
		}
	}
	printf("\n");
	return count;
}

int main(int argc, char *argv[]) {
	int i, count = 0;

	for (i = 1; i < argc; i++) {
		char c, *s;
		int n;
		if (sscanf(argv[i], "%c=%d", &c, &n) == 2
		&& n > 0 && (s = strchr(list, c)) != NULL)
			sizes[s-list] = n;
		else {
			fprintf(stderr, "usage: %s [ {csilhfdxp}=n ]...\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}
#define gop(x,n)
#define op(x,t,s) count += doop(x,t,#s,#x #t);
gop(CNST,1)
	op(CNST,F,fdx)
	op(CNST,I,csilh)
	op(CNST,P,p)
	op(CNST,U,csilh)
gop(ARG,2)
	op(ARG,B,-)
	op(ARG,F,fdx)
	op(ARG,I,ilh)
	op(ARG,P,p)
	op(ARG,U,ilh)
gop(ASGN,3)
	op(ASGN,B,-)
	op(ASGN,F,fdx)
	op(ASGN,I,csilh)
	op(ASGN,P,p)
	op(ASGN,U,csilh)
gop(INDIR,4)
	op(INDIR,B,-)
	op(INDIR,F,fdx)
	op(INDIR,I,csilh)
	op(INDIR,P,p)
	op(INDIR,U,csilh)
gop(CVF,7)
	op(CVF,F,fdx)
	op(CVF,I,ilh)
gop(CVI,8)
	op(CVI,F,fdx)
	op(CVI,I,csilh)
	op(CVI,U,csilhp)
gop(CVP,9)
	op(CVP,U,p)
gop(CVU,11)
	op(CVU,I,csilh)
	op(CVU,P,p)
	op(CVU,U,csilh)
gop(NEG,12)
	op(NEG,F,fdx)
	op(NEG,I,ilh)
gop(CALL,13)
	op(CALL,B,-)
	op(CALL,F,fdx)
	op(CALL,I,ilh)
	op(CALL,P,p)
	op(CALL,U,ilh)
	op(CALL,V,-)
gop(RET,15)
	op(RET,F,fdx)
	op(RET,I,ilh)
	op(RET,P,p)
	op(RET,U,ilh)
	op(RET,V,-)
gop(ADDRG,16)
	op(ADDRG,P,p)
gop(ADDRF,17)
	op(ADDRF,P,p)
gop(ADDRL,18)
	op(ADDRL,P,p)
gop(ADD,19)
	op(ADD,F,fdx)
	op(ADD,I,ilh)
	op(ADD,P,p)
	op(ADD,U,ilhp)
gop(SUB,20)
	op(SUB,F,fdx)
	op(SUB,I,ilh)
	op(SUB,P,p)
	op(SUB,U,ilhp)
gop(LSH,21)
	op(LSH,I,ilh)
	op(LSH,U,ilh)
gop(MOD,22)
	op(MOD,I,ilh)
	op(MOD,U,ilh)
gop(RSH,23)
	op(RSH,I,ilh)
	op(RSH,U,ilh)
gop(BAND,24)
	op(BAND,I,ilh)
	op(BAND,U,ilh)
gop(BCOM,25)
	op(BCOM,I,ilh)
	op(BCOM,U,ilh)
gop(BOR,26)
	op(BOR,I,ilh)
	op(BOR,U,ilh)
gop(BXOR,27)
	op(BXOR,I,ilh)
	op(BXOR,U,ilh)
gop(DIV,28)
	op(DIV,F,fdx)
	op(DIV,I,ilh)
	op(DIV,U,ilh)
gop(MUL,29)
	op(MUL,F,fdx)
	op(MUL,I,ilh)
	op(MUL,U,ilh)
gop(EQ,30)
	op(EQ,F,fdx)
	op(EQ,I,ilh)
	op(EQ,U,ilhp)
gop(GE,31)
	op(GE,F,fdx)
	op(GE,I,ilh)
	op(GE,U,ilhp)
gop(GT,32)
	op(GT,F,fdx)
	op(GT,I,ilh)
	op(GT,U,ilhp)
gop(LE,33)
	op(LE,F,fdx)
	op(LE,I,ilh)
	op(LE,U,ilhp)
gop(LT,34)
	op(LT,F,fdx)
	op(LT,I,ilh)
	op(LT,U,ilhp)
gop(NE,35)
	op(NE,F,fdx)
	op(NE,I,ilh)
	op(NE,U,ilhp)
gop(JUMP,36)
	op(JUMP,V,-)
gop(LABEL,37)
	op(LABEL,V,-)
gop(LOAD,14)
	op(LOAD,B,-)
	op(LOAD,F,fdx)
	op(LOAD,I,csilh)
	op(LOAD,P,p)
	op(LOAD,U,csilhp)
#undef gop
#undef op
	fprintf(stderr, "%d operators\n", count);
	return EXIT_SUCCESS;
}
