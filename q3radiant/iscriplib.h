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
// $Log: iscriplib.h,v $
// Revision 1.1.1.4  2000/01/18 00:17:12  ttimo
// merging in for RC
//
// Revision 1.3  2000/01/17 23:53:44  TBesset
// ready for merge in sourceforge (RC candidate)
//
// Revision 1.2  2000/01/07 16:40:13  TBesset
// merged from BSP frontend
//
// Revision 1.1.1.3  1999/12/29 18:31:27  TBesset
// Q3Radiant public version
//
// Revision 1.1.1.1.2.1  1999/12/29 21:39:45  TBesset
// updated to update3 from Robert
//
// Revision 1.1.1.3  1999/12/29 18:31:27  TBesset
// Q3Radiant public version
// Revision 1.1.1.3  1999/12/29 18:31:27  TBesset
// Q3Radiant public version
//
// Revision 1.2  1999/11/22 17:46:48  Timo & Christine
// merged EARadiant into the main tree
// bug fixes for Q3Plugin / EAPlugin
// export for Robert
//
// Revision 1.1.2.2  1999/11/14 16:26:14  Timo & Christine
// first beta of the ritualmap surface plugin
//
// Revision 1.1.2.1  1999/10/08 16:28:15  Timo & Christine
// started plugin extensions for EA features support in Q3Radiant
// MEAN files plugin, and Surface Properties plugin
//
//
// DESCRIPTION:
// all purpose scriplib interface for Q3Radiant plugins (cf. parse.h)
//

#ifndef __ISCRIPLIB_H_
#define __ISCRIPLIB_H_

// {4B115280-95FC-11d3-8EF3-0000E8E8657B}
static const GUID QERScripLibTable_GUID = 
{ 0x4b115280, 0x95fc, 0x11d3, { 0x8e, 0xf3, 0x0, 0x0, 0xe8, 0xe8, 0x65, 0x7b } };

typedef qboolean	(WINAPI* PFN_GETTOKEN)		(qboolean crossline);
typedef void		(WINAPI* PFN_UNGETTOKEN)	();
// only used to retrieve &token
typedef char*		(WINAPI* PFN_TOKEN) ();

struct _QERScripLibTable
{
	float m_fVersion;
	int m_nSize;
	PFN_GETTOKEN	m_pfnGetToken;
	PFN_UNGETTOKEN	m_pfnUnGetToken;
	PFN_TOKEN		m_pfnToken;
};

#endif