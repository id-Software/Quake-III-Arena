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

/*****************************************************************************
 * name:		snd_dma.c
 *
 * desc:		main control for any streaming sound output device
 *
 * $Archive: /MissionPack/code/client/snd_dma.c $
 *
 *****************************************************************************/

#include "snd_local.h"
#include "snd_codec.h"
#include "client.h"

void S_Update_( void );
void S_Base_StopAllSounds(void);
void S_Base_StopBackgroundTrack( void );

snd_stream_t	*s_backgroundStream = NULL;
static char		s_backgroundLoop[MAX_QPATH];
//static char		s_backgroundMusic[MAX_QPATH]; //TTimo: unused


// =======================================================================
// Internal sound data & structures
// =======================================================================

// only begin attenuating sound volumes when outside the FULLVOLUME range
#define		SOUND_FULLVOLUME	80

#define		SOUND_ATTENUATE		0.0008f

channel_t   s_channels[MAX_CHANNELS];
channel_t   loop_channels[MAX_CHANNELS];
int			numLoopChannels;

static int	s_soundStarted;
static		qboolean	s_soundMuted;

dma_t		dma;

static int			listener_number;
static vec3_t		listener_origin;
static vec3_t		listener_axis[3];

int			s_soundtime;		// sample PAIRS
int   		s_paintedtime; 		// sample PAIRS

// MAX_SFX may be larger than MAX_SOUNDS because
// of custom player sounds
#define		MAX_SFX			4096
sfx_t		s_knownSfx[MAX_SFX];
int			s_numSfx = 0;

#define		LOOP_HASH		128
static	sfx_t		*sfxHash[LOOP_HASH];

cvar_t		*s_testsound;
cvar_t		*s_show;
cvar_t		*s_mixahead;
cvar_t		*s_mixPreStep;

static loopSound_t		loopSounds[MAX_GENTITIES];
static	channel_t		*freelist = NULL;

int						s_rawend[MAX_RAW_STREAMS];
portable_samplepair_t s_rawsamples[MAX_RAW_STREAMS][MAX_RAW_SAMPLES];


// ====================================================================
// User-setable variables
// ====================================================================


void S_Base_SoundInfo(void) {	
	Com_Printf("----- Sound Info -----\n" );
	if (!s_soundStarted) {
		Com_Printf ("sound system not started\n");
	} else {
		Com_Printf("%5d stereo\n", dma.channels - 1);
		Com_Printf("%5d samples\n", dma.samples);
		Com_Printf("%5d samplebits\n", dma.samplebits);
		Com_Printf("%5d submission_chunk\n", dma.submission_chunk);
		Com_Printf("%5d speed\n", dma.speed);
		Com_Printf("%p dma buffer\n", dma.buffer);
		if ( s_backgroundStream ) {
			Com_Printf("Background file: %s\n", s_backgroundLoop );
		} else {
			Com_Printf("No background file.\n" );
		}

	}
	Com_Printf("----------------------\n" );
}


#ifdef USE_VOIP
static
void S_Base_StartCapture( void )
{
	// !!! FIXME: write me.
}

static
int S_Base_AvailableCaptureSamples( void )
{
	// !!! FIXME: write me.
	return 0;
}

static
void S_Base_Capture( int samples, byte *data )
{
	// !!! FIXME: write me.
}

static
void S_Base_StopCapture( void )
{
	// !!! FIXME: write me.
}

static
void S_Base_MasterGain( float val )
{
	// !!! FIXME: write me.
}
#endif



/*
=================
S_Base_SoundList
=================
*/
void S_Base_SoundList( void ) {
	int		i;
	sfx_t	*sfx;
	int		size, total;
	char	type[4][16];
	char	mem[2][16];

	strcpy(type[0], "16bit");
	strcpy(type[1], "adpcm");
	strcpy(type[2], "daub4");
	strcpy(type[3], "mulaw");
	strcpy(mem[0], "paged out");
	strcpy(mem[1], "resident ");
	total = 0;
	for (sfx=s_knownSfx, i=0 ; i<s_numSfx ; i++, sfx++) {
		size = sfx->soundLength;
		total += size;
		Com_Printf("%6i[%s] : %s[%s]\n", size, type[sfx->soundCompressionMethod],
				sfx->soundName, mem[sfx->inMemory] );
	}
	Com_Printf ("Total resident: %i\n", total);
	S_DisplayFreeMemory();
}



void S_ChannelFree(channel_t *v) {
	v->thesfx = NULL;
	*(channel_t **)v = freelist;
	freelist = (channel_t*)v;
}

channel_t*	S_ChannelMalloc( void ) {
	channel_t *v;
	if (freelist == NULL) {
		return NULL;
	}
	v = freelist;
	freelist = *(channel_t **)freelist;
	v->allocTime = Com_Milliseconds();
	return v;
}

void S_ChannelSetup( void ) {
	channel_t *p, *q;

	// clear all the sounds so they don't
	Com_Memset( s_channels, 0, sizeof( s_channels ) );

	p = s_channels;;
	q = p + MAX_CHANNELS;
	while (--q > p) {
		*(channel_t **)q = q-1;
	}
	
	*(channel_t **)q = NULL;
	freelist = p + MAX_CHANNELS - 1;
	Com_DPrintf("Channel memory manager started\n");
}



// =======================================================================
// Load a sound
// =======================================================================

/*
================
return a hash value for the sfx name
================
*/
static long S_HashSFXName(const char *name) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (name[i] != '\0') {
		letter = tolower(name[i]);
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash &= (LOOP_HASH-1);
	return hash;
}

