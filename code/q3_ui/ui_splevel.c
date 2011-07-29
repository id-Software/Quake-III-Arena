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
=============================================================================

SINGLE PLAYER LEVEL SELECT MENU

=============================================================================
*/

#include "ui_local.h"


#define ART_LEVELFRAME_FOCUS		"menu/art/maps_select"
#define ART_LEVELFRAME_SELECTED		"menu/art/maps_selected"
#define ART_ARROW					"menu/art/narrow_0"
#define ART_ARROW_FOCUS				"menu/art/narrow_1"
#define ART_MAP_UNKNOWN				"menu/art/unknownmap"
#define ART_MAP_COMPLETE1			"menu/art/level_complete1"
#define ART_MAP_COMPLETE2			"menu/art/level_complete2"
#define ART_MAP_COMPLETE3			"menu/art/level_complete3"
#define ART_MAP_COMPLETE4			"menu/art/level_complete4"
#define ART_MAP_COMPLETE5			"menu/art/level_complete5"
#define ART_BACK0					"menu/art/back_0"
#define ART_BACK1					"menu/art/back_1"	
#define ART_FIGHT0					"menu/art/fight_0"
#define ART_FIGHT1					"menu/art/fight_1"
#define ART_RESET0					"menu/art/reset_0"
#define ART_RESET1					"menu/art/reset_1"	
#define ART_CUSTOM0					"menu/art/skirmish_0"
#define ART_CUSTOM1					"menu/art/skirmish_1"

#define ID_LEFTARROW		10
#define ID_PICTURE0			11
#define ID_PICTURE1			12
#define ID_PICTURE2			13
#define ID_PICTURE3			14
#define ID_RIGHTARROW		15
#define ID_PLAYERPIC		16
#define ID_AWARD1			17
#define ID_AWARD2			18
#define ID_AWARD3			19
#define ID_AWARD4			20
#define ID_AWARD5			21
#define ID_AWARD6			22
#define ID_BACK				23
#define ID_RESET			24
#define ID_CUSTOM			25
#define ID_NEXT				26

#define PLAYER_Y			314
#define AWARDS_Y			(PLAYER_Y + 26)


typedef struct {
	menuframework_s	menu;
	menutext_s		item_banner;
	menubitmap_s	item_leftarrow;
	menubitmap_s	item_maps[4];
	menubitmap_s	item_rightarrow;
	menubitmap_s	item_player;
	menubitmap_s	item_awards[6];
	menubitmap_s	item_back;
	menubitmap_s	item_reset;
	menubitmap_s	item_custom;
	menubitmap_s	item_next;
	menubitmap_s	item_null;

	qboolean		reinit;

	const char *	selectedArenaInfo;
	int				numMaps;
	char			levelPicNames[4][MAX_QPATH];
	char			levelNames[4][16];
	int				levelScores[4];
	int				levelScoresSkill[4];
	qhandle_t		levelSelectedPic;
	qhandle_t		levelFocusPic;
	qhandle_t		levelCompletePic[5];

	char			playerModel[MAX_QPATH];
	char			playerPicName[MAX_QPATH];
	int				awardLevels[6];
	sfxHandle_t		awardSounds[6];

	int				numBots;
	qhandle_t		botPics[7];
	char			botNames[7][10];
} levelMenuInfo_t;

static levelMenuInfo_t	levelMenuInfo;

static int	selectedArenaSet;
static int	selectedArena;
static int	currentSet;
static int	currentGame;
static int	trainingTier;
static int	finalTier;
static int	minTier;
static int	maxTier;


/*
=================
PlayerIcon
=================
*/
static void PlayerIcon( const char *modelAndSkin, char *iconName, int iconNameMaxSize ) {
	char	*skin;
	char	model[MAX_QPATH];

	Q_strncpyz( model, modelAndSkin, sizeof(model));
	skin = strrchr( model, '/' );
	if ( skin ) {
		*skin++ = '\0';
	}
	else {
		skin = "default";
	}

	Com_sprintf(iconName, iconNameMaxSize, "models/players/%s/icon_%s.tga", model, skin );

	if( !trap_R_RegisterShaderNoMip( iconName ) && Q_stricmp( skin, "default" ) != 0 ) {
		Com_sprintf(iconName, iconNameMaxSize, "models/players/%s/icon_default.tga", model );
	}
}


/*
=================
PlayerIconhandle
=================
*/
static qhandle_t PlayerIconHandle( const char *modelAndSkin ) {
	char	iconName[MAX_QPATH];

	PlayerIcon( modelAndSkin, iconName, sizeof(iconName) );
	return trap_R_RegisterShaderNoMip( iconName );
}


