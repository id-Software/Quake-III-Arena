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
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "snd_local.h"
#include "snd_codec.h"
#include "client.h"

#ifdef USE_OPENAL

#include "qal.h"

// Console variables specific to OpenAL
cvar_t *s_alPrecache;
cvar_t *s_alGain;
cvar_t *s_alSources;
cvar_t *s_alDopplerFactor;
cvar_t *s_alDopplerSpeed;
cvar_t *s_alMinDistance;
cvar_t *s_alMaxDistance;
cvar_t *s_alRolloff;
cvar_t *s_alGraceDistance;
cvar_t *s_alDriver;
cvar_t *s_alDevice;
cvar_t *s_alInputDevice;
cvar_t *s_alAvailableDevices;
cvar_t *s_alAvailableInputDevices;

static qboolean enumeration_ext = qfalse;
static qboolean enumeration_all_ext = qfalse;
#ifdef USE_VOIP
static qboolean capture_ext = qfalse;
#endif

/*
=================
S_AL_Format
=================
*/
static
ALuint S_AL_Format(int width, int channels)
{
	ALuint format = AL_FORMAT_MONO16;

	// Work out format
	if(width == 1)
	{
		if(channels == 1)
			format = AL_FORMAT_MONO8;
		else if(channels == 2)
			format = AL_FORMAT_STEREO8;
	}
	else if(width == 2)
	{
		if(channels == 1)
			format = AL_FORMAT_MONO16;
		else if(channels == 2)
			format = AL_FORMAT_STEREO16;
	}

	return format;
}

/*
=================
S_AL_ErrorMsg
=================
*/
static const char *S_AL_ErrorMsg(ALenum error)
{
	switch(error)
	{
		case AL_NO_ERROR:
			return "No error";
		case AL_INVALID_NAME:
			return "Invalid name";
		case AL_INVALID_ENUM:
			return "Invalid enumerator";
		case AL_INVALID_VALUE:
			return "Invalid value";
		case AL_INVALID_OPERATION:
			return "Invalid operation";
		case AL_OUT_OF_MEMORY:
			return "Out of memory";
		default:
			return "Unknown error";
	}
}

/*
=================
S_AL_ClearError
=================
*/
static void S_AL_ClearError( qboolean quiet )
{
	int error = qalGetError();

	if( quiet )
		return;
	if(error != AL_NO_ERROR)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: unhandled AL error: %s\n",
			S_AL_ErrorMsg(error));
	}
}


//===========================================================================


typedef struct alSfx_s
{
	char			filename[MAX_QPATH];
	ALuint		buffer;					// OpenAL buffer
	snd_info_t	info;					// information for this sound like rate, sample count..

	qboolean	isDefault;				// Couldn't be loaded - use default FX
	qboolean	isDefaultChecked;		// Sound has been check if it isDefault
	qboolean	inMemory;				// Sound is stored in memory
	qboolean	isLocked;				// Sound is locked (can not be unloaded)
	int				lastUsedTime;		// Time last used

	int				loopCnt;		// number of loops using this sfx
	int				loopActiveCnt;		// number of playing loops using this sfx
	int				masterLoopSrc;		// All other sources looping this buffer are synced to this master src
} alSfx_t;

static qboolean alBuffersInitialised = qfalse;

// Sound effect storage, data structures
#define MAX_SFX 4096
static alSfx_t knownSfx[MAX_SFX];
static sfxHandle_t numSfx = 0;

static sfxHandle_t default_sfx;

/*
=================
S_AL_BufferFindFree

Find a free handle
=================
*/
static sfxHandle_t S_AL_BufferFindFree( void )
{
	int i;

	for(i = 0; i < MAX_SFX; i++)
	{
		// Got one
		if(knownSfx[i].filename[0] == '\0')
		{
			if(i >= numSfx)
				numSfx = i + 1;
			return i;
		}
	}

	// Shit...
	Com_Error(ERR_FATAL, "S_AL_BufferFindFree: No free sound handles");
	return -1;
}

/*
=================
S_AL_BufferFind

Find a sound effect if loaded, set up a handle otherwise
=================
*/
static sfxHandle_t S_AL_BufferFind(const char *filename)
{
	// Look it up in the table
	sfxHandle_t sfx = -1;
	int i;

	if ( !filename ) {
		Com_Error( ERR_FATAL, "Sound name is NULL" );
	}

	if ( !filename[0] ) {
		Com_Printf( S_COLOR_YELLOW "WARNING: Sound name is empty\n" );
		return 0;
	}

	if ( strlen( filename ) >= MAX_QPATH ) {
		Com_Printf( S_COLOR_YELLOW "WARNING: Sound name is too long: %s\n", filename );
		return 0;
	}

	if ( filename[0] == '*' ) {
		Com_Printf( S_COLOR_YELLOW "WARNING: Tried to load player sound directly: %s\n", filename );
		return 0;
	}

	for(i = 0; i < numSfx; i++)
	{
		if(!Q_stricmp(knownSfx[i].filename, filename))
		{
			sfx = i;
			break;
		}
	}

	// Not found in table?
	if(sfx == -1)
	{
		alSfx_t *ptr;

		sfx = S_AL_BufferFindFree();

		// Clear and copy the filename over
		ptr = &knownSfx[sfx];
		memset(ptr, 0, sizeof(*ptr));
		ptr->masterLoopSrc = -1;
		strcpy(ptr->filename, filename);
	}

	// Return the handle
	return sfx;
}

/*
=================
S_AL_BufferUseDefault
=================
*/
static void S_AL_BufferUseDefault(sfxHandle_t sfx)
{
	if(sfx == default_sfx)
		Com_Error(ERR_FATAL, "Can't load default sound effect %s", knownSfx[sfx].filename);

	Com_Printf( S_COLOR_YELLOW "WARNING: Using default sound for %s\n", knownSfx[sfx].filename);
	knownSfx[sfx].isDefault = qtrue;
	knownSfx[sfx].buffer = knownSfx[default_sfx].buffer;
}

/*
=================
S_AL_BufferUnload
=================
*/
static void S_AL_BufferUnload(sfxHandle_t sfx)
{
	if(knownSfx[sfx].filename[0] == '\0')
		return;

	if(!knownSfx[sfx].inMemory)
		return;

	// Delete it 
	S_AL_ClearError( qfalse );
	qalDeleteBuffers(1, &knownSfx[sfx].buffer);
	if(qalGetError() != AL_NO_ERROR)
		Com_Printf( S_COLOR_RED "ERROR: Can't delete sound buffer for %s\n",
				knownSfx[sfx].filename);

	knownSfx[sfx].inMemory = qfalse;
}

/*
=================
S_AL_BufferEvict
=================
*/
static qboolean S_AL_BufferEvict( void )
{
	int	i, oldestBuffer = -1;
	int	oldestTime = Sys_Milliseconds( );

	for( i = 0; i < numSfx; i++ )
	{
		if( !knownSfx[ i ].filename[ 0 ] )
			continue;

		if( !knownSfx[ i ].inMemory )
			continue;

		if( knownSfx[ i ].lastUsedTime < oldestTime )
		{
			oldestTime = knownSfx[ i ].lastUsedTime;
			oldestBuffer = i;
		}
	}

	if( oldestBuffer >= 0 )
	{
		S_AL_BufferUnload( oldestBuffer );
		return qtrue;
	}
	else
		return qfalse;
}

/*
=================
S_AL_GenBuffers
=================
*/
static qboolean S_AL_GenBuffers(ALsizei numBuffers, ALuint *buffers, const char *name)
{
	ALenum error;

	S_AL_ClearError( qfalse );
	qalGenBuffers( numBuffers, buffers );
	error = qalGetError();

	// If we ran out of buffers, start evicting the least recently used sounds
	while( error == AL_INVALID_VALUE )
	{
		if( !S_AL_BufferEvict( ) )
		{
			Com_Printf( S_COLOR_RED "ERROR: Out of audio buffers\n");
			return qfalse;
		}

		// Try again
		S_AL_ClearError( qfalse );
		qalGenBuffers( numBuffers, buffers );
		error = qalGetError();
	}

	if( error != AL_NO_ERROR )
	{
		Com_Printf( S_COLOR_RED "ERROR: Can't create a sound buffer for %s - %s\n",
				    name, S_AL_ErrorMsg(error));
		return qfalse;
	}

	return qtrue;
}

/*
=================
S_AL_BufferLoad
=================
*/
static void S_AL_BufferLoad(sfxHandle_t sfx, qboolean cache)
{
	ALenum error;
	ALuint format;

	void *data;
	snd_info_t info;
	alSfx_t *curSfx = &knownSfx[sfx];

	// Nothing?
	if(curSfx->filename[0] == '\0')
		return;

	// Already done?
	if((curSfx->inMemory) || (curSfx->isDefault) || (!cache && curSfx->isDefaultChecked))
		return;

	// Try to load
	data = S_CodecLoad(curSfx->filename, &info);
	if(!data)
	{
		S_AL_BufferUseDefault(sfx);
		return;
	}

	curSfx->isDefaultChecked = qtrue;

	if (!cache)
	{
		// Don't create AL cache
		Hunk_FreeTempMemory(data);
		return;
	}

	format = S_AL_Format(info.width, info.channels);

	// Create a buffer
	if (!S_AL_GenBuffers(1, &curSfx->buffer, curSfx->filename))
	{
		S_AL_BufferUseDefault(sfx);
		Hunk_FreeTempMemory(data);
		return;
	}

	// Fill the buffer
	if( info.size == 0 )
	{
		// We have no data to buffer, so buffer silence
		byte dummyData[ 2 ] = { 0 };

		qalBufferData(curSfx->buffer, AL_FORMAT_MONO16, (void *)dummyData, 2, 22050);
	}
	else
		qalBufferData(curSfx->buffer, format, data, info.size, info.rate);

	error = qalGetError();

	// If we ran out of memory, start evicting the least recently used sounds
	while(error == AL_OUT_OF_MEMORY)
	{
		if( !S_AL_BufferEvict( ) )
		{
			qalDeleteBuffers(1, &curSfx->buffer);
			S_AL_BufferUseDefault(sfx);
			Hunk_FreeTempMemory(data);
			Com_Printf( S_COLOR_RED "ERROR: Out of memory loading %s\n", curSfx->filename);
			return;
		}

		// Try load it again
		qalBufferData(curSfx->buffer, format, data, info.size, info.rate);
		error = qalGetError();
	}

	// Some other error condition
	if(error != AL_NO_ERROR)
	{
		qalDeleteBuffers(1, &curSfx->buffer);
		S_AL_BufferUseDefault(sfx);
		Hunk_FreeTempMemory(data);
		Com_Printf( S_COLOR_RED "ERROR: Can't fill sound buffer for %s - %s\n",
				curSfx->filename, S_AL_ErrorMsg(error));
		return;
	}

	curSfx->info = info;
	
	// Free the memory
	Hunk_FreeTempMemory(data);

	// Woo!
	curSfx->inMemory = qtrue;
}