/*
==================
S_FindName

Will allocate a new sfx if it isn't found
==================
*/
static sfx_t *S_FindName( const char *name ) {
	int		i;
	int		hash;

	sfx_t	*sfx;

	if (!name) {
		Com_Error(ERR_FATAL, "Sound name is NULL");
	}

	if (!name[0]) {
		Com_Printf( S_COLOR_YELLOW "WARNING: Sound name is empty\n" );
		return NULL;
	}

	if (strlen(name) >= MAX_QPATH) {
		Com_Printf( S_COLOR_YELLOW "WARNING: Sound name is too long: %s\n", name );
		return NULL;
	}

	if (name[0] == '*') {
		Com_Printf( S_COLOR_YELLOW "WARNING: Tried to load player sound directly: %s\n", name );
		return NULL;
	}

	hash = S_HashSFXName(name);

	sfx = sfxHash[hash];
	// see if already loaded
	while (sfx) {
		if (!Q_stricmp(sfx->soundName, name) ) {
			return sfx;
		}
		sfx = sfx->next;
	}

	// find a free sfx
	for (i=0 ; i < s_numSfx ; i++) {
		if (!s_knownSfx[i].soundName[0]) {
			break;
		}
	}

	if (i == s_numSfx) {
		if (s_numSfx == MAX_SFX) {
			Com_Error (ERR_FATAL, "S_FindName: out of sfx_t");
		}
		s_numSfx++;
	}
	
	sfx = &s_knownSfx[i];
	Com_Memset (sfx, 0, sizeof(*sfx));
	strcpy (sfx->soundName, name);

	sfx->next = sfxHash[hash];
	sfxHash[hash] = sfx;

	return sfx;
}

/*
=================
S_DefaultSound
=================
*/
void S_DefaultSound( sfx_t *sfx ) {
	
	int		i;

	sfx->soundLength = 512;
	sfx->soundData = SND_malloc();
	sfx->soundData->next = NULL;


	for ( i = 0 ; i < sfx->soundLength ; i++ ) {
		sfx->soundData->sndChunk[i] = i;
	}
}

/*
===================
S_DisableSounds

Disables sounds until the next S_BeginRegistration.
This is called when the hunk is cleared and the sounds
are no longer valid.
===================
*/
void S_Base_DisableSounds( void ) {
	S_Base_StopAllSounds();
	s_soundMuted = qtrue;
}

/*
==================
S_RegisterSound

Creates a default buzz sound if the file can't be loaded
==================
*/
sfxHandle_t	S_Base_RegisterSound( const char *name, qboolean compressed ) {
	sfx_t	*sfx;

	compressed = qfalse;
	if (!s_soundStarted) {
		return 0;
	}

	sfx = S_FindName( name );
	if ( !sfx ) {
		return 0;
	}

	if ( sfx->soundData ) {
		if ( sfx->defaultSound ) {
			Com_Printf( S_COLOR_YELLOW "WARNING: could not find %s - using default\n", sfx->soundName );
			return 0;
		}
		return sfx - s_knownSfx;
	}

	sfx->inMemory = qfalse;
	sfx->soundCompressed = compressed;

  S_memoryLoad(sfx);

	if ( sfx->defaultSound ) {
		Com_Printf( S_COLOR_YELLOW "WARNING: could not find %s - using default\n", sfx->soundName );
		return 0;
	}

	return sfx - s_knownSfx;
}

/*
=====================
S_BeginRegistration

=====================
*/
void S_Base_BeginRegistration( void ) {
	s_soundMuted = qfalse;		// we can play again

	if (s_numSfx == 0) {
		SND_setup();

		Com_Memset(s_knownSfx, '\0', sizeof(s_knownSfx));
		Com_Memset(sfxHash, '\0', sizeof(sfx_t *) * LOOP_HASH);

		S_Base_RegisterSound("sound/feedback/hit.wav", qfalse);		// changed to a sound in baseq3
	}
}

void S_memoryLoad(sfx_t	*sfx) {
	// load the sound file
	if ( !S_LoadSound ( sfx ) ) {
//		Com_Printf( S_COLOR_YELLOW "WARNING: couldn't load sound: %s\n", sfx->soundName );
		sfx->defaultSound = qtrue;
	}
	sfx->inMemory = qtrue;
}

//=============================================================================

/*
=================
S_SpatializeOrigin

Used for spatializing s_channels
=================
*/
void S_SpatializeOrigin (vec3_t origin, int master_vol, int *left_vol, int *right_vol)
{
    vec_t		dot;
    vec_t		dist;
    vec_t		lscale, rscale, scale;
    vec3_t		source_vec;
    vec3_t		vec;

	const float dist_mult = SOUND_ATTENUATE;
	
	// calculate stereo seperation and distance attenuation
	VectorSubtract(origin, listener_origin, source_vec);

	dist = VectorNormalize(source_vec);
	dist -= SOUND_FULLVOLUME;
	if (dist < 0)
		dist = 0;			// close enough to be at full volume
	dist *= dist_mult;		// different attenuation levels
	
	VectorRotate( source_vec, listener_axis, vec );

	dot = -vec[1];

	if (dma.channels == 1)
	{ // no attenuation = no spatialization
		rscale = 1.0;
		lscale = 1.0;
	}
	else
	{
		rscale = 0.5 * (1.0 + dot);
		lscale = 0.5 * (1.0 - dot);
		if ( rscale < 0 ) {
			rscale = 0;
		}
		if ( lscale < 0 ) {
			lscale = 0;
		}
	}

	// add in distance effect
	scale = (1.0 - dist) * rscale;
	*right_vol = (master_vol * scale);
	if (*right_vol < 0)
		*right_vol = 0;

	scale = (1.0 - dist) * lscale;
	*left_vol = (master_vol * scale);
	if (*left_vol < 0)
		*left_vol = 0;
}

