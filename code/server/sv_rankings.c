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
// sv_rankings.c -- global rankings interface

#include "server.h"
#include "..\rankings\1.0\gr\grapi.h"
#include "..\rankings\1.0\gr\grlog.h"

typedef struct
{
	GR_CONTEXT		context;
	uint64_t        game_id;
	uint64_t		match;
	uint64_t		player_id;
	GR_PLAYER_TOKEN     token;
	grank_status_t	grank_status;
	grank_status_t	final_status;	// status to set after cleanup
	uint32_t        grank;          // global rank
	char			name[32];
} ranked_player_t;

static int				s_rankings_contexts = 0;
static qboolean			s_rankings_active = qfalse;
static GR_CONTEXT		s_server_context = 0;
static uint64_t			s_server_match = 0;
static char*			s_rankings_game_key = NULL;
static uint64_t			s_rankings_game_id = 0;
static ranked_player_t*	s_ranked_players = NULL;
static qboolean			s_server_quitting = qfalse;
static const char		s_ascii_encoding[] = 
							"0123456789abcdef"
							"ghijklmnopqrstuv"
							"wxyzABCDEFGHIJKL"
							"MNOPQRSTUVWXYZ[]";

// private functions
static void		SV_RankNewGameCBF( GR_NEWGAME* gr_newgame, void* cbf_arg );
static void		SV_RankUserCBF( GR_LOGIN* gr_login, void* cbf_arg );
static void		SV_RankJoinGameCBF( GR_JOINGAME* gr_joingame, void* cbf_arg );
static void		SV_RankSendReportsCBF( GR_STATUS* gr_status, void* cbf_arg );
static void		SV_RankCleanupCBF( GR_STATUS* gr_status, void* cbf_arg );
static void		SV_RankCloseContext( ranked_player_t* ranked_player );
static int		SV_RankAsciiEncode( char* dest, const unsigned char* src, 
					int src_len );
static int		SV_RankAsciiDecode( unsigned char* dest, const char* src, 
					int src_len );
static void		SV_RankEncodeGameID( uint64_t game_id, char* result, 
					int len );
static uint64_t	SV_RankDecodePlayerID( const char* string );
static void		SV_RankDecodePlayerKey( const char* string, GR_PLAYER_TOKEN key );
static char*	SV_RankStatusString( GR_STATUS status );
static void		SV_RankError( const char* fmt, ... ) __attribute__ ((format (printf, 1, 2)));
static char     SV_RankGameKey[64];

/*
================
SV_RankBegin
================
*/
void SV_RankBegin( char *gamekey )
{
	GR_INIT		init;
	GR_STATUS	status;

	assert( s_rankings_contexts == 0 );
	assert( !s_rankings_active );
	assert( s_ranked_players == NULL );

	if( sv_enableRankings->integer == 0 || Cvar_VariableValue( "g_gametype" ) == GT_SINGLE_PLAYER )
	{
		s_rankings_active = qfalse;
		if( sv_rankingsActive->integer == 1 )
		{
			Cvar_Set( "sv_rankingsActive", "0" );
		}
		return;
	}

	// only allow official game key on pure servers
	if( strcmp(gamekey, GR_GAMEKEY) == 0 )
	{
/*
		if( Cvar_VariableValue("sv_pure") != 1 )
		{
			Cvar_Set( "sv_enableRankings", "0" );
			return;
		}
*/

		// substitute game-specific game key
		switch( (int)Cvar_VariableValue("g_gametype") )
		{
		case GT_FFA:
			gamekey = "Q3 Free For All";
			break;
		case GT_TOURNAMENT:
			gamekey = "Q3 Tournament";
			break;
		case GT_TEAM:
			gamekey = "Q3 Team Deathmatch";
			break;
		case GT_CTF:
			gamekey = "Q3 Capture the Flag";
			break;
		case GT_1FCTF:
			gamekey = "Q3 One Flag CTF";
			break;
		case GT_OBELISK:
			gamekey = "Q3 Overload";
			break;
		case GT_HARVESTER:
			gamekey = "Q3 Harvester";
			break;
		default:
			break;
		}
	}
	s_rankings_game_key = gamekey;

	// initialize rankings
	GRankLogLevel( GRLOG_OFF );
	memset(SV_RankGameKey,0,sizeof(SV_RankGameKey));
	strncpy(SV_RankGameKey,gamekey,sizeof(SV_RankGameKey)-1);
	init = GRankInit( 1, SV_RankGameKey, GR_OPT_POLL, GR_OPT_END );
	s_server_context = init.context;
	s_rankings_contexts++;
	Com_DPrintf( "SV_RankBegin(); GR_GAMEKEY is %s\n", gamekey );
	Com_DPrintf( "SV_RankBegin(); s_rankings_contexts=%d\n",s_rankings_contexts );
	Com_DPrintf( "SV_RankBegin(); s_server_context=%d\n",init.context );

	// new game
	if(!strlen(Cvar_VariableString( "sv_leagueName" )))
	{
		status = GRankNewGameAsync
			( 			
				s_server_context, 
				SV_RankNewGameCBF, 
				NULL, 
				GR_OPT_LEAGUENAME,
				(void*)(Cvar_VariableString( "sv_leagueName" )),
				GR_OPT_END 
			);
	}
	else
	{
		status = GRankNewGameAsync
			( 			
				s_server_context, 
				SV_RankNewGameCBF, 
				NULL, 
				GR_OPT_END 
			);
	}
		
	if( status != GR_STATUS_PENDING )
	{
		SV_RankError( "SV_RankBegin: Expected GR_STATUS_PENDING, got %s", 
			SV_RankStatusString( status ) );
		return;
	}

	// logging
	if( com_developer->value )
	{
		GRankLogLevel( GRLOG_TRACE );
	}
	
	// allocate rankings info for each player
	s_ranked_players = Z_Malloc( sv_maxclients->value * 
		sizeof(ranked_player_t) );
	memset( (void*)s_ranked_players, 0 ,sv_maxclients->value 
		* sizeof(ranked_player_t));
}

