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
//
// ui_login.c
//

#include "ui_local.h"


#define LOGIN_FRAME		"menu/art/cut_frame"

#define ID_NAME			100
#define ID_NAME_BOX		101
#define ID_PASSWORD		102
#define ID_PASSWORD_BOX	103
#define ID_LOGIN		104
#define ID_CANCEL		105


typedef struct
{
	menuframework_s	menu;
	menubitmap_s	frame;
	menutext_s		name;
	menufield_s		name_box;
	menutext_s		password;
	menufield_s		password_box;
	menutext_s		login;
	menutext_s		cancel;
} login_t;

static login_t	s_login;

static menuframework_s	s_login_menu;
static menuaction_s		s_login_login;
static menuaction_s		s_login_cancel;

static vec4_t s_login_color_prompt  = {1.00, 0.43, 0.00, 1.00};

/*
===============
Login_MenuEvent
===============
*/
static void Login_MenuEvent( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_LOGIN:
		// set name								``
		//trap_Cvar_Set( "name", s_login.name_box.field.buffer );
		/*
		trap_Cvar_Set( "rank_name", s_login.name_box.field.buffer );
		trap_Cvar_Set( "rank_pwd", s_login.password_box.field.buffer );
		*/

		// login
		trap_CL_UI_RankUserLogin(
			s_login.name_box.field.buffer, 
			s_login.password_box.field.buffer );

		UI_ForceMenuOff();
		break;
		
	case ID_CANCEL:
		UI_PopMenu();
		break;
	}
}


/*
===============
Login_MenuInit
===============
*/
void Login_MenuInit( void ) {
	int				y;

	memset( &s_login, 0, sizeof(s_login) );

	Login_Cache();

	s_login.menu.wrapAround = qtrue;
	s_login.menu.fullscreen = qfalse;

	s_login.frame.generic.type			= MTYPE_BITMAP;
	s_login.frame.generic.flags			= QMF_INACTIVE;
	s_login.frame.generic.name			= LOGIN_FRAME;
	s_login.frame.generic.x				= 142; //320-233;
	s_login.frame.generic.y				= 118; //240-166;
	s_login.frame.width					= 359; //466;
	s_login.frame.height				= 256; //332;

	y = 214;

	s_login.name.generic.type			= MTYPE_PTEXT;
	s_login.name.generic.flags			= QMF_RIGHT_JUSTIFY|QMF_INACTIVE;
	s_login.name.generic.id				= ID_NAME;
	s_login.name.generic.x				= 310;
	s_login.name.generic.y				= y;
	s_login.name.string					= "NAME";
	s_login.name.style					= UI_RIGHT|UI_SMALLFONT;
	s_login.name.color					= s_login_color_prompt;

	s_login.name_box.generic.type		= MTYPE_FIELD;
	s_login.name_box.generic.ownerdraw	= Rankings_DrawName;
	s_login.name_box.generic.name		= "";
	s_login.name_box.generic.flags		= 0;
	s_login.name_box.generic.x			= 330;
	s_login.name_box.generic.y			= y;
	s_login.name_box.field.widthInChars	= 16;
	s_login.name_box.field.maxchars		= 16;
	y += 20;
	
	s_login.password.generic.type		= MTYPE_PTEXT;
	s_login.password.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_INACTIVE;
	s_login.password.generic.id			= ID_PASSWORD;
	s_login.password.generic.x			= 310;
	s_login.password.generic.y			= y;
	s_login.password.string				= "PASSWORD";
	s_login.password.style				= UI_RIGHT|UI_SMALLFONT;
	s_login.password.color				= s_login_color_prompt;

	s_login.password_box.generic.type		= MTYPE_FIELD;
	s_login.password_box.generic.ownerdraw	= Rankings_DrawPassword;
	s_login.password_box.generic.name		= "";
	s_login.password_box.generic.flags		= 0;
	s_login.password_box.generic.x			= 330;
	s_login.password_box.generic.y			= y;
	s_login.password_box.field.widthInChars	= 16;
	s_login.password_box.field.maxchars		= 16;
	y += 40;

	s_login.login.generic.type				= MTYPE_PTEXT;
	s_login.login.generic.flags				= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_login.login.generic.id				= ID_LOGIN;
	s_login.login.generic.callback			= Login_MenuEvent;
	s_login.login.generic.x					= 310;
	s_login.login.generic.y					= y;
	s_login.login.string					= "LOGIN";
	s_login.login.style						= UI_RIGHT|UI_SMALLFONT;
	s_login.login.color						= colorRed;

	s_login.cancel.generic.type				= MTYPE_PTEXT;
	s_login.cancel.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_login.cancel.generic.id				= ID_CANCEL;
	s_login.cancel.generic.callback			= Login_MenuEvent;
	s_login.cancel.generic.x				= 330;
	s_login.cancel.generic.y				= y;
	s_login.cancel.string					= "CANCEL";
	s_login.cancel.style					= UI_LEFT|UI_SMALLFONT;
	s_login.cancel.color					= colorRed;
	y += 20;

	Menu_AddItem( &s_login.menu, (void*) &s_login.frame );
	Menu_AddItem( &s_login.menu, (void*) &s_login.name );
	Menu_AddItem( &s_login.menu, (void*) &s_login.name_box );
	Menu_AddItem( &s_login.menu, (void*) &s_login.password );
	Menu_AddItem( &s_login.menu, (void*) &s_login.password_box );
	Menu_AddItem( &s_login.menu, (void*) &s_login.login );
	Menu_AddItem( &s_login.menu, (void*) &s_login.cancel );
}


/*
===============
Login_Cache
===============
*/
void Login_Cache( void ) {
	trap_R_RegisterShaderNoMip( LOGIN_FRAME );
}


/*
===============
UI_LoginMenu
===============
*/
void UI_LoginMenu( void ) {
	Login_MenuInit();
	UI_PushMenu ( &s_login.menu );
}


