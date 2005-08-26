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
=============================================================================

SINGLE PLAYER SKILL MENU

=============================================================================
*/

#include "ui_local.h"


#define ART_FRAME					"menu/art/cut_frame"
#define ART_BACK					"menu/art/back_0.tga"
#define ART_BACK_FOCUS				"menu/art/back_1.tga"
#define ART_FIGHT					"menu/art/fight_0"
#define ART_FIGHT_FOCUS				"menu/art/fight_1"
#define ART_MAP_COMPLETE1			"menu/art/level_complete1"
#define ART_MAP_COMPLETE2			"menu/art/level_complete2"
#define ART_MAP_COMPLETE3			"menu/art/level_complete3"
#define ART_MAP_COMPLETE4			"menu/art/level_complete4"
#define ART_MAP_COMPLETE5			"menu/art/level_complete5"

#define ID_BABY						10
#define ID_EASY						11
#define ID_MEDIUM					12
#define ID_HARD						13
#define ID_NIGHTMARE				14
#define ID_BACK						15
#define ID_FIGHT					16


typedef struct {
	menuframework_s	menu;

	menubitmap_s	art_frame;
	menutext_s		art_banner;

	menutext_s		item_baby;
	menutext_s		item_easy;
	menutext_s		item_medium;
	menutext_s		item_hard;
	menutext_s		item_nightmare;

	menubitmap_s	art_skillPic;
	menubitmap_s	item_back;
	menubitmap_s	item_fight;

	const char		*arenaInfo;
	qhandle_t		skillpics[5];
	sfxHandle_t		nightmareSound;
	sfxHandle_t		silenceSound;
} skillMenuInfo_t;

static skillMenuInfo_t	skillMenuInfo;


static void SetSkillColor( int skill, vec4_t color ) {
	switch( skill ) {
	case 1:
		skillMenuInfo.item_baby.color = color;
		break;
	case 2:
		skillMenuInfo.item_easy.color = color;
		break;
	case 3:
		skillMenuInfo.item_medium.color = color;
		break;
	case 4:
		skillMenuInfo.item_hard.color = color;
		break;
	case 5:
		skillMenuInfo.item_nightmare.color = color;
		break;
	default:
		break;
	}
}


/*
=================
UI_SPSkillMenu_SkillEvent
=================
*/
static void UI_SPSkillMenu_SkillEvent( void *ptr, int notification ) {
	int		id;
	int		skill;

	if (notification != QM_ACTIVATED)
		return;

	SetSkillColor( (int)trap_Cvar_VariableValue( "g_spSkill" ), color_red );

	id = ((menucommon_s*)ptr)->id;
	skill = id - ID_BABY + 1;
	trap_Cvar_SetValue( "g_spSkill", skill );

	SetSkillColor( skill, color_white );
	skillMenuInfo.art_skillPic.shader = skillMenuInfo.skillpics[skill - 1];

	if( id == ID_NIGHTMARE ) {
		trap_S_StartLocalSound( skillMenuInfo.nightmareSound, CHAN_ANNOUNCER );
	}
	else {
		trap_S_StartLocalSound( skillMenuInfo.silenceSound, CHAN_ANNOUNCER );
	}
}


/*
=================
UI_SPSkillMenu_FightEvent
=================
*/
static void UI_SPSkillMenu_FightEvent( void *ptr, int notification ) {
	if (notification != QM_ACTIVATED)
		return;

	UI_SPArena_Start( skillMenuInfo.arenaInfo );
}


/*
=================
UI_SPSkillMenu_BackEvent
=================
*/
static void UI_SPSkillMenu_BackEvent( void* ptr, int notification ) {
	if (notification != QM_ACTIVATED) {
		return;
	}

	trap_S_StartLocalSound( skillMenuInfo.silenceSound, CHAN_ANNOUNCER );
	UI_PopMenu();
}


/*
=================
UI_SPSkillMenu_Key
=================
*/
static sfxHandle_t UI_SPSkillMenu_Key( int key ) {
	if( key == K_MOUSE2 || key == K_ESCAPE ) {
		trap_S_StartLocalSound( skillMenuInfo.silenceSound, CHAN_ANNOUNCER );
	}
	return Menu_DefaultKey( &skillMenuInfo.menu, key );
}


