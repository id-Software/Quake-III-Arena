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
//need to rewrite this

#ifndef __UTIL_STR_H__
#define __UTIL_STR_H__

#include <assert.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#pragma warning(disable : 4710)     // function 'blah' not inlined
#endif

void TestStringClass ();

class strdata
   {
   public:
      strdata () : len( 0 ), refcount ( 0 ), data ( NULL ), alloced ( 0 ) {}
      ~strdata () 
         {
         if ( data )
            delete [] data;
         }

      void AddRef () { refcount++; }
      bool DelRef () // True if killed
         {
         refcount--;
         if ( refcount < 0 )
            {
            delete this;
            return true;
            }
         
         return false;
         }

      int len;
      int refcount;
      char *data;
      int alloced;
   };

class idStr {
protected:
	strdata	*m_data;
	void EnsureAlloced ( int, bool keepold = true );
	void EnsureDataWritable ();

public:
	~idStr();
	idStr();
	idStr( const char *text );
	idStr( const idStr& string );
	idStr( const idStr string, int start, int end );
	idStr( const char ch );
	idStr( const int num );
	idStr( const float num );
	idStr( const unsigned num );
	int	length( void ) const;
	int	allocated( void ) const;
	const char * c_str( void ) const;

	void		append( const char *text );
	void		append( const idStr& text );
	char		operator[]( int index ) const;
	char&		operator[]( int index );

	void		operator=( const idStr& text );
	void		operator=( const char *text );

	friend	idStr		operator+( const idStr& a, const idStr& b );
	friend	idStr		operator+( const idStr& a, const char *b );
	friend	idStr		operator+( const char *a, const idStr& b );

    friend	idStr		operator+( const idStr& a, const float b );
    friend	idStr		operator+( const idStr& a, const int b );
    friend	idStr		operator+( const idStr& a, const unsigned b );
    friend	idStr		operator+( const idStr& a, const bool b );
    friend	idStr		operator+( const idStr& a, const char b );

	idStr&		operator+=( const idStr& a );
	idStr&		operator+=( const char *a );
	idStr&		operator+=( const float a );
	idStr&		operator+=( const char a );
	idStr&		operator+=( const int a );
	idStr&		operator+=( const unsigned a );
	idStr&		operator+=( const bool a );

	static   char     *toLower( char *s1 );
	static   char     *toUpper( char *s1 );

	friend	bool		operator==(	const idStr& a, const idStr& b );
	friend	bool		operator==(	const idStr& a, const char *b );
	friend	bool		operator==(	const char *a, const idStr& b );

	friend	bool		operator!=(	const idStr& a, const idStr& b );
	friend	bool		operator!=(	const idStr& a, const char *b );
	friend	bool		operator!=(	const char *a, const idStr& b );

	operator const char * () const;
	operator const char * ();

    int      icmpn( const char *text, int n ) const;
	int      icmpn( const idStr& text, int n ) const;
	int      icmp( const char *text ) const;
	int      icmp( const idStr& text ) const;
	int      cmpn( const char *text, int n ) const;
	int      cmpn( const idStr& text, int n ) const;
	int      cmp( const char *text ) const;
	int      cmp( const idStr& text ) const;

	void     tolower( void );
	void     toupper( void );

	static   int      icmpn( const char *s1, const char *s2, int n );
	static   int      icmp( const char *s1, const char *s2 );
	static   int      cmpn( const char *s1, const char *s2, int n );
	static   int      cmp( const char *s1, const char *s2 );

	static   void     snprintf ( char *dst, int size, const char *fmt, ... );

	static   bool	   isNumeric( const char *str );
    bool    isNumeric( void ) const;

	void     CapLength ( int );

	void     BackSlashesToSlashes ();

};

inline idStr::~idStr()
	{
   if ( m_data )
      {
      m_data->DelRef ();
      m_data = NULL;
      }
	}

inline idStr::idStr() : m_data ( NULL )
	{
   EnsureAlloced ( 1 );
	m_data->data[ 0 ] = 0;
	}

inline idStr::idStr
	(
	const char *text
   ) : m_data ( NULL )

	{
   int len;

	assert( text );

	if ( text )
		{
      len = strlen( text );
		EnsureAlloced ( len + 1 );
		strcpy( m_data->data, text );
      m_data->len = len;
		}
	else
		{
      EnsureAlloced ( 1 );
		m_data->data[ 0 ] = 0;
		m_data->len = 0;
		}
	}

inline idStr::idStr
	(
	const idStr& text
   ) : m_data ( NULL )

	{
   m_data = text.m_data;
   m_data->AddRef ();
   }