/*
=================
UI_SPLevelMenu_SetBots
=================
*/
static void UI_SPLevelMenu_SetBots( void ) {
	char	*p;
	char	*bot;
	char	*botInfo;
	char	bots[MAX_INFO_STRING];

	levelMenuInfo.numBots = 0;
	if ( selectedArenaSet > currentSet ) {
		return;
	}

	Q_strncpyz( bots, Info_ValueForKey( levelMenuInfo.selectedArenaInfo, "bots" ), sizeof(bots) );

	p = &bots[0];
	while( *p && levelMenuInfo.numBots < 7 ) {
		//skip spaces
		while( *p && *p == ' ' ) {
			p++;
		}
		if( !p ) {
			break;
		}

		// mark start of bot name
		bot = p;

		// skip until space of null
		while( *p && *p != ' ' ) {
			p++;
		}
		if( *p ) {
			*p++ = 0;
		}

		botInfo = UI_GetBotInfoByName( bot );
		if(!botInfo)
		{
			botInfo = UI_GetBotInfoByNumber( levelMenuInfo.numBots );
		}
	
		if( botInfo ) {
			levelMenuInfo.botPics[levelMenuInfo.numBots] = PlayerIconHandle( Info_ValueForKey( botInfo, "model" ) );
			Q_strncpyz( levelMenuInfo.botNames[levelMenuInfo.numBots], Info_ValueForKey( botInfo, "name" ), 10 );
		}
		else {
			levelMenuInfo.botPics[levelMenuInfo.numBots] = 0;
			Q_strncpyz( levelMenuInfo.botNames[levelMenuInfo.numBots], bot, 10 );
		}
		Q_CleanStr( levelMenuInfo.botNames[levelMenuInfo.numBots] );
		levelMenuInfo.numBots++;
	}
}


/*
=================
UI_SPLevelMenu_SetMenuItems
=================
*/
static void UI_SPLevelMenu_SetMenuArena( int n, int level, const char *arenaInfo ) {
	char		map[MAX_QPATH];

	Q_strncpyz( map, Info_ValueForKey( arenaInfo, "map" ), sizeof(map) );

	Q_strncpyz( levelMenuInfo.levelNames[n], map, sizeof(levelMenuInfo.levelNames[n]) );
	Q_strupr( levelMenuInfo.levelNames[n] );

	UI_GetBestScore( level, &levelMenuInfo.levelScores[n], &levelMenuInfo.levelScoresSkill[n] );
	if( levelMenuInfo.levelScores[n] > 8 ) {
		levelMenuInfo.levelScores[n] = 8;
	}

	strcpy( levelMenuInfo.levelPicNames[n], va( "levelshots/%s.tga", map ) );
	if( !trap_R_RegisterShaderNoMip( levelMenuInfo.levelPicNames[n] ) ) {
		strcpy( levelMenuInfo.levelPicNames[n], ART_MAP_UNKNOWN );
	}
	levelMenuInfo.item_maps[n].shader = 0;
	if ( selectedArenaSet > currentSet ) {
		levelMenuInfo.item_maps[n].generic.flags |= QMF_GRAYED;
	}
	else {
		levelMenuInfo.item_maps[n].generic.flags &= ~QMF_GRAYED;
	}

	levelMenuInfo.item_maps[n].generic.flags &= ~QMF_INACTIVE;
}

static void UI_SPLevelMenu_SetMenuItems( void ) {
	int			n;
	int			level;
	const char	*arenaInfo;

	if ( selectedArenaSet > currentSet ) {
		selectedArena = -1;
	}
	else if ( selectedArena == -1 ) {
		selectedArena = 0;
	}

	if( selectedArenaSet == trainingTier || selectedArenaSet == finalTier ) {
		selectedArena = 0;
	}

	if( selectedArena != -1 ) {
		trap_Cvar_SetValue( "ui_spSelection", selectedArenaSet * ARENAS_PER_TIER + selectedArena );
	}

	if( selectedArenaSet == trainingTier ) {
		arenaInfo = UI_GetSpecialArenaInfo( "training" );
		level = atoi( Info_ValueForKey( arenaInfo, "num" ) );
		UI_SPLevelMenu_SetMenuArena( 0, level, arenaInfo );
		levelMenuInfo.selectedArenaInfo = arenaInfo;

		levelMenuInfo.item_maps[0].generic.x = 256;
		Bitmap_Init( &levelMenuInfo.item_maps[0] );
		levelMenuInfo.item_maps[0].generic.bottom += 32;
		levelMenuInfo.numMaps = 1;

		levelMenuInfo.item_maps[1].generic.flags |= QMF_INACTIVE;
		levelMenuInfo.item_maps[2].generic.flags |= QMF_INACTIVE;
		levelMenuInfo.item_maps[3].generic.flags |= QMF_INACTIVE;
		levelMenuInfo.levelPicNames[1][0] = 0;
		levelMenuInfo.levelPicNames[2][0] = 0;
		levelMenuInfo.levelPicNames[3][0] = 0;
		levelMenuInfo.item_maps[1].shader = 0;
		levelMenuInfo.item_maps[2].shader = 0;
		levelMenuInfo.item_maps[3].shader = 0;
	}
	else if( selectedArenaSet == finalTier ) {
		arenaInfo = UI_GetSpecialArenaInfo( "final" );
		level = atoi( Info_ValueForKey( arenaInfo, "num" ) );
		UI_SPLevelMenu_SetMenuArena( 0, level, arenaInfo );
		levelMenuInfo.selectedArenaInfo = arenaInfo;

		levelMenuInfo.item_maps[0].generic.x = 256;
		Bitmap_Init( &levelMenuInfo.item_maps[0] );
		levelMenuInfo.item_maps[0].generic.bottom += 32;
		levelMenuInfo.numMaps = 1;

		levelMenuInfo.item_maps[1].generic.flags |= QMF_INACTIVE;
		levelMenuInfo.item_maps[2].generic.flags |= QMF_INACTIVE;
		levelMenuInfo.item_maps[3].generic.flags |= QMF_INACTIVE;
		levelMenuInfo.levelPicNames[1][0] = 0;
		levelMenuInfo.levelPicNames[2][0] = 0;
		levelMenuInfo.levelPicNames[3][0] = 0;
		levelMenuInfo.item_maps[1].shader = 0;
		levelMenuInfo.item_maps[2].shader = 0;
		levelMenuInfo.item_maps[3].shader = 0;
	}
	else {
		levelMenuInfo.item_maps[0].generic.x = 46;
		Bitmap_Init( &levelMenuInfo.item_maps[0] );
		levelMenuInfo.item_maps[0].generic.bottom += 18;
		levelMenuInfo.numMaps = 4;

		for ( n = 0; n < 4; n++ ) {
			level = selectedArenaSet * ARENAS_PER_TIER + n;
			arenaInfo = UI_GetArenaInfoByNumber( level );
			UI_SPLevelMenu_SetMenuArena( n, level, arenaInfo );
		}

		if( selectedArena != -1 ) {
			levelMenuInfo.selectedArenaInfo = UI_GetArenaInfoByNumber( selectedArenaSet * ARENAS_PER_TIER + selectedArena );
		}
	}

	// enable/disable arrows when they are valid/invalid
	if ( selectedArenaSet == minTier ) {
		levelMenuInfo.item_leftarrow.generic.flags |= ( QMF_INACTIVE | QMF_HIDDEN );
	}
	else {
		levelMenuInfo.item_leftarrow.generic.flags &= ~( QMF_INACTIVE | QMF_HIDDEN );
	}

	if ( selectedArenaSet == maxTier ) {
		levelMenuInfo.item_rightarrow.generic.flags |= ( QMF_INACTIVE | QMF_HIDDEN );
	}
	else {
		levelMenuInfo.item_rightarrow.generic.flags &= ~( QMF_INACTIVE | QMF_HIDDEN );
	}

	UI_SPLevelMenu_SetBots();
}