/*
================
SV_RankEnd
================
*/
void SV_RankEnd( void )
{
	GR_STATUS	status;
	int			i;
	
	Com_DPrintf( "SV_RankEnd();\n" );

	if( !s_rankings_active )
	{
		// cleanup after error during game
		if( s_ranked_players != NULL )
		{
			for( i = 0; i < sv_maxclients->value; i++ )
			{
				if( s_ranked_players[i].context != 0 )
				{
					SV_RankCloseContext( &(s_ranked_players[i]) );
				}
			}
		}
		if( s_server_context != 0 )
		{
			SV_RankCloseContext( NULL );
		}

		return;
	}

	for( i = 0; i < sv_maxclients->value; i++ )
	{
		if( s_ranked_players[i].grank_status == QGR_STATUS_ACTIVE )
		{
			SV_RankUserLogout( i );
			Com_DPrintf( "SV_RankEnd: SV_RankUserLogout %d\n",i );
		}
	}

	assert( s_server_context != 0 );
	
	// send match reports, proceed to SV_RankSendReportsCBF
	status = GRankSendReportsAsync
		( 
			s_server_context,
			0,
			SV_RankSendReportsCBF,
			NULL, 
			GR_OPT_END
		);
			
	if( status != GR_STATUS_PENDING )
	{
		SV_RankError( "SV_RankEnd: Expected GR_STATUS_PENDING, got %s", 
			SV_RankStatusString( status ) );
	}

	s_rankings_active = qfalse;
	Cvar_Set( "sv_rankingsActive", "0" );
}

/*
================
SV_RankPoll
================
*/
void SV_RankPoll( void )
{
	GRankPoll();
}

/*
================
SV_RankCheckInit
================
*/
qboolean SV_RankCheckInit( void )
{
	return (s_rankings_contexts > 0);
}

/*
================
SV_RankActive
================
*/
qboolean SV_RankActive( void )
{
	return s_rankings_active;
}

/*
=================
SV_RankUserStatus
=================
*/
grank_status_t SV_RankUserStatus( int index )
{
	if( !s_rankings_active )
	{
		return GR_STATUS_ERROR;
	}

	assert( s_ranked_players != NULL );
	assert( index >= 0 );
	assert( index < sv_maxclients->value );

	return s_ranked_players[index].grank_status;
}

/*
================
SV_RankUserGRank
================
*/
int SV_RankUserGrank( int index )
{
	if( !s_rankings_active )
	{
		return 0;
	}

	assert( s_ranked_players != NULL );
	assert( index >= 0 );
	assert( index < sv_maxclients->value );

	return s_ranked_players[index].grank;
}

/*
================
SV_RankUserReset
================
*/
void SV_RankUserReset( int index )
{
	if( !s_rankings_active )
	{
		return;
	}

	assert( s_ranked_players != NULL );
	assert( index >= 0 );
	assert( index < sv_maxclients->value );

	switch( s_ranked_players[index].grank_status )
	{
	case QGR_STATUS_SPECTATOR:
	case QGR_STATUS_NO_USER:
	case QGR_STATUS_BAD_PASSWORD:
	case QGR_STATUS_USER_EXISTS:
	case QGR_STATUS_NO_MEMBERSHIP:
	case QGR_STATUS_TIMEOUT:
	case QGR_STATUS_ERROR:
		s_ranked_players[index].grank_status = QGR_STATUS_NEW;
		break;
	default:
		break;
	}
}

/*
================
SV_RankUserSpectate
================
*/
void SV_RankUserSpectate( int index )
{
	if( !s_rankings_active )
	{
		return;
	}

	assert( s_ranked_players != NULL );
	assert( index >= 0 );
	assert( index < sv_maxclients->value );

	// GRANK_FIXME - check current status?
	s_ranked_players[index].grank_status = QGR_STATUS_SPECTATOR;
}

/*
================
SV_RankUserCreate
================
*/
void SV_RankUserCreate( int index, char* username, char* password, 
	char* email )
{
	GR_INIT		init;
	GR_STATUS	status;

	assert( index >= 0 );
	assert( index < sv_maxclients->value );
	assert( username != NULL );
	assert( password != NULL );
	assert( email != NULL );
	assert( s_ranked_players );
	assert( s_ranked_players[index].grank_status != QGR_STATUS_ACTIVE );
	
	Com_DPrintf( "SV_RankUserCreate( %d, %s, \"****\", %s );\n", index, 
		username, email );

	if( !s_rankings_active )
	{
		Com_DPrintf( "SV_RankUserCreate: Not ready to create\n" );
		s_ranked_players[index].grank_status = QGR_STATUS_ERROR;
		return;
	}
	
	if( s_ranked_players[index].grank_status == QGR_STATUS_ACTIVE )
	{
		Com_DPrintf( "SV_RankUserCreate: Got Create from active player\n" );
		return;
	}
	
	// get a separate context for the new user
	init = GRankInit( 0, SV_RankGameKey, GR_OPT_POLL, GR_OPT_END );
	s_ranked_players[index].context = init.context;
	s_rankings_contexts++;
	Com_DPrintf( "SV_RankUserCreate(); s_rankings_contexts=%d\n",s_rankings_contexts );
	Com_DPrintf( "SV_RankUserCreate(); s_ranked_players[%d].context=%d\n",index,init.context );
	
	// attempt to create a new account, proceed to SV_RankUserCBF
	status = GRankUserCreateAsync
		( 
			s_ranked_players[index].context, 
			username, 
			password, 
			email, 
			SV_RankUserCBF, 
			(void*)&s_ranked_players[index], 
			GR_OPT_END
		);

	if( status == GR_STATUS_PENDING )
	{
		s_ranked_players[index].grank_status = QGR_STATUS_PENDING;
		s_ranked_players[index].final_status = QGR_STATUS_NEW;
	}
	else
	{
		SV_RankError( "SV_RankUserCreate: Expected GR_STATUS_PENDING, got %s", 
			SV_RankStatusString( status ) );
	}
}