inline idStr::idStr
	(
	const idStr text, 
	int start,
	int end
   ) : m_data ( NULL )

	{
	int i;
   int len;

	if ( end > text.length() )
		{
		end = text.length();
		}

	if ( start > text.length() )
		{
		start = text.length();
		}

	len = end - start;
	if ( len < 0 )
		{
		len = 0;
		}

   EnsureAlloced ( len + 1 );

	for( i = 0; i < len; i++ )
		{
		m_data->data[ i ] = text[ start + i ];
		}

	m_data->data[ len ] = 0;
   m_data->len = len;
	}

inline idStr::idStr
   (
   const char ch
   ) : m_data ( NULL )

   {
   EnsureAlloced ( 2 );

   m_data->data[ 0 ] = ch;
   m_data->data[ 1 ] = 0;
   m_data->len = 1;
   }

inline idStr::idStr
   (
   const float num
   ) : m_data ( NULL )

   {
   char text[ 32 ];
   int len;

   sprintf( text, "%.3f", num );
   len = strlen( text );
   EnsureAlloced( len + 1 );
   strcpy( m_data->data, text );
   m_data->len = len;
   }

inline idStr::idStr
   (
   const int num
   ) : m_data ( NULL )

   {
   char text[ 32 ];
   int len;

   sprintf( text, "%d", num );
   len = strlen( text );
   EnsureAlloced( len + 1 );
   strcpy( m_data->data, text );
   m_data->len = len;
   }

inline idStr::idStr
   (
   const unsigned num
   ) : m_data ( NULL )

   {
   char text[ 32 ];
   int len;

   sprintf( text, "%u", num );
   len = strlen( text );
   EnsureAlloced( len + 1 );
   strcpy( m_data->data, text );
   m_data->len = len;
   }

inline int idStr::length( void ) const
	{
   return ( m_data != NULL ) ? m_data->len : 0;
	}

inline int idStr::allocated( void ) const
	{
   return ( m_data != NULL ) ? m_data->alloced + sizeof( *m_data ) : 0;
	}

inline const char *idStr::c_str( void ) const
	{
	assert( m_data );

	return m_data->data;
	}

inline void idStr::append
	(
	const char *text
	)

	{
   int len;

	assert( text );

	if ( text )
		{
		len = length() + strlen( text );
		EnsureAlloced( len + 1 );

      strcat( m_data->data, text );
      m_data->len = len;
		}
	}

inline void idStr::append
	(
	const idStr& text
	)

	{
   int len;

   len = length() + text.length();
   EnsureAlloced ( len + 1 );

   strcat ( m_data->data, text.c_str () );
   m_data->len = len;
	}

inline char idStr::operator[]( int index ) const
	{
   assert ( m_data );
   
   if ( !m_data )
      return 0;

	// don't include the '/0' in the test, because technically, it's out of bounds
	assert( ( index >= 0 ) && ( index < m_data->len ) );

	// In release mode, give them a null character
	// don't include the '/0' in the test, because technically, it's out of bounds
	if ( ( index < 0 ) || ( index >= m_data->len ) )
		{
		return 0;
		}

	return m_data->data[ index ];
	}

inline char& idStr::operator[]
	(
	int index
	)

	{
	// Used for result for invalid indices
	static char dummy = 0;
   assert ( m_data );

   // We don't know if they'll write to it or not
   // if it's not a const object
   EnsureDataWritable ();

   if ( !m_data )
      return dummy;

	// don't include the '/0' in the test, because technically, it's out of bounds
	assert( ( index >= 0 ) && ( index < m_data->len ) );

	// In release mode, let them change a safe variable
	// don't include the '/0' in the test, because technically, it's out of bounds
	if ( ( index < 0 ) || ( index >= m_data->len ) )
		{
		return dummy;
		}

	return m_data->data[ index ];
	}

inline void idStr::operator=
	(
	const idStr& text
	)

	{
   // adding the reference before deleting our current reference prevents
   // us from deleting our string if we are copying from ourself
   text.m_data->AddRef();
   m_data->DelRef();
   m_data = text.m_data;
   }

inline void idStr::operator=
	(
	const char *text
	)

	{
   int len;

	assert( text );

	if ( !text )
		{
		// safe behaviour if NULL
		EnsureAlloced ( 1, false );
      m_data->data[0] = 0;
      m_data->len = 0;
      return;
		}

   if ( !m_data )
      {
      len = strlen ( text );
      EnsureAlloced( len + 1, false );
      strcpy ( m_data->data, text );
      m_data->len = len;
      return;
      }

   if ( text == m_data->data )
      return; // Copying same thing.  Punt.

   // If we alias and I don't do this, I could corrupt other strings...  This 
   // will get called with EnsureAlloced anyway
   EnsureDataWritable ();

   // Now we need to check if we're aliasing..
   if ( text >= m_data->data && text <= m_data->data + m_data->len )
      {
      // Great, we're aliasing.  We're copying from inside ourselves.
      // This means that I don't have to ensure that anything is alloced,
      // though I'll assert just in case.
      int diff = text - m_data->data;
      int i;

      assert ( strlen ( text ) < (unsigned) m_data->len );
      
      for ( i = 0; text[i]; i++ )
         {
         m_data->data[i] = text[i];
         }

      m_data->data[i] = 0;

      m_data->len -= diff;

      return;
      }

	len = strlen( text );
   EnsureAlloced ( len + 1, false );
	strcpy( m_data->data, text );
   m_data->len = len;
	}

