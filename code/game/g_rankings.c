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
// g_rankings.c -- reports for global rankings system

#include "g_local.h"
#include "g_rankings.h"

/*
================
G_RankRunFrame
================
*/
void G_RankRunFrame()
{
	gentity_t*		ent;
	gentity_t*		ent2;
	grank_status_t	old_status;
	grank_status_t	status;
	int				time;
	int				i;
	int				j;

	if( !trap_RankCheckInit() ) 
	{
		trap_RankBegin( GR_GAMEKEY );
	}

	trap_RankPoll();
	
	if( trap_RankActive() )
	{
		for( i = 0; i < level.maxclients; i++ )
		{
			ent = &(g_entities[i]);
			if ( !ent->inuse )
				continue;
			if ( ent->client == NULL )
				continue;
			if ( ent->r.svFlags & SVF_BOT)
			{
				// no bots in ranked games
				trap_SendConsoleCommand( EXEC_INSERT, va("kick %s\n", 
					ent->client->pers.netname) );
				continue;
			}

			old_status = ent->client->client_status;
			status = trap_RankUserStatus( i );
			
			if( ent->client->client_status != status )
			{
				// inform client of current status
				// not needed for client side log in
				trap_SendServerCommand( i, va("rank_status %i\n",status) );
				if ( i == 0 )
				{
					int j = 0;
				}
				ent->client->client_status = status;
			}
			
			switch( status )
			{
			case QGR_STATUS_NEW:
			case QGR_STATUS_SPECTATOR:
				if( ent->client->sess.sessionTeam != TEAM_SPECTATOR )
				{
					ent->client->sess.sessionTeam = TEAM_SPECTATOR;
					ent->client->sess.spectatorState = SPECTATOR_FREE;
					ClientSpawn( ent );
					// make sure by now CS_GRAND rankingsGameID is ready
					trap_SendServerCommand( i, va("rank_status %i\n",status) );
					trap_SendServerCommand( i, "rank_menu\n" );
				}
				break;
			case QGR_STATUS_NO_USER:
			case QGR_STATUS_BAD_PASSWORD:
			case QGR_STATUS_TIMEOUT:
			case QGR_STATUS_NO_MEMBERSHIP:
			case QGR_STATUS_INVALIDUSER:
			case QGR_STATUS_ERROR:
				if( (ent->r.svFlags & SVF_BOT) == 0 )
				{
					trap_RankUserReset( ent->s.clientNum );
				}
				break;
			case QGR_STATUS_ACTIVE:
				if( (ent->client->sess.sessionTeam == TEAM_SPECTATOR) &&
					(g_gametype.integer < GT_TEAM) )
				{
					SetTeam( ent, "free" );
				}

				if( old_status != QGR_STATUS_ACTIVE )
				{
					// player has just become active
					for( j = 0; j < level.maxclients; j++ )
					{
						ent2 = &(g_entities[j]);
						if ( !ent2->inuse )
							continue;
						if ( ent2->client == NULL )
							continue;
						if ( ent2->r.svFlags & SVF_BOT)
							continue;

						if( (i != j) && (trap_RankUserStatus( j ) == QGR_STATUS_ACTIVE) )
						{
							trap_RankReportInt( i, j, QGR_KEY_PLAYED_WITH, 1, 0 );
						}

						// send current scores so the player's rank will show 
						// up under the crosshair immediately
						DeathmatchScoreboardMessage( ent2 );
					}
				}
				break;
			default:
				break;
			}
		}

		// don't let ranked games last forever
		if( ((g_fraglimit.integer == 0) || (g_fraglimit.integer > 100)) && 
			((g_timelimit.integer == 0) || (g_timelimit.integer > 1000)) )
		{
			trap_Cvar_Set( "timelimit", "1000" );
		}
	}

	// tell time to clients so they can show current match rating
	if( level.intermissiontime == 0 )
	{
		for( i = 0; i < level.maxclients; i++ )
		{
			ent = &(g_entities[i]);
			if( ent->client == NULL )
			{
				continue;
			}

			time = (level.time - ent->client->pers.enterTime) / 1000;
			ent->client->ps.persistant[PERS_MATCH_TIME] = time;
		}
	}
}

