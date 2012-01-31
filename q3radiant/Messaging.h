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
//-----------------------------------------------------------------------------
//
// $LogFile$
// $Revision: 1.1.2.1 $
// $Author: ttimo $
// $Date: 2000/02/04 22:59:34 $
// $Log: Messaging.h,v $
// Revision 1.1.2.1  2000/02/04 22:59:34  ttimo
// messaging API preview
//
//
// DESCRIPTION:
// headers for internal classes used in Messaging.cpp
// 

#ifndef __MESSAGING_H_
#define __MESSAGING_H_

class CXYWndWrapper : public IXYWndWrapper
{
public:
	void SnapToGrid( int x1, int y1, vec3_t pt );
};

#endif