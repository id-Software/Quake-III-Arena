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
// ui_rankings.c
//

#include "ui_local.h"


#define RANKINGS_FRAME	"menu/art/cut_frame"

#define ID_LOGIN		100
#define ID_LOGOUT		101
#define ID_CREATE		102
#define ID_SPECTATE		103
#define ID_SETUP		104
#define ID_LEAVE		105


typedef struct
{
	menuframework_s	menu;
	menubitmap_s	frame;
	menutext_s		login;
	menutext_s		logout;
	menutext_s		create;
	menutext_s		spectate;
	menutext_s		setup;
	menutext_s		leave;
} rankings_t;

static rankings_t	s_rankings;

static menuframework_s	s_rankings_menu;
static menuaction_s		s_rankings_login;
static menuaction_s		s_rankings_logout;
static menuaction_s		s_rankings_create;
static menuaction_s		s_rankings_spectate;
static menuaction_s		s_rankings_setup;
static menuaction_s		s_rankings_leave;


/*
===============
Rankings_DrawText
===============
*/
void Rankings_DrawText( void* self )
{
	menufield_s		*f;
	qboolean		focus;
	int				style;
	char			*txt;
	char			c;
	float			*color;
	int				basex, x, y;

	f = (menufield_s*)self;
	basex = f->generic.x;
	y = f->generic.y + 4;
	focus = (f->generic.parent->cursor == f->generic.menuPosition);

	style = UI_LEFT|UI_SMALLFONT;
	color = text_color_normal;
	if( focus ) {
		style |= UI_PULSE;
		color = text_color_highlight;
	}

	// draw the actual text
	txt = f->field.buffer;
	color = g_color_table[ColorIndex(COLOR_WHITE)];
	x = basex;
	while ( (c = *txt) != 0 ) {
		UI_DrawChar( x, y, c, style, color );
		txt++;
		x += SMALLCHAR_WIDTH;
	}

	// draw cursor if we have focus
	if( focus ) {
		if ( trap_Key_GetOverstrikeMode() ) {
			c = 11;
		} else {
			c = 10;
		}

		style &= ~UI_PULSE;
		style |= UI_BLINK;

		UI_DrawChar( basex + f->field.cursor * SMALLCHAR_WIDTH, y, c, style, color_white );
	}
}

/*
===============
Rankings_DrawName
===============
*/
void Rankings_DrawName( void* self )
{
	menufield_s		*f;
	int				length;
	char*			p;
	
	f = (menufield_s*)self;

	// GRANK_FIXME - enforce valid characters
	for( p = f->field.buffer; *p != '\0'; p++ )
	{
		//if( ispunct(*p) || isspace(*p) )
		if( !( ( (*p) >= '0' && (*p) <= '9') || Q_isalpha(*p)) )
		{
			*p = '\0';
		}
	}
	
	// strip color codes
	Q_CleanStr( f->field.buffer );
	length = strlen( f->field.buffer );
	if( f->field.cursor > length )
	{
		f->field.cursor = length;
	}	

	Rankings_DrawText( f );
}

#if 0 // old version
/*
===============
Rankings_DrawName
===============
*/
void Rankings_DrawName( void* self )
{
	menufield_s*	f;
	int				length;
	
	f = (menufield_s*)self;

	// strip color codes
	Q_CleanStr( f->field.buffer );
	length = strlen( f->field.buffer );
	if( f->field.cursor > length )
	{
		f->field.cursor = length;
	}
	
	// show beginning of long names
	/*
	if( Menu_ItemAtCursor( f->generic.parent ) != f )
	{
		if( f->field.scroll > 0 )
		{
			f->field.cursor = 0;
			f->field.scroll = 0;
		}
	}
	*/
	
	MenuField_Draw( f );
}
#endif