/*
================
SV_RankUserLogin
================
*/
void SV_RankUserLogin( int index, char* username, char* password )
{
	GR_INIT		init;
	GR_STATUS	status;

	assert( index >= 0 );
	assert( index < sv_maxclients->value );
	assert( username != NULL );
	assert( password != NULL );
	assert( s_ranked_players );
	assert( s_ranked_players[index].grank_status != QGR_STATUS_ACTIVE );

	Com_DPrintf( "SV_RankUserLogin( %d, %s, \"****\" );\n", index, username );

	if( !s_rankings_active )
	{
		Com_DPrintf( "SV_RankUserLogin: Not ready for login\n" );
		s_ranked_players[index].grank_status = QGR_STATUS_ERROR;
		return;
	}
	
	if( s_ranked_players[index].grank_status == QGR_STATUS_ACTIVE )
	{
		Com_DPrintf( "SV_RankUserLogin: Got Login from active player\n" );
		return;
	}
	
	// get a separate context for the new user
	init = GRankInit( 0, SV_RankGameKey, GR_OPT_POLL, GR_OPT_END );
	s_ranked_players[index].context = init.context;
	s_rankings_contexts++;
	Com_DPrintf( "SV_RankUserLogin(); s_rankings_contexts=%d\n",s_rankings_contexts );
	Com_DPrintf( "SV_RankUserLogin(); s_ranked_players[%d].context=%d\n",index,init.context );
	
	// login user, proceed to SV_RankUserCBF
	status = GRankUserLoginAsync
		(
			s_ranked_players[index].context, 
			username, 
			password, 
			SV_RankUserCBF, 
			(void*)&s_ranked_players[index], 
			GR_OPT_END 
		);

	if( status == GR_STATUS_PENDING )
	{
		s_ranked_players[index].grank_status = QGR_STATUS_PENDING;
		s_ranked_players[index].final_status = QGR_STATUS_NEW;
	}
	else
	{
		SV_RankError( "SV_RankUserLogin: Expected GR_STATUS_PENDING, got %s", 
			SV_RankStatusString( status )  );
	}
}

/*
===================
SV_RankUserValidate
===================
*/
qboolean SV_RankUserValidate( int index, const char* player_id, const char* key, int token_len, int rank, char* name )
{
	GR_INIT		init;
	GR_STATUS status;
	qboolean rVal;
	ranked_player_t* ranked_player;
	int i;

	assert( s_ranked_players );
	assert( s_ranked_players[index].grank_status != QGR_STATUS_ACTIVE );

	rVal = qfalse;
	
	if( !s_rankings_active )
	{
		Com_DPrintf( "SV_RankUserValidate: Not ready to validate\n" );
		s_ranked_players[index].grank_status = QGR_STATUS_ERROR;
		return rVal;
	}
	
	ranked_player = &(s_ranked_players[index]);
	
	if ( (player_id != NULL) && (key != NULL))
	{
		// the real player_id and key is set when SV_RankJoinGameCBF
		// is called we do this so that SV_RankUserValidate
		// can be shared by both server side login and client side login
		
		// for client side logined in players
		// server is creating GR_OPT_PLAYERCONTEXT
		init = GRankInit( 0, SV_RankGameKey, GR_OPT_POLL, GR_OPT_END );
		ranked_player->context   = init.context;
		s_rankings_contexts++;
		Com_DPrintf( "SV_RankUserValidate(); s_rankings_contexts=%d\n",s_rankings_contexts );
		Com_DPrintf( "SV_RankUserValidate(); s_ranked_players[%d].context=%d\n",index,init.context );
		
		// uudecode player id and player token
		ranked_player->player_id = SV_RankDecodePlayerID(player_id);
		Com_DPrintf( "SV_RankUserValidate(); ranked_player->player_id =%u\n", (uint32_t)ranked_player->player_id );
		SV_RankDecodePlayerKey(key, ranked_player->token);
		
		// save name and check for duplicates
		Q_strncpyz( ranked_player->name, name, sizeof(ranked_player->name) );
		for( i = 0; i < sv_maxclients->value; i++ )
		{
			if( (i != index) && (s_ranked_players[i].grank_status == QGR_STATUS_ACTIVE) && 
				(strcmp( s_ranked_players[i].name, name ) == 0) )
			{
				Com_DPrintf( "SV_RankUserValidate: Duplicate login\n" );
				ranked_player->grank_status = QGR_STATUS_NO_USER;
				ranked_player->final_status = QGR_STATUS_NEW;
				ranked_player->grank = 0;
				return qfalse;
			}
		}

		// then validate
		status  = GRankPlayerValidate(
							s_server_context,
							ranked_player->player_id, 
							ranked_player->token,
							token_len,
							GR_OPT_PLAYERCONTEXT,
							ranked_player->context,
							GR_OPT_END);
	}
	else
	{
		// make server side login (bots) happy
		status = GR_STATUS_OK;
	}

	if (status == GR_STATUS_OK)
	{
 		ranked_player->grank_status = QGR_STATUS_ACTIVE;
		ranked_player->final_status = QGR_STATUS_NEW;
		ranked_player->grank = rank;
		rVal = qtrue;
	}
	else if (status == GR_STATUS_INVALIDUSER)
	{
		ranked_player->grank_status = QGR_STATUS_INVALIDUSER;
		ranked_player->final_status = QGR_STATUS_NEW;
		ranked_player->grank = 0;
		rVal = qfalse;
	}
	else
	{
		SV_RankError( "SV_RankUserValidate: Unexpected status %s",
			SV_RankStatusString( status ) );
		s_ranked_players[index].grank_status = QGR_STATUS_ERROR;
		ranked_player->grank = 0;
	}
	
	return rVal;
}

