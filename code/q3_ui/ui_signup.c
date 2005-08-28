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
// ui_signup.c
//

#include "ui_local.h"


#define SIGNUP_FRAME		"menu/art/cut_frame"

#define ID_NAME			100
#define ID_NAME_BOX		101
#define ID_PASSWORD		102
#define ID_PASSWORD_BOX	103
#define ID_AGAIN		104
#define ID_AGAIN_BOX	105
#define ID_EMAIL		106
#define ID_EMAIL_BOX	107
#define ID_SIGNUP		108
#define ID_CANCEL		109


typedef struct
{
	menuframework_s	menu;
	menubitmap_s	frame;
	menutext_s		name;
	menufield_s		name_box;
	menutext_s		password;
	menufield_s		password_box;
	menutext_s		again;
	menufield_s		again_box;
	menutext_s		email;
	menufield_s		email_box;
	menutext_s		signup;
	menutext_s		cancel;
} signup_t;

static signup_t	s_signup;

static menuframework_s	s_signup_menu;
static menuaction_s		s_signup_signup;
static menuaction_s		s_signup_cancel;

static vec4_t s_signup_color_prompt  = {1.00, 0.43, 0.00, 1.00};

/*
===============
Signup_MenuEvent
===============
*/
static void Signup_MenuEvent( void* ptr, int event ) {
	//char	cmd[1024];
	
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_SIGNUP:
		if( strcmp(s_signup.password_box.field.buffer, 
			s_signup.again_box.field.buffer) != 0 )
		{
			// GRANK_FIXME - password mismatch
			break;
		}
		// set name
		//trap_Cvar_Set( "name", s_signup.name_box.field.buffer );
		/*
		trap_Cvar_Set( "rank_name", s_signup.name_box.field.buffer );
		trap_Cvar_Set( "rank_pwd", s_signup.password_box.field.buffer );
		*/

		// create account
		/*
		sprintf( cmd, "cmd rank_create \"%s\" \"%s\" \"%s\"\n", 
			s_signup.name_box.field.buffer, 
			s_signup.password_box.field.buffer, 
			s_signup.email_box.field.buffer );
		trap_Cmd_ExecuteText( EXEC_APPEND, cmd );
		*/
		trap_CL_UI_RankUserCreate(
			s_signup.name_box.field.buffer, 
			s_signup.password_box.field.buffer, 
			s_signup.email_box.field.buffer );

		UI_ForceMenuOff();
		break;
		
	case ID_CANCEL:
		UI_PopMenu();
		break;
	}
}

