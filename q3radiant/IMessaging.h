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
// $Log: IMessaging.h,v $
// Revision 1.1.2.1  2000/02/04 22:59:34  ttimo
// messaging API preview
//
//
// DESCRIPTION:
// interface for all-purpose messaging in Radiant

#ifndef __IMESSAGING_H_
#define __IMESSAGING_H_

// this one can be hooked in the GL window procs for customizing GUI through plugins
class IWindowListener
{
public:
	// Increment the number of references to this object
	virtual void IncRef () = 0;
	// Decrement the reference count
	virtual void DecRef () = 0;
	// since Radiant is MFC we don't use a WNDPROC, we wrap the MFC handlers
	// the handler is called first, if returns false Radiant continues processing
	//++timo maybe add more later ? OnKeyUp and OnKeyDown for instance
	//++timo TODO: add handlers everywhere
	virtual bool OnLButtonDown(UINT nFlags, int x, int y) = 0;
	virtual bool OnMButtonDown(UINT nFlags, int x, int y) = 0;
	virtual bool OnRButtonDown(UINT nFlags, int x, int y) = 0;
	virtual bool OnLButtonUp(UINT nFlags, int x, int y) = 0;
	virtual bool OnMButtonUp(UINT nFlags, int x, int y) = 0;
	virtual bool OnRButtonUp(UINT nFlags, int x, int y) = 0;
	virtual bool OnMouseMove(UINT nFlags, int x, int y) = 0;
};

// various Radiant messages --------
// this one holds the total number of supported messages (this is used to allocate structs)
#define RADIANT_MSGCOUNT 3
// they start with a 0, can be indexed in an array
// something was selected / deselected
#define RADIANT_SELECTION 0
// a brush face was selected / deselected
#define RADIANT_SFACE     1
// current texture / shader changed
#define RADIANT_TEXTURE   2

// this one can be used to listen for Radiant-specific events
class IListener
{
public:
	// Increment the number of references to this object
	virtual void IncRef () = 0;
	// Decrement the reference count
	virtual void DecRef () = 0;
	// message is one of the RADIANT_* consts
	virtual void DispatchRadiantMsg( int Msg ) = 0;
};

// this one is provided by Radiant, it's a wrapper for some usefull functions
class IXYWndWrapper
{
public:
	virtual void SnapToGrid( int x1, int y1, vec3_t pt ) = 0;
};

// define a GUID for this interface so plugins can access and reference it
// {41FD005C-D36B-11d3-A3E9-0004AC96D4C3}
static const GUID QERMessaging_GUID = 
{ 0x41fd005c, 0xd36b, 0x11d3, { 0xa3, 0xe9, 0x0, 0x4, 0xac, 0x96, 0xd4, 0xc3 } };

// will hook the given IWindowListener to the XY window and increment the ref count
//++timo TODO: add hooking in the CAM view and Z view
typedef void (WINAPI* PFN_QERAPP_HOOKWINDOW)	(IWindowListener *);
// will unhook the given IWindowListener
typedef void (WINAPI* PFN_QERAPP_UNHOOKWINDOW)	(IWindowListener *);
// to retrieve the IXYWndWrapper
typedef IXYWndWrapper* (WINAPI* PFN_QERAPP_GETXYWNDWRAPPER)	();

// will hook a given listener into Radiant listening for the given message and increment ref count
// call several times to listen for several messages
typedef void (WINAPI* PFN_QERAPP_HOOKLISTENER)	(IListener *, int Msg);
// will unhook the listener and return the number of messages the given listener was removed from
typedef int  (WINAPI* PFN_QERAPP_UNHOOKLISTENER)(IListener *);

struct _QERMessagingTable
{
	int m_nSize;
	PFN_QERAPP_HOOKWINDOW		m_pfnHookWindow;
	PFN_QERAPP_UNHOOKWINDOW		m_pfnUnHookWindow;
	PFN_QERAPP_GETXYWNDWRAPPER	m_pfnGetXYWndWrapper;
	PFN_QERAPP_HOOKLISTENER		m_pfnHookListener;
	PFN_QERAPP_UNHOOKLISTENER	m_pfnUnHookListener;
};

#endif