/*
================
SV_RankUserLogout
================
*/
void SV_RankUserLogout( int index )
{
	GR_STATUS	status;
	GR_STATUS	cleanup_status;

	if( !s_rankings_active )
	{
		return;
	}

	assert( index >= 0 );
	assert( index < sv_maxclients->value );
	assert( s_ranked_players );

	if( s_ranked_players[index].context == 0 ) {
		return;
	}

	Com_DPrintf( "SV_RankUserLogout( %d );\n", index );

	// masqueraded player may not be active yet, if they fail validation, 
	// but still they have a context needs to be cleaned 
	// what matters is the s_ranked_players[index].context
	
	// send reports, proceed to SV_RankSendReportsCBF
	status = GRankSendReportsAsync
		( 
			s_ranked_players[index].context,
			0,
			SV_RankSendReportsCBF,
			(void*)&s_ranked_players[index], 
			GR_OPT_END
		);
		
	if( status == GR_STATUS_PENDING )
	{
		s_ranked_players[index].grank_status = QGR_STATUS_PENDING;
		s_ranked_players[index].final_status = QGR_STATUS_NEW;
	}
	else
	{
		SV_RankError( "SV_RankUserLogout: Expected GR_STATUS_PENDING, got %s", 
			SV_RankStatusString( status ) );

		cleanup_status = GRankCleanupAsync
			(
				s_ranked_players[index].context,
				0,
				SV_RankCleanupCBF,
				(void*)&s_ranked_players[index],
				GR_OPT_END
			);
		
		if( cleanup_status != GR_STATUS_PENDING )
		{
			SV_RankError( "SV_RankUserLogout: Expected "
				"GR_STATUS_PENDING from GRankCleanupAsync, got %s", 
				SV_RankStatusString( cleanup_status ) );
			SV_RankCloseContext( &(s_ranked_players[index]) );
		}
	}
}

/*
================
SV_RankReportInt
================
*/
void SV_RankReportInt( int index1, int index2, int key, int value, 
	qboolean accum )
{
	GR_STATUS	status;
	GR_CONTEXT	context;
	uint64_t	match;
	uint64_t	user1;
	uint64_t	user2;
	int			opt_accum;

	if( !s_rankings_active )
	{
		return;
	}

	assert( index1 >= -1 );
	assert( index1 < sv_maxclients->value );
	assert( index2 >= -1 );
	assert( index2 < sv_maxclients->value );
	assert( s_ranked_players );

//	Com_DPrintf( "SV_RankReportInt( %d, %d, %d, %d, %d );\n", index1, index2, 
//		key, value, accum );

	// get context, match, and player_id for player index1
	if( index1 == -1 )
	{
		context = s_server_context;
		match = s_server_match;
		user1 = 0;
	}
	else
	{
		if( s_ranked_players[index1].grank_status != QGR_STATUS_ACTIVE )
		{
			Com_DPrintf( "SV_RankReportInt: Expecting QGR_STATUS_ACTIVE"
				" Got Unexpected status %d for player %d\n", 
				s_ranked_players[index1].grank_status, index1 );
			return;
		}
	
		context = s_ranked_players[index1].context;
		match = s_ranked_players[index1].match;
		user1 = s_ranked_players[index1].player_id;
	}

	// get player_id for player index2
	if( index2 == -1 )
	{
		user2 = 0;
	}
	else
	{
		if( s_ranked_players[index2].grank_status != QGR_STATUS_ACTIVE )
		{
			Com_DPrintf( "SV_RankReportInt: Expecting QGR_STATUS_ACTIVE"
				" Got Unexpected status %d for player %d\n", 
				s_ranked_players[index2].grank_status, index2 );
			return;
		}

		user2 = s_ranked_players[index2].player_id;
	}

	opt_accum = accum ? GR_OPT_ACCUM : GR_OPT_END;
	
	status = GRankReportInt
		(
			context,
			match,
			user1, 
			user2,
			key,
			value,
			opt_accum,
			GR_OPT_END
		);
		
	if( status != GR_STATUS_OK )
	{
		SV_RankError( "SV_RankReportInt: Unexpected status %s",
			SV_RankStatusString( status ) );
	}

	if( user2 != 0 )
	{
		context = s_ranked_players[index2].context;
		match   = s_ranked_players[index2].match;
		
		status = GRankReportInt
			(
				context,
				match,
				user1, 
				user2,
				key,
				value,
				opt_accum,
				GR_OPT_END
			);
			
		if( status != GR_STATUS_OK )
		{
			SV_RankError( "SV_RankReportInt: Unexpected status %s",
				SV_RankStatusString( status ) );
		}
	}
}