/*
=================
UI_SPLevelMenu_ResetEvent
=================
*/
static void UI_SPLevelMenu_ResetDraw( void ) {
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 0, "WARNING: This resets all of the", UI_CENTER|UI_SMALLFONT, color_yellow );
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 1, "single player game variables.", UI_CENTER|UI_SMALLFONT, color_yellow );
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 2, "Do this only if you want to", UI_CENTER|UI_SMALLFONT, color_yellow );
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 3, "start over from the beginning.", UI_CENTER|UI_SMALLFONT, color_yellow );
}

static void UI_SPLevelMenu_ResetAction( qboolean result ) {
	if( !result ) {
		return;
	}

	// clear game variables
	UI_NewGame();
	trap_Cvar_SetValue( "ui_spSelection", -4 );

	// make the level select menu re-initialize
	UI_PopMenu();
	UI_SPLevelMenu();
}

static void UI_SPLevelMenu_ResetEvent( void* ptr, int event )
{
	if (event != QM_ACTIVATED) {
		return;
	}

	UI_ConfirmMenu( "RESET GAME?", UI_SPLevelMenu_ResetDraw, UI_SPLevelMenu_ResetAction );
}


/*
=================
UI_SPLevelMenu_LevelEvent
=================
*/
static void UI_SPLevelMenu_LevelEvent( void* ptr, int notification ) {
	if (notification != QM_ACTIVATED) {
		return;
	}

	if ( selectedArenaSet == trainingTier || selectedArenaSet == finalTier ) {
		return;
	}

	selectedArena = ((menucommon_s*)ptr)->id - ID_PICTURE0;
	levelMenuInfo.selectedArenaInfo = UI_GetArenaInfoByNumber( selectedArenaSet * ARENAS_PER_TIER + selectedArena );
	UI_SPLevelMenu_SetBots();

	trap_Cvar_SetValue( "ui_spSelection", selectedArenaSet * ARENAS_PER_TIER + selectedArena );
}


/*
=================
UI_SPLevelMenu_LeftArrowEvent
=================
*/
static void UI_SPLevelMenu_LeftArrowEvent( void* ptr, int notification ) {
	if (notification != QM_ACTIVATED) {
		return;
	}

	if ( selectedArenaSet == minTier ) {
		return;
	}

	selectedArenaSet--;
	UI_SPLevelMenu_SetMenuItems();
}


/*
=================
UI_SPLevelMenu_RightArrowEvent
=================
*/
static void UI_SPLevelMenu_RightArrowEvent( void* ptr, int notification ) {
	if (notification != QM_ACTIVATED) {
		return;
	}

	if ( selectedArenaSet == maxTier ) {
		return;
	}

	selectedArenaSet++;
	UI_SPLevelMenu_SetMenuItems();
}