// =======================================================================
// Start a sound effect
// =======================================================================

/*
=================
S_Base_HearingThroughEntity

Also see S_AL_HearingThroughEntity
=================
*/
static qboolean S_Base_HearingThroughEntity( int entityNum, vec3_t origin )
{
	float	distanceSq;
	vec3_t	sorigin;

	if (origin)
		VectorCopy(origin, sorigin);
	else
		VectorCopy(loopSounds[entityNum].origin, sorigin);

	if( listener_number == entityNum )
	{
		// This is an outrageous hack to detect
		// whether or not the player is rendering in third person or not. We can't
		// ask the renderer because the renderer has no notion of entities and we
		// can't ask cgame since that would involve changing the API and hence mod
		// compatibility. I don't think there is any way around this, but I'll leave
		// the FIXME just in case anyone has a bright idea.
		distanceSq = DistanceSquared(
				sorigin,
				listener_origin );

		if( distanceSq > THIRD_PERSON_THRESHOLD_SQ )
			return qfalse; //we're the player, but third person
		else
			return qtrue;  //we're the player
	}
	else
		return qfalse; //not the player
}

/*
====================
S_Base_StartSoundEx

Validates the parms and ques the sound up
if origin is NULL, the sound will be dynamically sourced from the entity
Entchannel 0 will never override a playing sound
====================
*/
static void S_Base_StartSoundEx( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfxHandle, qboolean localSound ) {
	channel_t	*ch;
	sfx_t		*sfx;
  int i, oldest, chosen, time;
  int	inplay, allowed;
	qboolean	fullVolume;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	if ( !origin && ( entityNum < 0 || entityNum >= MAX_GENTITIES ) ) {
		Com_Error( ERR_DROP, "S_StartSound: bad entitynum %i", entityNum );
	}

	if ( sfxHandle < 0 || sfxHandle >= s_numSfx ) {
		Com_Printf( S_COLOR_YELLOW "S_StartSound: handle %i out of range\n", sfxHandle );
		return;
	}

	sfx = &s_knownSfx[ sfxHandle ];

	if (sfx->inMemory == qfalse) {
		S_memoryLoad(sfx);
	}

	if ( s_show->integer == 1 ) {
		Com_Printf( "%i : %s\n", s_paintedtime, sfx->soundName );
	}

	time = Com_Milliseconds();

//	Com_Printf("playing %s\n", sfx->soundName);
	// pick a channel to play on

	allowed = 4;
	if (entityNum == listener_number) {
		allowed = 8;
	}

	fullVolume = qfalse;
	if (localSound || S_Base_HearingThroughEntity(entityNum, origin)) {
		fullVolume = qtrue;
	}

	ch = s_channels;
	inplay = 0;
	for ( i = 0; i < MAX_CHANNELS ; i++, ch++ ) {		
		if (ch->entnum == entityNum && ch->thesfx == sfx) {
			if (time - ch->allocTime < 50) {
//				if (Cvar_VariableValue( "cg_showmiss" )) {
//					Com_Printf("double sound start\n");
//				}
				return;
			}
			inplay++;
		}
	}

	if (inplay>allowed) {
		return;
	}

	sfx->lastTimeUsed = time;

	ch = S_ChannelMalloc();	// entityNum, entchannel);
	if (!ch) {
		ch = s_channels;

		oldest = sfx->lastTimeUsed;
		chosen = -1;
		for ( i = 0 ; i < MAX_CHANNELS ; i++, ch++ ) {
			if (ch->entnum != listener_number && ch->entnum == entityNum && ch->allocTime<oldest && ch->entchannel != CHAN_ANNOUNCER) {
				oldest = ch->allocTime;
				chosen = i;
			}
		}
		if (chosen == -1) {
			ch = s_channels;
			for ( i = 0 ; i < MAX_CHANNELS ; i++, ch++ ) {
				if (ch->entnum != listener_number && ch->allocTime<oldest && ch->entchannel != CHAN_ANNOUNCER) {
					oldest = ch->allocTime;
					chosen = i;
				}
			}
			if (chosen == -1) {
				ch = s_channels;
				if (ch->entnum == listener_number) {
					for ( i = 0 ; i < MAX_CHANNELS ; i++, ch++ ) {
						if (ch->allocTime<oldest) {
							oldest = ch->allocTime;
							chosen = i;
						}
					}
				}
				if (chosen == -1) {
					Com_Printf("dropping sound\n");
					return;
				}
			}
		}
		ch = &s_channels[chosen];
		ch->allocTime = sfx->lastTimeUsed;
	}

	if (origin) {
		VectorCopy (origin, ch->origin);
		ch->fixed_origin = qtrue;
	} else {
		ch->fixed_origin = qfalse;
	}

	ch->master_vol = 127;
	ch->entnum = entityNum;
	ch->thesfx = sfx;
	ch->startSample = START_SAMPLE_IMMEDIATE;
	ch->entchannel = entchannel;
	ch->leftvol = ch->master_vol;		// these will get calced at next spatialize
	ch->rightvol = ch->master_vol;		// unless the game isn't running
	ch->doppler = qfalse;
	ch->fullVolume = fullVolume;
}

