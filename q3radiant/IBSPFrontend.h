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
// $Revision: 1.3 $
// $Author: ttimo $
// $Date: 2000/01/18 00:43:59 $
// $Log: IBSPFrontend.h,v $
// Revision 1.3  2000/01/18 00:43:59  ttimo
// RC
//
// Revision 1.2  2000/01/13 00:46:41  ttimo
// Merged in patches in / out
//
// Revision 1.1.1.1.2.1  2000/01/13 00:15:41  ttimo
// beta of patches in / out - tested with GenSurf
//
// Revision 1.1.1.1  2000/01/07 17:17:27  ttimo
// initial import of Q3Radiant module
//
// Revision 1.2  2000/01/07 16:40:09  TBesset
// merged from BSP frontend
//
// Revision 1.1.2.4  2000/01/07 00:16:26  TBesset
// frontend ready for public version
//
// Revision 1.1.2.3  2000/01/04 00:26:58  TBesset
// added a LoadPointFile in BSP frontend
//
// Revision 1.1.2.2  1999/12/31 19:43:20  TBesset
// no message
//
// Revision 1.1.2.1  1999/12/29 16:49:39  TBesset
// adding IBSPFrontend to the repository
//
//
// DESCRIPTION:
// interface for BSP frontends plugins
//

// DONE:	- change BSP menu to Q3Build menu ?
// DONE:    - detect when Q3Build dies ?
// DELAYED: - hotkeys !
// SUCCESS: - try again getting feedback from Q3Build

#ifndef __IBSPFRONTEND_H_
#define __IBSPFRONTEND_H_

// define a GUID for this interface so plugins can access and reference it
// {8ED6A480-BA5E-11d3-A3E3-0004AC96D4C3}
static const GUID QERPlugBSPFrontendTable_GUID = 
{ 0x8ed6a480, 0xba5e, 0x11d3, { 0xa3, 0xe3, 0x0, 0x4, 0xac, 0x96, 0xd4, 0xc3 } };

// ask the plugin about the items to show up in the BSP menu
typedef char * (WINAPI* PFN_GETBSPMENU)	();
// dispatch a BSP menu command
typedef void (WINAPI* PFN_DISPATCHBSPCOMMAND) (char *);

struct _QERPlugBSPFrontendTable
{
	int m_nSize;
	PFN_GETBSPMENU			m_pfnGetBSPMenu;
	PFN_DISPATCHBSPCOMMAND	m_pfnDispatchBSPCommand;
};

// interface provided by Radiant to the plugin
// {A2CCF366-BA60-11d3-A3E3-0004AC96D4C3}
static const GUID QERAppBSPFrontendTable_GUID = 
{ 0xa2ccf366, 0xba60, 0x11d3, { 0xa3, 0xe3, 0x0, 0x4, 0xac, 0x96, 0xd4, 0xc3 } };

typedef char * (WINAPI* PFN_GETMAPNAME) ();
typedef void (WINAPI* PFN_LOADPOINTFILE) ();

struct _QERAppBSPFrontendTable
{
	int m_nSize;
	PFN_GETMAPNAME		m_pfnGetMapName;
	PFN_LOADPOINTFILE	m_pfnLoadPointFile;
};

#endif