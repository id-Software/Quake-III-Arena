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
// $Revision: 1.1.1.4 $
// $Author: ttimo $
// $Date: 2000/01/18 00:17:10 $
// $Log: EpairsWrapper.h,v $
// Revision 1.1.1.4  2000/01/18 00:17:10  ttimo
// merging in for RC
//
// Revision 1.3  2000/01/17 23:53:41  TBesset
// ready for merge in sourceforge (RC candidate)
//
// Revision 1.1.1.3  1999/12/29 18:31:25  TBesset
// Q3Radiant public version
//
// DESCRIPTION:
// virtual class to allow plugin operations on entities epairs
// this is the internal header for our implementation

#ifndef _EPAIRSWRAPPER_H_
#define _EPAIRSWRAPPER_H_

class CEpairsWrapper : public IEpair
{
public:
	int refCount;
	entity_t *m_pEnt;
	virtual void IncRef() { refCount++; }
	virtual void DecRef() { refCount--; if (refCount <= 0) delete this; }
	CEpairsWrapper() { refCount = 0; }
	CEpairsWrapper( entity_t* ep ) { refCount = 0; m_pEnt = ep; }
	virtual ~CEpairsWrapper() { }
	virtual void GetVectorForKey( char* key, vec3_t vec );
	virtual float FloatForKey( char *key );
	virtual char* ValueForKey( char *key );
	virtual void SetKeyValue( char *key, char *value );
	virtual void GetEntityOrigin( vec3_t vec );
	virtual void CalculateRotatedBounds( vec3_t mins, vec3_t maxs );
};

#endif