/*
=================
UI_SPLevelMenu_PlayerEvent
=================
*/
static void UI_SPLevelMenu_PlayerEvent( void* ptr, int notification ) {
	if (notification != QM_ACTIVATED) {
		return;
	}

	UI_PlayerSettingsMenu();
}


/*
=================
UI_SPLevelMenu_AwardEvent
=================
*/
static void UI_SPLevelMenu_AwardEvent( void* ptr, int notification ) {
	int		n;

	if (notification != QM_ACTIVATED) {
		return;
	}

	n = ((menucommon_s*)ptr)->id - ID_AWARD1;
	trap_S_StartLocalSound( levelMenuInfo.awardSounds[n], CHAN_ANNOUNCER );
}


/*
=================
UI_SPLevelMenu_NextEvent
=================
*/
static void UI_SPLevelMenu_NextEvent( void* ptr, int notification ) {
	if (notification != QM_ACTIVATED) {
		return;
	}

	if ( selectedArenaSet > currentSet ) {
		return;
	}

	if ( selectedArena == -1 ) {
		selectedArena = 0;
	}

	UI_SPSkillMenu( levelMenuInfo.selectedArenaInfo );
}


/*
=================
UI_SPLevelMenu_BackEvent
=================
*/
static void UI_SPLevelMenu_BackEvent( void* ptr, int notification ) {
	if (notification != QM_ACTIVATED) {
		return;
	}

	if ( selectedArena == -1 ) {
		selectedArena = 0;
	}

	UI_PopMenu();
}


/*
=================
UI_SPLevelMenu_CustomEvent
=================
*/
static void UI_SPLevelMenu_CustomEvent( void* ptr, int notification ) {
	if (notification != QM_ACTIVATED) {
		return;
	}

	UI_StartServerMenu( qfalse );
}


/*
=================
UI_SPLevelMenu_MenuDraw
=================
*/
#define LEVEL_DESC_LEFT_MARGIN		332

static void UI_SPLevelMenu_MenuDraw( void ) {
	int				n, i;
	int				x, y;
	vec4_t			color;
	int				level;
//	int				fraglimit;
	int				pad;
	char			buf[MAX_INFO_VALUE];
	char			string[64];

	if(	levelMenuInfo.reinit ) {
		UI_PopMenu();
		UI_SPLevelMenu();
		return;
	}

	// draw player name
	trap_Cvar_VariableStringBuffer( "name", string, 32 );
	Q_CleanStr( string );
	UI_DrawProportionalString( 320, PLAYER_Y, string, UI_CENTER|UI_SMALLFONT, color_orange );

	// check for model changes
	trap_Cvar_VariableStringBuffer( "model", buf, sizeof(buf) );
	if( Q_stricmp( buf, levelMenuInfo.playerModel ) != 0 ) {
		Q_strncpyz( levelMenuInfo.playerModel, buf, sizeof(levelMenuInfo.playerModel) );
		PlayerIcon( levelMenuInfo.playerModel, levelMenuInfo.playerPicName, sizeof(levelMenuInfo.playerPicName) );
		levelMenuInfo.item_player.shader = 0;
	}

	// standard menu drawing
	Menu_Draw( &levelMenuInfo.menu );

	// draw player award levels
	y = AWARDS_Y;
	i = 0;
	for( n = 0; n < 6; n++ ) {
		level = levelMenuInfo.awardLevels[n];
		if( level > 0 ) {
			if( i & 1 ) {
				x = 224 - (i - 1 ) / 2 * (48 + 16);
			}
			else {
				x = 368 + i / 2 * (48 + 16);
			}
			i++;

			if( level == 1 ) {
				continue;
			}

			if( level >= 1000000 ) {
				Com_sprintf( string, sizeof(string), "%im", level / 1000000 );
			}
			else if( level >= 1000 ) {
				Com_sprintf( string, sizeof(string), "%ik", level / 1000 );
			}
			else {
				Com_sprintf( string, sizeof(string), "%i", level );
			}

			UI_DrawString( x + 24, y + 48, string, UI_CENTER, color_yellow );
		}
	}

	UI_DrawProportionalString( 18, 38, va( "Tier %i", selectedArenaSet + 1 ), UI_LEFT|UI_SMALLFONT, color_orange );

	for ( n = 0; n < levelMenuInfo.numMaps; n++ ) {
		x = levelMenuInfo.item_maps[n].generic.x;
		y = levelMenuInfo.item_maps[n].generic.y;
		UI_FillRect( x, y + 96, 128, 18, color_black );
	}

	if ( selectedArenaSet > currentSet ) {
		UI_DrawProportionalString( 320, 216, "ACCESS DENIED", UI_CENTER|UI_BIGFONT, color_red );
		return;
	}

	// show levelshots for levels of current tier
	Vector4Copy( color_white, color );
	color[3] = 0.5+0.5*sin(uis.realtime/PULSE_DIVISOR);
	for ( n = 0; n < levelMenuInfo.numMaps; n++ ) {
		x = levelMenuInfo.item_maps[n].generic.x;
		y = levelMenuInfo.item_maps[n].generic.y;

		UI_DrawString( x + 64, y + 96, levelMenuInfo.levelNames[n], UI_CENTER|UI_SMALLFONT, color_orange );

		if( levelMenuInfo.levelScores[n] == 1 ) {
			UI_DrawHandlePic( x, y, 128, 96, levelMenuInfo.levelCompletePic[levelMenuInfo.levelScoresSkill[n] - 1] ); 
		}

		if ( n == selectedArena ) {
			if( Menu_ItemAtCursor( &levelMenuInfo.menu ) == &levelMenuInfo.item_maps[n] ) {
				trap_R_SetColor( color );
			}
			UI_DrawHandlePic( x-1, y-1, 130, 130 - 14, levelMenuInfo.levelSelectedPic ); 
			trap_R_SetColor( NULL );
		}
		else if( Menu_ItemAtCursor( &levelMenuInfo.menu ) == &levelMenuInfo.item_maps[n] ) {
			trap_R_SetColor( color );
			UI_DrawHandlePic( x-31, y-30, 256, 256-27, levelMenuInfo.levelFocusPic); 
			trap_R_SetColor( NULL );
		}
	}

	// show map name and long name of selected level
	y = 192;
	Q_strncpyz( buf, Info_ValueForKey( levelMenuInfo.selectedArenaInfo, "map" ), 20 );
	Q_strupr( buf );
	Com_sprintf( string, sizeof(string), "%s: %s", buf, Info_ValueForKey( levelMenuInfo.selectedArenaInfo, "longname" ) );
	UI_DrawProportionalString( 320, y, string, UI_CENTER|UI_SMALLFONT, color_orange );

//	fraglimit = atoi( Info_ValueForKey( levelMenuInfo.selectedArenaInfo, "fraglimit" ) );
//	UI_DrawString( 18, 212, va("Frags %i", fraglimit) , UI_LEFT|UI_SMALLFONT, color_orange );

	// draw bot opponents
	y += 24;
	pad = (7 - levelMenuInfo.numBots) * (64 + 26) / 2;
	for( n = 0; n < levelMenuInfo.numBots; n++ ) {
		x = 18 + pad + (64 + 26) * n;
		if( levelMenuInfo.botPics[n] ) {
			UI_DrawHandlePic( x, y, 64, 64, levelMenuInfo.botPics[n]);
		}
		else {
			UI_FillRect( x, y, 64, 64, color_black );
			UI_DrawProportionalString( x+22, y+18, "?", UI_BIGFONT, color_orange );
		}
		UI_DrawString( x, y + 64, levelMenuInfo.botNames[n], UI_SMALLFONT|UI_LEFT, color_orange );
	}
}