/*
================
SV_RankReportStr
================
*/
void SV_RankReportStr( int index1, int index2, int key, char* value )
{
	GR_STATUS	status;
	GR_CONTEXT	context;
	uint64_t	match;
	uint64_t	user1;
	uint64_t	user2;

	if( !s_rankings_active )
	{
		return;
	}

	assert( index1 >= -1 );
	assert( index1 < sv_maxclients->value );
	assert( index2 >= -1 );
	assert( index2 < sv_maxclients->value );
	assert( s_ranked_players );

//	Com_DPrintf( "SV_RankReportStr( %d, %d, %d, \"%s\" );\n", index1, index2, 
//		key, value );
	
	// get context, match, and player_id for player index1
	if( index1 == -1 )
	{
		context = s_server_context;
		match = s_server_match;
		user1 = 0;
	}
	else
	{
		if( s_ranked_players[index1].grank_status != QGR_STATUS_ACTIVE )
		{
			Com_DPrintf( "SV_RankReportStr: Unexpected status %d\n", 
				s_ranked_players[index1].grank_status );
			return;
		}
	
		context = s_ranked_players[index1].context;
		match = s_ranked_players[index1].match;
		user1 = s_ranked_players[index1].player_id;
	}

	// get player_id for player index2
	if( index2 == -1 )
	{
		user2 = 0;
	}
	else
	{
		if( s_ranked_players[index2].grank_status != QGR_STATUS_ACTIVE )
		{
			Com_DPrintf( "SV_RankReportStr: Unexpected status %d\n", 
				s_ranked_players[index2].grank_status );
			return;
		}

		user2 = s_ranked_players[index2].player_id;
	}

	status = GRankReportStr
		(
			context,
			match,
			user1,
			user2,
			key,
			value,
			GR_OPT_END
		);
		
	if( status != GR_STATUS_OK )
	{
		SV_RankError( "SV_RankReportStr: Unexpected status %s",
			SV_RankStatusString( status ) );
	}
	
	if( user2 != 0 )
	{
		context = s_ranked_players[index2].context;
		match = s_ranked_players[index2].match;
		
		status = GRankReportStr
			(
				context,
				match,
				user1, 
				user2,
				key,
				value,
				GR_OPT_END
			);
			
		if( status != GR_STATUS_OK )
		{
			SV_RankError( "SV_RankReportInt: Unexpected status %s",
				SV_RankStatusString( status ) );
		}
	}
}

/*
================
SV_RankQuit
================
*/
void SV_RankQuit( void )
{
	int	i;
	int j = 0;	
	// yuck
	
	while( s_rankings_contexts > 1 )
	{
		assert(s_ranked_players);
		if( s_ranked_players != NULL )
		{
			for( i = 0; i < sv_maxclients->value; i++ )
			{
				// check for players that weren't yet active in SV_RankEnd
				if( s_ranked_players[i].grank_status == QGR_STATUS_ACTIVE )
				{
					SV_RankUserLogout( i );
					Com_DPrintf( "SV_RankQuit: SV_RankUserLogout %d\n",i );
				}
				else
				{
					if( s_ranked_players[i].context )
					{
						GR_STATUS cleanup_status;
						cleanup_status = GRankCleanupAsync
							(
								s_ranked_players[i].context,
								0,
								SV_RankCleanupCBF,
								(void*)&(s_ranked_players[i]),
								GR_OPT_END
							);
						
						if( cleanup_status != GR_STATUS_PENDING )
						{
							SV_RankError( "SV_RankQuit: Expected "
								"GR_STATUS_PENDING from GRankCleanupAsync, got %s", 
								SV_RankStatusString( cleanup_status ) );
						}
					}
				}
			}
		}
		SV_RankPoll();
		
		// should've finished by now
		assert( (j++) < 68 );
	}
}

/*
==============================================================================

Private Functions

==============================================================================
*/

/*
=================
SV_RankNewGameCBF
=================
*/
static void SV_RankNewGameCBF( GR_NEWGAME* gr_newgame, void* cbf_arg )
{
	GR_MATCH	match;
	int			i;
	
	assert( gr_newgame != NULL );
	assert( cbf_arg == NULL );

	Com_DPrintf( "SV_RankNewGameCBF( %08X, %08X );\n", gr_newgame, cbf_arg );
	
	if( gr_newgame->status == GR_STATUS_OK )
	{
		char info[MAX_INFO_STRING];
		char gameid[sizeof(s_ranked_players[i].game_id) * 4 / 3 + 2];
		
		// save game id
		s_rankings_game_id = gr_newgame->game_id;
		
		// encode gameid 
		memset(gameid,0,sizeof(gameid));
		SV_RankEncodeGameID(s_rankings_game_id,gameid,sizeof(gameid));
		
		// set CS_GRANK rankingsGameID to pass to client
		memset(info,0,sizeof(info));
		Info_SetValueForKey( info, "rankingsGameKey", s_rankings_game_key );
		Info_SetValueForKey( info, "rankingsGameID", gameid );
		SV_SetConfigstring( CS_GRANK, info );

		// initialize client status
		for( i = 0; i < sv_maxclients->value; i++ )
			s_ranked_players[i].grank_status = QGR_STATUS_NEW;

		// start new match
		match = GRankStartMatch( s_server_context );
		s_server_match = match.match;

		// ready to go
		s_rankings_active = qtrue;
		Cvar_Set( "sv_rankingsActive", "1" );

	}
	else if( gr_newgame->status == GR_STATUS_BADLEAGUE )
	{
		SV_RankError( "SV_RankNewGameCBF: Invalid League name" );
	}
	else
	{
		//GRank handle new game failure
		// force  SV_RankEnd() to run
		//SV_RankEnd();
		SV_RankError( "SV_RankNewGameCBF: Unexpected status %s", 
			SV_RankStatusString( gr_newgame->status ) );
	}
}