/*
====================
S_StartSound

if origin is NULL, the sound will be dynamically sourced from the entity
====================
*/
void S_Base_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfxHandle ) {
	S_Base_StartSoundEx( origin, entityNum, entchannel, sfxHandle, qfalse );
}

/*
==================
S_StartLocalSound
==================
*/
void S_Base_StartLocalSound( sfxHandle_t sfxHandle, int channelNum ) {
	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	if ( sfxHandle < 0 || sfxHandle >= s_numSfx ) {
		Com_Printf( S_COLOR_YELLOW "S_StartLocalSound: handle %i out of range\n", sfxHandle );
		return;
	}

	S_Base_StartSoundEx( NULL, listener_number, channelNum, sfxHandle, qtrue );
}


/*
==================
S_ClearSoundBuffer

If we are about to perform file access, clear the buffer
so sound doesn't stutter.
==================
*/
void S_Base_ClearSoundBuffer( void ) {
	int		clear;
		
	if (!s_soundStarted)
		return;

	// stop looping sounds
	Com_Memset(loopSounds, 0, MAX_GENTITIES*sizeof(loopSound_t));
	Com_Memset(loop_channels, 0, MAX_CHANNELS*sizeof(channel_t));
	numLoopChannels = 0;

	S_ChannelSetup();

	Com_Memset(s_rawend, '\0', sizeof (s_rawend));

	if (dma.samplebits == 8)
		clear = 0x80;
	else
		clear = 0;

	SNDDMA_BeginPainting ();
	if (dma.buffer)
		Com_Memset(dma.buffer, clear, dma.samples * dma.samplebits/8);
	SNDDMA_Submit ();
}

/*
==================
S_StopAllSounds
==================
*/
void S_Base_StopAllSounds(void) {
	if ( !s_soundStarted ) {
		return;
	}

	// stop the background music
	S_Base_StopBackgroundTrack();

	S_Base_ClearSoundBuffer ();
}

/*
==============================================================

continuous looping sounds are added each frame

==============================================================
*/

void S_Base_StopLoopingSound(int entityNum) {
	loopSounds[entityNum].active = qfalse;
//	loopSounds[entityNum].sfx = 0;
	loopSounds[entityNum].kill = qfalse;
}

/*
==================
S_ClearLoopingSounds

==================
*/
void S_Base_ClearLoopingSounds( qboolean killall ) {
	int i;
	for ( i = 0 ; i < MAX_GENTITIES ; i++) {
		if (killall || loopSounds[i].kill == qtrue || (loopSounds[i].sfx && loopSounds[i].sfx->soundLength == 0)) {
			S_Base_StopLoopingSound(i);
		}
	}
	numLoopChannels = 0;
}

/*
==================
S_AddLoopingSound

Called during entity generation for a frame
Include velocity in case I get around to doing doppler...
==================
*/
void S_Base_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfxHandle ) {
	sfx_t *sfx;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	if ( sfxHandle < 0 || sfxHandle >= s_numSfx ) {
		Com_Printf( S_COLOR_YELLOW "S_AddLoopingSound: handle %i out of range\n", sfxHandle );
		return;
	}

	sfx = &s_knownSfx[ sfxHandle ];

	if (sfx->inMemory == qfalse) {
		S_memoryLoad(sfx);
	}

	if ( !sfx->soundLength ) {
		Com_Error( ERR_DROP, "%s has length 0", sfx->soundName );
	}

	VectorCopy( origin, loopSounds[entityNum].origin );
	VectorCopy( velocity, loopSounds[entityNum].velocity );
	loopSounds[entityNum].active = qtrue;
	loopSounds[entityNum].kill = qtrue;
	loopSounds[entityNum].doppler = qfalse;
	loopSounds[entityNum].oldDopplerScale = 1.0;
	loopSounds[entityNum].dopplerScale = 1.0;
	loopSounds[entityNum].sfx = sfx;

	if (s_doppler->integer && VectorLengthSquared(velocity)>0.0) {
		vec3_t	out;
		float	lena, lenb;

		loopSounds[entityNum].doppler = qtrue;
		lena = DistanceSquared(loopSounds[listener_number].origin, loopSounds[entityNum].origin);
		VectorAdd(loopSounds[entityNum].origin, loopSounds[entityNum].velocity, out);
		lenb = DistanceSquared(loopSounds[listener_number].origin, out);
		if ((loopSounds[entityNum].framenum+1) != cls.framecount) {
			loopSounds[entityNum].oldDopplerScale = 1.0;
		} else {
			loopSounds[entityNum].oldDopplerScale = loopSounds[entityNum].dopplerScale;
		}
		loopSounds[entityNum].dopplerScale = lenb/(lena*100);
		if (loopSounds[entityNum].dopplerScale<=1.0) {
			loopSounds[entityNum].doppler = qfalse;			// don't bother doing the math
		} else if (loopSounds[entityNum].dopplerScale>MAX_DOPPLER_SCALE) {
			loopSounds[entityNum].dopplerScale = MAX_DOPPLER_SCALE;
		}
	}

	loopSounds[entityNum].framenum = cls.framecount;
}