/*
=================
UI_SPLevelMenu_Cache
=================
*/
void UI_SPLevelMenu_Cache( void ) {
	int				n;

	trap_R_RegisterShaderNoMip( ART_LEVELFRAME_FOCUS );
	trap_R_RegisterShaderNoMip( ART_LEVELFRAME_SELECTED );
	trap_R_RegisterShaderNoMip( ART_ARROW );
	trap_R_RegisterShaderNoMip( ART_ARROW_FOCUS );
	trap_R_RegisterShaderNoMip( ART_MAP_UNKNOWN );
	trap_R_RegisterShaderNoMip( ART_MAP_COMPLETE1 );
	trap_R_RegisterShaderNoMip( ART_MAP_COMPLETE2 );
	trap_R_RegisterShaderNoMip( ART_MAP_COMPLETE3 );
	trap_R_RegisterShaderNoMip( ART_MAP_COMPLETE4 );
	trap_R_RegisterShaderNoMip( ART_MAP_COMPLETE5 );
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FIGHT0 );
	trap_R_RegisterShaderNoMip( ART_FIGHT1 );
	trap_R_RegisterShaderNoMip( ART_RESET0 );
	trap_R_RegisterShaderNoMip( ART_RESET1 );
	trap_R_RegisterShaderNoMip( ART_CUSTOM0 );
	trap_R_RegisterShaderNoMip( ART_CUSTOM1 );

	for( n = 0; n < 6; n++ ) {
		trap_R_RegisterShaderNoMip( ui_medalPicNames[n] );
		levelMenuInfo.awardSounds[n] = trap_S_RegisterSound( ui_medalSounds[n], qfalse );
	}

	levelMenuInfo.levelSelectedPic = trap_R_RegisterShaderNoMip( ART_LEVELFRAME_SELECTED );
	levelMenuInfo.levelFocusPic = trap_R_RegisterShaderNoMip( ART_LEVELFRAME_FOCUS );
	levelMenuInfo.levelCompletePic[0] = trap_R_RegisterShaderNoMip( ART_MAP_COMPLETE1 );
	levelMenuInfo.levelCompletePic[1] = trap_R_RegisterShaderNoMip( ART_MAP_COMPLETE2 );
	levelMenuInfo.levelCompletePic[2] = trap_R_RegisterShaderNoMip( ART_MAP_COMPLETE3 );
	levelMenuInfo.levelCompletePic[3] = trap_R_RegisterShaderNoMip( ART_MAP_COMPLETE4 );
	levelMenuInfo.levelCompletePic[4] = trap_R_RegisterShaderNoMip( ART_MAP_COMPLETE5 );
}