/*
================
SV_RankUserCBF
================
*/
static void SV_RankUserCBF( GR_LOGIN* gr_login, void* cbf_arg )
{
	ranked_player_t*	ranked_player;
	GR_STATUS			join_status;
	GR_STATUS			cleanup_status;
	
	assert( gr_login != NULL );
	assert( cbf_arg != NULL );

	Com_DPrintf( "SV_RankUserCBF( %08X, %08X );\n", gr_login, cbf_arg );
	
	ranked_player = (ranked_player_t*)cbf_arg;
	assert(ranked_player);
	assert( ranked_player->context );
	
	switch( gr_login->status )
	{
		case GR_STATUS_OK:
			// attempt to join the game, proceed to SV_RankJoinGameCBF
			join_status = GRankJoinGameAsync
				( 
					ranked_player->context,
					s_rankings_game_id,
					SV_RankJoinGameCBF,
					cbf_arg,
					GR_OPT_END
				);

			if( join_status != GR_STATUS_PENDING )
			{
				SV_RankError( "SV_RankUserCBF: Expected GR_STATUS_PENDING "
					"from GRankJoinGameAsync, got %s", 
					SV_RankStatusString( join_status ) );
			}
			break;
		case GR_STATUS_NOUSER:
			Com_DPrintf( "SV_RankUserCBF: Got status %s\n",
				SV_RankStatusString( gr_login->status ) );
			ranked_player->final_status = QGR_STATUS_NO_USER;
			break;
		case GR_STATUS_BADPASSWORD:
			Com_DPrintf( "SV_RankUserCBF: Got status %s\n",
				SV_RankStatusString( gr_login->status ) );
			ranked_player->final_status = QGR_STATUS_BAD_PASSWORD;
			break;
		case GR_STATUS_TIMEOUT:
			Com_DPrintf( "SV_RankUserCBF: Got status %s\n",
				SV_RankStatusString( gr_login->status ) );
			ranked_player->final_status = QGR_STATUS_TIMEOUT;
			break;
		default:
			Com_DPrintf( "SV_RankUserCBF: Unexpected status %s\n",
				SV_RankStatusString( gr_login->status ) );
			ranked_player->final_status = QGR_STATUS_ERROR;
			break;
	}

	if( ranked_player->final_status != QGR_STATUS_NEW )
	{
		// login or create failed, so clean up before the next attempt
		cleanup_status = GRankCleanupAsync
			(
				ranked_player->context,
				0,
				SV_RankCleanupCBF,
				(void*)ranked_player,
				GR_OPT_END
			);
			
		if( cleanup_status != GR_STATUS_PENDING )
		{
			SV_RankError( "SV_RankUserCBF: Expected GR_STATUS_PENDING "
				"from GRankCleanupAsync, got %s", 
				SV_RankStatusString( cleanup_status ) );
			SV_RankCloseContext( ranked_player );
		}
	}
}

/*
================
SV_RankJoinGameCBF
================
*/
static void SV_RankJoinGameCBF( GR_JOINGAME* gr_joingame, void* cbf_arg )
{
	ranked_player_t*	ranked_player;
	GR_MATCH			match;
	GR_STATUS           cleanup_status;

	assert( gr_joingame != NULL );
	assert( cbf_arg != NULL );
	
	Com_DPrintf( "SV_RankJoinGameCBF( %08X, %08X );\n", gr_joingame, cbf_arg );
	
	ranked_player = (ranked_player_t*)cbf_arg;

	assert( ranked_player );
	assert( ranked_player->context != 0 );
	
	if( gr_joingame->status == GR_STATUS_OK )
	{
		int i;
		// save user id
		ranked_player->player_id = gr_joingame->player_id;
		memcpy(ranked_player->token,gr_joingame->token,
			sizeof(GR_PLAYER_TOKEN)) ;
		match = GRankStartMatch( ranked_player->context );
		ranked_player->match = match.match;
		ranked_player->grank = gr_joingame->rank;

		// find the index and call SV_RankUserValidate
		for (i=0;i<sv_maxclients->value;i++)
			if ( ranked_player == &s_ranked_players[i] )
				SV_RankUserValidate(i,NULL,NULL,0, gr_joingame->rank,ranked_player->name);
	}
	else
	{
		//GRand handle join game failure
		SV_RankError( "SV_RankJoinGameCBF: Unexpected status %s",
			SV_RankStatusString( gr_joingame->status ) );
		
		cleanup_status = GRankCleanupAsync
			(
				ranked_player->context,
				0,
				SV_RankCleanupCBF,
				cbf_arg,
				GR_OPT_END
			);
		
		if( cleanup_status != GR_STATUS_PENDING )
		{
			SV_RankError( "SV_RankJoinGameCBF: Expected "
				"GR_STATUS_PENDING from GRankCleanupAsync, got %s", 
				SV_RankStatusString( cleanup_status ) );
			SV_RankCloseContext( ranked_player );
		}
	}		
}