/*
=================
S_AL_BufferUse
=================
*/
static
void S_AL_BufferUse(sfxHandle_t sfx)
{
	if(knownSfx[sfx].filename[0] == '\0')
		return;

	if((!knownSfx[sfx].inMemory) && (!knownSfx[sfx].isDefault))
		S_AL_BufferLoad(sfx, qtrue);
	knownSfx[sfx].lastUsedTime = Sys_Milliseconds();
}

/*
=================
S_AL_BufferInit
=================
*/
static
qboolean S_AL_BufferInit( void )
{
	if(alBuffersInitialised)
		return qtrue;

	// Clear the hash table, and SFX table
	memset(knownSfx, 0, sizeof(knownSfx));
	numSfx = 0;

	// Load the default sound, and lock it
	default_sfx = S_AL_BufferFind("sound/feedback/hit.wav");
	S_AL_BufferUse(default_sfx);
	knownSfx[default_sfx].isLocked = qtrue;

	// All done
	alBuffersInitialised = qtrue;
	return qtrue;
}

/*
=================
S_AL_BufferShutdown
=================
*/
static
void S_AL_BufferShutdown( void )
{
	int i;

	if(!alBuffersInitialised)
		return;

	// Unlock the default sound effect
	knownSfx[default_sfx].isLocked = qfalse;

	// Free all used effects
	for(i = 0; i < numSfx; i++)
		S_AL_BufferUnload(i);

	// Clear the tables
	numSfx = 0;

	// All undone
	alBuffersInitialised = qfalse;
}

/*
=================
S_AL_RegisterSound
=================
*/
static
sfxHandle_t S_AL_RegisterSound( const char *sample, qboolean compressed )
{
	sfxHandle_t sfx = S_AL_BufferFind(sample);

	if((!knownSfx[sfx].inMemory) && (!knownSfx[sfx].isDefault))
		S_AL_BufferLoad(sfx, s_alPrecache->integer);
	knownSfx[sfx].lastUsedTime = Com_Milliseconds();

	if (knownSfx[sfx].isDefault) {
		return 0;
	}

	return sfx;
}

/*
=================
S_AL_BufferGet

Return's a sfx's buffer
=================
*/
static
ALuint S_AL_BufferGet(sfxHandle_t sfx)
{
	return knownSfx[sfx].buffer;
}


//===========================================================================


typedef struct src_s
{
	ALuint		alSource;		// OpenAL source object
	sfxHandle_t	sfx;			// Sound effect in use

	int		lastUsedTime;		// Last time used
	alSrcPriority_t	priority;		// Priority
	int		entity;			// Owning entity (-1 if none)
	int		channel;		// Associated channel (-1 if none)

	qboolean	isActive;		// Is this source currently in use?
	qboolean	isPlaying;		// Is this source currently playing, or stopped?
	qboolean	isLocked;		// This is locked (un-allocatable)
	qboolean	isLooping;		// Is this a looping effect (attached to an entity)
	qboolean	isTracking;		// Is this object tracking its owner
	qboolean	isStream;		// Is this source a stream

	float		curGain;		// gain employed if source is within maxdistance.
	float		scaleGain;		// Last gain value for this source. 0 if muted.
	
	float		lastTimePos;		// On stopped loops, the last position in the buffer
	int		lastSampleTime;		// Time when this was stopped
	vec3_t		loopSpeakerPos;		// Origin of the loop speaker
	
	qboolean	local;			// Is this local (relative to the cam)
} src_t;

#ifdef __APPLE__
	#define MAX_SRC 64
#else
	#define MAX_SRC 128
#endif
static src_t srcList[MAX_SRC];
static int srcCount = 0;
static int srcActiveCnt = 0;
static qboolean alSourcesInitialised = qfalse;
static int lastListenerNumber = -1;
static vec3_t lastListenerOrigin = { 0.0f, 0.0f, 0.0f };

typedef struct sentity_s
{
	vec3_t					origin;

	qboolean						srcAllocated; // If a src_t has been allocated to this entity
	int							srcIndex;

	qboolean				loopAddedThisFrame;
	alSrcPriority_t	loopPriority;
	sfxHandle_t			loopSfx;
	qboolean				startLoopingSound;
} sentity_t;

static sentity_t entityList[MAX_GENTITIES];

/*
=================
S_AL_SanitiseVector
=================
*/
#define S_AL_SanitiseVector(v) _S_AL_SanitiseVector(v,__LINE__)
static void _S_AL_SanitiseVector( vec3_t v, int line )
{
	if( Q_isnan( v[ 0 ] ) || Q_isnan( v[ 1 ] ) || Q_isnan( v[ 2 ] ) )
	{
		Com_DPrintf( S_COLOR_YELLOW "WARNING: vector with one or more NaN components "
				"being passed to OpenAL at %s:%d -- zeroing\n", __FILE__, line );
		VectorClear( v );
	}
}

/*
=================
S_AL_Gain
Set gain to 0 if muted, otherwise set it to given value.
=================
*/

static void S_AL_Gain(ALuint source, float gainval)
{
	if(s_muted->integer)
		qalSourcef(source, AL_GAIN, 0.0f);
	else
		qalSourcef(source, AL_GAIN, gainval);
}

/*
=================
S_AL_ScaleGain
Adapt the gain if necessary to get a quicker fadeout when the source is too far away.
=================
*/

static void S_AL_ScaleGain(src_t *chksrc, vec3_t origin)
{
	float distance;
	
	if(!chksrc->local)
		distance = Distance(origin, lastListenerOrigin);
		
	// If we exceed a certain distance, scale the gain linearly until the sound
	// vanishes into nothingness.
	if(!chksrc->local && (distance -= s_alMaxDistance->value) > 0)
	{
		float scaleFactor;

		if(distance >= s_alGraceDistance->value)
			scaleFactor = 0.0f;
		else
			scaleFactor = 1.0f - distance / s_alGraceDistance->value;
		
		scaleFactor *= chksrc->curGain;
		
		if(chksrc->scaleGain != scaleFactor)
		{
			chksrc->scaleGain = scaleFactor;
			S_AL_Gain(chksrc->alSource, chksrc->scaleGain);
		}
	}
	else if(chksrc->scaleGain != chksrc->curGain)
	{
		chksrc->scaleGain = chksrc->curGain;
		S_AL_Gain(chksrc->alSource, chksrc->scaleGain);
	}
}

/*
=================
S_AL_HearingThroughEntity

Also see S_Base_HearingThroughEntity
=================
*/
static qboolean S_AL_HearingThroughEntity( int entityNum )
{
	float	distanceSq;

	if( lastListenerNumber == entityNum )
	{
		// This is an outrageous hack to detect
		// whether or not the player is rendering in third person or not. We can't
		// ask the renderer because the renderer has no notion of entities and we
		// can't ask cgame since that would involve changing the API and hence mod
		// compatibility. I don't think there is any way around this, but I'll leave
		// the FIXME just in case anyone has a bright idea.
		distanceSq = DistanceSquared(
				entityList[ entityNum ].origin,
				lastListenerOrigin );

		if( distanceSq > THIRD_PERSON_THRESHOLD_SQ )
			return qfalse; //we're the player, but third person
		else
			return qtrue;  //we're the player
	}
	else
		return qfalse; //not the player
}

/*
=================
S_AL_SrcInit
=================
*/
static
qboolean S_AL_SrcInit( void )
{
	int i;
	int limit;

	// Clear the sources data structure
	memset(srcList, 0, sizeof(srcList));
	srcCount = 0;
	srcActiveCnt = 0;

	// Cap s_alSources to MAX_SRC
	limit = s_alSources->integer;
	if(limit > MAX_SRC)
		limit = MAX_SRC;
	else if(limit < 16)
		limit = 16;
 
	S_AL_ClearError( qfalse );
	// Allocate as many sources as possible
	for(i = 0; i < limit; i++)
	{
		qalGenSources(1, &srcList[i].alSource);
		if(qalGetError() != AL_NO_ERROR)
			break;
		srcCount++;
	}

	// All done. Print this for informational purposes
	Com_Printf( "Allocated %d sources.\n", srcCount);
	alSourcesInitialised = qtrue;
	return qtrue;
}

