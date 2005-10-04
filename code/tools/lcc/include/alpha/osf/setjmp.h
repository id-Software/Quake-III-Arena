#ifndef __SETJMP
#define __SETJMP



typedef int jmp_buf[35+1+48];
int setjmp(jmp_buf);
void longjmp(jmp_buf, int);

#endif /* __SETJMP */
