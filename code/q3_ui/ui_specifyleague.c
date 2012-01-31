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

#define MAX_LISTBOXITEMS		128
#define MAX_LISTBOXWIDTH		40
#define MAX_LEAGUENAME			80

#define SPECIFYLEAGUE_FRAMEL	"menu/art/frame2_l"
#define SPECIFYLEAGUE_FRAMER	"menu/art/frame1_r"
#define SPECIFYLEAGUE_BACK0		"menu/art/back_0"
#define SPECIFYLEAGUE_BACK1		"menu/art/back_1"
#define SPECIFYLEAGUE_ARROWS0	"menu/art/arrows_vert_0"
#define SPECIFYLEAGUE_UP		"menu/art/arrows_vert_top"
#define SPECIFYLEAGUE_DOWN		"menu/art/arrows_vert_bot"
#define GLOBALRANKINGS_LOGO		"menu/art/gr/grlogo"
#define GLOBALRANKINGS_LETTERS	"menu/art/gr/grletters"

#define ID_SPECIFYLEAGUENAME	100
#define ID_SPECIFYLEAGUELIST	101
#define ID_SPECIFYLEAGUEUP		102
#define ID_SPECIFYLEAGUEDOWN	103
#define ID_SPECIFYLEAGUEBACK	104

static char* specifyleague_artlist[] =
{
	SPECIFYLEAGUE_FRAMEL,
	SPECIFYLEAGUE_FRAMER,
	SPECIFYLEAGUE_ARROWS0,	
	SPECIFYLEAGUE_UP,	
	SPECIFYLEAGUE_DOWN,	
	SPECIFYLEAGUE_BACK0,	
	SPECIFYLEAGUE_BACK1,
	GLOBALRANKINGS_LOGO,
	GLOBALRANKINGS_LETTERS,
	NULL
};

static char playername[80];

typedef struct
{
	menuframework_s	menu;
	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;
	menufield_s		rankname;
	menulist_s		list;
	menubitmap_s	arrows;
	menubitmap_s	up;
	menubitmap_s	down;
	menubitmap_s	back;
	menubitmap_s	grlogo;
	menubitmap_s	grletters;
} specifyleague_t;

static specifyleague_t	s_specifyleague;


typedef struct {
	char			buff[MAX_LISTBOXWIDTH];
	char			leaguename[MAX_LEAGUENAME];
} table_t;

table_t league_table[MAX_LISTBOXITEMS];
char *leaguename_items[MAX_LISTBOXITEMS];


static void SpecifyLeague_GetList()
{
	int count = 0;
	int i;
	/* The Player Name has changed. We need to perform another search */
	Q_strncpyz( playername,
		s_specifyleague.rankname.field.buffer, 
		sizeof(playername) );

	count = trap_CL_UI_RankGetLeauges( playername );

	for(i = 0; i < count; i++)
	{
		char	s[MAX_LEAGUENAME];
		const char	*var;
		var = va( "leaguename%i", i+1 );
		trap_Cvar_VariableStringBuffer( var, s, sizeof(s) );
		Q_strncpyz(league_table[i].leaguename, s, sizeof(league_table[i].leaguename) );
		Q_strncpyz(league_table[i].buff, league_table[i].leaguename, sizeof(league_table[i].buff) );
	}

	s_specifyleague.list.numitems = count;
}

/*
=================
SpecifyLeague_Event
=================
*/
static void SpecifyLeague_Event( void* ptr, int event )
{
	int		id;
	id = ((menucommon_s*)ptr)->id;
	//if( event != QM_ACTIVATED && id != ID_SPECIFYLEAGUELIST ) {
	//	return;
	//}

	switch (id)
	{
		case ID_SPECIFYLEAGUELIST:
			if( event == QM_GOTFOCUS ) {
				//ArenaServers_UpdatePicture();
			}
		break;

		case ID_SPECIFYLEAGUEUP:
			if( event == QM_ACTIVATED )
				ScrollList_Key( &s_specifyleague.list, K_UPARROW );
		break;		
	
		case ID_SPECIFYLEAGUEDOWN:
			if( event == QM_ACTIVATED )
				ScrollList_Key( &s_specifyleague.list, K_DOWNARROW );
		break;
			
		case ID_SPECIFYLEAGUENAME:
			if( (event == QM_LOSTFOCUS) && 
				(Q_strncmp(playername, 
					s_specifyleague.rankname.field.buffer, 
					strlen(s_specifyleague.rankname.field.buffer)) != 0))
			{
				SpecifyLeague_GetList();
			}
		break;

		case ID_SPECIFYLEAGUEBACK:
			if( event == QM_ACTIVATED )
			{
				trap_Cvar_Set( "sv_leagueName", league_table[s_specifyleague.list.curvalue].leaguename);
				UI_PopMenu();
			}
		break;
	}
}