/*
=================
S_AL_SrcShutdown
=================
*/
static
void S_AL_SrcShutdown( void )
{
	int i;
	src_t *curSource;

	if(!alSourcesInitialised)
		return;

	// Destroy all the sources
	for(i = 0; i < srcCount; i++)
	{
		curSource = &srcList[i];
		
		if(curSource->isLocked)
			Com_DPrintf( S_COLOR_YELLOW "WARNING: Source %d is locked\n", i);

		if(curSource->entity > 0)
			entityList[curSource->entity].srcAllocated = qfalse;

		qalSourceStop(srcList[i].alSource);
		qalDeleteSources(1, &srcList[i].alSource);
	}

	memset(srcList, 0, sizeof(srcList));

	alSourcesInitialised = qfalse;
}

/*
=================
S_AL_SrcSetup
=================
*/
static void S_AL_SrcSetup(srcHandle_t src, sfxHandle_t sfx, alSrcPriority_t priority,
		int entity, int channel, qboolean local)
{
	src_t *curSource;

	// Set up src struct
	curSource = &srcList[src];
	
	curSource->lastUsedTime = Sys_Milliseconds();
	curSource->sfx = sfx;
	curSource->priority = priority;
	curSource->entity = entity;
	curSource->channel = channel;
	curSource->isPlaying = qfalse;
	curSource->isLocked = qfalse;
	curSource->isLooping = qfalse;
	curSource->isTracking = qfalse;
	curSource->isStream = qfalse;
	curSource->curGain = s_alGain->value * s_volume->value;
	curSource->scaleGain = curSource->curGain;
	curSource->local = local;

	// Set up OpenAL source
	if(sfx >= 0)
	{
        	// Mark the SFX as used, and grab the raw AL buffer
        	S_AL_BufferUse(sfx);
        	qalSourcei(curSource->alSource, AL_BUFFER, S_AL_BufferGet(sfx));
	}

	qalSourcef(curSource->alSource, AL_PITCH, 1.0f);
	S_AL_Gain(curSource->alSource, curSource->curGain);
	qalSourcefv(curSource->alSource, AL_POSITION, vec3_origin);
	qalSourcefv(curSource->alSource, AL_VELOCITY, vec3_origin);
	qalSourcei(curSource->alSource, AL_LOOPING, AL_FALSE);
	qalSourcef(curSource->alSource, AL_REFERENCE_DISTANCE, s_alMinDistance->value);

	if(local)
	{
		qalSourcei(curSource->alSource, AL_SOURCE_RELATIVE, AL_TRUE);
		qalSourcef(curSource->alSource, AL_ROLLOFF_FACTOR, 0.0f);
	}
	else
	{
		qalSourcei(curSource->alSource, AL_SOURCE_RELATIVE, AL_FALSE);
		qalSourcef(curSource->alSource, AL_ROLLOFF_FACTOR, s_alRolloff->value);
	}
}

/*
=================
S_AL_SaveLoopPos
Remove given source as loop master if it is the master and hand off master status to another source in this case.
=================
*/

static void S_AL_SaveLoopPos(src_t *dest, ALuint alSource)
{
	int error;
	
	S_AL_ClearError(qfalse);
	
	qalGetSourcef(alSource, AL_SEC_OFFSET, &dest->lastTimePos);
	if((error = qalGetError()) != AL_NO_ERROR)
	{
		// Old OpenAL implementations don't support AL_SEC_OFFSET

		if(error != AL_INVALID_ENUM)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: Could not get time offset for alSource %d: %s\n",
				   alSource, S_AL_ErrorMsg(error));
		}
		
		dest->lastTimePos = -1;
	}
	else
		dest->lastSampleTime = Sys_Milliseconds();
}

/*
=================
S_AL_NewLoopMaster
Remove given source as loop master if it is the master and hand off master status to another source in this case.
=================
*/

static void S_AL_NewLoopMaster(src_t *rmSource, qboolean iskilled)
{
	int index;
	src_t *curSource = NULL;
	alSfx_t *curSfx;
	
	curSfx = &knownSfx[rmSource->sfx];

	if(rmSource->isPlaying)
		curSfx->loopActiveCnt--;
	if(iskilled)
		curSfx->loopCnt--;
	
	if(curSfx->loopCnt)
	{
		if(rmSource->priority == SRCPRI_ENTITY)
		{
			if(!iskilled && rmSource->isPlaying)
			{
				// only sync ambient loops...
				// It makes more sense to have sounds for weapons/projectiles unsynced
				S_AL_SaveLoopPos(rmSource, rmSource->alSource);
			}
		}
		else if(rmSource == &srcList[curSfx->masterLoopSrc])
		{
			int firstInactive = -1;

			// Only if rmSource was the master and if there are still playing loops for
			// this sound will we need to find a new master.
	
			if(iskilled || curSfx->loopActiveCnt)
			{
				for(index = 0; index < srcCount; index++)
				{
					curSource = &srcList[index];
	
					if(curSource->sfx == rmSource->sfx && curSource != rmSource &&
					   curSource->isActive && curSource->isLooping && curSource->priority == SRCPRI_AMBIENT)
					{
						if(curSource->isPlaying)
						{
							curSfx->masterLoopSrc = index;
							break;
						}
						else if(firstInactive < 0)
							firstInactive = index;
					}
				}
			}
		
			if(!curSfx->loopActiveCnt)
			{
				if(firstInactive < 0)
				{
					if(iskilled)
					{
						curSfx->masterLoopSrc = -1;
						return;
					}
					else
						curSource = rmSource;
				}
				else
					curSource = &srcList[firstInactive];

				if(rmSource->isPlaying)
				{
					// this was the last not stopped source, save last sample position + time
					S_AL_SaveLoopPos(curSource, rmSource->alSource);
				}
				else
				{
					// second case: all loops using this sound have stopped due to listener being of of range,
					// and now the inactive master gets deleted. Just move over the soundpos settings to the
					// new master.
					curSource->lastTimePos = rmSource->lastTimePos;
					curSource->lastSampleTime = rmSource->lastSampleTime;
				}
			}
		}
	}
	else
		curSfx->masterLoopSrc = -1;
}

/*
=================
S_AL_SrcKill
=================
*/
static void S_AL_SrcKill(srcHandle_t src)
{
	src_t *curSource = &srcList[src];
	
	// I'm not touching it. Unlock it first.
	if(curSource->isLocked)
		return;

	// Remove the entity association and loop master status
	if(curSource->isLooping)
	{
		curSource->isLooping = qfalse;

		if(curSource->entity != -1)
		{
			sentity_t *curEnt = &entityList[curSource->entity];
			
			curEnt->srcAllocated = qfalse;
			curEnt->srcIndex = -1;
			curEnt->loopAddedThisFrame = qfalse;
			curEnt->startLoopingSound = qfalse;
		}
		
		S_AL_NewLoopMaster(curSource, qtrue);
	}

	// Stop it if it's playing
	if(curSource->isPlaying)
	{
		qalSourceStop(curSource->alSource);
		curSource->isPlaying = qfalse;
	}

	// Detach any buffers
	qalSourcei(curSource->alSource, AL_BUFFER, 0);

	curSource->sfx = 0;
	curSource->lastUsedTime = 0;
	curSource->priority = 0;
	curSource->entity = -1;
	curSource->channel = -1;
	if(curSource->isActive)
	{
		curSource->isActive = qfalse;
		srcActiveCnt--;
	}
	curSource->isLocked = qfalse;
	curSource->isTracking = qfalse;
	curSource->local = qfalse;
}

/*
=================
S_AL_SrcAlloc
=================
*/
static
srcHandle_t S_AL_SrcAlloc( alSrcPriority_t priority, int entnum, int channel )
{
	int i;
	int empty = -1;
	int weakest = -1;
	int weakest_time = Sys_Milliseconds();
	int weakest_pri = 999;
	float weakest_gain = 1000.0;
	qboolean weakest_isplaying = qtrue;
	int weakest_numloops = 0;
	src_t *curSource;

	for(i = 0; i < srcCount; i++)
	{
		curSource = &srcList[i];
		
		// If it's locked, we aren't even going to look at it
		if(curSource->isLocked)
			continue;

		// Is it empty or not?
		if(!curSource->isActive)
		{
			empty = i;
			break;
		}

		if(curSource->isPlaying)
		{
			if(weakest_isplaying && curSource->priority < priority &&
			   (curSource->priority < weakest_pri ||
			   (!curSource->isLooping && (curSource->scaleGain < weakest_gain || curSource->lastUsedTime < weakest_time))))
			{
				// If it has lower priority, is fainter or older, flag it as weak
				// the last two values are only compared if it's not a looping sound, because we want to prevent two
				// loops (loops are added EVERY frame) fighting for a slot
				weakest_pri = curSource->priority;
				weakest_time = curSource->lastUsedTime;
				weakest_gain = curSource->scaleGain;
				weakest = i;
			}
		}
		else
		{
			weakest_isplaying = qfalse;
			
			if(weakest < 0 ||
			   knownSfx[curSource->sfx].loopCnt > weakest_numloops ||
			   curSource->priority < weakest_pri ||
			   curSource->lastUsedTime < weakest_time)
			{
				// Sources currently not playing of course have lowest priority
				// also try to always keep at least one loop master for every loop sound
				weakest_pri = curSource->priority;
				weakest_time = curSource->lastUsedTime;
				weakest_numloops = knownSfx[curSource->sfx].loopCnt;
				weakest = i;
			}
		}

		// The channel system is not actually adhered to by baseq3, and not
		// implemented in snd_dma.c, so while the following is strictly correct, it
		// causes incorrect behaviour versus defacto baseq3
#if 0
		// Is it an exact match, and not on channel 0?
		if((curSource->entity == entnum) && (curSource->channel == channel) && (channel != 0))
		{
			S_AL_SrcKill(i);
			return i;
		}
#endif
	}

	if(empty == -1)
		empty = weakest;
	
	if(empty >= 0)
	{
		S_AL_SrcKill(empty);
		srcList[empty].isActive = qtrue;
		srcActiveCnt++;
	}

	return empty;
}

