#ifndef __SIGNAL
#define __SIGNAL

typedef int sig_atomic_t;

#define SIG_DFL ((void (*)(int))0)
#define SIG_ERR ((void (*)(int))-1)
#define SIG_IGN ((void (*)(int))1)

#define SIGABRT	6
#define SIGFPE	8
#define SIGILL	4
#define SIGINT	2
#define SIGSEGV	11
#define SIGTERM	15

void (*signal(int, void (*)(int)))(int);
int raise(int);

#endif /* __SIGNAL */
