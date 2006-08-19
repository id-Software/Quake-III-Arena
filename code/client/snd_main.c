/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2005 Stuart Dalton (badcdev@gmail.com)

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

#include "client.h"
#include "snd_codec.h"
#include "snd_local.h"
#include "snd_public.h"

cvar_t *s_volume;
cvar_t *s_musicVolume;
cvar_t *s_doppler;
cvar_t *s_backend;

static soundInterface_t si;

/*
=================
S_ValidateInterface
=================
*/
static qboolean S_ValidSoundInterface( soundInterface_t *si )
{
	if( !si->Shutdown ) return qfalse;
	if( !si->StartSound ) return qfalse;
	if( !si->StartLocalSound ) return qfalse;
	if( !si->StartBackgroundTrack ) return qfalse;
	if( !si->StopBackgroundTrack ) return qfalse;
	if( !si->RawSamples ) return qfalse;
	if( !si->StopAllSounds ) return qfalse;
	if( !si->ClearLoopingSounds ) return qfalse;
	if( !si->AddLoopingSound ) return qfalse;
	if( !si->AddRealLoopingSound ) return qfalse;
	if( !si->StopLoopingSound ) return qfalse;
	if( !si->Respatialize ) return qfalse;
	if( !si->UpdateEntityPosition ) return qfalse;
	if( !si->Update ) return qfalse;
	if( !si->DisableSounds ) return qfalse;
	if( !si->BeginRegistration ) return qfalse;
	if( !si->RegisterSound ) return qfalse;
	if( !si->ClearSoundBuffer ) return qfalse;
	if( !si->SoundInfo ) return qfalse;
	if( !si->SoundList ) return qfalse;

	return qtrue;
}

/*
=================
S_StartSound
=================
*/
void S_StartSound( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx )
{
	if( si.StartSound ) {
		si.StartSound( origin, entnum, entchannel, sfx );
	}
}

/*
=================
S_StartLocalSound
=================
*/
void S_StartLocalSound( sfxHandle_t sfx, int channelNum )
{
	if( si.StartLocalSound ) {
		si.StartLocalSound( sfx, channelNum );
	}
}

/*
=================
S_StartBackgroundTrack
=================
*/
void S_StartBackgroundTrack( const char *intro, const char *loop )
{
	if( si.StartBackgroundTrack ) {
		si.StartBackgroundTrack( intro, loop );
	}
}

/*
=================
S_StopBackgroundTrack
=================
*/
void S_StopBackgroundTrack( void )
{
	if( si.StopBackgroundTrack ) {
		si.StopBackgroundTrack( );
	}
}

/*
=================
S_RawSamples
=================
*/
void S_RawSamples (int samples, int rate, int width, int channels,
		   const byte *data, float volume)
{
	if( si.RawSamples ) {
		si.RawSamples( samples, rate, width, channels, data, volume );
	}
}

/*
=================
S_StopAllSounds
=================
*/
void S_StopAllSounds( void )
{
	if( si.StopAllSounds ) {
		si.StopAllSounds( );
	}
}

/*
=================
S_ClearLoopingSounds
=================
*/
void S_ClearLoopingSounds( qboolean killall )
{
	if( si.ClearLoopingSounds ) {
		si.ClearLoopingSounds( killall );
	}
}

/*
=================
S_AddLoopingSound
=================
*/
void S_AddLoopingSound( int entityNum, const vec3_t origin,
		const vec3_t velocity, sfxHandle_t sfx )
{
	if( si.AddLoopingSound ) {
		si.AddLoopingSound( entityNum, origin, velocity, sfx );
	}
}

/*
=================
S_AddRealLoopingSound
=================
*/
void S_AddRealLoopingSound( int entityNum, const vec3_t origin,
		const vec3_t velocity, sfxHandle_t sfx )
{
	if( si.AddRealLoopingSound ) {
		si.AddRealLoopingSound( entityNum, origin, velocity, sfx );
	}
}

/*
=================
S_StopLoopingSound
=================
*/
void S_StopLoopingSound( int entityNum )
{
	if( si.StopLoopingSound ) {
		si.StopLoopingSound( entityNum );
	}
}

/*
=================
S_Respatialize
=================
*/
void S_Respatialize( int entityNum, const vec3_t origin,
		vec3_t axis[3], int inwater )
{
	if( si.Respatialize ) {
		si.Respatialize( entityNum, origin, axis, inwater );
	}
}

/*
=================
S_UpdateEntityPosition
=================
*/
void S_UpdateEntityPosition( int entityNum, const vec3_t origin )
{
	if( si.UpdateEntityPosition ) {
		si.UpdateEntityPosition( entityNum, origin );
	}
}

/*
=================
S_Update
=================
*/
void S_Update( void )
{
	if( si.Update ) {
		si.Update( );
	}
}