/*
==================
S_AddLoopingSound

Called during entity generation for a frame
Include velocity in case I get around to doing doppler...
==================
*/
void S_Base_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfxHandle ) {
	sfx_t *sfx;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	if ( sfxHandle < 0 || sfxHandle >= s_numSfx ) {
		Com_Printf( S_COLOR_YELLOW "S_AddRealLoopingSound: handle %i out of range\n", sfxHandle );
		return;
	}

	sfx = &s_knownSfx[ sfxHandle ];

	if (sfx->inMemory == qfalse) {
		S_memoryLoad(sfx);
	}

	if ( !sfx->soundLength ) {
		Com_Error( ERR_DROP, "%s has length 0", sfx->soundName );
	}
	VectorCopy( origin, loopSounds[entityNum].origin );
	VectorCopy( velocity, loopSounds[entityNum].velocity );
	loopSounds[entityNum].sfx = sfx;
	loopSounds[entityNum].active = qtrue;
	loopSounds[entityNum].kill = qfalse;
	loopSounds[entityNum].doppler = qfalse;
}



/*
==================
S_AddLoopSounds

Spatialize all of the looping sounds.
All sounds are on the same cycle, so any duplicates can just
sum up the channel multipliers.
==================
*/
void S_AddLoopSounds (void) {
	int			i, j, time;
	int			left_total, right_total, left, right;
	channel_t	*ch;
	loopSound_t	*loop, *loop2;
	static int	loopFrame;


	numLoopChannels = 0;

	time = Com_Milliseconds();

	loopFrame++;
	for ( i = 0 ; i < MAX_GENTITIES ; i++) {
		loop = &loopSounds[i];
		if ( !loop->active || loop->mergeFrame == loopFrame ) {
			continue;	// already merged into an earlier sound
		}

		if (loop->kill) {
			S_SpatializeOrigin( loop->origin, 127, &left_total, &right_total);			// 3d
		} else {
			S_SpatializeOrigin( loop->origin, 90,  &left_total, &right_total);			// sphere
		}

		loop->sfx->lastTimeUsed = time;

		for (j=(i+1); j< MAX_GENTITIES ; j++) {
			loop2 = &loopSounds[j];
			if ( !loop2->active || loop2->doppler || loop2->sfx != loop->sfx) {
				continue;
			}
			loop2->mergeFrame = loopFrame;

			if (loop2->kill) {
				S_SpatializeOrigin( loop2->origin, 127, &left, &right);				// 3d
			} else {
				S_SpatializeOrigin( loop2->origin, 90,  &left, &right);				// sphere
			}

			loop2->sfx->lastTimeUsed = time;
			left_total += left;
			right_total += right;
		}
		if (left_total == 0 && right_total == 0) {
			continue;		// not audible
		}

		// allocate a channel
		ch = &loop_channels[numLoopChannels];
		
		if (left_total > 255) {
			left_total = 255;
		}
		if (right_total > 255) {
			right_total = 255;
		}
		
		ch->master_vol = 127;
		ch->leftvol = left_total;
		ch->rightvol = right_total;
		ch->thesfx = loop->sfx;
		ch->doppler = loop->doppler;
		ch->dopplerScale = loop->dopplerScale;
		ch->oldDopplerScale = loop->oldDopplerScale;
		ch->fullVolume = qfalse;
		numLoopChannels++;
		if (numLoopChannels == MAX_CHANNELS) {
			return;
		}
	}
}

//=============================================================================

/*
=================
S_ByteSwapRawSamples

If raw data has been loaded in little endien binary form, this must be done.
If raw data was calculated, as with ADPCM, this should not be called.
=================
*/
void S_ByteSwapRawSamples( int samples, int width, int s_channels, const byte *data ) {
	int		i;

	if ( width != 2 ) {
		return;
	}
	if ( LittleShort( 256 ) == 256 ) {
		return;
	}

	if ( s_channels == 2 ) {
		samples <<= 1;
	}
	for ( i = 0 ; i < samples ; i++ ) {
		((short *)data)[i] = LittleShort( ((short *)data)[i] );
	}
}