/*
=================
S_AL_SrcFind

Finds an active source with matching entity and channel numbers
Returns -1 if there isn't one
=================
*/
#if 0
static
srcHandle_t S_AL_SrcFind(int entnum, int channel)
{
	int i;
	for(i = 0; i < srcCount; i++)
	{
		if(!srcList[i].isActive)
			continue;
		if((srcList[i].entity == entnum) && (srcList[i].channel == channel))
			return i;
	}
	return -1;
}
#endif

/*
=================
S_AL_SrcLock

Locked sources will not be automatically reallocated or managed
=================
*/
static
void S_AL_SrcLock(srcHandle_t src)
{
	srcList[src].isLocked = qtrue;
}

/*
=================
S_AL_SrcUnlock

Once unlocked, the source may be reallocated again
=================
*/
static
void S_AL_SrcUnlock(srcHandle_t src)
{
	srcList[src].isLocked = qfalse;
}

/*
=================
S_AL_UpdateEntityPosition
=================
*/
static
void S_AL_UpdateEntityPosition( int entityNum, const vec3_t origin )
{
	vec3_t sanOrigin;

	VectorCopy( origin, sanOrigin );
	S_AL_SanitiseVector( sanOrigin );
	if ( entityNum < 0 || entityNum >= MAX_GENTITIES )
		Com_Error( ERR_DROP, "S_UpdateEntityPosition: bad entitynum %i", entityNum );
	VectorCopy( sanOrigin, entityList[entityNum].origin );
}

/*
=================
S_AL_CheckInput
Check whether input values from mods are out of range.
Necessary for i.g. Western Quake3 mod which is buggy.
=================
*/
static qboolean S_AL_CheckInput(int entityNum, sfxHandle_t sfx)
{
	if (entityNum < 0 || entityNum >= MAX_GENTITIES)
		Com_Error(ERR_DROP, "ERROR: S_AL_CheckInput: bad entitynum %i", entityNum);

	if (sfx < 0 || sfx >= numSfx)
	{
		Com_Printf(S_COLOR_RED "ERROR: S_AL_CheckInput: handle %i out of range\n", sfx);
		return qtrue;
	}

	return qfalse;
}

/*
=================
S_AL_StartLocalSound

Play a local (non-spatialized) sound effect
=================
*/
static
void S_AL_StartLocalSound(sfxHandle_t sfx, int channel)
{
	srcHandle_t src;
	
	if(S_AL_CheckInput(0, sfx))
		return;

	// Try to grab a source
	src = S_AL_SrcAlloc(SRCPRI_LOCAL, -1, channel);
	
	if(src == -1)
		return;

	// Set up the effect
	S_AL_SrcSetup(src, sfx, SRCPRI_LOCAL, -1, channel, qtrue);

	// Start it playing
	srcList[src].isPlaying = qtrue;
	qalSourcePlay(srcList[src].alSource);
}

/*
=================
S_AL_StartSound

Play a one-shot sound effect
=================
*/
static void S_AL_StartSound( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx )
{
	vec3_t sorigin;
	srcHandle_t src;
	src_t *curSource;

	if(origin)
	{
		if(S_AL_CheckInput(0, sfx))
			return;
		
		VectorCopy(origin, sorigin);
	}
	else
	{
		if(S_AL_CheckInput(entnum, sfx))
			return;

		if(S_AL_HearingThroughEntity(entnum))
		{
			S_AL_StartLocalSound(sfx, entchannel);
			return;
		}
		
		VectorCopy(entityList[entnum].origin, sorigin);
	}
	
	S_AL_SanitiseVector(sorigin);
	
	if((srcActiveCnt > 5 * srcCount / 3) &&
		(DistanceSquared(sorigin, lastListenerOrigin) >=
		(s_alMaxDistance->value + s_alGraceDistance->value) * (s_alMaxDistance->value + s_alGraceDistance->value)))
	{
		// We're getting tight on sources and source is not within hearing distance so don't add it
		return;
	}

	// Try to grab a source
	src = S_AL_SrcAlloc(SRCPRI_ONESHOT, entnum, entchannel);
	if(src == -1)
		return;

	S_AL_SrcSetup(src, sfx, SRCPRI_ONESHOT, entnum, entchannel, qfalse);
	
	curSource = &srcList[src];

	if(!origin)
		curSource->isTracking = qtrue;
		
	qalSourcefv(curSource->alSource, AL_POSITION, sorigin );
	S_AL_ScaleGain(curSource, sorigin);

	// Start it playing
	curSource->isPlaying = qtrue;
	qalSourcePlay(curSource->alSource);
}

/*
=================
S_AL_ClearLoopingSounds
=================
*/
static
void S_AL_ClearLoopingSounds( qboolean killall )
{
	int i;
	for(i = 0; i < srcCount; i++)
	{
		if((srcList[i].isLooping) && (srcList[i].entity != -1))
			entityList[srcList[i].entity].loopAddedThisFrame = qfalse;
	}
}

/*
=================
S_AL_SrcLoop
=================
*/
static void S_AL_SrcLoop( alSrcPriority_t priority, sfxHandle_t sfx,
		const vec3_t origin, const vec3_t velocity, int entityNum )
{
	int				src;
	sentity_t	*sent = &entityList[ entityNum ];
	src_t		*curSource;
	vec3_t		sorigin, svelocity;

	if(S_AL_CheckInput(entityNum, sfx))
		return;

	// Do we need to allocate a new source for this entity
	if( !sent->srcAllocated )
	{
		// Try to get a channel
		src = S_AL_SrcAlloc( priority, entityNum, -1 );
		if( src == -1 )
		{
			Com_DPrintf( S_COLOR_YELLOW "WARNING: Failed to allocate source "
					"for loop sfx %d on entity %d\n", sfx, entityNum );
			return;
		}

		curSource = &srcList[src];

		sent->startLoopingSound = qtrue;

		curSource->lastTimePos = -1.0;
		curSource->lastSampleTime = Sys_Milliseconds();
	}
	else
	{
		src = sent->srcIndex;
		curSource = &srcList[src];
	}

	sent->srcAllocated = qtrue;
	sent->srcIndex = src;

	sent->loopPriority = priority;
	sent->loopSfx = sfx;

	// If this is not set then the looping sound is stopped.
	sent->loopAddedThisFrame = qtrue;

	// UGH
	// These lines should be called via S_AL_SrcSetup, but we
	// can't call that yet as it buffers sfxes that may change
	// with subsequent calls to S_AL_SrcLoop
	curSource->entity = entityNum;
	curSource->isLooping = qtrue;

	if( S_AL_HearingThroughEntity( entityNum ) )
	{
		curSource->local = qtrue;

		VectorClear(sorigin);

		qalSourcefv(curSource->alSource, AL_POSITION, sorigin);
		qalSourcefv(curSource->alSource, AL_VELOCITY, vec3_origin);
	}
	else
	{
		curSource->local = qfalse;

		if(origin)
			VectorCopy(origin, sorigin);
		else
			VectorCopy(sent->origin, sorigin);

		S_AL_SanitiseVector(sorigin);
		
		VectorCopy(sorigin, curSource->loopSpeakerPos);
		
		if(velocity)
		{
			VectorCopy(velocity, svelocity);
			S_AL_SanitiseVector(svelocity);
		}
		else
			VectorClear(svelocity);

		qalSourcefv(curSource->alSource, AL_POSITION, (ALfloat *) sorigin);
		qalSourcefv(curSource->alSource, AL_VELOCITY, (ALfloat *) svelocity);
	}
}

/*
=================
S_AL_AddLoopingSound
=================
*/
static void S_AL_AddLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx)
{
	S_AL_SrcLoop(SRCPRI_ENTITY, sfx, origin, velocity, entityNum);
}

/*
=================
S_AL_AddRealLoopingSound
=================
*/
static void S_AL_AddRealLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx)
{
	S_AL_SrcLoop(SRCPRI_AMBIENT, sfx, origin, velocity, entityNum);
}

/*
=================
S_AL_StopLoopingSound
=================
*/
static
void S_AL_StopLoopingSound(int entityNum )
{
	if(entityList[entityNum].srcAllocated)
		S_AL_SrcKill(entityList[entityNum].srcIndex);
}

