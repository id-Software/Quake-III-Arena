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
// $Log: IPluginEntities.h,v $
// Revision 1.1.1.4  2000/01/18 00:17:12  ttimo
// merging in for RC
//
// Revision 1.3  2000/01/17 23:53:42  TBesset
// ready for merge in sourceforge (RC candidate)
//
// Revision 1.2  2000/01/07 16:40:10  TBesset
// merged from BSP frontend
//
// Revision 1.1.1.3  1999/12/29 18:31:27  TBesset
// Q3Radiant public version
//
// Revision 1.1.1.1.2.1  1999/12/29 21:39:35  TBesset
// updated to update3 from Robert
//
// Revision 1.1.1.3  1999/12/29 18:31:27  TBesset
// Q3Radiant public version
// Revision 1.1.1.3  1999/12/29 18:31:27  TBesset
// Q3Radiant public version
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
// _QERPluginEntitiesTable is used by plugins for various plugin entities commands
// _QERPlugEntitiesFactory is a set of commands Radiant uses to instanciate plugin entities
// next are basics for plugin entities ( interfaces )

#ifndef __IPLUGINENTITIES_H_
#define __IPLUGINENTITIES_H_

// define a GUID for this interface so plugins can access and reference it
// {9613F500-8C7C-11d3-8EF3-C9EB56B6B7BE}
static const GUID QERPluginEntitiesTable_GUID = 
{ 0x9613f500, 0x8c7c, 0x11d3, { 0x8e, 0xf3, 0xc9, 0xeb, 0x56, 0xb6, 0xb7, 0xbe } };

typedef int (WINAPI* PFN_ECLASSSCANDIR)	( char*, HMODULE );

struct _QERPluginEntitiesTable
{
	int m_nSize;
	PFN_ECLASSSCANDIR	m_pfnEClassScanDir;
};

// plugin entities
class IPluginEntity
{
public:
	// Increment the number of references to this object
	virtual void IncRef () = 0;
	// Decrement the reference count
	virtual void DecRef () = 0;
	// getting bounds of the brush used to select/move the object
	virtual void GetBounds( vec3_t mins, vec3_t maxs ) = 0;
	// rendering the object in the camera view
	virtual void CamRender() = 0;
};

// {DBC6B300-8E4B-11d3-8EF3-BA5E57D56399}
static const GUID QERPlugEntitiesFactory_GUID = 
{ 0xdbc6b300, 0x8e4b, 0x11d3, { 0x8e, 0xf3, 0xba, 0x5e, 0x57, 0xd5, 0x63, 0x99 } };

typedef IPluginEntity * (WINAPI* PFN_CREATEENTITY) ( eclass_t *, IEpair * );

struct _QERPlugEntitiesFactory
{
	int m_nSize;
	PFN_CREATEENTITY	m_pfnCreateEntity;
};

#endif