/*
================
G_RankFireWeapon
================
*/
void G_RankFireWeapon( int self, int weapon )
{
	if( level.warmupTime != 0 )
	{
		// no reports during warmup period
		return;
	}
	
	if( weapon == WP_GAUNTLET )
	{
		// the gauntlet only "fires" when it actually hits something
		return;
	}
	
	trap_RankReportInt( self, -1, QGR_KEY_SHOT_FIRED, 1, 1 );
	
	switch( weapon )
	{
	case WP_MACHINEGUN:
		trap_RankReportInt( self, -1, QGR_KEY_SHOT_FIRED_MACHINEGUN, 1, 1 );
		break;
	case WP_SHOTGUN:
		trap_RankReportInt( self, -1, QGR_KEY_SHOT_FIRED_SHOTGUN, 1, 1 );
		break;
	case WP_GRENADE_LAUNCHER:
		trap_RankReportInt( self, -1, QGR_KEY_SHOT_FIRED_GRENADE, 1, 1 );
		break;
	case WP_ROCKET_LAUNCHER:
		trap_RankReportInt( self, -1, QGR_KEY_SHOT_FIRED_ROCKET, 1, 1 );
		break;
	case WP_LIGHTNING:
		trap_RankReportInt( self, -1, QGR_KEY_SHOT_FIRED_LIGHTNING, 1, 1 );
		break;
	case WP_RAILGUN:
		trap_RankReportInt( self, -1, QGR_KEY_SHOT_FIRED_RAILGUN, 1, 1 );
		break;
	case WP_PLASMAGUN:
		trap_RankReportInt( self, -1, QGR_KEY_SHOT_FIRED_PLASMA, 1, 1 );
		break;
	case WP_BFG:
		trap_RankReportInt( self, -1, QGR_KEY_SHOT_FIRED_BFG, 1, 1 );
		break;
	case WP_GRAPPLING_HOOK:
		trap_RankReportInt( self, -1, QGR_KEY_SHOT_FIRED_GRAPPLE, 1, 1 );
		break;
	default:
		break;
	}
}

