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
// $Date: 2000/01/18 00:17:12 $
// $Log: IEpairs.cpp,v $
// Revision 1.1.1.4  2000/01/18 00:17:12  ttimo
// merging in for RC
//
// Revision 1.3  2000/01/17 23:53:41  TBesset
// ready for merge in sourceforge (RC candidate)
//
// Revision 1.2  2000/01/07 16:40:10  TBesset
// merged from BSP frontend
// Revision 1.1.1.3  1999/12/29 18:31:26  TBesset
// Q3Radiant public version
//
//
// Revision 1.2  1999/11/22 17:46:45  Timo & Christine
// merged EARadiant into the main tree
// bug fixes for Q3Plugin / EAPlugin
// export for Robert
//
// Revision 1.1.2.1  1999/11/03 20:37:59  Timo & Christine
// MEAN plugin for Q3Radiant, alpha version
//
//
// DESCRIPTION:
// virtual class to allow plugin operations on entities epairs
//

#include "stdafx.h"

void CEpairsWrapper::GetVectorForKey( char* key, vec3_t vec )
{
	::GetVectorForKey( m_pEnt, key, vec );
}

float CEpairsWrapper::FloatForKey( char *key )
{
	return ::FloatForKey( m_pEnt, key );
}

char* CEpairsWrapper::ValueForKey( char *key )
{
	return ::ValueForKey( m_pEnt, key );
}

void CEpairsWrapper::SetKeyValue( char *key, char *value )
{
	::SetKeyValue( m_pEnt, key, value );
}

void CEpairsWrapper::GetEntityOrigin( vec3_t vec )
{
	VectorCopy( m_pEnt->origin, vec );
}

// taken from Ritual's version of Q3Radiant ( Entity_CalculateRotatedBounds )
void CEpairsWrapper::CalculateRotatedBounds( vec3_t mins, vec3_t maxs )
{
	entity_t *ent = m_pEnt;
	int i;
	float angle;
	vec3_t angles;
	vec3_t forward,right,up;
	vec3_t rotmins, rotmaxs;
	float trans[3][3];
	qboolean changed;
	char tempangles[ 128 ]; 
	
	memset( angles, 0, sizeof(vec3_t) );
	::GetVectorForKey (ent, "angles", angles);
	
	changed = false;
	while ( angles[0] < 0 )
	{
		changed = true;
		angles[0] += 360;
	}
	while ( angles[0] > 359 )
	{
		changed = true;
		angles[0] -= 360;
	}
	while ( angles[1] < 0 )
	{
		changed = true;
		angles[1] += 360;
	}
	while ( angles[1] > 359 )
	{
		changed = true;
		angles[1] -= 360;
	}
	while ( angles[2] < 0 )
	{
		changed = true;
		angles[2] += 360;
	}
	while ( angles[2] > 359 )
	{
		changed = true;
		angles[2] -= 360;
	}
	
	if ( changed )
	{
		sprintf( tempangles, "%d %d %d", (int)angles[0], (int)angles[1], (int)angles[2] );
		::SetKeyValue ( ent, "angles", tempangles );
	}
	
	
	angle = ::FloatForKey (ent, "angle");
	if ( fabs(angle) > 2 )
	{
		angles[1] = angle;
	}
	else if (angle == -1)
	{
		angles[0] = -90;
	}
	else if (angle == -2)
	{
		angles[0] = 90;
	}
	::AngleVectors( angles, forward, right, up );
	for (i=0 ; i<3 ; i++)
	   {
		trans[i][0] =  forward[i];
		trans[i][1] =  -right[i];
		trans[i][2] =  up[i];
	   }
	ClearBounds( rotmins, rotmaxs );
	for ( i = 0; i < 8; i++ )
   	{
		int j;
		vec3_t   tmp, rottemp;
		
		if ( i & 1 )
			tmp[0] = mins[0];
		else
			tmp[0] = maxs[0];
		
		if ( i & 2 )
			tmp[1] = mins[1];
		else
			tmp[1] = maxs[1];
		
		if ( i & 4 )
			tmp[2] = mins[2];
		else
			tmp[2] = maxs[2];
		
		for (j=0; j<3 ; j++)
		{
			rottemp[j] =  DotProduct( tmp, trans[j] );
		}
		AddPointToBounds( rottemp, rotmins, rotmaxs );
	}
	VectorCopy( rotmins, mins );
	VectorCopy( rotmaxs, maxs );
}