/*
=================
S_DisableSounds
=================
*/
void S_DisableSounds( void )
{
	if( si.DisableSounds ) {
		si.DisableSounds( );
	}
}

/*
=================
S_BeginRegistration
=================
*/
void S_BeginRegistration( void )
{
	if( si.BeginRegistration ) {
		si.BeginRegistration( );
	}
}

/*
=================
S_RegisterSound
=================
*/
sfxHandle_t	S_RegisterSound( const char *sample, qboolean compressed )
{
	if( si.RegisterSound ) {
		return si.RegisterSound( sample, compressed );
	} else {
		return 0;
	}
}

/*
=================
S_ClearSoundBuffer
=================
*/
void S_ClearSoundBuffer( void )
{
	if( si.ClearSoundBuffer ) {
		si.ClearSoundBuffer( );
	}
}

/*
=================
S_SoundInfo
=================
*/
void S_SoundInfo( void )
{
	if( si.SoundInfo ) {
		si.SoundInfo( );
	}
}

/*
=================
S_SoundList
=================
*/
void S_SoundList( void )
{
	if( si.SoundList ) {
		si.SoundList( );
	}
}

//=============================================================================

/*
=================
S_Play_f
=================
*/
void S_Play_f( void ) {
	int 		i;
	sfxHandle_t	h;
	char		name[256];

	if( !si.RegisterSound || !si.StartLocalSound ) {
		return;
	}

	i = 1;
	while ( i<Cmd_Argc() ) {
		if ( !Q_strrchr(Cmd_Argv(i), '.') ) {
			Com_sprintf( name, sizeof(name), "%s.wav", Cmd_Argv(1) );
		} else {
			Q_strncpyz( name, Cmd_Argv(i), sizeof(name) );
		}
		h = si.RegisterSound( name, qfalse );
		if( h ) {
			si.StartLocalSound( h, CHAN_LOCAL_SOUND );
		}
		i++;
	}
}

/*
=================
S_Music_f
=================
*/
void S_Music_f( void ) {
	int		c;

	if( !si.StartBackgroundTrack ) {
		return;
	}

	c = Cmd_Argc();

	if ( c == 2 ) {
		si.StartBackgroundTrack( Cmd_Argv(1), NULL );
	} else if ( c == 3 ) {
		si.StartBackgroundTrack( Cmd_Argv(1), Cmd_Argv(2) );
	} else {
		Com_Printf ("music <musicfile> [loopfile]\n");
		return;
	}

}

//=============================================================================

/*
=================
S_Init
=================
*/
void S_Init( void )
{
	cvar_t		*cv;
	qboolean	started = qfalse;

	Com_Printf( "------ Initializing Sound ------\n" );

	s_volume = Cvar_Get( "s_volume", "0.8", CVAR_ARCHIVE );
	s_musicVolume = Cvar_Get( "s_musicvolume", "0.25", CVAR_ARCHIVE );
	s_doppler = Cvar_Get( "s_doppler", "1", CVAR_ARCHIVE );
	s_backend = Cvar_Get( "s_backend", "", CVAR_ROM );

	cv = Cvar_Get( "s_initsound", "1", 0 );
	if( !cv->integer ) {
		Com_Printf( "Sound disabled.\n" );
	} else {

		S_CodecInit( );

		Cmd_AddCommand( "play", S_Play_f );
		Cmd_AddCommand( "music", S_Music_f );
		Cmd_AddCommand( "s_list", S_SoundList );
		Cmd_AddCommand( "s_stop", S_StopAllSounds );
		Cmd_AddCommand( "s_info", S_SoundInfo );

		cv = Cvar_Get( "s_useOpenAL", "1", CVAR_ARCHIVE );
		if( cv->integer ) {
			//OpenAL
			started = S_AL_Init( &si );
			Cvar_Set( "s_backend", "OpenAL" );
		}

		if( !started ) {
			started = S_Base_Init( &si );
			Cvar_Set( "s_backend", "base" );
		}

		if( started ) {
			if( !S_ValidSoundInterface( &si ) ) {
				Com_Error( ERR_FATAL, "Sound interface invalid." );
			}

			S_SoundInfo( );
			Com_Printf( "Sound initialization successful.\n" );
		} else {
			Com_Printf( "Sound initialization failed.\n" );
		}
	}

	Com_Printf( "--------------------------------\n");
}

/*
=================
S_Shutdown
=================
*/
void S_Shutdown( void )
{
	if( si.Shutdown ) {
		si.Shutdown( );
	}

	Com_Memset( &si, 0, sizeof( soundInterface_t ) );

	Cmd_RemoveCommand( "play" );
	Cmd_RemoveCommand( "music");
	Cmd_RemoveCommand( "s_list" );
	Cmd_RemoveCommand( "s_stop" );
	Cmd_RemoveCommand( "s_info" );

	S_CodecShutdown( );
}