/*
================
G_RankDamage
================
*/
void G_RankDamage( int self, int attacker, int damage, int means_of_death )
{
	// state information to avoid counting each shotgun pellet as a hit
	static int	last_framenum = -1;
	static int	last_self = -1;
	static int	last_attacker = -1;
	static int	last_means_of_death = MOD_UNKNOWN;

	qboolean	new_hit;
	int			splash;
	int			key_hit;
	int			key_damage;
	int			key_splash;

	if( level.warmupTime != 0 )
	{
		// no reports during warmup period
		return;
	}
	
	new_hit = (level.framenum != last_framenum) || 
		(self != last_self) || 
		(attacker != last_attacker) || 
		(means_of_death != last_means_of_death);

	// update state information
	last_framenum = level.framenum;
	last_self = self;
	last_attacker = attacker;
	last_means_of_death = means_of_death;

	// the gauntlet only "fires" when it actually hits something
	if( (attacker != ENTITYNUM_WORLD) && (attacker != self) && 
		(means_of_death == MOD_GAUNTLET)  && 
		(g_entities[attacker].client) )
	{
		trap_RankReportInt( attacker, -1, QGR_KEY_SHOT_FIRED_GAUNTLET, 1, 1 );
	}

	// don't track hazard damage, just deaths
	switch( means_of_death )
	{
	case MOD_WATER:
	case MOD_SLIME:
	case MOD_LAVA:
	case MOD_CRUSH:
	case MOD_TELEFRAG:
	case MOD_FALLING:
	case MOD_SUICIDE:
	case MOD_TRIGGER_HURT:
		return;
	default:
		break;
	}

	// get splash damage
	switch( means_of_death )
	{
	case MOD_GRENADE_SPLASH:
	case MOD_ROCKET_SPLASH:
	case MOD_PLASMA_SPLASH:
	case MOD_BFG_SPLASH:
		splash = damage;
		break;
	default:
		splash = 0;
		key_splash = -1;
		break;
	}
	
	// hit, damage, and splash taken
	switch( means_of_death )
	{
	case MOD_GAUNTLET:
		key_hit = QGR_KEY_HIT_TAKEN_GAUNTLET;
		key_damage = QGR_KEY_DAMAGE_TAKEN_GAUNTLET;
		break;
	case MOD_MACHINEGUN:
		key_hit = QGR_KEY_HIT_TAKEN_MACHINEGUN;
		key_damage = QGR_KEY_DAMAGE_TAKEN_MACHINEGUN;
		break;
	case MOD_SHOTGUN:
		key_hit = QGR_KEY_HIT_TAKEN_SHOTGUN;
		key_damage = QGR_KEY_DAMAGE_TAKEN_SHOTGUN;
		break;
	case MOD_GRENADE:
	case MOD_GRENADE_SPLASH:
		key_hit = QGR_KEY_HIT_TAKEN_GRENADE;
		key_damage = QGR_KEY_DAMAGE_TAKEN_GRENADE;
		key_splash = QGR_KEY_SPLASH_TAKEN_GRENADE;
		break;
	case MOD_ROCKET:
	case MOD_ROCKET_SPLASH:
		key_hit = QGR_KEY_HIT_TAKEN_ROCKET;
		key_damage = QGR_KEY_DAMAGE_TAKEN_ROCKET;
		key_splash = QGR_KEY_SPLASH_TAKEN_ROCKET;
		break;
	case MOD_PLASMA:
	case MOD_PLASMA_SPLASH:
		key_hit = QGR_KEY_HIT_TAKEN_PLASMA;
		key_damage = QGR_KEY_DAMAGE_TAKEN_PLASMA;
		key_splash = QGR_KEY_SPLASH_TAKEN_PLASMA;
		break;
	case MOD_RAILGUN:
		key_hit = QGR_KEY_HIT_TAKEN_RAILGUN;
		key_damage = QGR_KEY_DAMAGE_TAKEN_RAILGUN;
		break;
	case MOD_LIGHTNING:
		key_hit = QGR_KEY_HIT_TAKEN_LIGHTNING;
		key_damage = QGR_KEY_DAMAGE_TAKEN_LIGHTNING;
		break;
	case MOD_BFG:
	case MOD_BFG_SPLASH:
		key_hit = QGR_KEY_HIT_TAKEN_BFG;
		key_damage = QGR_KEY_DAMAGE_TAKEN_BFG;
		key_splash = QGR_KEY_SPLASH_TAKEN_BFG;
		break;
	case MOD_GRAPPLE:
		key_hit = QGR_KEY_HIT_TAKEN_GRAPPLE;
		key_damage = QGR_KEY_DAMAGE_TAKEN_GRAPPLE;
		break;
	default:
		key_hit = QGR_KEY_HIT_TAKEN_UNKNOWN;
		key_damage = QGR_KEY_DAMAGE_TAKEN_UNKNOWN;
		break;
	}

	// report general and specific hit taken
	if( new_hit )
	{
		trap_RankReportInt( self, -1, QGR_KEY_HIT_TAKEN, 1, 1 );
		trap_RankReportInt( self, -1, key_hit, 1, 1 );
	}
	
	// report general and specific damage taken
	trap_RankReportInt( self, -1, QGR_KEY_DAMAGE_TAKEN, damage, 1 );
	trap_RankReportInt( self, -1, key_damage, damage, 1 );

	// report general and specific splash taken
	if( splash != 0 )
	{
		trap_RankReportInt( self, -1, QGR_KEY_SPLASH_TAKEN, splash, 1 );
		trap_RankReportInt( self, -1, key_splash, splash, 1 );
	}

	// hit, damage, and splash given
	if( (attacker != ENTITYNUM_WORLD) && (attacker != self) )
	{
		switch( means_of_death )
		{
		case MOD_GAUNTLET:
			key_hit = QGR_KEY_HIT_GIVEN_GAUNTLET;
			key_damage = QGR_KEY_DAMAGE_GIVEN_GAUNTLET;
			break;
		case MOD_MACHINEGUN:
			key_hit = QGR_KEY_HIT_GIVEN_MACHINEGUN;
			key_damage = QGR_KEY_DAMAGE_GIVEN_MACHINEGUN;
			break;
		case MOD_SHOTGUN:
			key_hit = QGR_KEY_HIT_GIVEN_SHOTGUN;
			key_damage = QGR_KEY_DAMAGE_GIVEN_SHOTGUN;
			break;
		case MOD_GRENADE:
		case MOD_GRENADE_SPLASH:
			key_hit = QGR_KEY_HIT_GIVEN_GRENADE;
			key_damage = QGR_KEY_DAMAGE_GIVEN_GRENADE;
			key_splash = QGR_KEY_SPLASH_GIVEN_GRENADE;
			break;
		case MOD_ROCKET:
		case MOD_ROCKET_SPLASH:
			key_hit = QGR_KEY_HIT_GIVEN_ROCKET;
			key_damage = QGR_KEY_DAMAGE_GIVEN_ROCKET;
			key_splash = QGR_KEY_SPLASH_GIVEN_ROCKET;
			break;
		case MOD_PLASMA:
		case MOD_PLASMA_SPLASH:
			key_hit = QGR_KEY_HIT_GIVEN_PLASMA;
			key_damage = QGR_KEY_DAMAGE_GIVEN_PLASMA;
			key_splash = QGR_KEY_SPLASH_GIVEN_PLASMA;
			break;
		case MOD_RAILGUN:
			key_hit = QGR_KEY_HIT_GIVEN_RAILGUN;
			key_damage = QGR_KEY_DAMAGE_GIVEN_RAILGUN;
			break;
		case MOD_LIGHTNING:
			key_hit = QGR_KEY_HIT_GIVEN_LIGHTNING;
			key_damage = QGR_KEY_DAMAGE_GIVEN_LIGHTNING;
			break;
		case MOD_BFG:
		case MOD_BFG_SPLASH:
			key_hit = QGR_KEY_HIT_GIVEN_BFG;
			key_damage = QGR_KEY_DAMAGE_GIVEN_BFG;
			key_splash = QGR_KEY_SPLASH_GIVEN_BFG;
			break;
		case MOD_GRAPPLE:
			key_hit = QGR_KEY_HIT_GIVEN_GRAPPLE;
			key_damage = QGR_KEY_DAMAGE_GIVEN_GRAPPLE;
			break;
		default:
			key_hit = QGR_KEY_HIT_GIVEN_UNKNOWN;
			key_damage = QGR_KEY_DAMAGE_GIVEN_UNKNOWN;
			break;
		}
		
		// report general and specific hit given
		// jwu 8/26/00
		// had a case where attacker is 245 which is grnadeshooter attacker is 
		// g_entities index not necessarilly clientnum
		if (g_entities[attacker].client) {
			if( new_hit )
			{
				trap_RankReportInt( attacker, -1, QGR_KEY_HIT_GIVEN, 1, 1 );
				trap_RankReportInt( attacker, -1, key_hit, 1, 1 );
			}
			
			// report general and specific damage given
			trap_RankReportInt( attacker, -1, QGR_KEY_DAMAGE_GIVEN, damage, 1 );
			trap_RankReportInt( attacker, -1, key_damage, damage, 1 );

			// report general and specific splash given
			if( splash != 0 )
			{
				trap_RankReportInt( attacker, -1, QGR_KEY_SPLASH_GIVEN, splash, 1 );
				trap_RankReportInt( attacker, -1, key_splash, splash, 1 );
			}
		}
	}

	// friendly fire
	if( (attacker != self) && 
		OnSameTeam( &(g_entities[self]), &(g_entities[attacker])) &&
		(g_entities[attacker].client) )
	{
		// report teammate hit
		if( new_hit )
		{
			trap_RankReportInt( self, -1, QGR_KEY_TEAMMATE_HIT_TAKEN, 1, 1 );
			trap_RankReportInt( attacker, -1, QGR_KEY_TEAMMATE_HIT_GIVEN, 1, 
				1 );
		}

		// report teammate damage
		trap_RankReportInt( self, -1, QGR_KEY_TEAMMATE_DAMAGE_TAKEN, damage, 
			1 );
		trap_RankReportInt( attacker, -1, QGR_KEY_TEAMMATE_DAMAGE_GIVEN, 
			damage, 1 );
			
		// report teammate splash
		if( splash != 0 )
		{
			trap_RankReportInt( self, -1, QGR_KEY_TEAMMATE_SPLASH_TAKEN, 
				splash, 1 );
			trap_RankReportInt( attacker, -1, QGR_KEY_TEAMMATE_SPLASH_GIVEN, 
				splash, 1 );
		}
	}
}

