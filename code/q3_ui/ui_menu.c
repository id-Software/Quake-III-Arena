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
/*
=======================================================================

MAIN MENU

=======================================================================
*/


#include "ui_local.h"


#define ID_SINGLEPLAYER			10
#define ID_MULTIPLAYER			11
#define ID_SETUP				12
#define ID_DEMOS				13
#define ID_CINEMATICS			14
#define ID_TEAMARENA		15
#define ID_MODS					16
#define ID_EXIT					17

#define MAIN_BANNER_MODEL				"models/mapobjects/banner/banner5.md3"
#define MAIN_MENU_VERTICAL_SPACING		34


typedef struct {
	menuframework_s	menu;

	menutext_s		singleplayer;
	menutext_s		multiplayer;
	menutext_s		setup;
	menutext_s		demos;
	menutext_s		cinematics;
	menutext_s		teamArena;
	menutext_s		mods;
	menutext_s		exit;
	menutext_s		agentControl;

	qhandle_t		bannerModel;
} mainmenu_t;


static mainmenu_t s_main;

typedef struct {
	menuframework_s menu;	
	char errorMessage[4096];
} errorMessage_t;

static errorMessage_t s_errorMessage;

/* Native in-game AI gateway configuration menu. */
#define ID_AGENT_GATEWAY  201
#define ID_AGENT_KEY      202
#define ID_AGENT_PROVIDER 203
#define ID_AGENT_SAVE     204
#define ID_AGENT_BACK     205

typedef struct {
	menuframework_s menu;
	menutext_s banner, gatewayLabel, keyLabel, providerLabel, save, back, note;
	menufield_s gateway, key;
	menulist_s provider;
} agentControlMenu_t;

static agentControlMenu_t s_agentControl;
static const char *s_agentProviders[] = {
	"OpenAI", "Anthropic", "Google Gemini", "Mistral", "Groq",
	"xAI", "DeepSeek", "OpenRouter", "NVIDIA NIM", "Ollama", "LM Studio"
};

static void AgentControl_MenuEvent( void *ptr, int event ) {
	if ( event != QM_ACTIVATED ) return;
	switch ( ((menucommon_s *)ptr)->id ) {
	case ID_AGENT_SAVE:
		trap_Cvar_Set( "q3_agent_gateway", s_agentControl.gateway.field.buffer );
		trap_Cvar_Set( "q3_agent_key", s_agentControl.key.field.buffer );
		{
			char provider[16];
			Com_sprintf( provider, sizeof(provider), "%d", s_agentControl.provider.curvalue );
			trap_Cvar_Set( "q3_agent_provider", provider );
		}
		s_agentControl.note.string = "SETTINGS SAVED. LIVE INPUT STILL USES THE GATEWAY BRIDGE.";
		s_agentControl.note.color = color_yellow;
		break;
	case ID_AGENT_BACK:
		UI_PopMenu();
		break;
	}
}

