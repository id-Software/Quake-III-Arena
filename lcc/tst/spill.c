main(){}

f(i){i=f()+f();}

f2(i){i=f()+(i?f():1);}

f3(int i,int *p){register r1=0,r2=0,r3=0,r4=0,r5=0,r6=0,r7=0,r8=0,r9=0,r10=0;*p++=i?f():0;}

double a[10],b[10];int i;f4(){register r6=0,r7=0,r8=0,r9=0,r10=0,r11=0;i=a[i]+b[i] && i && a[i]-b[i];}
/* f4 causes parent to spill child on vax when odd double regs are enabled */

int j, k, m, n;
double *A, *B, x;
f5(){
	x=A[k*m]*A[j*m]+B[k*n]*B[j*n];
	x=A[k*m]*B[j*n]-B[k*n]*A[j*m];
}  
