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
#ifndef __STR__
#define __STR__
//
// class Str
// loose replacement for CString from MFC
//
//#include "cmdlib.h"
#include <string.h>

char* __StrDup(char* pStr);
char* __StrDup(const char* pStr);



static char *g_pStrWork = NULL;

class Str
{
protected:
  bool m_bIgnoreCase;
  char *m_pStr;

public:
  Str()
  {
    m_bIgnoreCase = true;
    m_pStr = NULL;
  }

  Str(char *p)
  {
    m_bIgnoreCase = true;
    m_pStr = __StrDup(p);
  }

  Str(const char *p)
  {
    m_bIgnoreCase = true;
    m_pStr = __StrDup(p);
  }

  void Deallocate()
  {
    delete []m_pStr;
    m_pStr = NULL;
  }

  void Allocate(int n)
  {
    Deallocate();
    m_pStr = new char[n];
  }

  const char* GetBuffer()
  {
    return m_pStr;
  }

  void MakeEmpty()
  {
    Deallocate();
    m_pStr = __StrDup("");
  }

  ~Str()
  {
    Deallocate();
    delete []g_pStrWork;
    g_pStrWork = NULL;
  }

  void MakeLower()
  {
    if (m_pStr)
    {
      strlwr(m_pStr);
    }
  }

  int Find(const char *p)
  {
    char *pf = strstr(m_pStr, p);
    return (pf) ? (pf - m_pStr) : -1;
  }

  int GetLength()
  {
    return (m_pStr) ? strlen(m_pStr) : 0;
  }

  const char* Left(int n)
  {
    delete []g_pStrWork;
    if (n > 0)
    {
      g_pStrWork = new char[n+1];
      strncpy(g_pStrWork, m_pStr, n);
    }
    else
    {
      g_pStrWork = "";
      g_pStrWork = new char[1];
      g_pStrWork[0] = '\0';
    }
    return g_pStrWork;
  }

  const char* Right(int n)
  {
    delete []g_pStrWork;
    if (n > 0)
    {
      g_pStrWork = new char[n+1];
      int nStart = GetLength() - n;
      strncpy(g_pStrWork, &m_pStr[nStart], n);
      g_pStrWork[n] = '\0';
    }
    else
    {
      g_pStrWork = new char[1];
      g_pStrWork[0] = '\0';
    }
    return g_pStrWork;
  }


  char& operator *() { return *m_pStr; }
  char& operator *() const { return *const_cast<Str*>(this)->m_pStr; }
  operator void*() { return m_pStr; }
  operator char*() { return m_pStr; }
  operator const char*(){ return reinterpret_cast<const char*>(m_pStr); }
  operator unsigned char*() { return reinterpret_cast<unsigned char*>(m_pStr); }
  operator const unsigned char*() { return reinterpret_cast<const unsigned char*>(m_pStr); }
  Str& operator =(const Str& rhs)
  {
    if (&rhs != this)
    {
      delete[] m_pStr;
      m_pStr = __StrDup(rhs.m_pStr);
    }
    return *this;
  }
  
  Str& operator =(const char* pStr)
  {
    if (m_pStr != pStr)
    {
      delete[] m_pStr;
      m_pStr = __StrDup(pStr);
    }
    return *this;
  }

  Str& operator +=(const char *pStr)
  {
    if (pStr)
    {
      if (m_pStr)
      {
        char *p = new char[strlen(m_pStr) + strlen(pStr) + 1];
        strcpy(p, m_pStr);
        strcat(p, pStr);
        delete m_pStr;
        m_pStr = p;
      }
      else
      {
        m_pStr = __StrDup(pStr);
      }
    }
    return *this;
  }
  
  Str& operator +=(const char c)
  {
    return operator+=(&c);
  }


  bool operator ==(const Str& rhs) const { return (m_bIgnoreCase) ? stricmp(m_pStr, rhs.m_pStr) == 0 : strcmp(m_pStr, rhs.m_pStr) == 0; }
  bool operator ==(char* pStr) const { return (m_bIgnoreCase) ? stricmp(m_pStr, pStr) == 0 : strcmp(m_pStr, pStr) == 0; }
  bool operator ==(const char* pStr) const { return (m_bIgnoreCase) ? stricmp(m_pStr, pStr) == 0 : strcmp(m_pStr, pStr) == 0; }
  bool operator !=(Str& rhs) const { return (m_bIgnoreCase) ? stricmp(m_pStr, rhs.m_pStr) != 0 : strcmp(m_pStr, rhs.m_pStr) != 0; }
  bool operator !=(char* pStr) const { return (m_bIgnoreCase) ? stricmp(m_pStr, pStr) != 0 : strcmp(m_pStr, pStr) != 0; }
  bool operator !=(const char* pStr) const { return (m_bIgnoreCase) ? stricmp(m_pStr, pStr) != 0 : strcmp(m_pStr, pStr) != 0; }
  char& operator [](int nIndex) { return m_pStr[nIndex]; }
  char& operator [](int nIndex) const { return m_pStr[nIndex]; }
     
};



#endif