static void AgentControl_MenuInit( void ) {
	char value[256];
	memset( &s_agentControl, 0, sizeof(s_agentControl) );
	s_agentControl.menu.wrapAround = qtrue;
	s_agentControl.menu.fullscreen = qtrue;

	s_agentControl.banner.generic.type = MTYPE_BTEXT;
	s_agentControl.banner.generic.x = 320;
	s_agentControl.banner.generic.y = 24;
	s_agentControl.banner.string = "AI AGENT CONTROL";
	s_agentControl.banner.color = color_white;
	s_agentControl.banner.style = UI_CENTER;

	s_agentControl.gatewayLabel.generic.type = MTYPE_PTEXT;
	s_agentControl.gatewayLabel.generic.flags = QMF_RIGHT_JUSTIFY | QMF_INACTIVE;
	s_agentControl.gatewayLabel.generic.x = 250;
	s_agentControl.gatewayLabel.generic.y = 145;
	s_agentControl.gatewayLabel.string = "GATEWAY";
	s_agentControl.gatewayLabel.color = color_white;
	s_agentControl.gatewayLabel.style = UI_RIGHT | UI_SMALLFONT;

	s_agentControl.gateway.generic.type = MTYPE_FIELD;
	s_agentControl.gateway.generic.id = ID_AGENT_GATEWAY;
	s_agentControl.gateway.generic.x = 270;
	s_agentControl.gateway.generic.y = 145;
	s_agentControl.gateway.field.widthInChars = 40;
	s_agentControl.gateway.field.maxchars = 127;
	trap_Cvar_VariableStringBuffer( "q3_agent_gateway", value, sizeof(value) );
	Q_strncpyz( s_agentControl.gateway.field.buffer,
		value[0] ? value : "http://127.0.0.1:8765",
		sizeof(s_agentControl.gateway.field.buffer) );
	MenuField_Init( &s_agentControl.gateway );

	s_agentControl.keyLabel.generic.type = MTYPE_PTEXT;
	s_agentControl.keyLabel.generic.flags = QMF_RIGHT_JUSTIFY | QMF_INACTIVE;
	s_agentControl.keyLabel.generic.x = 250;
	s_agentControl.keyLabel.generic.y = 190;
	s_agentControl.keyLabel.string = "AGENT KEY";
	s_agentControl.keyLabel.color = color_white;
	s_agentControl.keyLabel.style = UI_RIGHT | UI_SMALLFONT;

	s_agentControl.key.generic.type = MTYPE_FIELD;
	s_agentControl.key.generic.id = ID_AGENT_KEY;
	s_agentControl.key.generic.x = 270;
	s_agentControl.key.generic.y = 190;
	s_agentControl.key.field.widthInChars = 40;
	s_agentControl.key.field.maxchars = 127;
	trap_Cvar_VariableStringBuffer( "q3_agent_key", value, sizeof(value) );
	Q_strncpyz( s_agentControl.key.field.buffer, value,
		sizeof(s_agentControl.key.field.buffer) );
	MenuField_Init( &s_agentControl.key );

	s_agentControl.providerLabel.generic.type = MTYPE_PTEXT;
	s_agentControl.providerLabel.generic.flags = QMF_RIGHT_JUSTIFY | QMF_INACTIVE;
	s_agentControl.providerLabel.generic.x = 250;
	s_agentControl.providerLabel.generic.y = 235;
	s_agentControl.providerLabel.string = "AI PROVIDER";
	s_agentControl.providerLabel.color = color_white;
	s_agentControl.providerLabel.style = UI_RIGHT | UI_SMALLFONT;

	s_agentControl.provider.generic.type = MTYPE_SPINCONTROL;
	s_agentControl.provider.generic.id = ID_AGENT_PROVIDER;
	s_agentControl.provider.generic.x = 270;
	s_agentControl.provider.generic.y = 235;
	s_agentControl.provider.itemnames = s_agentProviders;
	s_agentControl.provider.numitems = sizeof(s_agentProviders) / sizeof(s_agentProviders[0]);
	s_agentControl.provider.curvalue = (int)trap_Cvar_VariableValue( "q3_agent_provider" );
	if (s_agentControl.provider.curvalue < 0 ||
		s_agentControl.provider.curvalue >= s_agentControl.provider.numitems)
		s_agentControl.provider.curvalue = 0;

	s_agentControl.note.generic.type = MTYPE_PTEXT;
	s_agentControl.note.generic.flags = QMF_INACTIVE | QMF_CENTER_JUSTIFY;
	s_agentControl.note.generic.x = 320;
	s_agentControl.note.generic.y = 290;
	s_agentControl.note.string = "Start the Agent Gateway before connecting.";
	s_agentControl.note.color = color_yellow;
	s_agentControl.note.style = UI_CENTER | UI_SMALLFONT;

	s_agentControl.save.generic.type = MTYPE_PTEXT;
	s_agentControl.save.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
	s_agentControl.save.generic.id = ID_AGENT_SAVE;
	s_agentControl.save.generic.callback = AgentControl_MenuEvent;
	s_agentControl.save.generic.x = 320;
	s_agentControl.save.generic.y = 350;
	s_agentControl.save.string = "SAVE AI SETTINGS";
	s_agentControl.save.color = color_red;
	s_agentControl.save.style = UI_CENTER;

	s_agentControl.back.generic.type = MTYPE_PTEXT;
	s_agentControl.back.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
	s_agentControl.back.generic.id = ID_AGENT_BACK;
	s_agentControl.back.generic.callback = AgentControl_MenuEvent;
	s_agentControl.back.generic.x = 320;
	s_agentControl.back.generic.y = 384;
	s_agentControl.back.string = "BACK";
	s_agentControl.back.color = color_red;
	s_agentControl.back.style = UI_CENTER;

	Menu_AddItem( &s_agentControl.menu, &s_agentControl.banner );
	Menu_AddItem( &s_agentControl.menu, &s_agentControl.gatewayLabel );
	Menu_AddItem( &s_agentControl.menu, &s_agentControl.gateway );
	Menu_AddItem( &s_agentControl.menu, &s_agentControl.keyLabel );
	Menu_AddItem( &s_agentControl.menu, &s_agentControl.key );
	Menu_AddItem( &s_agentControl.menu, &s_agentControl.providerLabel );
	Menu_AddItem( &s_agentControl.menu, &s_agentControl.provider );
	Menu_AddItem( &s_agentControl.menu, &s_agentControl.note );
	Menu_AddItem( &s_agentControl.menu, &s_agentControl.save );
	Menu_AddItem( &s_agentControl.menu, &s_agentControl.back );
}