/*
================
SV_RankSendReportsCBF
================
*/
static void SV_RankSendReportsCBF( GR_STATUS* status, void* cbf_arg )
{
	ranked_player_t*	ranked_player;
	GR_CONTEXT			context;
	GR_STATUS			cleanup_status;

	assert( status != NULL );
	// NULL cbf_arg means server is sending match reports
	
	Com_DPrintf( "SV_RankSendReportsCBF( %08X, %08X );\n", status, cbf_arg );
	
	ranked_player = (ranked_player_t*)cbf_arg;
	if( ranked_player == NULL )
	{
		Com_DPrintf( "SV_RankSendReportsCBF: server\n" );
		context = s_server_context;
	}
	else
	{
		Com_DPrintf( "SV_RankSendReportsCBF: player\n" );
		context = ranked_player->context;
	}

	//assert( context != 0 );
	if( *status != GR_STATUS_OK )
	{
		SV_RankError( "SV_RankSendReportsCBF: Unexpected status %s",
			SV_RankStatusString( *status ) );
	}
	
	if( context == 0 )
	{
		Com_DPrintf( "SV_RankSendReportsCBF: WARNING: context == 0" );
		SV_RankCloseContext( ranked_player );
	}
	else
	{
		cleanup_status = GRankCleanupAsync
			(
				context,
				0,
				SV_RankCleanupCBF,
				cbf_arg,
				GR_OPT_END
			);
		
		if( cleanup_status != GR_STATUS_PENDING )
		{
			SV_RankError( "SV_RankSendReportsCBF: Expected "
				"GR_STATUS_PENDING from GRankCleanupAsync, got %s", 
				SV_RankStatusString( cleanup_status ) );
			SV_RankCloseContext( ranked_player );
		}
	}
}

/*
================
SV_RankCleanupCBF
================
*/
static void SV_RankCleanupCBF( GR_STATUS* status, void* cbf_arg )
{
	ranked_player_t*	ranked_player;
	ranked_player = (ranked_player_t*)cbf_arg;

	assert( status != NULL );
	// NULL cbf_arg means server is cleaning up

	Com_DPrintf( "SV_RankCleanupCBF( %08X, %08X );\n", status, cbf_arg );
	
	if( *status != GR_STATUS_OK )
	{
		SV_RankError( "SV_RankCleanupCBF: Unexpected status %s",
			SV_RankStatusString( *status ) );
	}

	SV_RankCloseContext( ranked_player );
}

/*
================
SV_RankCloseContext
================
*/
static void SV_RankCloseContext( ranked_player_t* ranked_player )
{
	if( ranked_player == NULL )
	{
		// server cleanup
		if( s_server_context == 0 )
		{
			return;
		}
		s_server_context = 0;
		s_server_match = 0;
	}
	else
	{
		// player cleanup
		if( s_ranked_players == NULL )
		{
			return;
		}
		if( ranked_player->context == 0 )
		{
			return;
		}
		ranked_player->context = 0;
		ranked_player->match = 0;
		ranked_player->player_id = 0;
		memset( ranked_player->token, 0, sizeof(GR_PLAYER_TOKEN) );
		ranked_player->grank_status = ranked_player->final_status;
		ranked_player->final_status = QGR_STATUS_NEW;
		ranked_player->name[0] = '\0';
	}

	assert( s_rankings_contexts > 0 );
	s_rankings_contexts--;
	Com_DPrintf( "SV_RankCloseContext: s_rankings_contexts = %d\n", 
		s_rankings_contexts );

	if( s_rankings_contexts == 0 )
	{
		GRankLogLevel( GRLOG_OFF );
		
		if( s_ranked_players != NULL )
		{
			Z_Free( s_ranked_players );
			s_ranked_players = NULL;
		}

		s_rankings_active = qfalse;
		Cvar_Set( "sv_rankingsActive", "0" );
	}
}

/*
================
SV_RankAsciiEncode

Encodes src_len bytes of binary data from the src buffer as ASCII text, 
using 6 bits per character. The result string is null-terminated and 
stored in the dest buffer.

The dest buffer must be at least (src_len * 4) / 3 + 2 bytes in length.

Returns the length of the result string, not including the null.
================
*/
static int SV_RankAsciiEncode( char* dest, const unsigned char* src, 
	int src_len )
{
	unsigned char	bin[3];
	unsigned char	txt[4];
	int				dest_len = 0;
	int				i;
	int				j;
	int				num_chars;

	assert( dest != NULL );
	assert( src != NULL );
	
	for( i = 0; i < src_len; i += 3 )
	{
		// read three bytes of input
		for( j = 0; j < 3; j++ )
		{
			bin[j] = (i + j < src_len) ? src[i + j] : 0;
		}

		// get four 6-bit values from three bytes
		txt[0] = bin[0] >> 2;
		txt[1] = ((bin[0] << 4) | (bin[1] >> 4)) & 63;
		txt[2] = ((bin[1] << 2) | (bin[2] >> 6)) & 63;
		txt[3] = bin[2] & 63;

		// store ASCII encoding of 6-bit values
		num_chars = (i + 2 < src_len) ? 4 : ((src_len - i) * 4) / 3 + 1;
		for( j = 0; j < num_chars; j++ )
		{
			dest[dest_len++] = s_ascii_encoding[txt[j]];
		}
	}
	
	dest[dest_len] = '\0';

	return dest_len;
}