/*
============
S_Base_RawSamples

Music streaming
============
*/
void S_Base_RawSamples( int stream, int samples, int rate, int width, int s_channels, const byte *data, float volume, int entityNum)
{
	int		i;
	int		src, dst;
	float	scale;
	int		intVolumeLeft, intVolumeRight;
	portable_samplepair_t *rawsamples;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	if ( (stream < 0) || (stream >= MAX_RAW_STREAMS) ) {
		return;
	}

	rawsamples = s_rawsamples[stream];

	if ( s_muted->integer ) {
		intVolumeLeft = intVolumeRight = 0;
	} else {
		int leftvol, rightvol;

		if ( entityNum >= 0 && entityNum < MAX_GENTITIES ) {
			// support spatialized raw streams, e.g. for VoIP
			S_SpatializeOrigin( loopSounds[ entityNum ].origin, 256, &leftvol, &rightvol );
		} else {
			leftvol = rightvol = 256;
		}

		intVolumeLeft = leftvol * volume * s_volume->value;
		intVolumeRight = rightvol * volume * s_volume->value;
	}

	if ( s_rawend[stream] < s_soundtime ) {
		Com_DPrintf( "S_Base_RawSamples: resetting minimum: %i < %i\n", s_rawend[stream], s_soundtime );
		s_rawend[stream] = s_soundtime;
	}

	scale = (float)rate / dma.speed;

//Com_Printf ("%i < %i < %i\n", s_soundtime, s_paintedtime, s_rawend[stream]);
	if (s_channels == 2 && width == 2)
	{
		if (scale == 1.0)
		{	// optimized case
			for (i=0 ; i<samples ; i++)
			{
				dst = s_rawend[stream]&(MAX_RAW_SAMPLES-1);
				s_rawend[stream]++;
				rawsamples[dst].left = ((short *)data)[i*2] * intVolumeLeft;
				rawsamples[dst].right = ((short *)data)[i*2+1] * intVolumeRight;
			}
		}
		else
		{
			for (i=0 ; ; i++)
			{
				src = i*scale;
				if (src >= samples)
					break;
				dst = s_rawend[stream]&(MAX_RAW_SAMPLES-1);
				s_rawend[stream]++;
				rawsamples[dst].left = ((short *)data)[src*2] * intVolumeLeft;
				rawsamples[dst].right = ((short *)data)[src*2+1] * intVolumeRight;
			}
		}
	}
	else if (s_channels == 1 && width == 2)
	{
		for (i=0 ; ; i++)
		{
			src = i*scale;
			if (src >= samples)
				break;
			dst = s_rawend[stream]&(MAX_RAW_SAMPLES-1);
			s_rawend[stream]++;
			rawsamples[dst].left = ((short *)data)[src] * intVolumeLeft;
			rawsamples[dst].right = ((short *)data)[src] * intVolumeRight;
		}
	}
	else if (s_channels == 2 && width == 1)
	{
		intVolumeLeft *= 256;
		intVolumeRight *= 256;

		for (i=0 ; ; i++)
		{
			src = i*scale;
			if (src >= samples)
				break;
			dst = s_rawend[stream]&(MAX_RAW_SAMPLES-1);
			s_rawend[stream]++;
			rawsamples[dst].left = ((char *)data)[src*2] * intVolumeLeft;
			rawsamples[dst].right = ((char *)data)[src*2+1] * intVolumeRight;
		}
	}
	else if (s_channels == 1 && width == 1)
	{
		intVolumeLeft *= 256;
		intVolumeRight *= 256;

		for (i=0 ; ; i++)
		{
			src = i*scale;
			if (src >= samples)
				break;
			dst = s_rawend[stream]&(MAX_RAW_SAMPLES-1);
			s_rawend[stream]++;
			rawsamples[dst].left = (((byte *)data)[src]-128) * intVolumeLeft;
			rawsamples[dst].right = (((byte *)data)[src]-128) * intVolumeRight;
		}
	}

	if ( s_rawend[stream] > s_soundtime + MAX_RAW_SAMPLES ) {
		Com_DPrintf( "S_Base_RawSamples: overflowed %i > %i\n", s_rawend[stream], s_soundtime );
	}
}

//=============================================================================

/*
=====================
S_UpdateEntityPosition

let the sound system know where an entity currently is
======================
*/
void S_Base_UpdateEntityPosition( int entityNum, const vec3_t origin ) {
	if ( entityNum < 0 || entityNum >= MAX_GENTITIES ) {
		Com_Error( ERR_DROP, "S_UpdateEntityPosition: bad entitynum %i", entityNum );
	}
	VectorCopy( origin, loopSounds[entityNum].origin );
}


/*
============
S_Respatialize

Change the volumes of all the playing sounds for changes in their positions
============
*/
void S_Base_Respatialize( int entityNum, const vec3_t head, vec3_t axis[3], int inwater ) {
	int			i;
	channel_t	*ch;
	vec3_t		origin;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	listener_number = entityNum;
	VectorCopy(head, listener_origin);
	VectorCopy(axis[0], listener_axis[0]);
	VectorCopy(axis[1], listener_axis[1]);
	VectorCopy(axis[2], listener_axis[2]);

	// update spatialization for dynamic sounds	
	ch = s_channels;
	for ( i = 0 ; i < MAX_CHANNELS ; i++, ch++ ) {
		if ( !ch->thesfx ) {
			continue;
		}
		// local and first person sounds will always be full volume
		if (ch->fullVolume) {
			ch->leftvol = ch->master_vol;
			ch->rightvol = ch->master_vol;
		} else {
			if (ch->fixed_origin) {
				VectorCopy( ch->origin, origin );
			} else {
				VectorCopy( loopSounds[ ch->entnum ].origin, origin );
			}

			S_SpatializeOrigin (origin, ch->master_vol, &ch->leftvol, &ch->rightvol);
		}
	}

	// add loopsounds
	S_AddLoopSounds ();
}


/*
========================
S_ScanChannelStarts

Returns qtrue if any new sounds were started since the last mix
========================
*/
qboolean S_ScanChannelStarts( void ) {
	channel_t		*ch;
	int				i;
	qboolean		newSamples;

	newSamples = qfalse;
	ch = s_channels;

	for (i=0; i<MAX_CHANNELS ; i++, ch++) {
		if ( !ch->thesfx ) {
			continue;
		}
		// if this channel was just started this frame,
		// set the sample count to it begins mixing
		// into the very first sample
		if ( ch->startSample == START_SAMPLE_IMMEDIATE ) {
			ch->startSample = s_paintedtime;
			newSamples = qtrue;
			continue;
		}

		// if it is completely finished by now, clear it
		if ( ch->startSample + (ch->thesfx->soundLength) <= s_paintedtime ) {
			S_ChannelFree(ch);
		}
	}

	return newSamples;
}

/*
============
S_Update

Called once each time through the main loop
============
*/
void S_Base_Update( void ) {
	int			i;
	int			total;
	channel_t	*ch;

	if ( !s_soundStarted || s_soundMuted ) {
//		Com_DPrintf ("not started or muted\n");
		return;
	}

	//
	// debugging output
	//
	if ( s_show->integer == 2 ) {
		total = 0;
		ch = s_channels;
		for (i=0 ; i<MAX_CHANNELS; i++, ch++) {
			if (ch->thesfx && (ch->leftvol || ch->rightvol) ) {
				Com_Printf ("%d %d %s\n", ch->leftvol, ch->rightvol, ch->thesfx->soundName);
				total++;
			}
		}
		
		Com_Printf ("----(%i)---- painted: %i\n", total, s_paintedtime);
	}

	// add raw data from streamed samples
	S_UpdateBackgroundTrack();

	// mix some sound
	S_Update_();
}