static void UI_AgentControlMenu( void ) {
	AgentControl_MenuInit();
	UI_PushMenu( &s_agentControl.menu );
}


/*
=================
MainMenu_ExitAction
=================
*/
static void MainMenu_ExitAction( qboolean result ) {
	if( !result ) {
		return;
	}
	UI_PopMenu();
	UI_CreditMenu();
}



/*
=================
Main_MenuEvent
=================
*/
void Main_MenuEvent (void* ptr, int event) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_SINGLEPLAYER:
		UI_SPLevelMenu();
		break;

	case ID_MULTIPLAYER:
		UI_ArenaServersMenu();
		break;

	case ID_SETUP:
		UI_SetupMenu();
		break;

	case ID_DEMOS:
		UI_DemosMenu();
		break;

	case ID_CINEMATICS:
		UI_CinematicsMenu();
		break;

	case ID_MODS:
		UI_ModsMenu();
		break;

	case ID_TEAMARENA:
		trap_Cvar_Set( "fs_game", "missionpack");
		trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
		break;

	case ID_AGENT_CONTROL:
		UI_AgentControlMenu();
		break;

	case ID_EXIT:
		UI_ConfirmMenu( "EXIT GAME?", NULL, MainMenu_ExitAction );
		break;
	}
}


/*
===============
MainMenu_Cache
===============
*/
void MainMenu_Cache( void ) {
	s_main.bannerModel = trap_R_RegisterModel( MAIN_BANNER_MODEL );
}

sfxHandle_t ErrorMessage_Key(int key)
{
	trap_Cvar_Set( "com_errorMessage", "" );
	UI_MainMenu();
	return (menu_null_sound);
}

/*
===============
Main_MenuDraw
TTimo: this function is common to the main menu and errorMessage menu
===============
*/

static void Main_MenuDraw( void ) {
	refdef_t		refdef;
	refEntity_t		ent;
	vec3_t			origin;
	vec3_t			angles;
	float			adjust;
	float			x, y, w, h;
	vec4_t			color = {0.5, 0, 0, 1};

	// setup the refdef

	memset( &refdef, 0, sizeof( refdef ) );

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	x = 0;
	y = 0;
	w = 640;
	h = 120;
	UI_AdjustFrom640( &x, &y, &w, &h );
	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	adjust = 0; // JDC: Kenneth asked me to stop this 1.0 * sin( (float)uis.realtime / 1000 );
	refdef.fov_x = 60 + adjust;
	refdef.fov_y = 19.6875 + adjust;

	refdef.time = uis.realtime;

	origin[0] = 300;
	origin[1] = 0;
	origin[2] = -32;

	trap_R_ClearScene();

	// add the model

	memset( &ent, 0, sizeof(ent) );

	adjust = 5.0 * sin( (float)uis.realtime / 5000 );
	VectorSet( angles, 0, 180 + adjust, 0 );
	AnglesToAxis( angles, ent.axis );
	ent.hModel = s_main.bannerModel;
	VectorCopy( origin, ent.origin );
	VectorCopy( origin, ent.lightingOrigin );
	ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
	VectorCopy( ent.origin, ent.oldorigin );

	trap_R_AddRefEntityToScene( &ent );

	trap_R_RenderScene( &refdef );
	
	if (strlen(s_errorMessage.errorMessage))
	{
		UI_DrawProportionalString_AutoWrapped( 320, 192, 600, 20, s_errorMessage.errorMessage, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );
	}
	else
	{
		// standard menu drawing
		Menu_Draw( &s_main.menu );		
	}

	if (uis.demoversion) {
		UI_DrawProportionalString( 320, 372, "DEMO      FOR MATURE AUDIENCES      DEMO", UI_CENTER|UI_SMALLFONT, color );
		UI_DrawString( 320, 400, "Quake III Arena(c) 1999-2000, Id Software, Inc.  All Rights Reserved", UI_CENTER|UI_SMALLFONT, color );
	} else {
		UI_DrawString( 320, 450, "Quake III Arena(c) 1999-2000, Id Software, Inc.  All Rights Reserved", UI_CENTER|UI_SMALLFONT, color );
	}
}


