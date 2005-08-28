#ifndef __STDARG
#define __STDARG

#if !defined(_VA_LIST)
#define _VA_LIST
typedef struct {
	char	*_a0;		/* pointer to first homed integer arg */
	int	_offset;	/* byte offset of next param */
	float	_tmp;
} __va_list;
#endif
typedef __va_list va_list;

#define va_start(list, start) ((void)( \
	(list)._a0 = (__typecode(__firstarg)==1 ? \
		(char*)&__firstarg+48 : (char *)&__firstarg), \
	(list)._offset = (__typecode(start)==1 ? \
		(char*)&start+56 : (char *)&start+8)-(list)._a0))
#define va_arg(list, mode) (*(mode *)( \
	(list)._offset += (int)((sizeof(mode)+7)&~7), \
	(__typecode(mode)==1 && sizeof(mode)==4) ? \
		((list)._tmp = (float)*(double *)((list)._a0 + (list)._offset - \
			((list)._offset <= 48 ? 56 : 8))), (char *)&(list)._tmp : \
	(__typecode(mode)==1 && (list)._offset <= 48) ? \
		(list)._a0 + (list)._offset - 56 : \
	(list)._a0 + (list)._offset - (int)((sizeof(mode)+7)&~7)))
#define va_end(list) ((void)0)
#endif
