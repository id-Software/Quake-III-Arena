#ifndef __SETJMP
#define __SETJMP



typedef int jmp_buf[12];
int setjmp(jmp_buf);
void longjmp(jmp_buf, int);

#endif /* __SETJMP */