/*
=================
UI_SPLevelMenu_Init
=================
*/
static void UI_SPLevelMenu_Init( void ) {
	int		skill;
	int		n;
	int		x, y;
	int		count;
	char	buf[MAX_QPATH];

	skill = (int)trap_Cvar_VariableValue( "g_spSkill" );
	if( skill < 1 || skill > 5 ) {
		trap_Cvar_Set( "g_spSkill", "2" );
	}

	memset( &levelMenuInfo, 0, sizeof(levelMenuInfo) );
	levelMenuInfo.menu.fullscreen = qtrue;
	levelMenuInfo.menu.wrapAround = qtrue;
	levelMenuInfo.menu.draw = UI_SPLevelMenu_MenuDraw;

	UI_SPLevelMenu_Cache();

	levelMenuInfo.item_banner.generic.type			= MTYPE_BTEXT;
	levelMenuInfo.item_banner.generic.x				= 320;
	levelMenuInfo.item_banner.generic.y				= 16;
	levelMenuInfo.item_banner.string				= "CHOOSE LEVEL";
	levelMenuInfo.item_banner.color					= color_red;
	levelMenuInfo.item_banner.style					= UI_CENTER;

	levelMenuInfo.item_leftarrow.generic.type		= MTYPE_BITMAP;
	levelMenuInfo.item_leftarrow.generic.name		= ART_ARROW;
	levelMenuInfo.item_leftarrow.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	levelMenuInfo.item_leftarrow.generic.x			= 18;
	levelMenuInfo.item_leftarrow.generic.y			= 64;
	levelMenuInfo.item_leftarrow.generic.callback	= UI_SPLevelMenu_LeftArrowEvent;
	levelMenuInfo.item_leftarrow.generic.id			= ID_LEFTARROW;
	levelMenuInfo.item_leftarrow.width				= 16;
	levelMenuInfo.item_leftarrow.height				= 114;
	levelMenuInfo.item_leftarrow.focuspic			= ART_ARROW_FOCUS;

	levelMenuInfo.item_maps[0].generic.type			= MTYPE_BITMAP;
	levelMenuInfo.item_maps[0].generic.name			= levelMenuInfo.levelPicNames[0];
	levelMenuInfo.item_maps[0].generic.flags		= QMF_LEFT_JUSTIFY;
	levelMenuInfo.item_maps[0].generic.x			= 46;
	levelMenuInfo.item_maps[0].generic.y			= 64;
	levelMenuInfo.item_maps[0].generic.id			= ID_PICTURE0;
	levelMenuInfo.item_maps[0].generic.callback		= UI_SPLevelMenu_LevelEvent;
	levelMenuInfo.item_maps[0].width				= 128;
	levelMenuInfo.item_maps[0].height				= 96;

	levelMenuInfo.item_maps[1].generic.type			= MTYPE_BITMAP;
	levelMenuInfo.item_maps[1].generic.name			= levelMenuInfo.levelPicNames[1];
	levelMenuInfo.item_maps[1].generic.flags		= QMF_LEFT_JUSTIFY;
	levelMenuInfo.item_maps[1].generic.x			= 186;
	levelMenuInfo.item_maps[1].generic.y			= 64;
	levelMenuInfo.item_maps[1].generic.id			= ID_PICTURE1;
	levelMenuInfo.item_maps[1].generic.callback		= UI_SPLevelMenu_LevelEvent;
	levelMenuInfo.item_maps[1].width				= 128;
	levelMenuInfo.item_maps[1].height				= 96;

	levelMenuInfo.item_maps[2].generic.type			= MTYPE_BITMAP;
	levelMenuInfo.item_maps[2].generic.name			= levelMenuInfo.levelPicNames[2];
	levelMenuInfo.item_maps[2].generic.flags		= QMF_LEFT_JUSTIFY;
	levelMenuInfo.item_maps[2].generic.x			= 326;
	levelMenuInfo.item_maps[2].generic.y			= 64;
	levelMenuInfo.item_maps[2].generic.id			= ID_PICTURE2;
	levelMenuInfo.item_maps[2].generic.callback		= UI_SPLevelMenu_LevelEvent;
	levelMenuInfo.item_maps[2].width				= 128;
	levelMenuInfo.item_maps[2].height				= 96;

	levelMenuInfo.item_maps[3].generic.type			= MTYPE_BITMAP;
	levelMenuInfo.item_maps[3].generic.name			= levelMenuInfo.levelPicNames[3];
	levelMenuInfo.item_maps[3].generic.flags		= QMF_LEFT_JUSTIFY;
	levelMenuInfo.item_maps[3].generic.x			= 466;
	levelMenuInfo.item_maps[3].generic.y			= 64;
	levelMenuInfo.item_maps[3].generic.id			= ID_PICTURE3;
	levelMenuInfo.item_maps[3].generic.callback		= UI_SPLevelMenu_LevelEvent;
	levelMenuInfo.item_maps[3].width				= 128;
	levelMenuInfo.item_maps[3].height				= 96;

	levelMenuInfo.item_rightarrow.generic.type		= MTYPE_BITMAP;
	levelMenuInfo.item_rightarrow.generic.name		= ART_ARROW;
	levelMenuInfo.item_rightarrow.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	levelMenuInfo.item_rightarrow.generic.x			= 606;
	levelMenuInfo.item_rightarrow.generic.y			= 64;
	levelMenuInfo.item_rightarrow.generic.callback	= UI_SPLevelMenu_RightArrowEvent;
	levelMenuInfo.item_rightarrow.generic.id		= ID_RIGHTARROW;
	levelMenuInfo.item_rightarrow.width				= -16;
	levelMenuInfo.item_rightarrow.height			= 114;
	levelMenuInfo.item_rightarrow.focuspic			= ART_ARROW_FOCUS;

	trap_Cvar_VariableStringBuffer( "model", levelMenuInfo.playerModel, sizeof(levelMenuInfo.playerModel) );
	PlayerIcon( levelMenuInfo.playerModel, levelMenuInfo.playerPicName, sizeof(levelMenuInfo.playerPicName) );
	levelMenuInfo.item_player.generic.type			= MTYPE_BITMAP;
	levelMenuInfo.item_player.generic.name			= levelMenuInfo.playerPicName;
	levelMenuInfo.item_player.generic.flags			= QMF_LEFT_JUSTIFY|QMF_MOUSEONLY;
	levelMenuInfo.item_player.generic.x				= 288;
	levelMenuInfo.item_player.generic.y				= AWARDS_Y;
	levelMenuInfo.item_player.generic.id			= ID_PLAYERPIC;
	levelMenuInfo.item_player.generic.callback		= UI_SPLevelMenu_PlayerEvent;
	levelMenuInfo.item_player.width					= 64;
	levelMenuInfo.item_player.height				= 64;

	for( n = 0; n < 6; n++ ) {
		levelMenuInfo.awardLevels[n] = UI_GetAwardLevel( n );
	}
	levelMenuInfo.awardLevels[AWARD_FRAGS] = 100 * (levelMenuInfo.awardLevels[AWARD_FRAGS] / 100);

	y = AWARDS_Y;
	count = 0;
	for( n = 0; n < 6; n++ ) {
		if( levelMenuInfo.awardLevels[n] ) {
			if( count & 1 ) {
				x = 224 - (count - 1 ) / 2 * (48 + 16);
			}
			else {
				x = 368 + count / 2 * (48 + 16);
			}

			levelMenuInfo.item_awards[count].generic.type		= MTYPE_BITMAP;
			levelMenuInfo.item_awards[count].generic.name		= ui_medalPicNames[n];
			levelMenuInfo.item_awards[count].generic.flags		= QMF_LEFT_JUSTIFY|QMF_SILENT|QMF_MOUSEONLY;
			levelMenuInfo.item_awards[count].generic.x			= x;
			levelMenuInfo.item_awards[count].generic.y			= y;
			levelMenuInfo.item_awards[count].generic.id			= ID_AWARD1 + n;
			levelMenuInfo.item_awards[count].generic.callback	= UI_SPLevelMenu_AwardEvent;
			levelMenuInfo.item_awards[count].width				= 48;
			levelMenuInfo.item_awards[count].height				= 48;
			count++;
		}
	}

	levelMenuInfo.item_back.generic.type			= MTYPE_BITMAP;
	levelMenuInfo.item_back.generic.name			= ART_BACK0;
	levelMenuInfo.item_back.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	levelMenuInfo.item_back.generic.x				= 0;
	levelMenuInfo.item_back.generic.y				= 480-64;
	levelMenuInfo.item_back.generic.callback		= UI_SPLevelMenu_BackEvent;
	levelMenuInfo.item_back.generic.id				= ID_BACK;
	levelMenuInfo.item_back.width					= 128;
	levelMenuInfo.item_back.height					= 64;
	levelMenuInfo.item_back.focuspic				= ART_BACK1;

	levelMenuInfo.item_reset.generic.type			= MTYPE_BITMAP;
	levelMenuInfo.item_reset.generic.name			= ART_RESET0;
	levelMenuInfo.item_reset.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	levelMenuInfo.item_reset.generic.x				= 170;
	levelMenuInfo.item_reset.generic.y				= 480-64;
	levelMenuInfo.item_reset.generic.callback		= UI_SPLevelMenu_ResetEvent;
	levelMenuInfo.item_reset.generic.id				= ID_RESET;
	levelMenuInfo.item_reset.width					= 128;
	levelMenuInfo.item_reset.height					= 64;
	levelMenuInfo.item_reset.focuspic				= ART_RESET1;

	levelMenuInfo.item_custom.generic.type			= MTYPE_BITMAP;
	levelMenuInfo.item_custom.generic.name			= ART_CUSTOM0;
	levelMenuInfo.item_custom.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	levelMenuInfo.item_custom.generic.x				= 342;
	levelMenuInfo.item_custom.generic.y				= 480-64;
	levelMenuInfo.item_custom.generic.callback		= UI_SPLevelMenu_CustomEvent;
	levelMenuInfo.item_custom.generic.id			= ID_CUSTOM;
	levelMenuInfo.item_custom.width					= 128;
	levelMenuInfo.item_custom.height				= 64;
	levelMenuInfo.item_custom.focuspic				= ART_CUSTOM1;

	levelMenuInfo.item_next.generic.type			= MTYPE_BITMAP;
	levelMenuInfo.item_next.generic.name			= ART_FIGHT0;
	levelMenuInfo.item_next.generic.flags			= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	levelMenuInfo.item_next.generic.x				= 640;
	levelMenuInfo.item_next.generic.y				= 480-64;
	levelMenuInfo.item_next.generic.callback		= UI_SPLevelMenu_NextEvent;
	levelMenuInfo.item_next.generic.id				= ID_NEXT;
	levelMenuInfo.item_next.width					= 128;
	levelMenuInfo.item_next.height					= 64;
	levelMenuInfo.item_next.focuspic				= ART_FIGHT1;

	levelMenuInfo.item_null.generic.type			= MTYPE_BITMAP;
	levelMenuInfo.item_null.generic.flags			= QMF_LEFT_JUSTIFY|QMF_MOUSEONLY|QMF_SILENT;
	levelMenuInfo.item_null.generic.x				= 0;
	levelMenuInfo.item_null.generic.y				= 0;
	levelMenuInfo.item_null.width					= 640;
	levelMenuInfo.item_null.height					= 480;

	Menu_AddItem( &levelMenuInfo.menu, &levelMenuInfo.item_banner );

	Menu_AddItem( &levelMenuInfo.menu, &levelMenuInfo.item_leftarrow );
	Menu_AddItem( &levelMenuInfo.menu, &levelMenuInfo.item_maps[0] );
	Menu_AddItem( &levelMenuInfo.menu, &levelMenuInfo.item_maps[1] );
	Menu_AddItem( &levelMenuInfo.menu, &levelMenuInfo.item_maps[2] );
	Menu_AddItem( &levelMenuInfo.menu, &levelMenuInfo.item_maps[3] );
	levelMenuInfo.item_maps[0].generic.bottom += 18;
	levelMenuInfo.item_maps[1].generic.bottom += 18;
	levelMenuInfo.item_maps[2].generic.bottom += 18;
	levelMenuInfo.item_maps[3].generic.bottom += 18;
	Menu_AddItem( &levelMenuInfo.menu, &levelMenuInfo.item_rightarrow );

	Menu_AddItem( &levelMenuInfo.menu, &levelMenuInfo.item_player );

	for( n = 0; n < count; n++ ) {
		Menu_AddItem( &levelMenuInfo.menu, &levelMenuInfo.item_awards[n] );
	}
	Menu_AddItem( &levelMenuInfo.menu, &levelMenuInfo.item_back );
	Menu_AddItem( &levelMenuInfo.menu, &levelMenuInfo.item_reset );
	Menu_AddItem( &levelMenuInfo.menu, &levelMenuInfo.item_custom );
	Menu_AddItem( &levelMenuInfo.menu, &levelMenuInfo.item_next );
	Menu_AddItem( &levelMenuInfo.menu, &levelMenuInfo.item_null );

	trap_Cvar_VariableStringBuffer( "ui_spSelection", buf, sizeof(buf) );
	if( *buf ) {
		n = atoi( buf );
		selectedArenaSet = n / ARENAS_PER_TIER;
		selectedArena = n % ARENAS_PER_TIER;
	}
	else {
		selectedArenaSet = currentSet;
		selectedArena = currentGame;
	}

	UI_SPLevelMenu_SetMenuItems();
}