/*
================
SV_RankAsciiDecode

Decodes src_len characters of ASCII text from the src buffer, stores 
the binary result in the dest buffer.

The dest buffer must be at least (src_len * 3) / 4 bytes in length.

Returns the length of the binary result, or zero for invalid input.
================
*/
static int SV_RankAsciiDecode( unsigned char* dest, const char* src, 
	int src_len )
{
	static unsigned char	s_inverse_encoding[256];
	static char				s_init = 0;
	
	unsigned char	bin[3];
	unsigned char	txt[4];
	int				dest_len = 0;
	int				i;
	int				j;
	int				num_bytes;
	
	assert( dest != NULL );
	assert( src != NULL );

	if( !s_init )
	{
		// initialize lookup table for decoding
		memset( s_inverse_encoding, 255, sizeof(s_inverse_encoding) );
		for( i = 0; i < 64; i++ )
		{
			s_inverse_encoding[s_ascii_encoding[i]] = i;
		}
		s_init = 1;
	}
	
	for( i = 0; i < src_len; i += 4 )
	{
		// read four characters of input, decode them to 6-bit values
		for( j = 0; j < 4; j++ )
		{
			txt[j] = (i + j < src_len) ? s_inverse_encoding[src[i + j]] : 0;
			if (txt[j] == 255)
			{
				return 0; // invalid input character
			}
		}
		
		// get three bytes from four 6-bit values
		bin[0] = (txt[0] << 2) | (txt[1] >> 4);
		bin[1] = (txt[1] << 4) | (txt[2] >> 2);
		bin[2] = (txt[2] << 6) | txt[3];

		// store binary data
		num_bytes = (i + 3 < src_len) ? 3 : ((src_len - i) * 3) / 4;
		for( j = 0; j < num_bytes; j++ )
		{
			dest[dest_len++] = bin[j];
		}
	}

	return dest_len;
}

/*
================
SV_RankEncodeGameID
================
*/
static void SV_RankEncodeGameID( uint64_t game_id, char* result, 
	int len )
{
	assert( result != NULL );

	if( len < ( ( sizeof(game_id) * 4) / 3 + 2) )
	{
		Com_DPrintf( "SV_RankEncodeGameID: result buffer too small\n" );
		result[0] = '\0';
	}
	else
	{
		qint64 gameid = LittleLong64(*(qint64*)&game_id);
		SV_RankAsciiEncode( result, (unsigned char*)&gameid, 
			sizeof(qint64) );
	}
}

/*
================
SV_RankDecodePlayerID
================
*/
static uint64_t SV_RankDecodePlayerID( const char* string )
{
	unsigned char	buffer[9];
	int len;
	qint64	player_id;

	assert( string != NULL );
	
	len = strlen (string) ;
	Com_DPrintf( "SV_RankDecodePlayerID: string length %d\n",len );
	SV_RankAsciiDecode( buffer, string, len );
	player_id = LittleLong64(*(qint64*)buffer);
	return *(uint64_t*)&player_id;
}

/*
================
SV_RankDecodePlayerKey
================
*/
static void SV_RankDecodePlayerKey( const char* string, GR_PLAYER_TOKEN key )
{
	unsigned char	buffer[1400];
	int len;
	assert( string != NULL );

	len = strlen (string) ;
	Com_DPrintf( "SV_RankDecodePlayerKey: string length %d\n",len );
	
	memset(key,0,sizeof(GR_PLAYER_TOKEN));
	memset(buffer,0,sizeof(buffer));
	memcpy( key, buffer, SV_RankAsciiDecode( buffer, string, len ) );
}

/*
================
SV_RankStatusString
================
*/
static char* SV_RankStatusString( GR_STATUS status )
{
	switch( status )
	{
		case GR_STATUS_OK:				return "GR_STATUS_OK";
		case GR_STATUS_ERROR:			return "GR_STATUS_ERROR";
		case GR_STATUS_BADPARAMS:		return "GR_STATUS_BADPARAMS";
		case GR_STATUS_NETWORK:			return "GR_STATUS_NETWORK";
		case GR_STATUS_NOUSER:			return "GR_STATUS_NOUSER";
		case GR_STATUS_BADPASSWORD:		return "GR_STATUS_BADPASSWORD";
		case GR_STATUS_BADGAME:			return "GR_STATUS_BADGAME";
		case GR_STATUS_PENDING:			return "GR_STATUS_PENDING";
		case GR_STATUS_BADDOMAIN:		return "GR_STATUS_BADDOMAIN";
		case GR_STATUS_DOMAINLOCK:		return "GR_STATUS_DOMAINLOCK";
		case GR_STATUS_TIMEOUT:			return "GR_STATUS_TIMEOUT";
		case GR_STATUS_INVALIDUSER:	    return "GR_STATUS_INVALIDUSER";
		case GR_STATUS_INVALIDCONTEXT:	return "GR_STATUS_INVALIDCONTEXT";
		default:						return "(UNKNOWN)";
	}
}

/*
================
SV_RankError
================
*/
static void SV_RankError( const char* fmt, ... )
{
	va_list	arg_ptr;
	char	text[1024];

	va_start( arg_ptr, fmt );
	Q_vsnprintf(text, sizeof(text), fmt, arg_ptr );
	va_end( arg_ptr );

	Com_DPrintf( "****************************************\n" );
	Com_DPrintf( "SV_RankError: %s\n", text );
	Com_DPrintf( "****************************************\n" );

	s_rankings_active = qfalse;
	Cvar_Set( "sv_rankingsActive", "0" );
	// FIXME - attempt clean shutdown?
}

