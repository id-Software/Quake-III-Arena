/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// bg_lib.h -- standard C library replacement routines used by code
// compiled for the virtual machine

// This file is NOT included on native builds

typedef int size_t;

typedef char *  va_list;
#define _INTSIZEOF(n)   ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )
#define va_start(ap,v)  ( ap = (va_list)&v + _INTSIZEOF(v) )
#define va_arg(ap,t)    ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap)      ( ap = (va_list)0 )

#define CHAR_BIT      8         /* number of bits in a char */
#define SCHAR_MIN   (-128)      /* minimum signed char value */
#define SCHAR_MAX     127       /* maximum signed char value */
#define UCHAR_MAX     0xff      /* maximum unsigned char value */

#define SHRT_MIN    (-32768)        /* minimum (signed) short value */
#define SHRT_MAX      32767         /* maximum (signed) short value */
#define USHRT_MAX     0xffff        /* maximum unsigned short value */
#define INT_MIN     (-2147483647 - 1) /* minimum (signed) int value */
#define INT_MAX       2147483647    /* maximum (signed) int value */
#define UINT_MAX      0xffffffff    /* maximum unsigned int value */
#define LONG_MIN    (-2147483647L - 1) /* minimum (signed) long value */
#define LONG_MAX      2147483647L   /* maximum (signed) long value */
#define ULONG_MAX     0xffffffffUL  /* maximum unsigned long value */

// Misc functions
typedef int cmp_t(const void *, const void *);
void qsort(void *a, size_t n, size_t es, cmp_t *cmp);
void	srand( unsigned seed );
int		rand( void );

// String functions
size_t strlen( const char *string );
char *strcat( char *strDestination, const char *strSource );
char *strcpy( char *strDestination, const char *strSource );
int strcmp( const char *string1, const char *string2 );
char *strchr( const char *string, int c );
char *strstr( const char *string, const char *strCharSet );
char *strncpy( char *strDest, const char *strSource, size_t count );
int tolower( int c );
int toupper( int c );

double atof( const char *string );
double _atof( const char **stringPtr );
int atoi( const char *string );
int _atoi( const char **stringPtr );

int vsprintf( char *buffer, const char *fmt, va_list argptr );
int sscanf( const char *buffer, const char *fmt, ... );

// Memory functions
void *memmove( void *dest, const void *src, size_t count );
void *memset( void *dest, int c, size_t count );
void *memcpy( void *dest, const void *src, size_t count );

// Math functions
double ceil( double x );
double floor( double x );
double sqrt( double x );
double sin( double x );
double cos( double x );
double atan2( double y, double x );
double tan( double x );
int abs( int n );
double fabs( double x );
double acos( double x );