/*
================
G_RankPlayerDie
================
*/
void G_RankPlayerDie( int self, int attacker, int means_of_death )
{
	int	p1;
	int	p2;

	if( level.warmupTime != 0 )
	{
		// no reports during warmup period
		return;
	}
	
	if( attacker == ENTITYNUM_WORLD )
	{
		p1 = self;
		p2 = -1;
		
		trap_RankReportInt( p1, p2, QGR_KEY_HAZARD_DEATH, 1, 1 );

		switch( means_of_death )
		{
		case MOD_WATER:
			trap_RankReportInt( p1, p2, QGR_KEY_WATER, 1, 1 );
			break;
		case MOD_SLIME:
			trap_RankReportInt( p1, p2, QGR_KEY_SLIME, 1, 1 );
			break;
		case MOD_LAVA:
			trap_RankReportInt( p1, p2, QGR_KEY_LAVA, 1, 1 );
			break;
		case MOD_CRUSH:
			trap_RankReportInt( p1, p2, QGR_KEY_CRUSH, 1, 1 );
			break;
		case MOD_TELEFRAG:
			trap_RankReportInt( p1, p2, QGR_KEY_TELEFRAG, 1, 1 );
			break;
		case MOD_FALLING:
			trap_RankReportInt( p1, p2, QGR_KEY_FALLING, 1, 1 );
			break;
		case MOD_SUICIDE:
			trap_RankReportInt( p1, p2, QGR_KEY_SUICIDE_CMD, 1, 1 );
			break;
		case MOD_TRIGGER_HURT:
			trap_RankReportInt( p1, p2, QGR_KEY_TRIGGER_HURT, 1, 1 );
			break;
		default:
			trap_RankReportInt( p1, p2, QGR_KEY_HAZARD_MISC, 1, 1 );
			break;
		}
	}
	else if( attacker == self )
	{
		p1 = self;
		p2 = -1;
		
		trap_RankReportInt( p1, p2, QGR_KEY_SUICIDE, 1, 1 );
		
		switch( means_of_death )
		{
		case MOD_GAUNTLET:
			trap_RankReportInt( p1, p2, QGR_KEY_SUICIDE_GAUNTLET, 1, 1 );
			break;
		case MOD_MACHINEGUN:
			trap_RankReportInt( p1, p2, QGR_KEY_SUICIDE_MACHINEGUN, 1, 1 );
			break;
		case MOD_SHOTGUN:
			trap_RankReportInt( p1, p2, QGR_KEY_SUICIDE_SHOTGUN, 1, 1 );
			break;
		case MOD_GRENADE:
		case MOD_GRENADE_SPLASH:
			trap_RankReportInt( p1, p2, QGR_KEY_SUICIDE_GRENADE, 1, 1 );
			break;
		case MOD_ROCKET:
		case MOD_ROCKET_SPLASH:
			trap_RankReportInt( p1, p2, QGR_KEY_SUICIDE_ROCKET, 1, 1 );
			break;
		case MOD_PLASMA:
		case MOD_PLASMA_SPLASH:
			trap_RankReportInt( p1, p2, QGR_KEY_SUICIDE_PLASMA, 1, 1 );
			break;
		case MOD_RAILGUN:
			trap_RankReportInt( p1, p2, QGR_KEY_SUICIDE_RAILGUN, 1, 1 );
			break;
		case MOD_LIGHTNING:
			trap_RankReportInt( p1, p2, QGR_KEY_SUICIDE_LIGHTNING, 1, 1 );
			break;
		case MOD_BFG:
		case MOD_BFG_SPLASH:
			trap_RankReportInt( p1, p2, QGR_KEY_SUICIDE_BFG, 1, 1 );
			break;
		case MOD_GRAPPLE:
			trap_RankReportInt( p1, p2, QGR_KEY_SUICIDE_GRAPPLE, 1, 1 );
			break;
		default:
			trap_RankReportInt( p1, p2, QGR_KEY_SUICIDE_UNKNOWN, 1, 1 );
			break;
		}
	}
	else
	{
		p1 = attacker;
		p2 = self;

		trap_RankReportInt( p1, p2, QGR_KEY_FRAG, 1, 1 );
		
		switch( means_of_death )
		{
		case MOD_GAUNTLET:
			trap_RankReportInt( p1, p2, QGR_KEY_FRAG_GAUNTLET, 1, 1 );
			break;
		case MOD_MACHINEGUN:
			trap_RankReportInt( p1, p2, QGR_KEY_FRAG_MACHINEGUN, 1, 1 );
			break;
		case MOD_SHOTGUN:
			trap_RankReportInt( p1, p2, QGR_KEY_FRAG_SHOTGUN, 1, 1 );
			break;
		case MOD_GRENADE:
		case MOD_GRENADE_SPLASH:
			trap_RankReportInt( p1, p2, QGR_KEY_FRAG_GRENADE, 1, 1 );
			break;
		case MOD_ROCKET:
		case MOD_ROCKET_SPLASH:
			trap_RankReportInt( p1, p2, QGR_KEY_FRAG_ROCKET, 1, 1 );
			break;
		case MOD_PLASMA:
		case MOD_PLASMA_SPLASH:
			trap_RankReportInt( p1, p2, QGR_KEY_FRAG_PLASMA, 1, 1 );
			break;
		case MOD_RAILGUN:
			trap_RankReportInt( p1, p2, QGR_KEY_FRAG_RAILGUN, 1, 1 );
			break;
		case MOD_LIGHTNING:
			trap_RankReportInt( p1, p2, QGR_KEY_FRAG_LIGHTNING, 1, 1 );
			break;
		case MOD_BFG:
		case MOD_BFG_SPLASH:
			trap_RankReportInt( p1, p2, QGR_KEY_FRAG_BFG, 1, 1 );
			break;
		case MOD_GRAPPLE:
			trap_RankReportInt( p1, p2, QGR_KEY_FRAG_GRAPPLE, 1, 1 );
			break;
		default:
			trap_RankReportInt( p1, p2, QGR_KEY_FRAG_UNKNOWN, 1, 1 );
			break;
		}
	}
}