/*
===============
Signup_MenuInit
===============
*/
void Signup_MenuInit( void ) {
	grank_status_t	status;
	int				y;

	memset( &s_signup, 0, sizeof(s_signup) );

	Signup_Cache();

	s_signup.menu.wrapAround = qtrue;
	s_signup.menu.fullscreen = qfalse;

	s_signup.frame.generic.type				= MTYPE_BITMAP;
	s_signup.frame.generic.flags			= QMF_INACTIVE;
	s_signup.frame.generic.name				= SIGNUP_FRAME;
	s_signup.frame.generic.x				= 142; //320-233;
	s_signup.frame.generic.y				= 118; //240-166;
	s_signup.frame.width					= 359; //466;
	s_signup.frame.height					= 256; //332;

	y = 194;

	s_signup.name.generic.type				= MTYPE_PTEXT;
	s_signup.name.generic.flags				= QMF_RIGHT_JUSTIFY|QMF_INACTIVE;
	s_signup.name.generic.id				= ID_NAME;
	s_signup.name.generic.x					= 310;
	s_signup.name.generic.y					= y;
	s_signup.name.string					= "NAME";
	s_signup.name.style						= UI_RIGHT|UI_SMALLFONT;
	s_signup.name.color						= s_signup_color_prompt;

	s_signup.name_box.generic.type			= MTYPE_FIELD;
	s_signup.name_box.generic.ownerdraw		= Rankings_DrawName;
	s_signup.name_box.generic.name			= "";
	s_signup.name_box.generic.flags			= 0;
	s_signup.name_box.generic.x				= 330;
	s_signup.name_box.generic.y				= y;
	s_signup.name_box.field.widthInChars	= 16;
	s_signup.name_box.field.maxchars		= 16;
	y += 20;
	
	s_signup.password.generic.type			= MTYPE_PTEXT;
	s_signup.password.generic.flags			= QMF_RIGHT_JUSTIFY|QMF_INACTIVE;
	s_signup.password.generic.id			= ID_PASSWORD;
	s_signup.password.generic.x				= 310;
	s_signup.password.generic.y				= y;
	s_signup.password.string				= "PASSWORD";
	s_signup.password.style					= UI_RIGHT|UI_SMALLFONT;
	s_signup.password.color					= s_signup_color_prompt;

	s_signup.password_box.generic.type			= MTYPE_FIELD;
	s_signup.password_box.generic.ownerdraw		= Rankings_DrawPassword;
	s_signup.password_box.generic.name			= "";
	s_signup.password_box.generic.flags			= 0;
	s_signup.password_box.generic.x				= 330;
	s_signup.password_box.generic.y				= y;
	s_signup.password_box.field.widthInChars	= 16;
	s_signup.password_box.field.maxchars		= 16;
	y += 20;

	s_signup.again.generic.type				= MTYPE_PTEXT;
	s_signup.again.generic.flags			= QMF_RIGHT_JUSTIFY|QMF_INACTIVE;
	s_signup.again.generic.id				= ID_AGAIN;
	s_signup.again.generic.x				= 310;
	s_signup.again.generic.y				= y;
	s_signup.again.string					= "(AGAIN)";
	s_signup.again.style					= UI_RIGHT|UI_SMALLFONT;
	s_signup.again.color					= s_signup_color_prompt;

	s_signup.again_box.generic.type			= MTYPE_FIELD;
	s_signup.again_box.generic.ownerdraw	= Rankings_DrawPassword;
	s_signup.again_box.generic.name			= "";
	s_signup.again_box.generic.flags		= 0;
	s_signup.again_box.generic.x			= 330;
	s_signup.again_box.generic.y			= y;
	s_signup.again_box.field.widthInChars	= 16;
	s_signup.again_box.field.maxchars		= 16;
	y += 20;

	s_signup.email.generic.type				= MTYPE_PTEXT;
	s_signup.email.generic.flags			= QMF_RIGHT_JUSTIFY|QMF_INACTIVE;
	s_signup.email.generic.id				= ID_EMAIL;
	s_signup.email.generic.x				= 310;
	s_signup.email.generic.y				= y;
	s_signup.email.string					= "EMAIL";
	s_signup.email.style					= UI_RIGHT|UI_SMALLFONT;
	s_signup.email.color					= s_signup_color_prompt;

	s_signup.email_box.generic.type			= MTYPE_FIELD;
	s_signup.email_box.generic.ownerdraw	= Rankings_DrawText;
	s_signup.email_box.generic.name			= "";
	s_signup.email_box.generic.flags		= 0;
	s_signup.email_box.generic.x			= 330;
	s_signup.email_box.generic.y			= y;
	s_signup.email_box.field.widthInChars	= 16;
	s_signup.email_box.field.maxchars		= MAX_EDIT_LINE;
	y += 40;

	s_signup.signup.generic.type			= MTYPE_PTEXT;
	s_signup.signup.generic.flags			= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_signup.signup.generic.id				= ID_SIGNUP;
	s_signup.signup.generic.callback		= Signup_MenuEvent;
	s_signup.signup.generic.x				= 310;
	s_signup.signup.generic.y				= y;
	s_signup.signup.string					= "SIGN UP";
	s_signup.signup.style					= UI_RIGHT|UI_SMALLFONT;
	s_signup.signup.color					= colorRed;

	s_signup.cancel.generic.type			= MTYPE_PTEXT;
	s_signup.cancel.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_signup.cancel.generic.id				= ID_CANCEL;
	s_signup.cancel.generic.callback		= Signup_MenuEvent;
	s_signup.cancel.generic.x				= 330;
	s_signup.cancel.generic.y				= y;
	s_signup.cancel.string					= "CANCEL";
	s_signup.cancel.style					= UI_LEFT|UI_SMALLFONT;
	s_signup.cancel.color					= colorRed;
	y += 20;

	status = (grank_status_t)trap_Cvar_VariableValue("client_status");
	if( (status != QGR_STATUS_NEW) && (status != QGR_STATUS_SPECTATOR) )
	{
		s_signup.name_box.generic.flags |= QMF_INACTIVE;	
		s_signup.password_box.generic.flags |= QMF_INACTIVE;	
		s_signup.again_box.generic.flags |= QMF_INACTIVE;	
		s_signup.email_box.generic.flags |= QMF_INACTIVE;	
		s_signup.signup.generic.flags |= QMF_INACTIVE;
		
		s_signup.signup.color = colorMdGrey;
	}
	
	Menu_AddItem( &s_signup.menu, (void*) &s_signup.frame );
	Menu_AddItem( &s_signup.menu, (void*) &s_signup.name );
	Menu_AddItem( &s_signup.menu, (void*) &s_signup.name_box );
	Menu_AddItem( &s_signup.menu, (void*) &s_signup.password );
	Menu_AddItem( &s_signup.menu, (void*) &s_signup.password_box );
	Menu_AddItem( &s_signup.menu, (void*) &s_signup.again );
	Menu_AddItem( &s_signup.menu, (void*) &s_signup.again_box );
	Menu_AddItem( &s_signup.menu, (void*) &s_signup.email );
	Menu_AddItem( &s_signup.menu, (void*) &s_signup.email_box );
	Menu_AddItem( &s_signup.menu, (void*) &s_signup.signup );
	Menu_AddItem( &s_signup.menu, (void*) &s_signup.cancel );
}


/*
===============
Signup_Cache
===============
*/
void Signup_Cache( void ) {
	trap_R_RegisterShaderNoMip( SIGNUP_FRAME );
}


/*
===============
UI_SignupMenu
===============
*/
void UI_SignupMenu( void ) {
	Signup_MenuInit();
	UI_PushMenu ( &s_signup.menu );
}