/*
=================
UI_SPLevelMenu
=================
*/
void UI_SPLevelMenu( void ) {
	int			level;
	int			trainingLevel;
	const char	*arenaInfo;

	trainingTier = -1;
	arenaInfo = UI_GetSpecialArenaInfo( "training" );
	if( arenaInfo ) {
		minTier = trainingTier;
		trainingLevel = atoi( Info_ValueForKey( arenaInfo, "num" ) );
	}
	else {
		minTier = 0;
		trainingLevel = -2;
	}

	finalTier = UI_GetNumSPTiers();
	arenaInfo = UI_GetSpecialArenaInfo( "final" );
	if( arenaInfo ) {
		maxTier = finalTier;
	}
	else {
		maxTier = finalTier - 1;
		if( maxTier < minTier ) {
			maxTier = minTier;
		}
	}

	level = UI_GetCurrentGame();
	if ( level == -1 ) {
		level = UI_GetNumSPArenas() - 1;
		if( maxTier == finalTier ) {
			level++;
		}
	}

	if( level == trainingLevel ) {
		currentSet = -1;
		currentGame = 0;
	}
	else {
		currentSet = level / ARENAS_PER_TIER;
		currentGame = level % ARENAS_PER_TIER;
	}

	UI_SPLevelMenu_Init();
	UI_PushMenu( &levelMenuInfo.menu );
	Menu_SetCursorToItem( &levelMenuInfo.menu, &levelMenuInfo.item_next );
}


/*
=================
UI_SPLevelMenu_f
=================
*/
void UI_SPLevelMenu_f( void ) {
	trap_Key_SetCatcher( KEYCATCH_UI );
	uis.menusp = 0;
	UI_SPLevelMenu();
}


/*
=================
UI_SPLevelMenu_ReInit
=================
*/
void UI_SPLevelMenu_ReInit( void ) {
	levelMenuInfo.reinit = qtrue;
}
