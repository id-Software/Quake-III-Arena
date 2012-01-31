#ifndef __STDIO
#define __STDIO

#define _IOFBF 0
#define _IOLBF 0100
#define _IONBF 04
#define BUFSIZ 1024
#define EOF (-1)

typedef struct {
	int	_cnt;
	unsigned char *_ptr;
	unsigned char *_base;
	unsigned char _flag;
	unsigned char _file;
} FILE;
extern FILE __iob[];
#define FILENAME_MAX 1024
#define FOPEN_MAX 20

#if !defined(_FPOS_T) && !defined(_FPOS_T_)
#define _FPOS_T
#define _FPOS_T_
typedef long fpos_t;
#endif

#define L_tmpnam 25
#define NULL 0
#define SEEK_CUR 1
#define SEEK_END 2
#define SEEK_SET 0

#if !defined(_SIZE_T) && !defined(_SIZE_T_)
#define _SIZE_T
#define _SIZE_T_
typedef unsigned long size_t;
#endif

#if !defined(_VA_LIST)
#define _VA_LIST
typedef char *__va_list;
#endif

#define stderr (&__iob[2])
#define stdin  (&__iob[0])
#define stdout (&__iob[1])
#define TMP_MAX 17576

extern int remove(const char *);
extern int rename(const char *, const char *);
extern FILE *tmpfile(void);
extern char *tmpnam(char *);
extern int fclose(FILE *);
extern int fflush(FILE *);
extern FILE *fopen(const char *, const char *);
extern FILE *freopen(const char *, const char *, FILE *);
extern void setbuf(FILE *, char *);
extern int setvbuf(FILE *, char *, int, size_t);
extern int fprintf(FILE *, const char *, ...);
extern int fscanf(FILE *, const char *, ...);
extern int printf(const char *, ...);
extern int scanf(const char *, ...);
extern int sprintf(char *, const char *, ...);
extern int sscanf(const char *, const char *, ...);
extern int vfprintf(FILE *, const char *, __va_list);
extern int vprintf(const char *, __va_list);
extern int vsprintf(char *, const char *, __va_list);
extern int fgetc(FILE *);
extern char *fgets(char *, int, FILE *);
extern int fputc(int, FILE *);
extern int fputs(const char *, FILE *);
extern int getc(FILE *);
extern int getchar(void);
extern char *gets(char *);
extern int putc(int, FILE *);
extern int putchar(int);
extern int puts(const char *);
extern int ungetc(int, FILE *);
extern size_t fread(void *, size_t, size_t, FILE *);
extern size_t fwrite(const void *, size_t, size_t, FILE *);
extern int fgetpos(FILE *, fpos_t *);
extern int fseek(FILE *, long int, int);
extern int fsetpos(FILE *, const fpos_t *);
extern long int ftell(FILE *);
extern void rewind(FILE *);
extern void clearerr(FILE *);
extern int feof(FILE *);
extern int ferror(FILE *);
extern void perror(const char *);

#define	_IOEOF 020
#define	_IOERR 040

#define	getc(p) (--(p)->_cnt < 0 ? __filbuf(p) : (int)*(p)->_ptr++)
#define putc(x, p) (--(p)->_cnt < 0 ? __flsbuf((unsigned char) (x), (p)) \
			: (int)(*(p)->_ptr++ = (x)))
extern int __filbuf(FILE *), __flsbuf(unsigned, FILE *);
#define feof(p) ((p)->_flag&_IOEOF)
#define ferror(p) ((p)->_flag&_IOERR)
#define	clearerr(p) ((p)->_flag &= ~(_IOERR|_IOEOF))
#define	getchar() getc(stdin)
#define	putchar(x) putc((x),stdout)

#endif /* __STDIO */