/*
===============
Rankings_DrawPassword
===============
*/
void Rankings_DrawPassword( void* self )
{
	menufield_s*	f;
	char			password[MAX_EDIT_LINE];
	int				length;
	int				i;
	char*			p;

	f = (menufield_s*)self;
	
	// GRANK_FIXME - enforce valid characters
	for( p = f->field.buffer; *p != '\0'; p++ )
	{
		//if( ispunct(*p) || isspace(*p) )
		if( !( ( (*p) >= '0' && (*p) <= '9') || Q_isalpha(*p)) )
		{
			*p = '\0';
		}
	}
	
	length = strlen( f->field.buffer );
	if( f->field.cursor > length )
	{
		f->field.cursor = length;
	}
	
	// save password
	Q_strncpyz( password, f->field.buffer, sizeof(password) );

	// mask password with *
	for( i = 0; i < length; i++ )
	{
		f->field.buffer[i] = '*';
	}

	// draw masked password
	Rankings_DrawText( f );
	//MenuField_Draw( f );

	// restore password
	Q_strncpyz( f->field.buffer, password, sizeof(f->field.buffer) );
}

/*
===============
Rankings_MenuEvent
===============
*/
static void Rankings_MenuEvent( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_LOGIN:
		UI_LoginMenu();
		break;

	case ID_LOGOUT:
		// server side masqueraded player logout first
		trap_CL_UI_RankUserRequestLogout();
		UI_ForceMenuOff();
		break;
		
	case ID_CREATE:
		UI_SignupMenu();
		break;

	case ID_SPECTATE:
		trap_Cmd_ExecuteText( EXEC_APPEND, "cmd rank_spectate\n" );
		UI_ForceMenuOff();
		break;

	case ID_SETUP:
		UI_SetupMenu();
		break;
		
	case ID_LEAVE:
		trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
		UI_ForceMenuOff();
		break;

	}
}


