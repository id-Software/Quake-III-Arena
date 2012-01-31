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
#include "stdafx.h"
#include <stdlib.h>

#define Reg1 register
int match(char *mask, char *name)
{
	Reg1	unsigned char	*m = (unsigned char *)mask, *n = (unsigned char *)name;
	char	*ma = mask, *na = name;
	int	wild = 0, q = 0;

	while (1)
	    {
		if (*m == '*')
		   {
			while (*m == '*')
				m++;
			wild = 1;
			ma = (char *)m;
			na = (char *)n;
		    }

		if (!*m)
		    {
	  		if (!*n)
				return 0;
	  		for (m--; (m > (unsigned char *)mask) && (*m == '?'); m--)
				;
			if ((*m == '*') && (m > (unsigned char *)mask) &&
			    (m[-1] != '\\'))
				return 0;
			if (!wild) 
				return 1;
			m = (unsigned char *)ma;
			n = (unsigned char *)++na;
		    }
		else if (!*n)
		    {
			while(*m == '*')
				m++;
			return (*m != 0);
		    }
		if ((*m == '\\') && ((m[1] == '*') || (m[1] == '?')))
		    {
			m++;
			q = 1;
		    }
		else
			q = 0;

		if ((tolower(*m) != tolower(*n)) && ((*m != '?') || q))
		    {
			if (!wild)
				return 1;
			m = (unsigned char *)ma;
			n = (unsigned char *)++na;
		    }
		else
		    {
			if (*m)
				m++;
			if (*n)
				n++;
		    }
	    }
}

