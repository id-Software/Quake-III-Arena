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
/*
=======================================================================

RESET MENU

=======================================================================
*/

#include "ui_local.h"


#define ART_FRAME					"menu/art/cut_frame"

#define ID_NO		100
#define ID_YES		101

typedef struct
{
	menuframework_s menu;
	menutext_s		no;
	menutext_s		yes;
	int				slashX;
} resetMenu_t;

static resetMenu_t	s_reset;


/*
=================
Reset_MenuEvent
=================
*/
void Reset_MenuEvent(void* ptr, int event) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	UI_PopMenu();

	if( ((menucommon_s*)ptr)->id == ID_NO ) {
		return;
	}

	// reset the game, pop the level menu and restart it so it updates
	UI_NewGame();
	trap_Cvar_SetValue( "ui_spSelection", 0 );
	UI_PopMenu();
	UI_SPLevelMenu();
}


/*
=================
Reset_MenuKey
=================
*/
static sfxHandle_t Reset_MenuKey( int key ) {
	switch ( key ) {
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
		key = K_TAB;
		break;

	case 'n':
	case 'N':
		Reset_MenuEvent( &s_reset.no, QM_ACTIVATED );
		break;

	case 'y':
	case 'Y':
		Reset_MenuEvent( &s_reset.yes, QM_ACTIVATED );
		break;
	}

	return Menu_DefaultKey( &s_reset.menu, key );
}


/*
=================
Reset_MenuDraw
=================
*/
static void Reset_MenuDraw( void ) {
	UI_DrawNamedPic( 142, 118, 359, 256, ART_FRAME );
	UI_DrawProportionalString( 320, 194 + 10, "RESET GAME?", UI_CENTER|UI_INVERSE, color_red );
	UI_DrawProportionalString( s_reset.slashX, 265, "/", UI_LEFT|UI_INVERSE, color_red );
	Menu_Draw( &s_reset.menu );

	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 0, "WARNING: This resets all of the", UI_CENTER|UI_SMALLFONT, color_yellow );
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 1, "single player game variables.", UI_CENTER|UI_SMALLFONT, color_yellow );
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 2, "Do this only if you want to", UI_CENTER|UI_SMALLFONT, color_yellow );
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 3, "start over from the beginning.", UI_CENTER|UI_SMALLFONT, color_yellow );
}


/*
=================
Reset_Cache
=================
*/
void Reset_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_FRAME );
}


/*
=================
UI_ResetMenu
=================
*/
void UI_ResetMenu(void) {
	uiClientState_t	cstate;
	int	n1, n2, n3;
	int	l1, l2, l3;

	// zero set all our globals
	memset( &s_reset, 0, sizeof(s_reset) );

	Reset_Cache();

	n1 = UI_ProportionalStringWidth( "YES/NO" );
	n2 = UI_ProportionalStringWidth( "YES" ) + PROP_GAP_WIDTH;
	n3 = UI_ProportionalStringWidth( "/" )  + PROP_GAP_WIDTH;
	l1 = 320 - ( n1 / 2 );
	l2 = l1 + n2;
	l3 = l2 + n3;
	s_reset.slashX = l2;

	s_reset.menu.draw       = Reset_MenuDraw;
	s_reset.menu.key        = Reset_MenuKey;
	s_reset.menu.wrapAround = qtrue;

	trap_GetClientState( &cstate );

	if ( cstate.connState >= CA_CONNECTED ) {
		// float on top of running game
		s_reset.menu.fullscreen = qfalse;
	}
	else {
		// game not running
		s_reset.menu.fullscreen = qtrue;
	}

	s_reset.yes.generic.type		= MTYPE_PTEXT;      
	s_reset.yes.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS; 
	s_reset.yes.generic.callback	= Reset_MenuEvent;
	s_reset.yes.generic.id			= ID_YES;
	s_reset.yes.generic.x			= l1;
	s_reset.yes.generic.y			= 264;
	s_reset.yes.string				= "YES";
	s_reset.yes.color				= color_red;
	s_reset.yes.style				= UI_LEFT;

	s_reset.no.generic.type			= MTYPE_PTEXT;      
	s_reset.no.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS; 
	s_reset.no.generic.callback		= Reset_MenuEvent;
	s_reset.no.generic.id			= ID_NO;
	s_reset.no.generic.x		    = l3;
	s_reset.no.generic.y		    = 264;
	s_reset.no.string				= "NO";
	s_reset.no.color			    = color_red;
	s_reset.no.style			    = UI_LEFT;

	Menu_AddItem( &s_reset.menu,	&s_reset.yes );             
	Menu_AddItem( &s_reset.menu,	&s_reset.no );

	UI_PushMenu( &s_reset.menu );

	Menu_SetCursorToItem( &s_reset.menu, &s_reset.no );
}