/*
===============
Rankings_MenuInit
===============
*/
void Rankings_MenuInit( void ) {
	grank_status_t	status;
	int				y;

	memset( &s_rankings, 0, sizeof(s_rankings) );

	Rankings_Cache();

	s_rankings.menu.wrapAround = qtrue;
	s_rankings.menu.fullscreen = qfalse;

	s_rankings.frame.generic.type		= MTYPE_BITMAP;
	s_rankings.frame.generic.flags		= QMF_INACTIVE;
	s_rankings.frame.generic.name		= RANKINGS_FRAME;
	s_rankings.frame.generic.x			= 142;
	s_rankings.frame.generic.y			= 118;
	s_rankings.frame.width				= 359;
	s_rankings.frame.height				= 256;

	y = 194;

	s_rankings.login.generic.type		= MTYPE_PTEXT;
	s_rankings.login.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_rankings.login.generic.id			= ID_LOGIN;
	s_rankings.login.generic.callback	= Rankings_MenuEvent;
	s_rankings.login.generic.x			= 320;
	s_rankings.login.generic.y			= y;
	s_rankings.login.string				= "LOGIN";
	s_rankings.login.style				= UI_CENTER|UI_SMALLFONT;
	s_rankings.login.color				= colorRed;
	y += 20;

	s_rankings.logout.generic.type		= MTYPE_PTEXT;
	s_rankings.logout.generic.flags		= QMF_HIDDEN|QMF_INACTIVE|QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_rankings.logout.generic.id		= ID_LOGOUT;
	s_rankings.logout.generic.callback	= Rankings_MenuEvent;
	s_rankings.logout.generic.x			= 320;
	s_rankings.logout.generic.y			= y;
	s_rankings.logout.string				= "LOGOUT";
	s_rankings.logout.style				= UI_CENTER|UI_SMALLFONT;
	s_rankings.logout.color				= colorRed;

	s_rankings.create.generic.type		= MTYPE_PTEXT;
	s_rankings.create.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_rankings.create.generic.id		= ID_CREATE;
	s_rankings.create.generic.callback	= Rankings_MenuEvent;
	s_rankings.create.generic.x			= 320;
	s_rankings.create.generic.y			= y;
	s_rankings.create.string			= "SIGN UP";
	s_rankings.create.style				= UI_CENTER|UI_SMALLFONT;
	s_rankings.create.color				= colorRed;
	y += 20;

	s_rankings.spectate.generic.type		= MTYPE_PTEXT;
	s_rankings.spectate.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_rankings.spectate.generic.id			= ID_SPECTATE;
	s_rankings.spectate.generic.callback	= Rankings_MenuEvent;
	s_rankings.spectate.generic.x			= 320;
	s_rankings.spectate.generic.y			= y;
	s_rankings.spectate.string				= "SPECTATE";
	s_rankings.spectate.style				= UI_CENTER|UI_SMALLFONT;
	s_rankings.spectate.color				= colorRed;
	y += 20;

	s_rankings.setup.generic.type		= MTYPE_PTEXT;
	s_rankings.setup.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_rankings.setup.generic.id			= ID_SETUP;
	s_rankings.setup.generic.callback	= Rankings_MenuEvent;
	s_rankings.setup.generic.x			= 320;
	s_rankings.setup.generic.y			= y;
	s_rankings.setup.string				= "SETUP";
	s_rankings.setup.style				= UI_CENTER|UI_SMALLFONT;
	s_rankings.setup.color				= colorRed;
	y += 20;

	s_rankings.leave.generic.type		= MTYPE_PTEXT;
	s_rankings.leave.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_rankings.leave.generic.id			= ID_LEAVE;
	s_rankings.leave.generic.callback	= Rankings_MenuEvent;
	s_rankings.leave.generic.x			= 320;
	s_rankings.leave.generic.y			= y;
	s_rankings.leave.string				= "LEAVE ARENA";
	s_rankings.leave.style				= UI_CENTER|UI_SMALLFONT;
	s_rankings.leave.color				= colorRed;
	y += 20;

	status = (grank_status_t)trap_Cvar_VariableValue("client_status");
	if( (status != QGR_STATUS_NEW) && (status != QGR_STATUS_SPECTATOR) )
	{
		s_rankings.login.generic.flags |= QMF_HIDDEN | QMF_INACTIVE;	
		s_rankings.create.generic.flags |= QMF_HIDDEN | QMF_INACTIVE;
		s_rankings.spectate.generic.flags |= QMF_HIDDEN | QMF_INACTIVE;

		s_rankings.logout.generic.flags &= ~(QMF_HIDDEN | QMF_INACTIVE);
	}
	
	if ( (status == QGR_STATUS_VALIDATING) ||
		 (status == QGR_STATUS_PENDING) ||
		 (status == QGR_STATUS_LEAVING) )
	{
		s_rankings.login.generic.flags  |= QMF_GRAYED;
		s_rankings.create.generic.flags |= QMF_GRAYED;
		s_rankings.logout.generic.flags |= QMF_GRAYED;
	}
	
	//GRank FIXME -- don't need setup option any more
	s_rankings.setup.generic.flags |= QMF_HIDDEN | QMF_INACTIVE;

	Menu_AddItem( &s_rankings.menu, (void*) &s_rankings.frame );
	Menu_AddItem( &s_rankings.menu, (void*) &s_rankings.login );
	Menu_AddItem( &s_rankings.menu, (void*) &s_rankings.logout );
	Menu_AddItem( &s_rankings.menu, (void*) &s_rankings.create );
	Menu_AddItem( &s_rankings.menu, (void*) &s_rankings.spectate );
	Menu_AddItem( &s_rankings.menu, (void*) &s_rankings.setup );
	Menu_AddItem( &s_rankings.menu, (void*) &s_rankings.leave );
}


/*
===============
Rankings_Cache
===============
*/
void Rankings_Cache( void ) {
	trap_R_RegisterShaderNoMip( RANKINGS_FRAME );
}


/*
===============
UI_RankingsMenu
===============
*/
void UI_RankingsMenu( void ) {
	Rankings_MenuInit();
	UI_PushMenu ( &s_rankings.menu );
}