/*
=================
SpecifyLeague_MenuInit
=================
*/
void SpecifyLeague_MenuInit( void )
{
	int i;
	// zero set all our globals
	memset( &s_specifyleague, 0 ,sizeof(specifyleague_t) );

	SpecifyLeague_Cache();

	s_specifyleague.menu.wrapAround = qtrue;
	s_specifyleague.menu.fullscreen = qtrue;

	s_specifyleague.banner.generic.type	 = MTYPE_BTEXT;
	s_specifyleague.banner.generic.x     = 320;
	s_specifyleague.banner.generic.y     = 16;
	s_specifyleague.banner.string		 = "CHOOSE LEAGUE";
	s_specifyleague.banner.color  		 = color_white;
	s_specifyleague.banner.style  		 = UI_CENTER;

	s_specifyleague.framel.generic.type  = MTYPE_BITMAP;
	s_specifyleague.framel.generic.name  = SPECIFYLEAGUE_FRAMEL;
	s_specifyleague.framel.generic.flags = QMF_INACTIVE;
	s_specifyleague.framel.generic.x	 = 0;  
	s_specifyleague.framel.generic.y	 = 78;
	s_specifyleague.framel.width  	     = 256;
	s_specifyleague.framel.height  	     = 334;

	s_specifyleague.framer.generic.type  = MTYPE_BITMAP;
	s_specifyleague.framer.generic.name  = SPECIFYLEAGUE_FRAMER;
	s_specifyleague.framer.generic.flags = QMF_INACTIVE;
	s_specifyleague.framer.generic.x	 = 376;
	s_specifyleague.framer.generic.y	 = 76;
	s_specifyleague.framer.width  	     = 256;
	s_specifyleague.framer.height  	     = 334;

	s_specifyleague.grlogo.generic.type  = MTYPE_BITMAP;
	s_specifyleague.grlogo.generic.name  = GLOBALRANKINGS_LOGO;
	s_specifyleague.grlogo.generic.flags = QMF_INACTIVE;
	s_specifyleague.grlogo.generic.x	 = 0;
	s_specifyleague.grlogo.generic.y	 = 0;
	s_specifyleague.grlogo.width		 = 64;
	s_specifyleague.grlogo.height		 = 128;

	s_specifyleague.rankname.generic.type       = MTYPE_FIELD;
	s_specifyleague.rankname.generic.name       = "Player Name:";
	s_specifyleague.rankname.generic.flags      = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_specifyleague.rankname.generic.callback   = SpecifyLeague_Event;
	s_specifyleague.rankname.generic.id	        = ID_SPECIFYLEAGUENAME;
	s_specifyleague.rankname.generic.x	        = 226;
	s_specifyleague.rankname.generic.y	        = 128;
	s_specifyleague.rankname.field.widthInChars = 32;
	s_specifyleague.rankname.field.maxchars     = 80;

	s_specifyleague.list.generic.type			= MTYPE_SCROLLLIST;
	s_specifyleague.list.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_specifyleague.list.generic.id				= ID_SPECIFYLEAGUELIST;
	s_specifyleague.list.generic.callback		= SpecifyLeague_Event;
	s_specifyleague.list.generic.x				= 160;
	s_specifyleague.list.generic.y				= 200;
	s_specifyleague.list.width					= MAX_LISTBOXWIDTH;
	s_specifyleague.list.height					= 8;
	s_specifyleague.list.itemnames				= (const char **)leaguename_items;
	s_specifyleague.list.numitems               = 0;
	for( i = 0; i < MAX_LISTBOXITEMS; i++ ) {
		league_table[i].buff[0] = 0;
		league_table[i].leaguename[0] = 0;
		leaguename_items[i] = league_table[i].buff;
	}
	
	s_specifyleague.arrows.generic.type			= MTYPE_BITMAP;
	s_specifyleague.arrows.generic.name			= SPECIFYLEAGUE_ARROWS0;
	s_specifyleague.arrows.generic.flags		= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	s_specifyleague.arrows.generic.callback		= SpecifyLeague_Event;
	s_specifyleague.arrows.generic.x			= 512;
	s_specifyleague.arrows.generic.y			= 240-64+16;
	s_specifyleague.arrows.width				= 64;
	s_specifyleague.arrows.height				= 128;

	s_specifyleague.up.generic.type				= MTYPE_BITMAP;
	s_specifyleague.up.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	s_specifyleague.up.generic.callback			= SpecifyLeague_Event;
	s_specifyleague.up.generic.id				= ID_SPECIFYLEAGUEUP;
	s_specifyleague.up.generic.x				= 512;
	s_specifyleague.up.generic.y				= 240-64+16;
	s_specifyleague.up.width					= 64;
	s_specifyleague.up.height					= 64;
	s_specifyleague.up.focuspic					= SPECIFYLEAGUE_UP;

	s_specifyleague.down.generic.type			= MTYPE_BITMAP;
	s_specifyleague.down.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	s_specifyleague.down.generic.callback		= SpecifyLeague_Event;
	s_specifyleague.down.generic.id				= ID_SPECIFYLEAGUEDOWN;
	s_specifyleague.down.generic.x				= 512;
	s_specifyleague.down.generic.y				= 240+16;
	s_specifyleague.down.width					= 64;
	s_specifyleague.down.height					= 64;
	s_specifyleague.down.focuspic				= SPECIFYLEAGUE_DOWN;

	s_specifyleague.back.generic.type	  = MTYPE_BITMAP;
	s_specifyleague.back.generic.name     = SPECIFYLEAGUE_BACK0;
	s_specifyleague.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_specifyleague.back.generic.callback = SpecifyLeague_Event;
	s_specifyleague.back.generic.id	      = ID_SPECIFYLEAGUEBACK;
	s_specifyleague.back.generic.x		  = 0;
	s_specifyleague.back.generic.y		  = 480-64;
	s_specifyleague.back.width  		  = 128;
	s_specifyleague.back.height  		  = 64;
	s_specifyleague.back.focuspic         = SPECIFYLEAGUE_BACK1;

	Menu_AddItem( &s_specifyleague.menu, &s_specifyleague.banner );
	Menu_AddItem( &s_specifyleague.menu, &s_specifyleague.framel );
	Menu_AddItem( &s_specifyleague.menu, &s_specifyleague.framer );
	Menu_AddItem( &s_specifyleague.menu, &s_specifyleague.grlogo );
	Menu_AddItem( &s_specifyleague.menu, &s_specifyleague.rankname );
	Menu_AddItem( &s_specifyleague.menu, &s_specifyleague.list );
	Menu_AddItem( &s_specifyleague.menu, &s_specifyleague.arrows );
	Menu_AddItem( &s_specifyleague.menu, &s_specifyleague.up );
	Menu_AddItem( &s_specifyleague.menu, &s_specifyleague.down );
	Menu_AddItem( &s_specifyleague.menu, &s_specifyleague.back );


	// initialize any menu variables
	Q_strncpyz( s_specifyleague.rankname.field.buffer, 
		UI_Cvar_VariableString("name"), 
		sizeof(s_specifyleague.rankname.field.buffer) );

	Q_strncpyz( playername,
		UI_Cvar_VariableString("name"), 
		sizeof(playername) );

	SpecifyLeague_GetList();
}

/*
=================
SpecifyLeague_Cache
=================
*/
void SpecifyLeague_Cache( void )
{
	int	i;

	// touch all our pics
	for (i=0; ;i++)
	{
		if (!specifyleague_artlist[i])
			break;
		trap_R_RegisterShaderNoMip(specifyleague_artlist[i]);
	}
}

/*
=================
UI_SpecifyLeagueMenu
=================
*/
void UI_SpecifyLeagueMenu( void )
{
	SpecifyLeague_MenuInit();
	UI_PushMenu( &s_specifyleague.menu );
}