/*
=================
S_AL_SrcUpdate

Update state (move things around, manage sources, and so on)
=================
*/
static
void S_AL_SrcUpdate( void )
{
	int i;
	int entityNum;
	ALint state;
	src_t *curSource;
	
	for(i = 0; i < srcCount; i++)
	{
		entityNum = srcList[i].entity;
		curSource = &srcList[i];

		if(curSource->isLocked)
			continue;

		if(!curSource->isActive)
			continue;

		// Update source parameters
		if((s_alGain->modified) || (s_volume->modified))
			curSource->curGain = s_alGain->value * s_volume->value;
		if((s_alRolloff->modified) && (!curSource->local))
			qalSourcef(curSource->alSource, AL_ROLLOFF_FACTOR, s_alRolloff->value);
		if(s_alMinDistance->modified)
			qalSourcef(curSource->alSource, AL_REFERENCE_DISTANCE, s_alMinDistance->value);

		if(curSource->isLooping)
		{
			sentity_t *sent = &entityList[ entityNum ];

			// If a looping effect hasn't been touched this frame, pause or kill it
			if(sent->loopAddedThisFrame)
			{
				alSfx_t *curSfx;
			
				// The sound has changed without an intervening removal
				if(curSource->isActive && !sent->startLoopingSound &&
						curSource->sfx != sent->loopSfx)
				{
					S_AL_NewLoopMaster(curSource, qtrue);

					curSource->isPlaying = qfalse;
					qalSourceStop(curSource->alSource);
					qalSourcei(curSource->alSource, AL_BUFFER, 0);
					sent->startLoopingSound = qtrue;
				}

				// The sound hasn't been started yet
				if(sent->startLoopingSound)
				{
					S_AL_SrcSetup(i, sent->loopSfx, sent->loopPriority,
							entityNum, -1, curSource->local);
					curSource->isLooping = qtrue;
					
					knownSfx[curSource->sfx].loopCnt++;
					sent->startLoopingSound = qfalse;
				}
				
				curSfx = &knownSfx[curSource->sfx];

				S_AL_ScaleGain(curSource, curSource->loopSpeakerPos);
				if(!curSource->scaleGain)
				{
					if(curSource->isPlaying)
					{
						// Sound is mute, stop playback until we are in range again
						S_AL_NewLoopMaster(curSource, qfalse);
						qalSourceStop(curSource->alSource);
						curSource->isPlaying = qfalse;
					}
					else if(!curSfx->loopActiveCnt && curSfx->masterLoopSrc < 0)
						curSfx->masterLoopSrc = i;
					
					continue;
				}

				if(!curSource->isPlaying)
				{
					qalSourcei(curSource->alSource, AL_LOOPING, AL_TRUE);
					curSource->isPlaying = qtrue;
					qalSourcePlay(curSource->alSource);

					if(curSource->priority == SRCPRI_AMBIENT)
					{
						// If there are other ambient looping sources with the same sound,
						// make sure the sound of these sources are in sync.

						if(curSfx->loopActiveCnt)
						{
							int offset, error;
						
							// we already have a master loop playing, get buffer position.
							S_AL_ClearError(qfalse);
							qalGetSourcei(srcList[curSfx->masterLoopSrc].alSource, AL_SAMPLE_OFFSET, &offset);
							if((error = qalGetError()) != AL_NO_ERROR)
							{
								if(error != AL_INVALID_ENUM)
								{
									Com_Printf(S_COLOR_YELLOW "WARNING: Cannot get sample offset from source %d: "
										   "%s\n", i, S_AL_ErrorMsg(error));
								}
							}
							else
								qalSourcei(curSource->alSource, AL_SAMPLE_OFFSET, offset);
						}
						else if(curSfx->loopCnt && curSfx->masterLoopSrc >= 0)
						{
							float secofs;
						
							src_t *master = &srcList[curSfx->masterLoopSrc];
							// This loop sound used to be played, but all sources are stopped. Use last sample position/time
							// to calculate offset so the player thinks the sources continued playing while they were inaudible.
						
							if(master->lastTimePos >= 0)
							{
								secofs = master->lastTimePos + (Sys_Milliseconds() - master->lastSampleTime) / 1000.0f;
								secofs = fmodf(secofs, (float) curSfx->info.samples / curSfx->info.rate);
						
								qalSourcef(curSource->alSource, AL_SEC_OFFSET, secofs);
							}

							// I be the master now
							curSfx->masterLoopSrc = i;
						}
						else
							curSfx->masterLoopSrc = i;
					}
					else if(curSource->lastTimePos >= 0)
					{
						float secofs;
						
						// For unsynced loops (SRCPRI_ENTITY) just carry on playing as if the sound was never stopped
						
						secofs = curSource->lastTimePos + (Sys_Milliseconds() - curSource->lastSampleTime) / 1000.0f;
						secofs = fmodf(secofs, (float) curSfx->info.samples / curSfx->info.rate);
						qalSourcef(curSource->alSource, AL_SEC_OFFSET, secofs);
					}
						
					curSfx->loopActiveCnt++;
				}

				// Update locality
				if(curSource->local)
				{
					qalSourcei(curSource->alSource, AL_SOURCE_RELATIVE, AL_TRUE);
					qalSourcef(curSource->alSource, AL_ROLLOFF_FACTOR, 0.0f);
				}
				else
				{
					qalSourcei(curSource->alSource, AL_SOURCE_RELATIVE, AL_FALSE);
					qalSourcef(curSource->alSource, AL_ROLLOFF_FACTOR, s_alRolloff->value);
				}
				
			}
			else if(curSource->priority == SRCPRI_AMBIENT)
			{
				if(curSource->isPlaying)
				{
					S_AL_NewLoopMaster(curSource, qfalse);
					qalSourceStop(curSource->alSource);
					curSource->isPlaying = qfalse;
				}
			}
			else
				S_AL_SrcKill(i);

			continue;
		}

		if(!curSource->isStream)
		{
        		// Check if it's done, and flag it
	        	qalGetSourcei(curSource->alSource, AL_SOURCE_STATE, &state);
	        	if(state == AL_STOPPED)
        		{
	        		curSource->isPlaying = qfalse;
		        	S_AL_SrcKill(i);
		        	continue;
        		}
                }

		// Query relativity of source, don't move if it's true
		qalGetSourcei(curSource->alSource, AL_SOURCE_RELATIVE, &state);

		// See if it needs to be moved
		if(curSource->isTracking && !state)
		{
			qalSourcefv(curSource->alSource, AL_POSITION, entityList[entityNum].origin);
 			S_AL_ScaleGain(curSource, entityList[entityNum].origin);
		}
	}
}

/*
=================
S_AL_SrcShutup
=================
*/
static
void S_AL_SrcShutup( void )
{
	int i;
	for(i = 0; i < srcCount; i++)
		S_AL_SrcKill(i);
}

/*
=================
S_AL_SrcGet
=================
*/
static
ALuint S_AL_SrcGet(srcHandle_t src)
{
	return srcList[src].alSource;
}


//===========================================================================

// Q3A cinematics use up to 12 buffers at once
#define MAX_STREAM_BUFFERS 20

static srcHandle_t streamSourceHandles[MAX_RAW_STREAMS];
static qboolean streamPlaying[MAX_RAW_STREAMS];
static ALuint streamSources[MAX_RAW_STREAMS];
static ALuint streamBuffers[MAX_RAW_STREAMS][MAX_STREAM_BUFFERS];
static int streamNumBuffers[MAX_RAW_STREAMS];
static int streamBufIndex[MAX_RAW_STREAMS];

/*
=================
S_AL_AllocateStreamChannel
=================
*/
static void S_AL_AllocateStreamChannel(int stream, int entityNum)
{
        srcHandle_t cursrc;
        ALuint alsrc;
        
	if ((stream < 0) || (stream >= MAX_RAW_STREAMS))
		return;

        if(entityNum >= 0)
        {
                // This is a stream that tracks an entity
        	// Allocate a streamSource at normal priority
        	cursrc = S_AL_SrcAlloc(SRCPRI_ENTITY, entityNum, 0);
        	if(cursrc < 0)
	        	return;

        	S_AL_SrcSetup(cursrc, -1, SRCPRI_ENTITY, entityNum, 0, qfalse);
        	alsrc = S_AL_SrcGet(cursrc);
        	srcList[cursrc].isTracking = qtrue;
        	srcList[cursrc].isStream = qtrue;
        }
        else
        {
                // Unspatialized stream source

        	// Allocate a streamSource at high priority
        	cursrc = S_AL_SrcAlloc(SRCPRI_STREAM, -2, 0);
        	if(cursrc < 0)
	        	return;

        	alsrc = S_AL_SrcGet(cursrc);

        	// Lock the streamSource so nobody else can use it, and get the raw streamSource
        	S_AL_SrcLock(cursrc);
        
        	// make sure that after unmuting the S_AL_Gain in S_Update() does not turn
        	// volume up prematurely for this source
        	srcList[cursrc].scaleGain = 0.0f;

        	// Set some streamSource parameters
        	qalSourcei (alsrc, AL_BUFFER,          0            );
        	qalSourcei (alsrc, AL_LOOPING,         AL_FALSE     );
        	qalSource3f(alsrc, AL_POSITION,        0.0, 0.0, 0.0);
        	qalSource3f(alsrc, AL_VELOCITY,        0.0, 0.0, 0.0);
        	qalSource3f(alsrc, AL_DIRECTION,       0.0, 0.0, 0.0);
        	qalSourcef (alsrc, AL_ROLLOFF_FACTOR,  0.0          );
        	qalSourcei (alsrc, AL_SOURCE_RELATIVE, AL_TRUE      );
        }

        streamSourceHandles[stream] = cursrc;
       	streamSources[stream] = alsrc;

	streamNumBuffers[stream] = 0;
	streamBufIndex[stream] = 0;
}

/*
=================
S_AL_FreeStreamChannel
=================
*/
static void S_AL_FreeStreamChannel( int stream )
{
	if ((stream < 0) || (stream >= MAX_RAW_STREAMS))
		return;

	// Detach any buffers
	qalSourcei(streamSources[stream], AL_BUFFER, 0);

	// Delete the buffers
	if (streamNumBuffers[stream] > 0) {
		qalDeleteBuffers(streamNumBuffers[stream], streamBuffers[stream]);
		streamNumBuffers[stream] = 0;
	}

	// Release the output streamSource
	S_AL_SrcUnlock(streamSourceHandles[stream]);
	S_AL_SrcKill(streamSourceHandles[stream]);
	streamSources[stream] = 0;
	streamSourceHandles[stream] = -1;
}