/*
================
G_RankWeaponTime
================
*/
void G_RankWeaponTime( int self, int weapon )
{
	gclient_t*	client;
	int			time;

	if( level.warmupTime != 0 )
	{
		// no reports during warmup period
		return;
	}
	
	client = g_entities[self].client;
	time = (level.time - client->weapon_change_time) / 1000;
	client->weapon_change_time = level.time;

	if( time <= 0 )
	{
		return;
	}
	
	trap_RankReportInt( self, -1, QGR_KEY_TIME, time, 1 );

	switch( weapon )
	{
	case WP_GAUNTLET:
		trap_RankReportInt( self, -1, QGR_KEY_TIME_GAUNTLET, time, 1 );
		break;
	case WP_MACHINEGUN:
		trap_RankReportInt( self, -1, QGR_KEY_TIME_MACHINEGUN, time, 1 );
		break;
	case WP_SHOTGUN:
		trap_RankReportInt( self, -1, QGR_KEY_TIME_SHOTGUN, time, 1 );
		break;
	case WP_GRENADE_LAUNCHER:
		trap_RankReportInt( self, -1, QGR_KEY_TIME_GRENADE, time, 1 );
		break;
	case WP_ROCKET_LAUNCHER:
		trap_RankReportInt( self, -1, QGR_KEY_TIME_ROCKET, time, 1 );
		break;
	case WP_LIGHTNING:
		trap_RankReportInt( self, -1, QGR_KEY_TIME_LIGHTNING, time, 1 );
		break;
	case WP_RAILGUN:
		trap_RankReportInt( self, -1, QGR_KEY_TIME_RAILGUN, time, 1 );
		break;
	case WP_PLASMAGUN:
		trap_RankReportInt( self, -1, QGR_KEY_TIME_PLASMA, time, 1 );
		break;
	case WP_BFG:
		trap_RankReportInt( self, -1, QGR_KEY_TIME_BFG, time, 1 );
		break;
	case WP_GRAPPLING_HOOK:
		trap_RankReportInt( self, -1, QGR_KEY_TIME_GRAPPLE, time, 1 );
		break;
	default:
		break;
	}
}