/*
=================
UI_SPSkillMenu_Cache
=================
*/
void UI_SPSkillMenu_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_FRAME );
	trap_R_RegisterShaderNoMip( ART_BACK );
	trap_R_RegisterShaderNoMip( ART_BACK_FOCUS );
	trap_R_RegisterShaderNoMip( ART_FIGHT );
	trap_R_RegisterShaderNoMip( ART_FIGHT_FOCUS );
	skillMenuInfo.skillpics[0] = trap_R_RegisterShaderNoMip( ART_MAP_COMPLETE1 );
	skillMenuInfo.skillpics[1] = trap_R_RegisterShaderNoMip( ART_MAP_COMPLETE2 );
	skillMenuInfo.skillpics[2] = trap_R_RegisterShaderNoMip( ART_MAP_COMPLETE3 );
	skillMenuInfo.skillpics[3] = trap_R_RegisterShaderNoMip( ART_MAP_COMPLETE4 );
	skillMenuInfo.skillpics[4] = trap_R_RegisterShaderNoMip( ART_MAP_COMPLETE5 );

	skillMenuInfo.nightmareSound = trap_S_RegisterSound( "sound/misc/nightmare.wav", qfalse );
	skillMenuInfo.silenceSound = trap_S_RegisterSound( "sound/misc/silence.wav", qfalse );
}