/*
=================
S_AL_RawSamples
=================
*/
static
void S_AL_RawSamples(int stream, int samples, int rate, int width, int channels, const byte *data, float volume, int entityNum)
{
	int numBuffers;
	ALuint buffer;
	ALuint format;

	if ((stream < 0) || (stream >= MAX_RAW_STREAMS))
		return;

	format = S_AL_Format( width, channels );

	// Create the streamSource if necessary
	if(streamSourceHandles[stream] == -1)
	{
		S_AL_AllocateStreamChannel(stream, entityNum);
	
		// Failed?
		if(streamSourceHandles[stream] == -1)
		{
			Com_Printf( S_COLOR_RED "ERROR: Can't allocate streaming streamSource\n");
			return;
		}
	}

	qalGetSourcei(streamSources[stream], AL_BUFFERS_QUEUED, &numBuffers);

	if (numBuffers == MAX_STREAM_BUFFERS)
	{
		Com_DPrintf(S_COLOR_RED"WARNING: Steam dropping raw samples, reached MAX_STREAM_BUFFERS\n");
		return;
	}

	// Allocate a new AL buffer if needed
	if (numBuffers == streamNumBuffers[stream])
	{
		ALuint oldBuffers[MAX_STREAM_BUFFERS];
		int	i;

		if (!S_AL_GenBuffers(1, &buffer, "stream"))
			return;

		Com_Memcpy(oldBuffers, &streamBuffers[stream], sizeof (oldBuffers));

		// Reorder buffer array in order of oldest to newest
		for ( i = 0; i < streamNumBuffers[stream]; ++i )
			streamBuffers[stream][i] = oldBuffers[(streamBufIndex[stream] + i) % streamNumBuffers[stream]];

		// Add the new buffer to end
		streamBuffers[stream][streamNumBuffers[stream]] = buffer;
		streamBufIndex[stream] = streamNumBuffers[stream];
		streamNumBuffers[stream]++;
	}

	// Select next buffer in loop
	buffer = streamBuffers[stream][ streamBufIndex[stream] ];
	streamBufIndex[stream] = (streamBufIndex[stream] + 1) % streamNumBuffers[stream];

	// Fill buffer
	qalBufferData(buffer, format, (ALvoid *)data, (samples * width * channels), rate);

	// Shove the data onto the streamSource
	qalSourceQueueBuffers(streamSources[stream], 1, &buffer);

	if(entityNum < 0)
	{
        	// Volume
        	S_AL_Gain (streamSources[stream], volume * s_volume->value * s_alGain->value);
        }

	// Start stream
	if(!streamPlaying[stream])
	{
		qalSourcePlay( streamSources[stream] );
		streamPlaying[stream] = qtrue;
	}
}

/*
=================
S_AL_StreamUpdate
=================
*/
static
void S_AL_StreamUpdate( int stream )
{
	int		numBuffers;
	ALint	state;

	if ((stream < 0) || (stream >= MAX_RAW_STREAMS))
		return;

	if(streamSourceHandles[stream] == -1)
		return;

	// Un-queue any buffers
	qalGetSourcei( streamSources[stream], AL_BUFFERS_PROCESSED, &numBuffers );
	while( numBuffers-- )
	{
		ALuint buffer;
		qalSourceUnqueueBuffers(streamSources[stream], 1, &buffer);
	}

	// Start the streamSource playing if necessary
	qalGetSourcei( streamSources[stream], AL_BUFFERS_QUEUED, &numBuffers );

	qalGetSourcei(streamSources[stream], AL_SOURCE_STATE, &state);
	if(state == AL_STOPPED)
	{
		streamPlaying[stream] = qfalse;

		// If there are no buffers queued up, release the streamSource
		if( !numBuffers )
			S_AL_FreeStreamChannel( stream );
	}

	if( !streamPlaying[stream] && numBuffers )
	{
		qalSourcePlay( streamSources[stream] );
		streamPlaying[stream] = qtrue;
	}
}

/*
=================
S_AL_StreamDie
=================
*/
static
void S_AL_StreamDie( int stream )
{
	if ((stream < 0) || (stream >= MAX_RAW_STREAMS))
		return;

	if(streamSourceHandles[stream] == -1)
		return;

	streamPlaying[stream] = qfalse;
	qalSourceStop(streamSources[stream]);

	S_AL_FreeStreamChannel(stream);
}


//===========================================================================


#define NUM_MUSIC_BUFFERS	4
#define	MUSIC_BUFFER_SIZE 4096

static qboolean musicPlaying = qfalse;
static srcHandle_t musicSourceHandle = -1;
static ALuint musicSource;
static ALuint musicBuffers[NUM_MUSIC_BUFFERS];

static snd_stream_t *mus_stream;
static snd_stream_t *intro_stream;
static char s_backgroundLoop[MAX_QPATH];

static byte decode_buffer[MUSIC_BUFFER_SIZE];

/*
=================
S_AL_MusicSourceGet
=================
*/
static void S_AL_MusicSourceGet( void )
{
	// Allocate a musicSource at high priority
	musicSourceHandle = S_AL_SrcAlloc(SRCPRI_STREAM, -2, 0);
	if(musicSourceHandle == -1)
		return;

	// Lock the musicSource so nobody else can use it, and get the raw musicSource
	S_AL_SrcLock(musicSourceHandle);
	musicSource = S_AL_SrcGet(musicSourceHandle);

	// make sure that after unmuting the S_AL_Gain in S_Update() does not turn
	// volume up prematurely for this source
	srcList[musicSourceHandle].scaleGain = 0.0f;

	// Set some musicSource parameters
	qalSource3f(musicSource, AL_POSITION,        0.0, 0.0, 0.0);
	qalSource3f(musicSource, AL_VELOCITY,        0.0, 0.0, 0.0);
	qalSource3f(musicSource, AL_DIRECTION,       0.0, 0.0, 0.0);
	qalSourcef (musicSource, AL_ROLLOFF_FACTOR,  0.0          );
	qalSourcei (musicSource, AL_SOURCE_RELATIVE, AL_TRUE      );
}

/*
=================
S_AL_MusicSourceFree
=================
*/
static void S_AL_MusicSourceFree( void )
{
	// Release the output musicSource
	S_AL_SrcUnlock(musicSourceHandle);
	S_AL_SrcKill(musicSourceHandle);
	musicSource = 0;
	musicSourceHandle = -1;
}

/*
=================
S_AL_CloseMusicFiles
=================
*/
static void S_AL_CloseMusicFiles(void)
{
	if(intro_stream)
	{
		S_CodecCloseStream(intro_stream);
		intro_stream = NULL;
	}
	
	if(mus_stream)
	{
		S_CodecCloseStream(mus_stream);
		mus_stream = NULL;
	}
}

/*
=================
S_AL_StopBackgroundTrack
=================
*/
static
void S_AL_StopBackgroundTrack( void )
{
	if(!musicPlaying)
		return;

	// Stop playing
	qalSourceStop(musicSource);

	// Detach any buffers
	qalSourcei(musicSource, AL_BUFFER, 0);

	// Delete the buffers
	qalDeleteBuffers(NUM_MUSIC_BUFFERS, musicBuffers);

	// Free the musicSource
	S_AL_MusicSourceFree();

	// Unload the stream
	S_AL_CloseMusicFiles();

	musicPlaying = qfalse;
}

/*
=================
S_AL_MusicProcess
=================
*/
static
void S_AL_MusicProcess(ALuint b)
{
	ALenum error;
	int l;
	ALuint format;
	snd_stream_t *curstream;

	S_AL_ClearError( qfalse );

	if(intro_stream)
		curstream = intro_stream;
	else
		curstream = mus_stream;

	if(!curstream)
		return;

	l = S_CodecReadStream(curstream, MUSIC_BUFFER_SIZE, decode_buffer);

	// Run out data to read, start at the beginning again
	if(l == 0)
	{
		S_CodecCloseStream(curstream);

		// the intro stream just finished playing so we don't need to reopen
		// the music stream.
		if(intro_stream)
			intro_stream = NULL;
		else
			mus_stream = S_CodecOpenStream(s_backgroundLoop);
		
		curstream = mus_stream;

		if(!curstream)
		{
			S_AL_StopBackgroundTrack();
			return;
		}

		l = S_CodecReadStream(curstream, MUSIC_BUFFER_SIZE, decode_buffer);
	}

	format = S_AL_Format(curstream->info.width, curstream->info.channels);

	if( l == 0 )
	{
		// We have no data to buffer, so buffer silence
		byte dummyData[ 2 ] = { 0 };

		qalBufferData( b, AL_FORMAT_MONO16, (void *)dummyData, 2, 22050 );
	}
	else
		qalBufferData(b, format, decode_buffer, l, curstream->info.rate);

	if( ( error = qalGetError( ) ) != AL_NO_ERROR )
	{
		S_AL_StopBackgroundTrack( );
		Com_Printf( S_COLOR_RED "ERROR: while buffering data for music stream - %s\n",
				S_AL_ErrorMsg( error ) );
		return;
	}
}

/*
=================
S_AL_StartBackgroundTrack
=================
*/
static
void S_AL_StartBackgroundTrack( const char *intro, const char *loop )
{
	int i;
	qboolean issame;

	// Stop any existing music that might be playing
	S_AL_StopBackgroundTrack();

	if((!intro || !*intro) && (!loop || !*loop))
		return;

	// Allocate a musicSource
	S_AL_MusicSourceGet();
	if(musicSourceHandle == -1)
		return;

	if (!loop || !*loop)
	{
		loop = intro;
		issame = qtrue;
	}
	else if(intro && *intro && !strcmp(intro, loop))
		issame = qtrue;
	else
		issame = qfalse;

	// Copy the loop over
	Q_strncpyz( s_backgroundLoop, loop, sizeof( s_backgroundLoop ) );

	if(!issame)
	{
		// Open the intro and don't mind whether it succeeds.
		// The important part is the loop.
		intro_stream = S_CodecOpenStream(intro);
	}
	else
		intro_stream = NULL;

	mus_stream = S_CodecOpenStream(s_backgroundLoop);
	if(!mus_stream)
	{
		S_AL_CloseMusicFiles();
		S_AL_MusicSourceFree();
		return;
	}

	// Generate the musicBuffers
	if (!S_AL_GenBuffers(NUM_MUSIC_BUFFERS, musicBuffers, "music"))
		return;
	
	// Queue the musicBuffers up
	for(i = 0; i < NUM_MUSIC_BUFFERS; i++)
	{
		S_AL_MusicProcess(musicBuffers[i]);
	}

	qalSourceQueueBuffers(musicSource, NUM_MUSIC_BUFFERS, musicBuffers);

	// Set the initial gain property
	S_AL_Gain(musicSource, s_alGain->value * s_musicVolume->value);
	
	// Start playing
	qalSourcePlay(musicSource);

	musicPlaying = qtrue;
}