/*
================
G_RankPickupWeapon
================
*/
void G_RankPickupWeapon( int self, int weapon )
{
	if( level.warmupTime != 0 )
	{
		// no reports during warmup period
		return;
	}
	
	trap_RankReportInt( self, -1, QGR_KEY_PICKUP_WEAPON, 1, 1 );
	switch( weapon )
	{
	case WP_GAUNTLET:
		trap_RankReportInt( self, -1, QGR_KEY_PICKUP_GAUNTLET, 1, 1 );
		break;
	case WP_MACHINEGUN:
		trap_RankReportInt( self, -1, QGR_KEY_PICKUP_MACHINEGUN, 1, 1 );
		break;
	case WP_SHOTGUN:
		trap_RankReportInt( self, -1, QGR_KEY_PICKUP_SHOTGUN, 1, 1 );
		break;
	case WP_GRENADE_LAUNCHER:
		trap_RankReportInt( self, -1, QGR_KEY_PICKUP_GRENADE, 1, 1 );
		break;
	case WP_ROCKET_LAUNCHER:
		trap_RankReportInt( self, -1, QGR_KEY_PICKUP_ROCKET, 1, 1 );
		break;
	case WP_LIGHTNING:
		trap_RankReportInt( self, -1, QGR_KEY_PICKUP_LIGHTNING, 1, 1 );
		break;
	case WP_RAILGUN:
		trap_RankReportInt( self, -1, QGR_KEY_PICKUP_RAILGUN, 1, 1 );
		break;
	case WP_PLASMAGUN:
		trap_RankReportInt( self, -1, QGR_KEY_PICKUP_PLASMA, 1, 1 );
		break;
	case WP_BFG:
		trap_RankReportInt( self, -1, QGR_KEY_PICKUP_BFG, 1, 1 );
		break;
	case WP_GRAPPLING_HOOK:
		trap_RankReportInt( self, -1, QGR_KEY_PICKUP_GRAPPLE, 1, 1 );
		break;
	default:
		break;
	}
}

/*
================
G_RankPickupAmmo
================
*/
void G_RankPickupAmmo( int self, int weapon, int quantity )
{
	if( level.warmupTime != 0 )
	{
		// no reports during warmup period
		return;
	}
	
	trap_RankReportInt( self, -1, QGR_KEY_BOXES, 1, 1 );
	trap_RankReportInt( self, -1, QGR_KEY_ROUNDS, quantity, 1 );
	
	switch( weapon )
	{
	case WP_MACHINEGUN:
		trap_RankReportInt( self, -1, QGR_KEY_BOXES_BULLETS, 1, 1 );
		trap_RankReportInt( self, -1, QGR_KEY_ROUNDS_BULLETS, quantity, 1 );
		break;
	case WP_SHOTGUN:
		trap_RankReportInt( self, -1, QGR_KEY_BOXES_SHELLS, 1, 1 );
		trap_RankReportInt( self, -1, QGR_KEY_ROUNDS_SHELLS, quantity, 1 );
		break;
	case WP_GRENADE_LAUNCHER:
		trap_RankReportInt( self, -1, QGR_KEY_BOXES_GRENADES, 1, 1 );
		trap_RankReportInt( self, -1, QGR_KEY_ROUNDS_GRENADES, quantity, 1 );
		break;
	case WP_ROCKET_LAUNCHER:
		trap_RankReportInt( self, -1, QGR_KEY_BOXES_ROCKETS, 1, 1 );
		trap_RankReportInt( self, -1, QGR_KEY_ROUNDS_ROCKETS, quantity, 1 );
		break;
	case WP_LIGHTNING:
		trap_RankReportInt( self, -1, QGR_KEY_BOXES_LG_AMMO, 1, 1 );
		trap_RankReportInt( self, -1, QGR_KEY_ROUNDS_LG_AMMO, quantity, 1 );
		break;
	case WP_RAILGUN:
		trap_RankReportInt( self, -1, QGR_KEY_BOXES_SLUGS, 1, 1 );
		trap_RankReportInt( self, -1, QGR_KEY_ROUNDS_SLUGS, quantity, 1 );
		break;
	case WP_PLASMAGUN:
		trap_RankReportInt( self, -1, QGR_KEY_BOXES_CELLS, 1, 1 );
		trap_RankReportInt( self, -1, QGR_KEY_ROUNDS_CELLS, quantity, 1 );
		break;
	case WP_BFG:
		trap_RankReportInt( self, -1, QGR_KEY_BOXES_BFG_AMMO, 1, 1 );
		trap_RankReportInt( self, -1, QGR_KEY_ROUNDS_BFG_AMMO, quantity, 1 );
		break;
	default:
		break;
	}
}