void S_GetSoundtime(void)
{
	int		samplepos;
	static	int		buffers;
	static	int		oldsamplepos;
	int		fullsamples;
	
	fullsamples = dma.samples / dma.channels;

	if( CL_VideoRecording( ) )
	{
		float fps = MIN(cl_aviFrameRate->value, 1000.0f);
		float frameDuration = MAX(dma.speed / fps, 1.0f) + clc.aviSoundFrameRemainder;

		int msec = (int)frameDuration;
		s_soundtime += msec;
		clc.aviSoundFrameRemainder = frameDuration - msec;

		return;
	}

	// it is possible to miscount buffers if it has wrapped twice between
	// calls to S_Update.  Oh well.
	samplepos = SNDDMA_GetDMAPos();
	if (samplepos < oldsamplepos)
	{
		buffers++;					// buffer wrapped
		
		if (s_paintedtime > 0x40000000)
		{	// time to chop things off to avoid 32 bit limits
			buffers = 0;
			s_paintedtime = fullsamples;
			S_Base_StopAllSounds ();
		}
	}
	oldsamplepos = samplepos;

	s_soundtime = buffers*fullsamples + samplepos/dma.channels;

#if 0
// check to make sure that we haven't overshot
	if (s_paintedtime < s_soundtime)
	{
		Com_DPrintf ("S_Update_ : overflow\n");
		s_paintedtime = s_soundtime;
	}
#endif

	if ( dma.submission_chunk < 256 ) {
		s_paintedtime = s_soundtime + s_mixPreStep->value * dma.speed;
	} else {
		s_paintedtime = s_soundtime + dma.submission_chunk;
	}
}


void S_Update_(void) {
	unsigned        endtime;
	int				samps;
	static			float	lastTime = 0.0f;
	float			ma, op;
	float			thisTime, sane;
	static			int ot = -1;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	thisTime = Com_Milliseconds();

	// Updates s_soundtime
	S_GetSoundtime();

	if (s_soundtime == ot) {
		return;
	}
	ot = s_soundtime;

	// clear any sound effects that end before the current time,
	// and start any new sounds
	S_ScanChannelStarts();

	sane = thisTime - lastTime;
	if (sane<11) {
		sane = 11;			// 85hz
	}

	ma = s_mixahead->value * dma.speed;
	op = s_mixPreStep->value + sane*dma.speed*0.01;

	if (op < ma) {
		ma = op;
	}

	// mix ahead of current position
	endtime = s_soundtime + ma;

	// mix to an even submission block size
	endtime = (endtime + dma.submission_chunk-1)
		& ~(dma.submission_chunk-1);

	// never mix more than the complete buffer
	samps = dma.samples >> (dma.channels-1);
	if (endtime - s_soundtime > samps)
		endtime = s_soundtime + samps;



	SNDDMA_BeginPainting ();

	S_PaintChannels (endtime);

	SNDDMA_Submit ();

	lastTime = thisTime;
}



/*
===============================================================================

background music functions

===============================================================================
*/

/*
======================
S_StopBackgroundTrack
======================
*/
void S_Base_StopBackgroundTrack( void ) {
	if(!s_backgroundStream)
		return;
	S_CodecCloseStream(s_backgroundStream);
	s_backgroundStream = NULL;
	s_rawend[0] = 0;
}

/*
======================
S_OpenBackgroundStream
======================
*/
static void S_OpenBackgroundStream( const char *filename ) {
	// close the background track, but DON'T reset s_rawend
	// if restarting the same back ground track
	if(s_backgroundStream)
	{
		S_CodecCloseStream(s_backgroundStream);
		s_backgroundStream = NULL;
	}

	// Open stream
	s_backgroundStream = S_CodecOpenStream(filename);
	if(!s_backgroundStream) {
		Com_Printf( S_COLOR_YELLOW "WARNING: couldn't open music file %s\n", filename );
		return;
	}

	if(s_backgroundStream->info.channels != 2 || s_backgroundStream->info.rate != 22050) {
		Com_Printf(S_COLOR_YELLOW "WARNING: music file %s is not 22k stereo\n", filename );
	}
}

/*
======================
S_StartBackgroundTrack
======================
*/
void S_Base_StartBackgroundTrack( const char *intro, const char *loop ){
	if ( !intro ) {
		intro = "";
	}
	if ( !loop || !loop[0] ) {
		loop = intro;
	}
	Com_DPrintf( "S_StartBackgroundTrack( %s, %s )\n", intro, loop );

	if(!*intro)
	{
		S_Base_StopBackgroundTrack();
		return;
	}

	Q_strncpyz( s_backgroundLoop, loop, sizeof( s_backgroundLoop ) );

	S_OpenBackgroundStream( intro );
}