/*
=================
S_AL_MusicUpdate
=================
*/
static
void S_AL_MusicUpdate( void )
{
	int		numBuffers;
	ALint	state;

	if(!musicPlaying)
		return;

	qalGetSourcei( musicSource, AL_BUFFERS_PROCESSED, &numBuffers );
	while( numBuffers-- )
	{
		ALuint b;
		qalSourceUnqueueBuffers(musicSource, 1, &b);
		S_AL_MusicProcess(b);
		qalSourceQueueBuffers(musicSource, 1, &b);
	}

	// Hitches can cause OpenAL to be starved of buffers when streaming.
	// If this happens, it will stop playback. This restarts the source if
	// it is no longer playing, and if there are buffers available
	qalGetSourcei( musicSource, AL_SOURCE_STATE, &state );
	qalGetSourcei( musicSource, AL_BUFFERS_QUEUED, &numBuffers );
	if( state == AL_STOPPED && numBuffers )
	{
		Com_DPrintf( S_COLOR_YELLOW "Restarted OpenAL music\n" );
		qalSourcePlay(musicSource);
	}

	// Set the gain property
	S_AL_Gain(musicSource, s_alGain->value * s_musicVolume->value);
}


//===========================================================================


// Local state variables
static ALCdevice *alDevice;
static ALCcontext *alContext;

#ifdef USE_VOIP
static ALCdevice *alCaptureDevice;
static cvar_t *s_alCapture;
#endif

#ifdef _WIN32
#define ALDRIVER_DEFAULT "OpenAL32.dll"
#elif defined(__APPLE__)
#define ALDRIVER_DEFAULT "/System/Library/Frameworks/OpenAL.framework/OpenAL"
#elif defined(__OpenBSD__)
#define ALDRIVER_DEFAULT "libopenal.so"
#else
#define ALDRIVER_DEFAULT "libopenal.so.1"
#endif

/*
=================
S_AL_StopAllSounds
=================
*/
static
void S_AL_StopAllSounds( void )
{
	int i;
	S_AL_SrcShutup();
	S_AL_StopBackgroundTrack();
	for (i = 0; i < MAX_RAW_STREAMS; i++)
		S_AL_StreamDie(i);
}

/*
=================
S_AL_Respatialize
=================
*/
static
void S_AL_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater )
{
	float		orientation[6];
	vec3_t	sorigin;

	VectorCopy( origin, sorigin );
	S_AL_SanitiseVector( sorigin );

	S_AL_SanitiseVector( axis[ 0 ] );
	S_AL_SanitiseVector( axis[ 1 ] );
	S_AL_SanitiseVector( axis[ 2 ] );

	orientation[0] = axis[0][0]; orientation[1] = axis[0][1]; orientation[2] = axis[0][2];
	orientation[3] = axis[2][0]; orientation[4] = axis[2][1]; orientation[5] = axis[2][2];

	lastListenerNumber = entityNum;
	VectorCopy( sorigin, lastListenerOrigin );

	// Set OpenAL listener paramaters
	qalListenerfv(AL_POSITION, (ALfloat *)sorigin);
	qalListenerfv(AL_VELOCITY, vec3_origin);
	qalListenerfv(AL_ORIENTATION, orientation);
}

/*
=================
S_AL_Update
=================
*/
static
void S_AL_Update( void )
{
	int i;

	if(s_muted->modified)
	{
		// muted state changed. Let S_AL_Gain turn up all sources again.
		for(i = 0; i < srcCount; i++)
		{
			if(srcList[i].isActive)
				S_AL_Gain(srcList[i].alSource, srcList[i].scaleGain);
		}
		
		s_muted->modified = qfalse;
	}

	// Update SFX channels
	S_AL_SrcUpdate();

	// Update streams
	for (i = 0; i < MAX_RAW_STREAMS; i++)
		S_AL_StreamUpdate(i);
	S_AL_MusicUpdate();

	// Doppler
	if(s_doppler->modified)
	{
		s_alDopplerFactor->modified = qtrue;
		s_doppler->modified = qfalse;
	}

	// Doppler parameters
	if(s_alDopplerFactor->modified)
	{
		if(s_doppler->integer)
			qalDopplerFactor(s_alDopplerFactor->value);
		else
			qalDopplerFactor(0.0f);
		s_alDopplerFactor->modified = qfalse;
	}
	if(s_alDopplerSpeed->modified)
	{
		qalSpeedOfSound(s_alDopplerSpeed->value);
		s_alDopplerSpeed->modified = qfalse;
	}

	// Clear the modified flags on the other cvars
	s_alGain->modified = qfalse;
	s_volume->modified = qfalse;
	s_musicVolume->modified = qfalse;
	s_alMinDistance->modified = qfalse;
	s_alRolloff->modified = qfalse;
}

/*
=================
S_AL_DisableSounds
=================
*/
static
void S_AL_DisableSounds( void )
{
	S_AL_StopAllSounds();
}

/*
=================
S_AL_BeginRegistration
=================
*/
static
void S_AL_BeginRegistration( void )
{
	if(!numSfx)
		S_AL_BufferInit();
}

/*
=================
S_AL_ClearSoundBuffer
=================
*/
static
void S_AL_ClearSoundBuffer( void )
{
}

/*
=================
S_AL_SoundList
=================
*/
static
void S_AL_SoundList( void )
{
}

#ifdef USE_VOIP
static
void S_AL_StartCapture( void )
{
	if (alCaptureDevice != NULL)
		qalcCaptureStart(alCaptureDevice);
}

static
int S_AL_AvailableCaptureSamples( void )
{
	int retval = 0;
	if (alCaptureDevice != NULL)
	{
		ALint samples = 0;
		qalcGetIntegerv(alCaptureDevice, ALC_CAPTURE_SAMPLES, sizeof (samples), &samples);
		retval = (int) samples;
	}
	return retval;
}

static
void S_AL_Capture( int samples, byte *data )
{
	if (alCaptureDevice != NULL)
		qalcCaptureSamples(alCaptureDevice, data, samples);
}

void S_AL_StopCapture( void )
{
	if (alCaptureDevice != NULL)
		qalcCaptureStop(alCaptureDevice);
}

void S_AL_MasterGain( float gain )
{
	qalListenerf(AL_GAIN, gain);
}
#endif


/*
=================
S_AL_SoundInfo
=================
*/
static void S_AL_SoundInfo(void)
{
	Com_Printf( "OpenAL info:\n" );
	Com_Printf( "  Vendor:         %s\n", qalGetString( AL_VENDOR ) );
	Com_Printf( "  Version:        %s\n", qalGetString( AL_VERSION ) );
	Com_Printf( "  Renderer:       %s\n", qalGetString( AL_RENDERER ) );
	Com_Printf( "  AL Extensions:  %s\n", qalGetString( AL_EXTENSIONS ) );
	Com_Printf( "  ALC Extensions: %s\n", qalcGetString( alDevice, ALC_EXTENSIONS ) );

	if(enumeration_all_ext)
		Com_Printf("  Device:         %s\n", qalcGetString(alDevice, ALC_ALL_DEVICES_SPECIFIER));
	else if(enumeration_ext)
		Com_Printf("  Device:         %s\n", qalcGetString(alDevice, ALC_DEVICE_SPECIFIER));

	if(enumeration_all_ext || enumeration_ext)
		Com_Printf("  Available Devices:\n%s", s_alAvailableDevices->string);

#ifdef USE_VOIP
	if(capture_ext)
	{
		Com_Printf("  Input Device:   %s\n", qalcGetString(alCaptureDevice, ALC_CAPTURE_DEVICE_SPECIFIER));
		Com_Printf("  Available Input Devices:\n%s", s_alAvailableInputDevices->string);
	}
#endif
}



/*
=================
S_AL_Shutdown
=================
*/
static
void S_AL_Shutdown( void )
{
	// Shut down everything
	int i;
	for (i = 0; i < MAX_RAW_STREAMS; i++)
		S_AL_StreamDie(i);
	S_AL_StopBackgroundTrack( );
	S_AL_SrcShutdown( );
	S_AL_BufferShutdown( );

	qalcDestroyContext(alContext);
	qalcCloseDevice(alDevice);

#ifdef USE_VOIP
	if (alCaptureDevice != NULL) {
		qalcCaptureStop(alCaptureDevice);
		qalcCaptureCloseDevice(alCaptureDevice);
		alCaptureDevice = NULL;
		Com_Printf( "OpenAL capture device closed.\n" );
	}
#endif

	for (i = 0; i < MAX_RAW_STREAMS; i++) {
		streamSourceHandles[i] = -1;
		streamPlaying[i] = qfalse;
		streamSources[i] = 0;
	}

	QAL_Shutdown();
}

#endif