/*
================
G_RankPickupHealth
================
*/
void G_RankPickupHealth( int self, int quantity )
{
	if( level.warmupTime != 0 )
	{
		// no reports during warmup period
		return;
	}
	
	trap_RankReportInt( self, -1, QGR_KEY_HEALTH, 1, 1 );
	trap_RankReportInt( self, -1, QGR_KEY_HEALTH_TOTAL, quantity, 1 );

	switch( quantity )
	{
	case 5:
		trap_RankReportInt( self, -1, QGR_KEY_HEALTH_5, 1, 1 );
		break;
	case 25:
		trap_RankReportInt( self, -1, QGR_KEY_HEALTH_25, 1, 1 );
		break;
	case 50:
		trap_RankReportInt( self, -1, QGR_KEY_HEALTH_50, 1, 1 );
		break;
	case 100:
		trap_RankReportInt( self, -1, QGR_KEY_HEALTH_MEGA, 1, 1 );
		break;
	default:
		break;
	}
}

/*
================
G_RankPickupArmor
================
*/
void G_RankPickupArmor( int self, int quantity )
{
	if( level.warmupTime != 0 )
	{
		// no reports during warmup period
		return;
	}
	
	trap_RankReportInt( self, -1, QGR_KEY_ARMOR, 1, 1 );
	trap_RankReportInt( self, -1, QGR_KEY_ARMOR_TOTAL, quantity, 1 );

	switch( quantity )
	{
	case 5:
		trap_RankReportInt( self, -1, QGR_KEY_ARMOR_SHARD, 1, 1 );
		break;
	case 50:
		trap_RankReportInt( self, -1, QGR_KEY_ARMOR_YELLOW, 1, 1 );
		break;
	case 100:
		trap_RankReportInt( self, -1, QGR_KEY_ARMOR_RED, 1, 1 );
		break;
	default:
		break;
	}
}

/*
================
G_RankPickupPowerup
================
*/
void G_RankPickupPowerup( int self, int powerup )
{
	if( level.warmupTime != 0 )
	{
		// no reports during warmup period
		return;
	}
	
	// ctf flags are treated as powerups
	if( (powerup == PW_REDFLAG) || (powerup == PW_BLUEFLAG) )
	{
		trap_RankReportInt( self, -1, QGR_KEY_FLAG_PICKUP, 1, 1 );
		return;
	}

	trap_RankReportInt( self, -1, QGR_KEY_POWERUP, 1, 1 );
	
	switch( powerup )
	{
	case PW_QUAD:
		trap_RankReportInt( self, -1, QGR_KEY_QUAD, 1, 1 );
		break;
	case PW_BATTLESUIT:
		trap_RankReportInt( self, -1, QGR_KEY_SUIT, 1, 1 );
		break;
	case PW_HASTE:
		trap_RankReportInt( self, -1, QGR_KEY_HASTE, 1, 1 );
		break;
	case PW_INVIS:
		trap_RankReportInt( self, -1, QGR_KEY_INVIS, 1, 1 );
		break;
	case PW_REGEN:
		trap_RankReportInt( self, -1, QGR_KEY_REGEN, 1, 1 );
		break;
	case PW_FLIGHT:
		trap_RankReportInt( self, -1, QGR_KEY_FLIGHT, 1, 1 );
		break;
	default:
		break;
	}
}

/*
================
G_RankPickupHoldable
================
*/
void G_RankPickupHoldable( int self, int holdable )
{
	if( level.warmupTime != 0 )
	{
		// no reports during warmup period
		return;
	}
	
	switch( holdable )
	{
	case HI_MEDKIT:
		trap_RankReportInt( self, -1, QGR_KEY_MEDKIT, 1, 1 );
		break;
	case HI_TELEPORTER:
		trap_RankReportInt( self, -1, QGR_KEY_TELEPORTER, 1, 1 );
		break;
	default:
		break;
	}
}