/*
======================
S_UpdateBackgroundTrack
======================
*/
void S_UpdateBackgroundTrack( void ) {
	int		bufferSamples;
	int		fileSamples;
	byte	raw[30000];		// just enough to fit in a mac stack frame
	int		fileBytes;
	int		r;

	if(!s_backgroundStream) {
		return;
	}

	// don't bother playing anything if musicvolume is 0
	if ( s_musicVolume->value <= 0 ) {
		return;
	}

	// see how many samples should be copied into the raw buffer
	if ( s_rawend[0] < s_soundtime ) {
		s_rawend[0] = s_soundtime;
	}

	while ( s_rawend[0] < s_soundtime + MAX_RAW_SAMPLES ) {
		bufferSamples = MAX_RAW_SAMPLES - (s_rawend[0] - s_soundtime);

		// decide how much data needs to be read from the file
		fileSamples = bufferSamples * s_backgroundStream->info.rate / dma.speed;

		if (!fileSamples)
			return;

		// our max buffer size
		fileBytes = fileSamples * (s_backgroundStream->info.width * s_backgroundStream->info.channels);
		if ( fileBytes > sizeof(raw) ) {
			fileBytes = sizeof(raw);
			fileSamples = fileBytes / (s_backgroundStream->info.width * s_backgroundStream->info.channels);
		}

		// Read
		r = S_CodecReadStream(s_backgroundStream, fileBytes, raw);
		if(r < fileBytes)
		{
			fileSamples = r / (s_backgroundStream->info.width * s_backgroundStream->info.channels);
		}

		if(r > 0)
		{
			// add to raw buffer
			S_Base_RawSamples(0, fileSamples, s_backgroundStream->info.rate,
				s_backgroundStream->info.width, s_backgroundStream->info.channels, raw, s_musicVolume->value, -1);
		}
		else
		{
			// loop
			if(s_backgroundLoop[0])
			{
				S_OpenBackgroundStream( s_backgroundLoop );
				if(!s_backgroundStream)
					return;
			}
			else
			{
				S_Base_StopBackgroundTrack();
				return;
			}
		}

	}
}


/*
======================
S_FreeOldestSound
======================
*/

void S_FreeOldestSound( void ) {
	int	i, oldest, used;
	sfx_t	*sfx;
	sndBuffer	*buffer, *nbuffer;

	oldest = Com_Milliseconds();
	used = 0;

	for (i=1 ; i < s_numSfx ; i++) {
		sfx = &s_knownSfx[i];
		if (sfx->inMemory && sfx->lastTimeUsed<oldest) {
			used = i;
			oldest = sfx->lastTimeUsed;
		}
	}

	sfx = &s_knownSfx[used];

	Com_DPrintf("S_FreeOldestSound: freeing sound %s\n", sfx->soundName);

	buffer = sfx->soundData;
	while(buffer != NULL) {
		nbuffer = buffer->next;
		SND_free(buffer);
		buffer = nbuffer;
	}
	sfx->inMemory = qfalse;
	sfx->soundData = NULL;
}

// =======================================================================
// Shutdown sound engine
// =======================================================================

void S_Base_Shutdown( void ) {
	if ( !s_soundStarted ) {
		return;
	}

	SNDDMA_Shutdown();
	SND_shutdown();

	s_soundStarted = 0;
	s_numSfx = 0;

	Cmd_RemoveCommand("s_info");
}

/*
================
S_Init
================
*/
qboolean S_Base_Init( soundInterface_t *si ) {
	qboolean	r;

	if( !si ) {
		return qfalse;
	}

	s_mixahead = Cvar_Get ("s_mixahead", "0.2", CVAR_ARCHIVE);
	s_mixPreStep = Cvar_Get ("s_mixPreStep", "0.05", CVAR_ARCHIVE);
	s_show = Cvar_Get ("s_show", "0", CVAR_CHEAT);
	s_testsound = Cvar_Get ("s_testsound", "0", CVAR_CHEAT);

	r = SNDDMA_Init();

	if ( r ) {
		s_soundStarted = 1;
		s_soundMuted = 1;
//		s_numSfx = 0;

		Com_Memset(sfxHash, 0, sizeof(sfx_t *)*LOOP_HASH);

		s_soundtime = 0;
		s_paintedtime = 0;

		S_Base_StopAllSounds( );
	} else {
		return qfalse;
	}

	si->Shutdown = S_Base_Shutdown;
	si->StartSound = S_Base_StartSound;
	si->StartLocalSound = S_Base_StartLocalSound;
	si->StartBackgroundTrack = S_Base_StartBackgroundTrack;
	si->StopBackgroundTrack = S_Base_StopBackgroundTrack;
	si->RawSamples = S_Base_RawSamples;
	si->StopAllSounds = S_Base_StopAllSounds;
	si->ClearLoopingSounds = S_Base_ClearLoopingSounds;
	si->AddLoopingSound = S_Base_AddLoopingSound;
	si->AddRealLoopingSound = S_Base_AddRealLoopingSound;
	si->StopLoopingSound = S_Base_StopLoopingSound;
	si->Respatialize = S_Base_Respatialize;
	si->UpdateEntityPosition = S_Base_UpdateEntityPosition;
	si->Update = S_Base_Update;
	si->DisableSounds = S_Base_DisableSounds;
	si->BeginRegistration = S_Base_BeginRegistration;
	si->RegisterSound = S_Base_RegisterSound;
	si->ClearSoundBuffer = S_Base_ClearSoundBuffer;
	si->SoundInfo = S_Base_SoundInfo;
	si->SoundList = S_Base_SoundList;

#ifdef USE_VOIP
	si->StartCapture = S_Base_StartCapture;
	si->AvailableCaptureSamples = S_Base_AvailableCaptureSamples;
	si->Capture = S_Base_Capture;
	si->StopCapture = S_Base_StopCapture;
	si->MasterGain = S_Base_MasterGain;
#endif

	return qtrue;
}
