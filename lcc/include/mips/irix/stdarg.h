#ifndef __STDARG
#define __STDARG

#if !defined(_VA_LIST)
#define _VA_LIST
typedef char *__va_list;
#endif
typedef __va_list va_list;

#define va_start(list, start) ((void)((list) = (sizeof(start)<4 ? \
	(char *)((int *)&(start)+1) : (char *)(&(start)+1))))
#define __va_arg(list, mode, n) \
	(*(mode *)(&(list += ((sizeof(mode)+n)&~n))[-(int)((sizeof(mode)+n)&~n)]))
#define _bigendian_va_arg(list, mode, n) (\
	sizeof(mode)==1 ? *(mode *)(&(list += 4)[-1]) : \
	sizeof(mode)==2 ? *(mode *)(&(list += 4)[-2]) : __va_arg(list, mode, n))
#define _littleendian_va_arg(list, mode, n) __va_arg(list, mode, n)
#define va_end(list) ((void)0)
#define va_arg(list, mode) (sizeof(mode)==8 ? \
	*(mode *)(&(list = (char*)(((int)list + 15)&~7U))[-8]) : \
	_bigendian_va_arg(list, mode, 3U))
#endif