/*
=================
S_AL_Init
=================
*/
qboolean S_AL_Init( soundInterface_t *si )
{
#ifdef USE_OPENAL
	const char* device = NULL;
	const char* inputdevice = NULL;
	int i;

	if( !si ) {
		return qfalse;
	}

	for (i = 0; i < MAX_RAW_STREAMS; i++) {
		streamSourceHandles[i] = -1;
		streamPlaying[i] = qfalse;
		streamSources[i] = 0;
		streamNumBuffers[i] = 0;
		streamBufIndex[i] = 0;
	}

	// New console variables
	s_alPrecache = Cvar_Get( "s_alPrecache", "1", CVAR_ARCHIVE );
	s_alGain = Cvar_Get( "s_alGain", "1.0", CVAR_ARCHIVE );
	s_alSources = Cvar_Get( "s_alSources", "96", CVAR_ARCHIVE );
	s_alDopplerFactor = Cvar_Get( "s_alDopplerFactor", "1.0", CVAR_ARCHIVE );
	s_alDopplerSpeed = Cvar_Get( "s_alDopplerSpeed", "9000", CVAR_ARCHIVE );
	s_alMinDistance = Cvar_Get( "s_alMinDistance", "120", CVAR_CHEAT );
	s_alMaxDistance = Cvar_Get("s_alMaxDistance", "1024", CVAR_CHEAT);
	s_alRolloff = Cvar_Get( "s_alRolloff", "2", CVAR_CHEAT);
	s_alGraceDistance = Cvar_Get("s_alGraceDistance", "512", CVAR_CHEAT);

	s_alDriver = Cvar_Get( "s_alDriver", ALDRIVER_DEFAULT, CVAR_ARCHIVE | CVAR_LATCH );

	s_alInputDevice = Cvar_Get( "s_alInputDevice", "", CVAR_ARCHIVE | CVAR_LATCH );
	s_alDevice = Cvar_Get("s_alDevice", "", CVAR_ARCHIVE | CVAR_LATCH);

	// Load QAL
	if( !QAL_Init( s_alDriver->string ) )
	{
		Com_Printf( "Failed to load library: \"%s\".\n", s_alDriver->string );
		if( !Q_stricmp( s_alDriver->string, ALDRIVER_DEFAULT ) || !QAL_Init( ALDRIVER_DEFAULT ) ) {
			return qfalse;
		}
	}

	device = s_alDevice->string;
	if(device && !*device)
		device = NULL;

	inputdevice = s_alInputDevice->string;
	if(inputdevice && !*inputdevice)
		inputdevice = NULL;


	// Device enumeration support
	enumeration_all_ext = qalcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT");
	enumeration_ext = qalcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");

	if(enumeration_ext || enumeration_all_ext)
	{
		char devicenames[16384] = "";
		const char *devicelist;
#ifdef _WIN32
		const char *defaultdevice;
#endif
		int curlen;

		// get all available devices + the default device name.
		if(enumeration_all_ext)
		{
			devicelist = qalcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
#ifdef _WIN32
			defaultdevice = qalcGetString(NULL, ALC_DEFAULT_ALL_DEVICES_SPECIFIER);
#endif
		}
		else
		{
			// We don't have ALC_ENUMERATE_ALL_EXT but normal enumeration.
			devicelist = qalcGetString(NULL, ALC_DEVICE_SPECIFIER);
#ifdef _WIN32
			defaultdevice = qalcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
#endif
			enumeration_ext = qtrue;
		}

#ifdef _WIN32
		// check whether the default device is generic hardware. If it is, change to
		// Generic Software as that one works more reliably with various sound systems.
		// If it's not, use OpenAL's default selection as we don't want to ignore
		// native hardware acceleration.
		if(!device && defaultdevice && !strcmp(defaultdevice, "Generic Hardware"))
			device = "Generic Software";
#endif

		// dump a list of available devices to a cvar for the user to see.

		if(devicelist)
		{
			while((curlen = strlen(devicelist)))
			{
				Q_strcat(devicenames, sizeof(devicenames), devicelist);
				Q_strcat(devicenames, sizeof(devicenames), "\n");

				devicelist += curlen + 1;
			}
		}

		s_alAvailableDevices = Cvar_Get("s_alAvailableDevices", devicenames, CVAR_ROM | CVAR_NORESTART);
	}

	alDevice = qalcOpenDevice(device);
	if( !alDevice && device )
	{
		Com_Printf( "Failed to open OpenAL device '%s', trying default.\n", device );
		alDevice = qalcOpenDevice(NULL);
	}

	if( !alDevice )
	{
		QAL_Shutdown( );
		Com_Printf( "Failed to open OpenAL device.\n" );
		return qfalse;
	}

	// Create OpenAL context
	alContext = qalcCreateContext( alDevice, NULL );
	if( !alContext )
	{
		QAL_Shutdown( );
		qalcCloseDevice( alDevice );
		Com_Printf( "Failed to create OpenAL context.\n" );
		return qfalse;
	}
	qalcMakeContextCurrent( alContext );

	// Initialize sources, buffers, music
	S_AL_BufferInit( );
	S_AL_SrcInit( );

	// Set up OpenAL parameters (doppler, etc)
	qalDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
	qalDopplerFactor( s_alDopplerFactor->value );
	qalSpeedOfSound( s_alDopplerSpeed->value );

#ifdef USE_VOIP
	// !!! FIXME: some of these alcCaptureOpenDevice() values should be cvars.
	// !!! FIXME: add support for capture device enumeration.
	// !!! FIXME: add some better error reporting.
	s_alCapture = Cvar_Get( "s_alCapture", "1", CVAR_ARCHIVE | CVAR_LATCH );
	if (!s_alCapture->integer)
	{
		Com_Printf("OpenAL capture support disabled by user ('+set s_alCapture 1' to enable)\n");
	}
#if USE_MUMBLE
	else if (cl_useMumble->integer)
	{
		Com_Printf("OpenAL capture support disabled for Mumble support\n");
	}
#endif
	else
	{
#ifdef __APPLE__
		// !!! FIXME: Apple has a 1.1-compliant OpenAL, which includes
		// !!! FIXME:  capture support, but they don't list it in the
		// !!! FIXME:  extension string. We need to check the version string,
		// !!! FIXME:  then the extension string, but that's too much trouble,
		// !!! FIXME:  so we'll just check the function pointer for now.
		if (qalcCaptureOpenDevice == NULL)
#else
		if (!qalcIsExtensionPresent(NULL, "ALC_EXT_capture"))
#endif
		{
			Com_Printf("No ALC_EXT_capture support, can't record audio.\n");
		}
		else
		{
			char inputdevicenames[16384] = "";
			const char *inputdevicelist;
			const char *defaultinputdevice;
			int curlen;

			capture_ext = qtrue;

			// get all available input devices + the default input device name.
			inputdevicelist = qalcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
			defaultinputdevice = qalcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);

			// dump a list of available devices to a cvar for the user to see.
			if (inputdevicelist)
			{
				while((curlen = strlen(inputdevicelist)))
				{
					Q_strcat(inputdevicenames, sizeof(inputdevicenames), inputdevicelist);
					Q_strcat(inputdevicenames, sizeof(inputdevicenames), "\n");
					inputdevicelist += curlen + 1;
				}
			}

			s_alAvailableInputDevices = Cvar_Get("s_alAvailableInputDevices", inputdevicenames, CVAR_ROM | CVAR_NORESTART);

			Com_Printf("OpenAL default capture device is '%s'\n", defaultinputdevice ? defaultinputdevice : "none");
			alCaptureDevice = qalcCaptureOpenDevice(inputdevice, 48000, AL_FORMAT_MONO16, VOIP_MAX_PACKET_SAMPLES*4);
			if( !alCaptureDevice && inputdevice )
			{
				Com_Printf( "Failed to open OpenAL Input device '%s', trying default.\n", inputdevice );
				alCaptureDevice = qalcCaptureOpenDevice(NULL, 48000, AL_FORMAT_MONO16, VOIP_MAX_PACKET_SAMPLES*4);
			}
			Com_Printf( "OpenAL capture device %s.\n",
				    (alCaptureDevice == NULL) ? "failed to open" : "opened");
		}
	}
#endif

	si->Shutdown = S_AL_Shutdown;
	si->StartSound = S_AL_StartSound;
	si->StartLocalSound = S_AL_StartLocalSound;
	si->StartBackgroundTrack = S_AL_StartBackgroundTrack;
	si->StopBackgroundTrack = S_AL_StopBackgroundTrack;
	si->RawSamples = S_AL_RawSamples;
	si->StopAllSounds = S_AL_StopAllSounds;
	si->ClearLoopingSounds = S_AL_ClearLoopingSounds;
	si->AddLoopingSound = S_AL_AddLoopingSound;
	si->AddRealLoopingSound = S_AL_AddRealLoopingSound;
	si->StopLoopingSound = S_AL_StopLoopingSound;
	si->Respatialize = S_AL_Respatialize;
	si->UpdateEntityPosition = S_AL_UpdateEntityPosition;
	si->Update = S_AL_Update;
	si->DisableSounds = S_AL_DisableSounds;
	si->BeginRegistration = S_AL_BeginRegistration;
	si->RegisterSound = S_AL_RegisterSound;
	si->ClearSoundBuffer = S_AL_ClearSoundBuffer;
	si->SoundInfo = S_AL_SoundInfo;
	si->SoundList = S_AL_SoundList;

#ifdef USE_VOIP
	si->StartCapture = S_AL_StartCapture;
	si->AvailableCaptureSamples = S_AL_AvailableCaptureSamples;
	si->Capture = S_AL_Capture;
	si->StopCapture = S_AL_StopCapture;
	si->MasterGain = S_AL_MasterGain;
#endif

	return qtrue;
#else
	return qfalse;
#endif
}

