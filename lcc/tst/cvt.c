signed char c;
signed short s;
signed int i;
signed long int l;
unsigned char C;
unsigned short S;
unsigned int I;
unsigned long int L;
float f;
double d;
long double D;
void *p;
void (*P)(void);

void print(void) {
	printf("%d %d %d %ld %u %u %u %lu %f %f %lf\n",c,s,i,l,C,S,I,L,f,d,D);
}

main() {
	c= 1;     s=c;i=c;l=c;C=c;S=c;I=c;L=c;f=c;d=c;D=c; print();
	s= 2; c=s;    i=s;l=s;C=s;S=s;I=s;L=s;f=s;d=s;D=s; print();
	i= 3; c=i;s=i;    l=i;C=i;S=i;I=i;L=i;f=i;d=i;D=i; print();
	l= 4; c=l;s=l;i=l;    C=l;S=l;I=l;L=l;f=l;d=l;D=l; print();
	C= 5; c=C;s=C;i=C;l=C;    S=C;I=C;L=C;f=C;d=C;D=C; print();
	S= 6; c=S;s=S;i=S;l=S;C=S;    I=S;L=S;f=S;d=S;D=S; print();
	I= 7; c=I;s=I;i=I;l=I;C=I;S=I;    L=I;f=I;d=I;D=I; print();
	L= 8; c=L;s=L;i=L;l=L;C=L;S=L;I=S;    f=L;d=L;D=L; print();
	f= 9; c=f;s=f;i=f;l=f;C=f;S=f;I=f;L=f;    d=f;D=f; print();
	d=10; c=d;s=d;i=d;l=d;C=d;S=d;I=d;L=d;f=d;    D=d; print();
	D=11; c=D;s=D;i=D;l=D;C=D;S=D;I=D;L=D;f=D;d=D;     print();

	p=0; p=0L; p=0U; p=0UL; p=P;
	P=0; P=0L; P=0U; P=0UL; P=p;
	return 0;
}
