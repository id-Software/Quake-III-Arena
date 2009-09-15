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
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#ifndef MACOS_X
#error This file is for Mac OS X only. You probably should not compile it.
#endif

// Please note that this file is just some Mac-specific bits. Most of the
//  Mac OS X code is shared with other Unix platforms in sys_unix.c ...

#import <Cocoa/Cocoa.h>

void Sys_Cocoa_MsgBox( const char *text )
{
	NSRunInformationalAlertPanel(@"ioquake3", 
	                             [NSString stringWithUTF8String:text],
	                             @"OK", nil, nil);
}

// end of sys_cocoa.m ...

