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
//
#include "ui_local.h"

/*********************************************************************************
	SPECIFY SERVER
*********************************************************************************/

#define SPECIFYSERVER_FRAMEL	"menu/art/frame2_l"
#define SPECIFYSERVER_FRAMER	"menu/art/frame1_r"
#define SPECIFYSERVER_BACK0		"menu/art/back_0"
#define SPECIFYSERVER_BACK1		"menu/art/back_1"
#define SPECIFYSERVER_FIGHT0	"menu/art/fight_0"
#define SPECIFYSERVER_FIGHT1	"menu/art/fight_1"

#define ID_SPECIFYSERVERBACK	102
#define ID_SPECIFYSERVERGO		103

static char* specifyserver_artlist[] =
{
	SPECIFYSERVER_FRAMEL,
	SPECIFYSERVER_FRAMER,
	SPECIFYSERVER_BACK0,	
	SPECIFYSERVER_BACK1,	
	SPECIFYSERVER_FIGHT0,
	SPECIFYSERVER_FIGHT1,
	NULL
};

typedef struct
{
	menuframework_s	menu;
	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;
	menufield_s		domain;
	menufield_s		port;
	menubitmap_s	go;
	menubitmap_s	back;
} specifyserver_t;

static specifyserver_t	s_specifyserver;

/*
=================
SpecifyServer_Event
=================
*/
static void SpecifyServer_Event( void* ptr, int event )
{
	char	buff[256];

	switch (((menucommon_s*)ptr)->id)
	{
		case ID_SPECIFYSERVERGO:
			if (event != QM_ACTIVATED)
				break;

			if (s_specifyserver.domain.field.buffer[0])
			{
				strcpy(buff,s_specifyserver.domain.field.buffer);
				if (s_specifyserver.port.field.buffer[0])
					Com_sprintf( buff+strlen(buff), 128, ":%s", s_specifyserver.port.field.buffer );

				trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", buff ) );
			}
			break;

		case ID_SPECIFYSERVERBACK:
			if (event != QM_ACTIVATED)
				break;

			UI_PopMenu();
			break;
	}
}

/*
=================
SpecifyServer_MenuInit
=================
*/
void SpecifyServer_MenuInit( void )
{
	// zero set all our globals
	memset( &s_specifyserver, 0 ,sizeof(specifyserver_t) );

	SpecifyServer_Cache();

	s_specifyserver.menu.wrapAround = qtrue;
	s_specifyserver.menu.fullscreen = qtrue;

	s_specifyserver.banner.generic.type	 = MTYPE_BTEXT;
	s_specifyserver.banner.generic.x     = 320;
	s_specifyserver.banner.generic.y     = 16;
	s_specifyserver.banner.string		 = "SPECIFY SERVER";
	s_specifyserver.banner.color  		 = color_white;
	s_specifyserver.banner.style  		 = UI_CENTER;

	s_specifyserver.framel.generic.type  = MTYPE_BITMAP;
	s_specifyserver.framel.generic.name  = SPECIFYSERVER_FRAMEL;
	s_specifyserver.framel.generic.flags = QMF_INACTIVE;
	s_specifyserver.framel.generic.x	 = 0;  
	s_specifyserver.framel.generic.y	 = 78;
	s_specifyserver.framel.width  	     = 256;
	s_specifyserver.framel.height  	     = 329;

	s_specifyserver.framer.generic.type  = MTYPE_BITMAP;
	s_specifyserver.framer.generic.name  = SPECIFYSERVER_FRAMER;
	s_specifyserver.framer.generic.flags = QMF_INACTIVE;
	s_specifyserver.framer.generic.x	 = 376;
	s_specifyserver.framer.generic.y	 = 76;
	s_specifyserver.framer.width  	     = 256;
	s_specifyserver.framer.height  	     = 334;

	s_specifyserver.domain.generic.type       = MTYPE_FIELD;
	s_specifyserver.domain.generic.name       = "Address:";
	s_specifyserver.domain.generic.flags      = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_specifyserver.domain.generic.x	      = 206;
	s_specifyserver.domain.generic.y	      = 220;
	s_specifyserver.domain.field.widthInChars = 38;
	s_specifyserver.domain.field.maxchars     = 80;

	s_specifyserver.port.generic.type       = MTYPE_FIELD;
	s_specifyserver.port.generic.name	    = "Port:";
	s_specifyserver.port.generic.flags	    = QMF_PULSEIFFOCUS|QMF_SMALLFONT|QMF_NUMBERSONLY;
	s_specifyserver.port.generic.x	        = 206;
	s_specifyserver.port.generic.y	        = 250;
	s_specifyserver.port.field.widthInChars = 6;
	s_specifyserver.port.field.maxchars     = 5;

	s_specifyserver.go.generic.type	    = MTYPE_BITMAP;
	s_specifyserver.go.generic.name     = SPECIFYSERVER_FIGHT0;
	s_specifyserver.go.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_specifyserver.go.generic.callback = SpecifyServer_Event;
	s_specifyserver.go.generic.id	    = ID_SPECIFYSERVERGO;
	s_specifyserver.go.generic.x		= 640;
	s_specifyserver.go.generic.y		= 480-64;
	s_specifyserver.go.width  		    = 128;
	s_specifyserver.go.height  		    = 64;
	s_specifyserver.go.focuspic         = SPECIFYSERVER_FIGHT1;

	s_specifyserver.back.generic.type	  = MTYPE_BITMAP;
	s_specifyserver.back.generic.name     = SPECIFYSERVER_BACK0;
	s_specifyserver.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_specifyserver.back.generic.callback = SpecifyServer_Event;
	s_specifyserver.back.generic.id	      = ID_SPECIFYSERVERBACK;
	s_specifyserver.back.generic.x		  = 0;
	s_specifyserver.back.generic.y		  = 480-64;
	s_specifyserver.back.width  		  = 128;
	s_specifyserver.back.height  		  = 64;
	s_specifyserver.back.focuspic         = SPECIFYSERVER_BACK1;

	Menu_AddItem( &s_specifyserver.menu, &s_specifyserver.banner );
	Menu_AddItem( &s_specifyserver.menu, &s_specifyserver.framel );
	Menu_AddItem( &s_specifyserver.menu, &s_specifyserver.framer );
	Menu_AddItem( &s_specifyserver.menu, &s_specifyserver.domain );
	Menu_AddItem( &s_specifyserver.menu, &s_specifyserver.port );
	Menu_AddItem( &s_specifyserver.menu, &s_specifyserver.go );
	Menu_AddItem( &s_specifyserver.menu, &s_specifyserver.back );

	Com_sprintf( s_specifyserver.port.field.buffer, 6, "%i", 27960 );
}

/*
=================
SpecifyServer_Cache
=================
*/
void SpecifyServer_Cache( void )
{
	int	i;

	// touch all our pics
	for (i=0; ;i++)
	{
		if (!specifyserver_artlist[i])
			break;
		trap_R_RegisterShaderNoMip(specifyserver_artlist[i]);
	}
}

/*
=================
UI_SpecifyServerMenu
=================
*/
void UI_SpecifyServerMenu( void )
{
	SpecifyServer_MenuInit();
	UI_PushMenu( &s_specifyserver.menu );
}