inline idStr operator+
	(
	const idStr& a,
	const idStr& b
	)

	{
	idStr result( a );

	result.append( b );

	return result;
	}

inline idStr operator+
	(
	const idStr& a,
	const char *b
	)

	{
	idStr result( a );

	result.append( b );

	return result;
	}

inline idStr operator+
	(
	const char *a,
	const idStr& b
	)

	{
	idStr result( a );

	result.append( b );

	return result;
	}

inline idStr operator+
   (
   const idStr& a,
   const bool b
   )

   {
	idStr result( a );

   result.append( b ? "true" : "false" );

	return result;
   }

inline idStr operator+
	(
   const idStr& a,
	const char b
	)

	{
   char text[ 2 ];

   text[ 0 ] = b;
   text[ 1 ] = 0;

	return a + text;
	}

inline idStr& idStr::operator+=
	(
	const idStr& a
	)

	{
	append( a );
	return *this;
	}

inline idStr& idStr::operator+=
	(
	const char *a
	)

	{
	append( a );
	return *this;
	}

inline idStr& idStr::operator+=
	(
	const char a
	)

	{
   char text[ 2 ];

   text[ 0 ] = a;
   text[ 1 ] = 0;
	append( text );

   return *this;
	}

inline idStr& idStr::operator+=
	(
	const bool a
	)

	{
   append( a ? "true" : "false" );
	return *this;
	}

inline bool operator==
	(
	const idStr& a,
	const idStr& b
	)

	{
	return ( !strcmp( a.c_str(), b.c_str() ) );
	}

inline bool operator==
	(
	const idStr& a,
	const char *b
	)

	{
	assert( b );
	if ( !b )
		{
		return false;
		}
	return ( !strcmp( a.c_str(), b ) );
	}

inline bool operator==
	(
	const char *a,
	const idStr& b
	)

	{
	assert( a );
	if ( !a )
		{
		return false;
		}
	return ( !strcmp( a, b.c_str() ) );
	}

inline bool operator!=
	(
	const idStr& a,
	const idStr& b
	)

	{
	return !( a == b );
	}

inline bool operator!=
	(
	const idStr& a,
	const char *b
	)

	{
	return !( a == b );
	}

inline bool operator!=
	(
	const char *a,
	const idStr& b
	)

	{
	return !( a == b );
	}

inline int idStr::icmpn
   (
   const char *text, 
   int n
   ) const

   {
	assert( m_data );
	assert( text );

   return idStr::icmpn( m_data->data, text, n );
   }

inline int idStr::icmpn
   (
   const idStr& text, 
   int n
   ) const

   {
	assert( m_data );
	assert( text.m_data );

   return idStr::icmpn( m_data->data, text.m_data->data, n );
   }

inline int idStr::icmp
   (
   const char *text
   ) const

   {
	assert( m_data );
	assert( text );

   return idStr::icmp( m_data->data, text );
   }

inline int idStr::icmp
   (
   const idStr& text
   ) const

   {
	assert( c_str () );
	assert( text.c_str () );

   return idStr::icmp( c_str () , text.c_str () );
   }

inline int idStr::cmp
   (
   const char *text
   ) const

   {
	assert( m_data );
	assert( text );

   return idStr::cmp( m_data->data, text );
   }

inline int idStr::cmp
   (
   const idStr& text
   ) const

   {
	assert( c_str () );
	assert( text.c_str () );

   return idStr::cmp( c_str () , text.c_str () );
   }

inline int idStr::cmpn
   (
   const char *text, 
   int n
   ) const

   {
	assert( c_str () );
	assert( text );

   return idStr::cmpn( c_str () , text, n );
   }

inline int idStr::cmpn
   (
   const idStr& text, 
   int n
   ) const

   {
	assert( c_str () );
	assert( text.c_str ()  );

   return idStr::cmpn( c_str () , text.c_str () , n );
   }

inline void idStr::tolower
   (
   void
   )

   {
   assert( m_data );

   EnsureDataWritable ();

   idStr::toLower( m_data->data );
   }

inline void idStr::toupper
   (
   void
   )

   {
   assert( m_data );

   EnsureDataWritable ();

   idStr::toUpper( m_data->data );
   }

inline bool idStr::isNumeric
   (
   void
   ) const

   {
   assert( m_data );
   return idStr::isNumeric( m_data->data );
   }

inline idStr::operator const char *() {
	return c_str();
}

inline idStr::operator const char *
   (
   void
   ) const

   {
   return c_str ();
   }

#endif
