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
// entity.h

#define DlgXBorder 5
#define DlgYBorder 5


#define EntList		0
#define EntComment	1
#define EntCheck1	2
#define EntCheck2	3
#define EntCheck3	4
#define EntCheck4	5
#define EntCheck5	6
#define EntCheck6	7
#define EntCheck7	8
#define EntCheck8	9
#define EntCheck9	10
#define EntCheck10	11
#define EntCheck11	12
#define EntCheck12	13
#define EntProps	14
#define	EntDir0		15
#define	EntDir45	16
#define	EntDir90	17
#define	EntDir135	18
#define	EntDir180	19
#define	EntDir225	20
#define	EntDir270	21
#define	EntDir315	22
#define	EntDirUp	23
#define	EntDirDown	24
#define EntDelProp	25
#define	EntKeyLabel	26
#define	EntKeyField	27
#define	EntValueLabel 28
#define	EntValueField 29
#define EntColor      30
#define EntAssignSounds 31
#define EntAssignModels 32
#define EntTab 33

#define EntLast		34

extern HWND hwndEnt[EntLast];

extern int rgIds[EntLast];