/*
================
G_RankUseHoldable
================
*/
void G_RankUseHoldable( int self, int holdable )
{
	if( level.warmupTime != 0 )
	{
		// no reports during warmup period
		return;
	}
	
	switch( holdable )
	{
	case HI_MEDKIT:
		trap_RankReportInt( self, -1, QGR_KEY_MEDKIT_USE, 1, 1 );
		break;
	case HI_TELEPORTER:
		trap_RankReportInt( self, -1, QGR_KEY_TELEPORTER_USE, 1, 1 );
		break;
	default:
		break;
	}
}

/*
================
G_RankReward
================
*/
void G_RankReward( int self, int award )
{
	if( level.warmupTime != 0 )
	{
		// no reports during warmup period
		return;
	}
	
	switch( award )
	{
	case EF_AWARD_IMPRESSIVE:
		trap_RankReportInt( self, -1, QGR_KEY_IMPRESSIVE, 1, 1 );
		break;
	case EF_AWARD_EXCELLENT:
		trap_RankReportInt( self, -1, QGR_KEY_EXCELLENT, 1, 1 );
		break;
	default:
		break;
	}
}

/*
================
G_RankCapture
================
*/
void G_RankCapture( int self )
{
	if( level.warmupTime != 0 )
	{
		// no reports during warmup period
		return;
	}
	
	trap_RankReportInt( self, -1, QGR_KEY_FLAG_CAPTURE, 1, 1 );
}

/*
================
G_RankUserTeamName
================
*/
void G_RankUserTeamName( int self, char* team_name )
{
	if( level.warmupTime != 0 )
	{
		// no reports during warmup period
		return;
	}
	
	trap_RankReportStr( self, -1, QGR_KEY_TEAM_NAME, team_name );
}

/*
================
G_RankClientDisconnect
================
*/
void G_RankClientDisconnect( int self )
{
	gclient_t*	client;
	int			time;
	int			match_rating;
	
	if( level.warmupTime != 0 )
	{
		// no reports during warmup period
		return;
	}
	
	// match rating
	client = g_entities[self].client;
	time = (level.time - client->pers.enterTime) / 1000;
	if( time < 60 )
	{
		match_rating = 0;
	}
	else
	{
		match_rating = client->ps.persistant[PERS_MATCH_RATING] / time;
	}
	trap_RankReportInt( self, -1, QGR_KEY_MATCH_RATING, match_rating, 0 );
}

/*
================
G_RankGameOver
================
*/
void G_RankGameOver( void )
{
	int		i;
	char	str[MAX_INFO_VALUE];
	int		num;
	
	if( level.warmupTime != 0 )
	{
		// no reports during warmup period
		return;
	}
	
	for( i = 0; i < level.maxclients; i++ )
	{
		if( trap_RankUserStatus( i ) == QGR_STATUS_ACTIVE )
		{
			G_RankClientDisconnect( i );
		}
	}
	
	// hostname
	trap_Cvar_VariableStringBuffer( "sv_hostname", str, sizeof(str) );
	trap_RankReportStr( -1, -1, QGR_KEY_HOSTNAME, str );

	// map
	trap_Cvar_VariableStringBuffer( "mapname", str, sizeof(str) );
	trap_RankReportStr( -1, -1, QGR_KEY_MAP, str );

	// mod
	trap_Cvar_VariableStringBuffer( "fs_game", str, sizeof(str) );
	trap_RankReportStr( -1, -1, QGR_KEY_MOD, str );

	// gametype
	num = trap_Cvar_VariableIntegerValue("g_gametype");
	trap_RankReportInt( -1, -1, QGR_KEY_GAMETYPE, num, 0 );
	
	// fraglimit
	num = trap_Cvar_VariableIntegerValue("fraglimit");
	trap_RankReportInt( -1, -1, QGR_KEY_FRAGLIMIT, num, 0 );
	
	// timelimit
	num = trap_Cvar_VariableIntegerValue("timelimit");
	trap_RankReportInt( -1, -1, QGR_KEY_TIMELIMIT, num, 0 );

	// maxclients
	num = trap_Cvar_VariableIntegerValue("sv_maxclients");
	trap_RankReportInt( -1, -1, QGR_KEY_MAXCLIENTS, num, 0 );

	// maxrate
	num = trap_Cvar_VariableIntegerValue("sv_maxRate");
	trap_RankReportInt( -1, -1, QGR_KEY_MAXRATE, num, 0 );

	// minping
	num = trap_Cvar_VariableIntegerValue("sv_minPing");
	trap_RankReportInt( -1, -1, QGR_KEY_MINPING, num, 0 );

	// maxping
	num = trap_Cvar_VariableIntegerValue("sv_maxPing");
	trap_RankReportInt( -1, -1, QGR_KEY_MAXPING, num, 0 );

	// dedicated
	num = trap_Cvar_VariableIntegerValue("dedicated");
	trap_RankReportInt( -1, -1, QGR_KEY_DEDICATED, num, 0 );

	// version
	trap_Cvar_VariableStringBuffer( "version", str, sizeof(str) );
	trap_RankReportStr( -1, -1, QGR_KEY_VERSION, str );
}

