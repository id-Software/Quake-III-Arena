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
// ui_rankstatus.c
//

#include "ui_local.h"


#define RANKSTATUS_FRAME		"menu/art/cut_frame"

#define ID_MESSAGE		100
#define ID_OK			101


typedef struct
{
	menuframework_s	menu;
	menubitmap_s	frame;
	menutext_s		message;
	menutext_s		ok;
} rankstatus_t;

static rankstatus_t	s_rankstatus;

static menuframework_s	s_rankstatus_menu;
static menuaction_s		s_rankstatus_ok;

static grank_status_t	s_status = 0;
static char*			s_rankstatus_message = NULL;

static vec4_t s_rankingstatus_color_prompt  = {1.00, 0.43, 0.00, 1.00};

/*
===============
RankStatus_MenuEvent
===============
*/
static void RankStatus_MenuEvent( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_OK:
		UI_PopMenu();
		
		switch( s_status )
		{
		case QGR_STATUS_NO_USER:
			UI_RankingsMenu();
			break;
		case QGR_STATUS_BAD_PASSWORD:
			UI_RankingsMenu();
			UI_LoginMenu();
			break;
		case QGR_STATUS_USER_EXISTS:
			UI_RankingsMenu();
			UI_SignupMenu();
			break;
		case QGR_STATUS_NO_MEMBERSHIP:
			UI_RankingsMenu();
			break;
		case QGR_STATUS_TIMEOUT:
			UI_RankingsMenu();
			break;
		case QGR_STATUS_INVALIDUSER:
			UI_RankingsMenu();
			break;
		case QGR_STATUS_ERROR:
			UI_RankingsMenu();
			break;
		default:
			break;
		}

		break;
	}
}


/*
===============
RankStatus_MenuInit
===============
*/
void RankStatus_MenuInit( void ) {
	int		y;

	memset( &s_rankstatus, 0, sizeof(s_rankstatus) );

	RankStatus_Cache();

	s_rankstatus.menu.wrapAround = qtrue;
	s_rankstatus.menu.fullscreen = qfalse;

	s_rankstatus.frame.generic.type			= MTYPE_BITMAP;
	s_rankstatus.frame.generic.flags		= QMF_INACTIVE;
	s_rankstatus.frame.generic.name			= RANKSTATUS_FRAME;
	s_rankstatus.frame.generic.x			= 142; //320-233;
	s_rankstatus.frame.generic.y			= 118; //240-166;
	s_rankstatus.frame.width				= 359; //466;
	s_rankstatus.frame.height				= 256; //332;

	y = 214;

	s_rankstatus.message.generic.type			= MTYPE_PTEXT;
	s_rankstatus.message.generic.flags			= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_rankstatus.message.generic.id				= ID_MESSAGE;
	s_rankstatus.message.generic.x				= 320;
	s_rankstatus.message.generic.y				= y;
	s_rankstatus.message.string					= s_rankstatus_message;
	s_rankstatus.message.style					= UI_CENTER|UI_SMALLFONT;
	s_rankstatus.message.color					= s_rankingstatus_color_prompt;
	y += 40;

	s_rankstatus.ok.generic.type				= MTYPE_PTEXT;
	s_rankstatus.ok.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_rankstatus.ok.generic.id					= ID_OK;
	s_rankstatus.ok.generic.callback			= RankStatus_MenuEvent;
	s_rankstatus.ok.generic.x					= 320;
	s_rankstatus.ok.generic.y					= y;
	s_rankstatus.ok.string						= "OK";
	s_rankstatus.ok.style						= UI_CENTER|UI_SMALLFONT;
	s_rankstatus.ok.color						= colorRed;

	Menu_AddItem( &s_rankstatus.menu, (void*) &s_rankstatus.frame );
	Menu_AddItem( &s_rankstatus.menu, (void*) &s_rankstatus.message );
	Menu_AddItem( &s_rankstatus.menu, (void*) &s_rankstatus.ok );
}


/*
===============
RankStatus_Cache
===============
*/
void RankStatus_Cache( void ) {
	trap_R_RegisterShaderNoMip( RANKSTATUS_FRAME );
}


/*
===============
UI_RankStatusMenu
===============
*/
void UI_RankStatusMenu( void ) {

	s_status = (grank_status_t)trap_Cvar_VariableValue("client_status");

	switch( s_status )
	{
	case QGR_STATUS_NEW:
		return;
	case QGR_STATUS_PENDING:
		// GRANK_FIXME
		return;
	case QGR_STATUS_NO_USER:
		// GRANK_FIXME - get this when user exists
		s_rankstatus_message = "Username unavailable";
		break;
	case QGR_STATUS_BAD_PASSWORD:
		s_rankstatus_message = "Invalid password";
		break;
	case QGR_STATUS_TIMEOUT:
		s_rankstatus_message = "Timed out";
		break;
	case QGR_STATUS_NO_MEMBERSHIP:
		s_rankstatus_message = "No membership";
		break;
	case QGR_STATUS_INVALIDUSER:
		s_rankstatus_message = "Validation failed";
		break;
	case QGR_STATUS_ERROR:
		s_rankstatus_message = "Error";
		break;
	case QGR_STATUS_SPECTATOR:
	case QGR_STATUS_ACTIVE:
		UI_ForceMenuOff();
		return;
	default:
		return;
	}
	RankStatus_MenuInit();
	trap_CL_UI_RankUserReset();
	UI_PushMenu ( &s_rankstatus.menu );
}

