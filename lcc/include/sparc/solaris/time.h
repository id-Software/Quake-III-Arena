#ifndef __TIME
#define __TIME

#define CLOCKS_PER_SEC 1000000
#ifndef NULL
#define NULL 0
#endif

#if !defined(_CLOCK_T) && !defined(_CLOCK_T_)
#define _CLOCK_T
#define _CLOCK_T_
typedef long clock_t;
#endif

#if !defined(_TIME_T) && !defined(_TIME_T_)
#define _TIME_T
#define _TIME_T_
typedef long time_t;
#endif

#if !defined(_SIZE_T) && !defined(_SIZE_T_)
#define _SIZE_T
#define _SIZE_T_
typedef unsigned long size_t;
#endif

struct tm {
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
};
extern clock_t clock(void);
extern double difftime(time_t, time_t);
extern time_t mktime(struct tm *);
extern time_t time(time_t *);
extern char *asctime(const struct tm *);
extern char *ctime(const time_t *);
extern struct tm *gmtime(const time_t *);
extern struct tm *localtime(const time_t *);
extern size_t strftime(char *, size_t, const char *, const struct tm *);

#endif /* __TIME */
