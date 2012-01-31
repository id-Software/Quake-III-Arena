main() {
	exit(0);
}

nested(a,b) {
	if ((a<4 && b == 'r')
		|| (a == 1 && (b == 'h' || b == 'i'))
		|| (a == 2 && (b == 'o' || b == 'y'))
	) a=b;
}

/* type name scope */

void s(struct D *d) {}	/* this struct D differs from the one below */
typedef struct D D;
struct D {int x, y;} Dy={0};
D Dz={1};
Dfunc(){
	D a; a.y=1;
	s(&Dy);		/* error */
}

/* qualifiers */

const a; int b;
const int a, *x; int b, *y;
volatile unsigned z;

f() {
	x = y;
	z = z + z;	/* should be 2 references to z's r-value */
}
f1() {
	x = &a;
	x = &b;
	y = &a;		/* error */
	y = &b;
}
f2(int **a, int **b) {
	f(&x, &y);
	**a = 0;
	return **b;
}
g(const int *p) {
	g(&a);
	g(&b);
	return *p;
}
h(int *p) {
	f(&a);
	f(&b);
	return *p;
}
h1(const int x, int y) {
	h1(a,b);
	h1(b,a);
	return x + y;
}
h2() {
	char *b; const void *p;
	p = b;
	b = p;		/* error */
}


/* static naming */

extern int yy; set1() { { static yy=1; yy=2;} yy=4;}
static int yy; set2() { yy=5; {static yy=2; yy=3; }}
static void goo() {}
sss() { int goo; { static int goo();} goo=1;}
rrr(p) float *p; { extern int xr;
 { static float xr;
 { extern int *xr; } p=&xr; }}

/* local extern */

static int ss1;
int ss3;
extern int ss5;
setstatic() { extern int ss1,ss2,ss3,ss4; ss1 = ss2; ss3 = ss4; ss5 = 0;}
static int ss2;
int ss4;
static int ss5;

/* function prototypes */

int fx1(void);
int fx1();

int gx1(double x);
int gx1(x) double x; { gx1(&x); }	/* error */

int hx1();
int hx1(double x,...);	/* error */

int ff1(double x, int *y);
int ff1(x,y) float x; int y[]; {x=y[0];}

int gg1(int a);
int gg1(a,b){a=b;}

int hh1(const int x);
hh1(a) {return a;}

extern int strcmp(const char*, const char*);
extern void qsort(void*, int, int, int (*)(const void*, const void*));
extern int cmp(char**a, char**b) { return strcmp(*a,*b); }
sort() {
	int n; char *a[100];
	qsort(a, n, sizeof(char*), (int (*)(const void*, const void*))cmp);
	qsort(a, n, sizeof(char*), cmp);	/* error */
}

/* nasty calls */

onearg(){
	int a,b,c,d;
	f( ( (a? (b = 1): (c = 2)), (d ? 3 : 4) ) );	/* 1 argument */
}
