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
/*
=======================================================================

CD KEY MENU

=======================================================================
*/


#include "ui_local.h"


#define ART_FRAME		"menu/art/cut_frame"
#define ART_ACCEPT0		"menu/art/accept_0"
#define ART_ACCEPT1		"menu/art/accept_1"	
#define ART_BACK0		"menu/art/back_0"
#define ART_BACK1		"menu/art/back_1"	

#define ID_CDKEY		10
#define ID_ACCEPT		11
#define ID_BACK			12


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	frame;

	menufield_s		cdkey;

	menubitmap_s	accept;
	menubitmap_s	back;
} cdkeyMenuInfo_t;

static cdkeyMenuInfo_t	cdkeyMenuInfo;


/*
===============
UI_CDKeyMenu_Event
===============
*/
static void UI_CDKeyMenu_Event( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_ACCEPT:
		if( cdkeyMenuInfo.cdkey.field.buffer[0] ) {
			trap_SetCDKey( cdkeyMenuInfo.cdkey.field.buffer );
		}
		UI_PopMenu();
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


/*
=================
UI_CDKeyMenu_PreValidateKey
=================
*/
static int UI_CDKeyMenu_PreValidateKey( const char *key ) {
	char	ch;

	if( strlen( key ) != 16 ) {
		return 1;
	}

	while( ( ch = *key++ ) ) {
		switch( ch ) {
		case '2':
		case '3':
		case '7':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'g':
		case 'h':
		case 'j':
		case 'l':
		case 'p':
		case 'r':
		case 's':
		case 't':
		case 'w':
			continue;
		default:
			return -1;
		}
	}

	return 0;
}


/*
=================
UI_CDKeyMenu_DrawKey
=================
*/
static void UI_CDKeyMenu_DrawKey( void *self ) {
	menufield_s		*f;
	qboolean		focus;
	int				style;
	char			c;
	float			*color;
	int				x, y;
	int				val;

	f = (menufield_s *)self;

	focus = (f->generic.parent->cursor == f->generic.menuPosition);

	style = UI_LEFT;
	if( focus ) {
		color = color_yellow;
	}
	else {
		color = color_orange;
	}

	x = 320 - 8 * BIGCHAR_WIDTH;
	y = 240 - BIGCHAR_HEIGHT / 2;
	UI_FillRect( x, y, 16 * BIGCHAR_WIDTH, BIGCHAR_HEIGHT, listbar_color );
	UI_DrawString( x, y, f->field.buffer, style, color );

	// draw cursor if we have focus
	if( focus ) {
		if ( trap_Key_GetOverstrikeMode() ) {
			c = 11;
		} else {
			c = 10;
		}

		style &= ~UI_PULSE;
		style |= UI_BLINK;

		UI_DrawChar( x + f->field.cursor * BIGCHAR_WIDTH, y, c, style, color_white );
	}

	val = UI_CDKeyMenu_PreValidateKey( f->field.buffer );
	if( val == 1 ) {
		UI_DrawProportionalString( 320, 376, "Please enter your CD Key", UI_CENTER|UI_SMALLFONT, color_yellow );
	}
	else if ( val == 0 ) {
		UI_DrawProportionalString( 320, 376, "The CD Key appears to be valid, thank you", UI_CENTER|UI_SMALLFONT, color_white );
	}
	else {
		UI_DrawProportionalString( 320, 376, "The CD Key is not valid", UI_CENTER|UI_SMALLFONT, color_red );
	}
}


/*
===============
UI_CDKeyMenu_Init
===============
*/
static void UI_CDKeyMenu_Init( void ) {
	trap_Cvar_Set( "ui_cdkeychecked", "1" );

	UI_CDKeyMenu_Cache();

	memset( &cdkeyMenuInfo, 0, sizeof(cdkeyMenuInfo) );
	cdkeyMenuInfo.menu.wrapAround = qtrue;
	cdkeyMenuInfo.menu.fullscreen = qtrue;

	cdkeyMenuInfo.banner.generic.type				= MTYPE_BTEXT;
	cdkeyMenuInfo.banner.generic.x					= 320;
	cdkeyMenuInfo.banner.generic.y					= 16;
	cdkeyMenuInfo.banner.string						= "CD KEY";
	cdkeyMenuInfo.banner.color						= color_white;
	cdkeyMenuInfo.banner.style						= UI_CENTER;

	cdkeyMenuInfo.frame.generic.type				= MTYPE_BITMAP;
	cdkeyMenuInfo.frame.generic.name				= ART_FRAME;
	cdkeyMenuInfo.frame.generic.flags				= QMF_INACTIVE;
	cdkeyMenuInfo.frame.generic.x					= 142;
	cdkeyMenuInfo.frame.generic.y					= 118;
	cdkeyMenuInfo.frame.width  						= 359;
	cdkeyMenuInfo.frame.height  					= 256;

	cdkeyMenuInfo.cdkey.generic.type				= MTYPE_FIELD;
	cdkeyMenuInfo.cdkey.generic.name				= "CD Key:";
	cdkeyMenuInfo.cdkey.generic.flags				= QMF_LOWERCASE;
	cdkeyMenuInfo.cdkey.generic.x					= 320 - BIGCHAR_WIDTH * 2.5;
	cdkeyMenuInfo.cdkey.generic.y					= 240 - BIGCHAR_HEIGHT / 2;
	cdkeyMenuInfo.cdkey.field.widthInChars			= 16;
	cdkeyMenuInfo.cdkey.field.maxchars				= 16;
	cdkeyMenuInfo.cdkey.generic.ownerdraw			= UI_CDKeyMenu_DrawKey;

	cdkeyMenuInfo.accept.generic.type				= MTYPE_BITMAP;
	cdkeyMenuInfo.accept.generic.name				= ART_ACCEPT0;
	cdkeyMenuInfo.accept.generic.flags				= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	cdkeyMenuInfo.accept.generic.id					= ID_ACCEPT;
	cdkeyMenuInfo.accept.generic.callback			= UI_CDKeyMenu_Event;
	cdkeyMenuInfo.accept.generic.x					= 640;
	cdkeyMenuInfo.accept.generic.y					= 480-64;
	cdkeyMenuInfo.accept.width						= 128;
	cdkeyMenuInfo.accept.height						= 64;
	cdkeyMenuInfo.accept.focuspic					= ART_ACCEPT1;

	cdkeyMenuInfo.back.generic.type					= MTYPE_BITMAP;
	cdkeyMenuInfo.back.generic.name					= ART_BACK0;
	cdkeyMenuInfo.back.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	cdkeyMenuInfo.back.generic.id					= ID_BACK;
	cdkeyMenuInfo.back.generic.callback				= UI_CDKeyMenu_Event;
	cdkeyMenuInfo.back.generic.x					= 0;
	cdkeyMenuInfo.back.generic.y					= 480-64;
	cdkeyMenuInfo.back.width						= 128;
	cdkeyMenuInfo.back.height						= 64;
	cdkeyMenuInfo.back.focuspic						= ART_BACK1;

	Menu_AddItem( &cdkeyMenuInfo.menu, &cdkeyMenuInfo.banner );
	Menu_AddItem( &cdkeyMenuInfo.menu, &cdkeyMenuInfo.frame );
	Menu_AddItem( &cdkeyMenuInfo.menu, &cdkeyMenuInfo.cdkey );
	Menu_AddItem( &cdkeyMenuInfo.menu, &cdkeyMenuInfo.accept );
	if( uis.menusp ) {
		Menu_AddItem( &cdkeyMenuInfo.menu, &cdkeyMenuInfo.back );
	}

	trap_GetCDKey( cdkeyMenuInfo.cdkey.field.buffer, cdkeyMenuInfo.cdkey.field.maxchars + 1 );
	if( trap_VerifyCDKey( cdkeyMenuInfo.cdkey.field.buffer, NULL ) == qfalse ) {
		cdkeyMenuInfo.cdkey.field.buffer[0] = 0;
	}
}


/*
=================
UI_CDKeyMenu_Cache
=================
*/
void UI_CDKeyMenu_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_ACCEPT0 );
	trap_R_RegisterShaderNoMip( ART_ACCEPT1 );
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FRAME );
}


/*
===============
UI_CDKeyMenu
===============
*/
void UI_CDKeyMenu( void ) {
	UI_CDKeyMenu_Init();
	UI_PushMenu( &cdkeyMenuInfo.menu );
}


/*
===============
UI_CDKeyMenu_f
===============
*/
void UI_CDKeyMenu_f( void ) {
	UI_CDKeyMenu();
}