/*
=================
UI_SPSkillMenu_Init
=================
*/
static void UI_SPSkillMenu_Init( void ) {
	int		skill;

	memset( &skillMenuInfo, 0, sizeof(skillMenuInfo) );
	skillMenuInfo.menu.fullscreen = qtrue;
	skillMenuInfo.menu.key = UI_SPSkillMenu_Key;

	UI_SPSkillMenu_Cache();

	skillMenuInfo.art_frame.generic.type		= MTYPE_BITMAP;
	skillMenuInfo.art_frame.generic.name		= ART_FRAME;
	skillMenuInfo.art_frame.generic.flags		= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	skillMenuInfo.art_frame.generic.x			= 142;
	skillMenuInfo.art_frame.generic.y			= 118;
	skillMenuInfo.art_frame.width				= 359;
	skillMenuInfo.art_frame.height				= 256;

	skillMenuInfo.art_banner.generic.type		= MTYPE_BTEXT;
	skillMenuInfo.art_banner.generic.flags		= QMF_CENTER_JUSTIFY;
	skillMenuInfo.art_banner.generic.x			= 320;
	skillMenuInfo.art_banner.generic.y			= 16;
	skillMenuInfo.art_banner.string				= "DIFFICULTY";
	skillMenuInfo.art_banner.color				= color_white;
	skillMenuInfo.art_banner.style				= UI_CENTER;

	skillMenuInfo.item_baby.generic.type		= MTYPE_PTEXT;
	skillMenuInfo.item_baby.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	skillMenuInfo.item_baby.generic.x			= 320;
	skillMenuInfo.item_baby.generic.y			= 170;
	skillMenuInfo.item_baby.generic.callback	= UI_SPSkillMenu_SkillEvent;
	skillMenuInfo.item_baby.generic.id			= ID_BABY;
	skillMenuInfo.item_baby.string				= "I Can Win";
	skillMenuInfo.item_baby.color				= color_red;
	skillMenuInfo.item_baby.style				= UI_CENTER;

	skillMenuInfo.item_easy.generic.type		= MTYPE_PTEXT;
	skillMenuInfo.item_easy.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	skillMenuInfo.item_easy.generic.x			= 320;
	skillMenuInfo.item_easy.generic.y			= 198;
	skillMenuInfo.item_easy.generic.callback	= UI_SPSkillMenu_SkillEvent;
	skillMenuInfo.item_easy.generic.id			= ID_EASY;
	skillMenuInfo.item_easy.string				= "Bring It On";
	skillMenuInfo.item_easy.color				= color_red;
	skillMenuInfo.item_easy.style				= UI_CENTER;

	skillMenuInfo.item_medium.generic.type		= MTYPE_PTEXT;
	skillMenuInfo.item_medium.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	skillMenuInfo.item_medium.generic.x			= 320;
	skillMenuInfo.item_medium.generic.y			= 227;
	skillMenuInfo.item_medium.generic.callback	= UI_SPSkillMenu_SkillEvent;
	skillMenuInfo.item_medium.generic.id		= ID_MEDIUM;
	skillMenuInfo.item_medium.string			= "Hurt Me Plenty";
	skillMenuInfo.item_medium.color				= color_red;
	skillMenuInfo.item_medium.style				= UI_CENTER;

	skillMenuInfo.item_hard.generic.type		= MTYPE_PTEXT;
	skillMenuInfo.item_hard.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	skillMenuInfo.item_hard.generic.x			= 320;
	skillMenuInfo.item_hard.generic.y			= 255;
	skillMenuInfo.item_hard.generic.callback	= UI_SPSkillMenu_SkillEvent;
	skillMenuInfo.item_hard.generic.id			= ID_HARD;
	skillMenuInfo.item_hard.string				= "Hardcore";
	skillMenuInfo.item_hard.color				= color_red;
	skillMenuInfo.item_hard.style				= UI_CENTER;

	skillMenuInfo.item_nightmare.generic.type		= MTYPE_PTEXT;
	skillMenuInfo.item_nightmare.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	skillMenuInfo.item_nightmare.generic.x			= 320;
	skillMenuInfo.item_nightmare.generic.y			= 283;
	skillMenuInfo.item_nightmare.generic.callback	= UI_SPSkillMenu_SkillEvent;
	skillMenuInfo.item_nightmare.generic.id			= ID_NIGHTMARE;
	skillMenuInfo.item_nightmare.string				= "NIGHTMARE!";
	skillMenuInfo.item_nightmare.color				= color_red;
	skillMenuInfo.item_nightmare.style				= UI_CENTER;

	skillMenuInfo.item_back.generic.type		= MTYPE_BITMAP;
	skillMenuInfo.item_back.generic.name		= ART_BACK;
	skillMenuInfo.item_back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	skillMenuInfo.item_back.generic.x			= 0;
	skillMenuInfo.item_back.generic.y			= 480-64;
	skillMenuInfo.item_back.generic.callback	= UI_SPSkillMenu_BackEvent;
	skillMenuInfo.item_back.generic.id			= ID_BACK;
	skillMenuInfo.item_back.width				= 128;
	skillMenuInfo.item_back.height				= 64;
	skillMenuInfo.item_back.focuspic			= ART_BACK_FOCUS;

	skillMenuInfo.art_skillPic.generic.type		= MTYPE_BITMAP;
	skillMenuInfo.art_skillPic.generic.flags	= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	skillMenuInfo.art_skillPic.generic.x		= 320-64;
	skillMenuInfo.art_skillPic.generic.y		= 368;
	skillMenuInfo.art_skillPic.width			= 128;
	skillMenuInfo.art_skillPic.height			= 96;

	skillMenuInfo.item_fight.generic.type		= MTYPE_BITMAP;
	skillMenuInfo.item_fight.generic.name		= ART_FIGHT;
	skillMenuInfo.item_fight.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	skillMenuInfo.item_fight.generic.callback	= UI_SPSkillMenu_FightEvent;
	skillMenuInfo.item_fight.generic.id			= ID_FIGHT;
	skillMenuInfo.item_fight.generic.x			= 640;
	skillMenuInfo.item_fight.generic.y			= 480-64;
	skillMenuInfo.item_fight.width				= 128;
	skillMenuInfo.item_fight.height				= 64;
	skillMenuInfo.item_fight.focuspic			= ART_FIGHT_FOCUS;

	Menu_AddItem( &skillMenuInfo.menu, ( void * )&skillMenuInfo.art_frame );
	Menu_AddItem( &skillMenuInfo.menu, ( void * )&skillMenuInfo.art_banner );
	Menu_AddItem( &skillMenuInfo.menu, ( void * )&skillMenuInfo.item_baby );
	Menu_AddItem( &skillMenuInfo.menu, ( void * )&skillMenuInfo.item_easy );
	Menu_AddItem( &skillMenuInfo.menu, ( void * )&skillMenuInfo.item_medium );
	Menu_AddItem( &skillMenuInfo.menu, ( void * )&skillMenuInfo.item_hard );
	Menu_AddItem( &skillMenuInfo.menu, ( void * )&skillMenuInfo.item_nightmare );
	Menu_AddItem( &skillMenuInfo.menu, ( void * )&skillMenuInfo.art_skillPic );
	Menu_AddItem( &skillMenuInfo.menu, ( void * )&skillMenuInfo.item_back );
	Menu_AddItem( &skillMenuInfo.menu, ( void * )&skillMenuInfo.item_fight );

	skill = (int)Com_Clamp( 1, 5, trap_Cvar_VariableValue( "g_spSkill" ) );
	SetSkillColor( skill, color_white );
	skillMenuInfo.art_skillPic.shader = skillMenuInfo.skillpics[skill - 1];
	if( skill == 5 ) {
		trap_S_StartLocalSound( skillMenuInfo.nightmareSound, CHAN_ANNOUNCER );
	}
}


void UI_SPSkillMenu( const char *arenaInfo ) {
	UI_SPSkillMenu_Init();
	skillMenuInfo.arenaInfo = arenaInfo;
	UI_PushMenu( &skillMenuInfo.menu );
	Menu_SetCursorToItem( &skillMenuInfo.menu, &skillMenuInfo.item_fight );
}