/*
===============
UI_TeamArenaExists
===============
*/
static qboolean UI_TeamArenaExists( void ) {
	int		numdirs;
	char	dirlist[2048];
	char	*dirptr;
  char  *descptr;
	int		i;
	int		dirlen;

	numdirs = trap_FS_GetFileList( "$modlist", "", dirlist, sizeof(dirlist) );
	dirptr  = dirlist;
	for( i = 0; i < numdirs; i++ ) {
		dirlen = strlen( dirptr ) + 1;
    descptr = dirptr + dirlen;
		if (Q_stricmp(dirptr, "missionpack") == 0) {
			return qtrue;
		}
    dirptr += dirlen + strlen(descptr) + 1;
	}
	return qfalse;
}


/*
===============
UI_MainMenu

The main menu only comes up when not in a game,
so make sure that the attract loop server is down
and that local cinematics are killed
===============
*/
void UI_MainMenu( void ) {
	int		y;
	qboolean teamArena = qfalse;
	int		style = UI_CENTER | UI_DROPSHADOW;

	trap_Cvar_Set( "sv_killserver", "1" );

	if( !uis.demoversion && !ui_cdkeychecked.integer ) {
		char	key[17];

		trap_GetCDKey( key, sizeof(key) );
		if( trap_VerifyCDKey( key, NULL ) == qfalse ) {
			UI_CDKeyMenu();
			return;
		}
	}
	
	memset( &s_main, 0 ,sizeof(mainmenu_t) );
	memset( &s_errorMessage, 0 ,sizeof(errorMessage_t) );

	// com_errorMessage would need that too
	MainMenu_Cache();
	
	trap_Cvar_VariableStringBuffer( "com_errorMessage", s_errorMessage.errorMessage, sizeof(s_errorMessage.errorMessage) );
	if (strlen(s_errorMessage.errorMessage))
	{	
		s_errorMessage.menu.draw = Main_MenuDraw;
		s_errorMessage.menu.key = ErrorMessage_Key;
		s_errorMessage.menu.fullscreen = qtrue;
		s_errorMessage.menu.wrapAround = qtrue;
		s_errorMessage.menu.showlogo = qtrue;		

		trap_Key_SetCatcher( KEYCATCH_UI );
		uis.menusp = 0;
		UI_PushMenu ( &s_errorMessage.menu );
		
		return;
	}

	s_main.menu.draw = Main_MenuDraw;
	s_main.menu.fullscreen = qtrue;
	s_main.menu.wrapAround = qtrue;
	s_main.menu.showlogo = qtrue;

	y = 134;
	s_main.singleplayer.generic.type		= MTYPE_PTEXT;
	s_main.singleplayer.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.singleplayer.generic.x			= 320;
	s_main.singleplayer.generic.y			= y;
	s_main.singleplayer.generic.id			= ID_SINGLEPLAYER;
	s_main.singleplayer.generic.callback	= Main_MenuEvent; 
	s_main.singleplayer.string				= "SINGLE PLAYER";
	s_main.singleplayer.color				= color_red;
	s_main.singleplayer.style				= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.multiplayer.generic.type			= MTYPE_PTEXT;
	s_main.multiplayer.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.multiplayer.generic.x			= 320;
	s_main.multiplayer.generic.y			= y;
	s_main.multiplayer.generic.id			= ID_MULTIPLAYER;
	s_main.multiplayer.generic.callback		= Main_MenuEvent; 
	s_main.multiplayer.string				= "MULTIPLAYER";
	s_main.multiplayer.color				= color_red;
	s_main.multiplayer.style				= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.setup.generic.type				= MTYPE_PTEXT;
	s_main.setup.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.setup.generic.x					= 320;
	s_main.setup.generic.y					= y;
	s_main.setup.generic.id					= ID_SETUP;
	s_main.setup.generic.callback			= Main_MenuEvent; 
	s_main.setup.string						= "SETUP";
	s_main.setup.color						= color_red;
	s_main.setup.style						= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.demos.generic.type				= MTYPE_PTEXT;
	s_main.demos.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.demos.generic.x					= 320;
	s_main.demos.generic.y					= y;
	s_main.demos.generic.id					= ID_DEMOS;
	s_main.demos.generic.callback			= Main_MenuEvent; 
	s_main.demos.string						= "DEMOS";
	s_main.demos.color						= color_red;
	s_main.demos.style						= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.cinematics.generic.type			= MTYPE_PTEXT;
	s_main.cinematics.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.cinematics.generic.x				= 320;
	s_main.cinematics.generic.y				= y;
	s_main.cinematics.generic.id			= ID_CINEMATICS;
	s_main.cinematics.generic.callback		= Main_MenuEvent; 
	s_main.cinematics.string				= "CINEMATICS";
	s_main.cinematics.color					= color_red;
	s_main.cinematics.style					= style;

	if (UI_TeamArenaExists()) {
		teamArena = qtrue;
		y += MAIN_MENU_VERTICAL_SPACING;
		s_main.teamArena.generic.type			= MTYPE_PTEXT;
		s_main.teamArena.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		s_main.teamArena.generic.x				= 320;
		s_main.teamArena.generic.y				= y;
		s_main.teamArena.generic.id				= ID_TEAMARENA;
		s_main.teamArena.generic.callback		= Main_MenuEvent; 
		s_main.teamArena.string					= "TEAM ARENA";
		s_main.teamArena.color					= color_red;
		s_main.teamArena.style					= style;
	}

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.mods.generic.type			= MTYPE_PTEXT;
	s_main.mods.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.mods.generic.x				= 320;
	s_main.mods.generic.y				= y;
	s_main.mods.generic.id				= ID_MODS;
	s_main.mods.generic.callback		= Main_MenuEvent; 
	s_main.mods.string					= "MODS";
	s_main.mods.color					= color_red;
	s_main.mods.style					= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.agentControl.generic.type		= MTYPE_PTEXT;
	s_main.agentControl.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.agentControl.generic.x			= 320;
	s_main.agentControl.generic.y			= y;
	s_main.agentControl.generic.id			= ID_AGENT_CONTROL;
	s_main.agentControl.generic.callback	= Main_MenuEvent;
	s_main.agentControl.string				= "AI CONTROL";
	s_main.agentControl.color				= color_red;
	s_main.agentControl.style				= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.exit.generic.type				= MTYPE_PTEXT;
	s_main.exit.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.exit.generic.x					= 320;
	s_main.exit.generic.y					= y;
	s_main.exit.generic.id					= ID_EXIT;
	s_main.exit.generic.callback			= Main_MenuEvent; 
	s_main.exit.string						= "EXIT";
	s_main.exit.color						= color_red;
	s_main.exit.style						= style;

	Menu_AddItem( &s_main.menu,	&s_main.singleplayer );
	Menu_AddItem( &s_main.menu,	&s_main.multiplayer );
	Menu_AddItem( &s_main.menu,	&s_main.setup );
	Menu_AddItem( &s_main.menu,	&s_main.demos );
	Menu_AddItem( &s_main.menu,	&s_main.cinematics );
	if (teamArena) {
		Menu_AddItem( &s_main.menu,	&s_main.teamArena );
	}
	Menu_AddItem( &s_main.menu,	&s_main.mods );
	Menu_AddItem( &s_main.menu,	&s_main.agentControl );
	Menu_AddItem( &s_main.menu,	&s_main.exit );             

	trap_Key_SetCatcher( KEYCATCH_UI );
	uis.menusp = 0;
	UI_PushMenu ( &s_main.menu );
		
}
