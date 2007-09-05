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
//
#include "ui_local.h"

#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"	
#define ART_FIGHT0			"menu/art/load_0"
#define ART_FIGHT1			"menu/art/load_1"
#define ART_FRAMEL			"menu/art/frame2_l"
#define ART_FRAMER			"menu/art/frame1_r"

#define MAX_MODS			64
#define NAMEBUFSIZE			( MAX_MODS * 48 )
#define GAMEBUFSIZE			( MAX_MODS * 16 )

#define ID_BACK				10
#define ID_GO				11
#define ID_LIST				12


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;

	menulist_s		list;

	menubitmap_s	back;
	menubitmap_s	go;

	char			description[NAMEBUFSIZE];
	char			fs_game[GAMEBUFSIZE];

	char			*descriptionPtr;
	char			*fs_gamePtr;

	char			*descriptionList[MAX_MODS];
	char			*fs_gameList[MAX_MODS];
} mods_t;

static mods_t	s_mods;


/*
===============
UI_Mods_MenuEvent
===============
*/
static void UI_Mods_MenuEvent( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch ( ((menucommon_s*)ptr)->id ) {
	case ID_GO:
		trap_Cvar_Set( "fs_game", s_mods.fs_gameList[s_mods.list.curvalue] );
		trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
		UI_PopMenu();
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


/*
===============
UI_Mods_ParseInfos
===============
*/
static void UI_Mods_ParseInfos( char *modDir, char *modDesc ) {
	s_mods.fs_gameList[s_mods.list.numitems] = s_mods.fs_gamePtr;
	Q_strncpyz( s_mods.fs_gamePtr, modDir, 16 );

	s_mods.descriptionList[s_mods.list.numitems] = s_mods.descriptionPtr;
	Q_strncpyz( s_mods.descriptionPtr, modDesc, 48 );

	s_mods.list.itemnames[s_mods.list.numitems] = s_mods.descriptionPtr;
	s_mods.descriptionPtr += strlen( s_mods.descriptionPtr ) + 1;
	s_mods.fs_gamePtr += strlen( s_mods.fs_gamePtr ) + 1;
	s_mods.list.numitems++;
}


/*
===============
UI_Mods_LoadMods
===============
*/
static void UI_Mods_LoadMods( void ) {
	int		numdirs;
	char	dirlist[2048];
	char	*dirptr;
  char  *descptr;
	int		i;
	int		dirlen;

	s_mods.list.itemnames = (const char **)s_mods.descriptionList;
	s_mods.descriptionPtr = s_mods.description;
	s_mods.fs_gamePtr = s_mods.fs_game;

	// always start off with baseq3
	s_mods.list.numitems = 1;
	s_mods.list.itemnames[0] = s_mods.descriptionList[0] = "Quake III Arena";
	s_mods.fs_gameList[0] = "";

	numdirs = trap_FS_GetFileList( "$modlist", "", dirlist, sizeof(dirlist) );
	dirptr  = dirlist;
	for( i = 0; i < numdirs; i++ ) {
		dirlen = strlen( dirptr ) + 1;
    descptr = dirptr + dirlen;
  	UI_Mods_ParseInfos( dirptr, descptr);
    dirptr += dirlen + strlen(descptr) + 1;
	}

	trap_Print( va( "%i mods parsed\n", s_mods.list.numitems ) );
	if (s_mods.list.numitems > MAX_MODS) {
		s_mods.list.numitems = MAX_MODS;
	}
}


/*
===============
UI_Mods_MenuInit
===============
*/
static void UI_Mods_MenuInit( void ) {
	UI_ModsMenu_Cache();

	memset( &s_mods, 0 ,sizeof(mods_t) );
	s_mods.menu.wrapAround = qtrue;
	s_mods.menu.fullscreen = qtrue;

	s_mods.banner.generic.type		= MTYPE_BTEXT;
	s_mods.banner.generic.x			= 320;
	s_mods.banner.generic.y			= 16;
	s_mods.banner.string			= "MODS";
	s_mods.banner.color				= color_white;
	s_mods.banner.style				= UI_CENTER;

	s_mods.framel.generic.type		= MTYPE_BITMAP;
	s_mods.framel.generic.name		= ART_FRAMEL;
	s_mods.framel.generic.flags		= QMF_INACTIVE;
	s_mods.framel.generic.x			= 0;  
	s_mods.framel.generic.y			= 78;
	s_mods.framel.width				= 256;
	s_mods.framel.height			= 329;

	s_mods.framer.generic.type		= MTYPE_BITMAP;
	s_mods.framer.generic.name		= ART_FRAMER;
	s_mods.framer.generic.flags		= QMF_INACTIVE;
	s_mods.framer.generic.x			= 376;
	s_mods.framer.generic.y			= 76;
	s_mods.framer.width				= 256;
	s_mods.framer.height			= 334;

	s_mods.back.generic.type		= MTYPE_BITMAP;
	s_mods.back.generic.name		= ART_BACK0;
	s_mods.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_mods.back.generic.id			= ID_BACK;
	s_mods.back.generic.callback	= UI_Mods_MenuEvent;
	s_mods.back.generic.x			= 0;
	s_mods.back.generic.y			= 480-64;
	s_mods.back.width				= 128;
	s_mods.back.height				= 64;
	s_mods.back.focuspic			= ART_BACK1;

	s_mods.go.generic.type			= MTYPE_BITMAP;
	s_mods.go.generic.name			= ART_FIGHT0;
	s_mods.go.generic.flags			= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_mods.go.generic.id			= ID_GO;
	s_mods.go.generic.callback		= UI_Mods_MenuEvent;
	s_mods.go.generic.x				= 640;
	s_mods.go.generic.y				= 480-64;
	s_mods.go.width					= 128;
	s_mods.go.height				= 64;
	s_mods.go.focuspic				= ART_FIGHT1;

	// scan for mods
	s_mods.list.generic.type		= MTYPE_SCROLLLIST;
	s_mods.list.generic.flags		= QMF_PULSEIFFOCUS|QMF_CENTER_JUSTIFY;
	s_mods.list.generic.callback	= UI_Mods_MenuEvent;
	s_mods.list.generic.id			= ID_LIST;
	s_mods.list.generic.x			= 320;
	s_mods.list.generic.y			= 130;
	s_mods.list.width				= 48;
	s_mods.list.height				= 14;

	UI_Mods_LoadMods();

	Menu_AddItem( &s_mods.menu, &s_mods.banner );
	Menu_AddItem( &s_mods.menu, &s_mods.framel );
	Menu_AddItem( &s_mods.menu, &s_mods.framer );
	Menu_AddItem( &s_mods.menu, &s_mods.list );
	Menu_AddItem( &s_mods.menu, &s_mods.back );
	Menu_AddItem( &s_mods.menu, &s_mods.go );
}

/*
=================
UI_Mods_Cache
=================
*/
void UI_ModsMenu_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FIGHT0 );
	trap_R_RegisterShaderNoMip( ART_FIGHT1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
}


/*
===============
UI_ModsMenu
===============
*/
void UI_ModsMenu( void ) {
	UI_Mods_MenuInit();
	UI_PushMenu( &s_mods.menu );
}
