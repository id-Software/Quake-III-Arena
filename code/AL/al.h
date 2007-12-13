#ifndef __al_h_
#define __al_h_

/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2000 by authors.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */
#include "altypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* WIN32, not Xbox */
#ifdef _WIN32
#ifndef _XBOX
#ifdef _OPENAL32LIB
#define ALAPI __declspec(dllexport)
#else
#define ALAPI __declspec(dllimport)
#endif
#define ALAPIENTRY __cdecl
#define AL_CALLBACK 
#endif
#endif

#ifdef TARGET_OS_MAC
#if TARGET_OS_MAC
#pragma export on
#endif
#endif

#ifndef ALAPI
#define ALAPI
#endif

#ifndef ALAPIENTRY
#define ALAPIENTRY
#endif

#ifndef CALLBACK
#define AL_CALLBACK 
#endif

#define OPENAL

#ifndef AL_NO_PROTOTYPES

/*
 * Renderer State management
 */
ALAPI void ALAPIENTRY alEnable( ALenum capability );

ALAPI void ALAPIENTRY alDisable( ALenum capability ); 

ALAPI ALboolean ALAPIENTRY alIsEnabled( ALenum capability ); 


/*
 * State retrieval
 */
ALAPI const ALchar* ALAPIENTRY alGetString( ALenum param );

ALAPI void ALAPIENTRY alGetBooleanv( ALenum param, ALboolean* data );

ALAPI void ALAPIENTRY alGetIntegerv( ALenum param, ALint* data );

ALAPI void ALAPIENTRY alGetFloatv( ALenum param, ALfloat* data );

ALAPI void ALAPIENTRY alGetDoublev( ALenum param, ALdouble* data );

ALAPI ALboolean ALAPIENTRY alGetBoolean( ALenum param );

ALAPI ALint ALAPIENTRY alGetInteger( ALenum param );

ALAPI ALfloat ALAPIENTRY alGetFloat( ALenum param );

ALAPI ALdouble ALAPIENTRY alGetDouble( ALenum param );


/*
 * Error support.
 * Obtain the most recent error generated in the AL state machine.
 */
ALAPI ALenum ALAPIENTRY alGetError( ALvoid );


/* 
 * Extension support.
 * Query for the presence of an extension, and obtain any appropriate
 * function pointers and enum values.
 */
ALAPI ALboolean ALAPIENTRY alIsExtensionPresent( const ALchar* extname );

ALAPI void* ALAPIENTRY alGetProcAddress( const ALchar* fname );

ALAPI ALenum ALAPIENTRY alGetEnumValue( const ALchar* ename );


/*
 * LISTENER
 * Listener represents the location and orientation of the
 * 'user' in 3D-space.
 *
 * Properties include: -
 *
 * Gain         AL_GAIN         ALfloat
 * Position     AL_POSITION     ALfloat[3]
 * Velocity     AL_VELOCITY     ALfloat[3]
 * Orientation  AL_ORIENTATION  ALfloat[6] (Forward then Up vectors)
*/

/*
 * Set Listener parameters
 */
ALAPI void ALAPIENTRY alListenerf( ALenum param, ALfloat value );

ALAPI void ALAPIENTRY alListener3f( ALenum param, ALfloat value1, ALfloat value2, ALfloat value3 );

ALAPI void ALAPIENTRY alListenerfv( ALenum param, const ALfloat* values ); 

ALAPI void ALAPIENTRY alListeneri( ALenum param, ALint value );

ALAPI void ALAPIENTRY alListener3i( ALenum param, ALint value1, ALint value2, ALint value3 );

ALAPI void ALAPIENTRY alListeneriv( ALenum param, const ALint* values );

/*
 * Get Listener parameters
 */
ALAPI void ALAPIENTRY alGetListenerf( ALenum param, ALfloat* value );

ALAPI void ALAPIENTRY alGetListener3f( ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3 );

ALAPI void ALAPIENTRY alGetListenerfv( ALenum param, ALfloat* values );

ALAPI void ALAPIENTRY alGetListeneri( ALenum param, ALint* value );

ALAPI void ALAPIENTRY alGetListener3i( ALenum param, ALint *value1, ALint *value2, ALint *value3 );

ALAPI void ALAPIENTRY alGetListeneriv( ALenum param, ALint* values );


/**
 * SOURCE
 * Sources represent individual sound objects in 3D-space.
 * Sources take the PCM data provided in the specified Buffer,
 * apply Source-specific modifications, and then
 * submit them to be mixed according to spatial arrangement etc.
 * 
 * Properties include: -
 *
 * Gain                              AL_GAIN                 ALfloat
 * Min Gain                          AL_MIN_GAIN             ALfloat
 * Max Gain                          AL_MAX_GAIN             ALfloat
 * Position                          AL_POSITION             ALfloat[3]
 * Velocity                          AL_VELOCITY             ALfloat[3]
 * Direction                         AL_DIRECTION            ALfloat[3]
 * Head Relative Mode                AL_SOURCE_RELATIVE      ALint (AL_TRUE or AL_FALSE)
 * Reference Distance                AL_REFERENCE_DISTANCE   ALfloat
 * Max Distance                      AL_MAX_DISTANCE         ALfloat
 * RollOff Factor                    AL_ROLLOFF_FACTOR       ALfloat
 * Inner Angle                       AL_CONE_INNER_ANGLE     ALint or ALfloat
 * Outer Angle                       AL_CONE_OUTER_ANGLE     ALint or ALfloat
 * Cone Outer Gain                   AL_CONE_OUTER_GAIN      ALint or ALfloat
 * Pitch                             AL_PITCH                ALfloat
 * Looping                           AL_LOOPING			     ALint (AL_TRUE or AL_FALSE)
 * MS Offset                         AL_MSEC_OFFSET          ALint or ALfloat
 * Byte Offset                       AL_BYTE_OFFSET          ALint or ALfloat
 * Sample Offset                     AL_SAMPLE_OFFSET        ALint or ALfloat
 * Attached Buffer                   AL_BUFFER               ALint
 * State (Query only)                AL_SOURCE_STATE         ALint
 * Buffers Queued (Query only)       AL_BUFFERS_QUEUED       ALint
 * Buffers Processed (Query only)    AL_BUFFERS_PROCESSED    ALint
 */

/* Create Source objects */
ALAPI void ALAPIENTRY alGenSources( ALsizei n, ALuint* sources ); 

/* Delete Source objects */
ALAPI void ALAPIENTRY alDeleteSources( ALsizei n, const ALuint* sources );

/* Verify a handle is a valid Source */ 
ALAPI ALboolean ALAPIENTRY alIsSource( ALuint sid ); 

/*
 * Set Source parameters
 */
ALAPI void ALAPIENTRY alSourcef( ALuint sid, ALenum param, ALfloat value ); 

ALAPI void ALAPIENTRY alSource3f( ALuint sid, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3 );

ALAPI void ALAPIENTRY alSourcefv( ALuint sid, ALenum param, const ALfloat* values ); 

ALAPI void ALAPIENTRY alSourcei( ALuint sid, ALenum param, ALint value ); 

ALAPI void ALAPIENTRY alSource3i( ALuint sid, ALenum param, ALint value1, ALint value2, ALint value3 );

ALAPI void ALAPIENTRY alSourceiv( ALuint sid, ALenum param, const ALint* values );

/*
 * Get Source parameters
 */
ALAPI void ALAPIENTRY alGetSourcef( ALuint sid, ALenum param, ALfloat* value );

ALAPI void ALAPIENTRY alGetSource3f( ALuint sid, ALenum param, ALfloat* value1, ALfloat* value2, ALfloat* value3);

ALAPI void ALAPIENTRY alGetSourcefv( ALuint sid, ALenum param, ALfloat* values );

ALAPI void ALAPIENTRY alGetSourcei( ALuint sid,  ALenum param, ALint* value );

ALAPI void ALAPIENTRY alGetSource3i( ALuint sid, ALenum param, ALint* value1, ALint* value2, ALint* value3);

ALAPI void ALAPIENTRY alGetSourceiv( ALuint sid,  ALenum param, ALint* values );


/*
 * Source vector based playback calls
 */

/* Play, replay, or resume (if paused) a list of Sources */
ALAPI void ALAPIENTRY alSourcePlayv( ALsizei ns, const ALuint *sids );

/* Stop a list of Sources */
ALAPI void ALAPIENTRY alSourceStopv( ALsizei ns, const ALuint *sids );

/* Rewind a list of Sources */
ALAPI void ALAPIENTRY alSourceRewindv( ALsizei ns, const ALuint *sids );

/* Pause a list of Sources */
ALAPI void ALAPIENTRY alSourcePausev( ALsizei ns, const ALuint *sids );

/*
 * Source based playback calls
 */

/* Play, replay, or resume a Source */
ALAPI void ALAPIENTRY alSourcePlay( ALuint sid );

/* Stop a Source */
ALAPI void ALAPIENTRY alSourceStop( ALuint sid );

/* Rewind a Source (set playback postiton to beginning) */
ALAPI void ALAPIENTRY alSourceRewind( ALuint sid );

/* Pause a Source */
ALAPI void ALAPIENTRY alSourcePause( ALuint sid );

/*
 * Source Queuing 
 */
ALAPI void ALAPIENTRY alSourceQueueBuffers( ALuint sid, ALsizei numEntries, const ALuint *bids );

ALAPI void ALAPIENTRY alSourceUnqueueBuffers( ALuint sid, ALsizei numEntries, ALuint *bids );


/**
 * BUFFER
 * Buffer objects are storage space for sample data.
 * Buffers are referred to by Sources. One Buffer can be used
 * by multiple Sources.
 *
 * Properties include: -
 *
 * Frequency (Query only)    AL_FREQUENCY      ALint
 * Size (Query only)         AL_SIZE           ALint
 * Bits (Query only)         AL_BITS           ALint
 * Channels (Query only)     AL_CHANNELS       ALint
 */

/* Create Buffer objects */
ALAPI void ALAPIENTRY alGenBuffers( ALsizei n, ALuint* buffers );

/* Delete Buffer objects */
ALAPI void ALAPIENTRY alDeleteBuffers( ALsizei n, const ALuint* buffers );

/* Verify a handle is a valid Buffer */
ALAPI ALboolean ALAPIENTRY alIsBuffer( ALuint bid );

/* Specify the data to be copied into a buffer */
ALAPI void ALAPIENTRY alBufferData( ALuint bid, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq );

/*
 * Set Buffer parameters
 */
ALAPI void ALAPIENTRY alBufferf( ALuint bid, ALenum param, ALfloat value );

ALAPI void ALAPIENTRY alBuffer3f( ALuint bid, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3 );

ALAPI void ALAPIENTRY alBufferfv( ALuint bid, ALenum param, const ALfloat* values );

ALAPI void ALAPIENTRY alBufferi( ALuint bid, ALenum param, ALint value );

ALAPI void ALAPIENTRY alBuffer3i( ALuint bid, ALenum param, ALint value1, ALint value2, ALint value3 );

ALAPI void ALAPIENTRY alBufferiv( ALuint bid, ALenum param, const ALint* values );

/*
 * Get Buffer parameters
 */
ALAPI void ALAPIENTRY alGetBufferf( ALuint bid, ALenum param, ALfloat* value );

ALAPI void ALAPIENTRY alGetBuffer3f( ALuint bid, ALenum param, ALfloat* value1, ALfloat* value2, ALfloat* value3);

ALAPI void ALAPIENTRY alGetBufferfv( ALuint bid, ALenum param, ALfloat* values );

ALAPI void ALAPIENTRY alGetBufferi( ALuint bid, ALenum param, ALint* value );

ALAPI void ALAPIENTRY alGetBuffer3i( ALuint bid, ALenum param, ALint* value1, ALint* value2, ALint* value3);

ALAPI void ALAPIENTRY alGetBufferiv( ALuint bid, ALenum param, ALint* values );


/*
 * Global Parameters
 */
ALAPI void ALAPIENTRY alDopplerFactor( ALfloat value );

ALAPI void ALAPIENTRY alDopplerVelocity( ALfloat value );

ALAPI void ALAPIENTRY alSpeedOfSound( ALfloat value );

ALAPI void ALAPIENTRY alDistanceModel( ALenum distanceModel );

#else /* AL_NO_PROTOTYPES */

/*
void          (ALAPIENTRY *alEnable)( ALenum capability );
void          (ALAPIENTRY *alDisable)( ALenum capability ); 
ALboolean     (ALAPIENTRY *alIsEnabled)( ALenum capability ); 
const ALchar* (ALAPIENTRY *alGetString)( ALenum param );
void          (ALAPIENTRY *alGetBooleanv)( ALenum param, ALboolean* data );
void          (ALAPIENTRY *alGetIntegerv)( ALenum param, ALint* data );
void          (ALAPIENTRY *alGetFloatv)( ALenum param, ALfloat* data );
void          (ALAPIENTRY *alGetDoublev)( ALenum param, ALdouble* data );
ALboolean     (ALAPIENTRY *alGetBoolean)( ALenum param );
ALint         (ALAPIENTRY *alGetInteger)( ALenum param );
ALfloat       (ALAPIENTRY *alGetFloat)( ALenum param );
ALdouble      (ALAPIENTRY *alGetDouble)( ALenum param );
ALenum        (ALAPIENTRY *alGetError)( ALvoid );
ALboolean     (ALAPIENTRY *alIsExtensionPresent)(const ALchar* extname );
void*         (ALAPIENTRY *alGetProcAddress)( const ALchar* fname );
ALenum        (ALAPIENTRY *alGetEnumValue)( const ALchar* ename );
void          (ALAPIENTRY *alListenerf)( ALenum param, ALfloat value );
void          (ALAPIENTRY *alListener3f)( ALenum param, ALfloat value1, ALfloat value2, ALfloat value3 );
void          (ALAPIENTRY *alListenerfv)( ALenum param, const ALfloat* values );
void          (ALAPIENTRY *alListeneri)( ALenum param, ALint value );
void          (ALAPIENTRY *alListener3i)( ALenum param, ALint value1, ALint value2, ALint value3 );
void          (ALAPIENTRY *alListeneriv)( ALenum param, const ALint* values );
void          (ALAPIENTRY *alGetListenerf)( ALenum param, ALfloat* value );
void          (ALAPIENTRY *alGetListener3f)( ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3 );
void          (ALAPIENTRY *alGetListenerfv)( ALenum param, ALfloat* values );
void          (ALAPIENTRY *alGetListeneri)( ALenum param, ALint* value );
void          (ALAPIENTRY *alGetListener3i)( ALenum param, ALint *value1, ALint *value2, ALint *value3 );
void          (ALAPIENTRY *alGetListeneriv)( ALenum param, ALint* values );
void          (ALAPIENTRY *alGenSources)( ALsizei n, ALuint* sources );
void          (ALAPIENTRY *alDeleteSources)( ALsizei n, const ALuint* sources );
ALboolean     (ALAPIENTRY *alIsSource)( ALuint sid ); 
void          (ALAPIENTRY *alSourcef)( ALuint sid, ALenum param, ALfloat value);
void          (ALAPIENTRY *alSource3f)( ALuint sid, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3 );
void          (ALAPIENTRY *alSourcefv)( ALuint sid, ALenum param, const ALfloat* values );
void          (ALAPIENTRY *alSourcei)( ALuint sid, ALenum param, ALint value);
void          (ALAPIENTRY *alSource3i)( ALuint sid, ALenum param, ALint value1, ALint value2, ALint value3 );
void          (ALAPIENTRY *alSourceiv)( ALuint sid, ALenum param, const ALint* values );
void          (ALAPIENTRY *alGetSourcef)( ALuint sid, ALenum param, ALfloat* value );
void          (ALAPIENTRY *alGetSource3f)( ALuint sid, ALenum param, ALfloat* value1, ALfloat* value2, ALfloat* value3);
void          (ALAPIENTRY *alGetSourcefv)( ALuint sid, ALenum param, ALfloat* values );
void          (ALAPIENTRY *alGetSourcei)( ALuint sid, ALenum param, ALint* value );
void          (ALAPIENTRY *alGetSource3i)( ALuint sid, ALenum param, ALint* value1, ALint* value2, ALint* value3);
void          (ALAPIENTRY *alGetSourceiv)( ALuint sid, ALenum param, ALint* values );
void          (ALAPIENTRY *alSourcePlayv)( ALsizei ns, const ALuint *sids );
void          (ALAPIENTRY *alSourceStopv)( ALsizei ns, const ALuint *sids );
void          (ALAPIENTRY *alSourceRewindv)( ALsizei ns, const ALuint *sids );
void          (ALAPIENTRY *alSourcePausev)( ALsizei ns, const ALuint *sids );
void          (ALAPIENTRY *alSourcePlay)( ALuint sid );
void          (ALAPIENTRY *alSourceStop)( ALuint sid );
void          (ALAPIENTRY *alSourceRewind)( ALuint sid );
void          (ALAPIENTRY *alSourcePause)( ALuint sid );
void          (ALAPIENTRY *alSourceQueueBuffers)( ALuint sid, ALsizei numEntries, const ALuint *bids );
void          (ALAPIENTRY *alSourceUnqueueBuffers)( ALuint sid, ALsizei numEntries, ALuint *bids );
void          (ALAPIENTRY *alGenBuffers)( ALsizei n, ALuint* buffers );
void          (ALAPIENTRY *alDeleteBuffers)( ALsizei n, const ALuint* buffers );
ALboolean     (ALAPIENTRY *alIsBuffer)( ALuint bid );
void          (ALAPIENTRY *alBufferData)( ALuint bid, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq );
void          (ALAPIENTRY *alBufferf)( ALuint bid, ALenum param, ALfloat value);
void          (ALAPIENTRY *alBuffer3f)( ALuint bid, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3 );
void          (ALAPIENTRY *alBufferfv)( ALuint bid, ALenum param, const ALfloat* values );
void          (ALAPIENTRY *alBufferi)( ALuint bid, ALenum param, ALint value);
void          (ALAPIENTRY *alBuffer3i)( ALuint bid, ALenum param, ALint value1, ALint value2, ALint value3 );
void          (ALAPIENTRY *alBufferiv)( ALuint bid, ALenum param, const ALint* values );
void          (ALAPIENTRY *alGetBufferf)( ALuint bid, ALenum param, ALfloat* value );
void          (ALAPIENTRY *alGetBuffer3f)( ALuint bid, ALenum param, ALfloat* value1, ALfloat* value2, ALfloat* value3);
void          (ALAPIENTRY *alGetBufferfv)( ALuint bid, ALenum param, ALfloat* values );
void          (ALAPIENTRY *alGetBufferi)( ALuint bid, ALenum param, ALint* value );
void          (ALAPIENTRY *alGetBuffer3i)( ALuint bid, ALenum param, ALint* value1, ALint* value2, ALint* value3);
void          (ALAPIENTRY *alGetBufferiv)( ALuint bid, ALenum param, ALint* values );
void          (ALAPIENTRY *alDopplerFactor)( ALfloat value );
void          (ALAPIENTRY *alDopplerVelocity)( ALfloat value );
void          (ALAPIENTRY *alSpeedOfSound)( ALfloat value );
void          (ALAPIENTRY *alDistanceModel)( ALenum distanceModel );
*/
/* Type Definitions */

typedef void           (ALAPIENTRY *LPALENABLE)( ALenum capability );
typedef void           (ALAPIENTRY *LPALDISABLE)( ALenum capability ); 
typedef ALboolean      (ALAPIENTRY *LPALISENABLED)( ALenum capability ); 
typedef const ALchar*  (ALAPIENTRY *LPALGETSTRING)( ALenum param );
typedef void           (ALAPIENTRY *LPALGETBOOLEANV)( ALenum param, ALboolean* data );
typedef void           (ALAPIENTRY *LPALGETINTEGERV)( ALenum param, ALint* data );
typedef void           (ALAPIENTRY *LPALGETFLOATV)( ALenum param, ALfloat* data );
typedef void           (ALAPIENTRY *LPALGETDOUBLEV)( ALenum param, ALdouble* data );
typedef ALboolean      (ALAPIENTRY *LPALGETBOOLEAN)( ALenum param );
typedef ALint          (ALAPIENTRY *LPALGETINTEGER)( ALenum param );
typedef ALfloat        (ALAPIENTRY *LPALGETFLOAT)( ALenum param );
typedef ALdouble       (ALAPIENTRY *LPALGETDOUBLE)( ALenum param );
typedef ALenum         (ALAPIENTRY *LPALGETERROR)( ALvoid );
typedef ALboolean      (ALAPIENTRY *LPALISEXTENSIONPRESENT)(const ALchar* extname );
typedef void*          (ALAPIENTRY *LPALGETPROCADDRESS)( const ALchar* fname );
typedef ALenum         (ALAPIENTRY *LPALGETENUMVALUE)( const ALchar* ename );
typedef void           (ALAPIENTRY *LPALLISTENERF)( ALenum param, ALfloat value );
typedef void           (ALAPIENTRY *LPALLISTENER3F)( ALenum param, ALfloat value1, ALfloat value2, ALfloat value3 );
typedef void           (ALAPIENTRY *LPALLISTENERFV)( ALenum param, const ALfloat* values );
typedef void           (ALAPIENTRY *LPALLISTENERI)( ALenum param, ALint value );
typedef void           (ALAPIENTRY *LPALLISTENER3I)( ALenum param, ALint value1, ALint value2, ALint value3 );
typedef void           (ALAPIENTRY *LPALLISTENERIV)( ALenum param, const ALint* values );
typedef void           (ALAPIENTRY *LPALGETLISTENERF)( ALenum param, ALfloat* value );
typedef void           (ALAPIENTRY *LPALGETLISTENER3F)( ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3 );
typedef void           (ALAPIENTRY *LPALGETLISTENERFV)( ALenum param, ALfloat* values );
typedef void           (ALAPIENTRY *LPALGETLISTENERI)( ALenum param, ALint* value );
typedef void           (ALAPIENTRY *LPALGETLISTENER3I)( ALenum param, ALint *value1, ALint *value2, ALint *value3 );
typedef void           (ALAPIENTRY *LPALGETLISTENERIV)( ALenum param, ALint* values );
typedef void           (ALAPIENTRY *LPALGENSOURCES)( ALsizei n, ALuint* sources ); 
typedef void           (ALAPIENTRY *LPALDELETESOURCES)( ALsizei n, const ALuint* sources );
typedef ALboolean      (ALAPIENTRY *LPALISSOURCE)( ALuint sid ); 
typedef void           (ALAPIENTRY *LPALSOURCEF)( ALuint sid, ALenum param, ALfloat value); 
typedef void           (ALAPIENTRY *LPALSOURCE3F)( ALuint sid, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3 );
typedef void           (ALAPIENTRY *LPALSOURCEFV)( ALuint sid, ALenum param, const ALfloat* values );
typedef void           (ALAPIENTRY *LPALSOURCEI)( ALuint sid, ALenum param, ALint value); 
typedef void           (ALAPIENTRY *LPALSOURCE3I)( ALuint sid, ALenum param, ALint value1, ALint value2, ALint value3 );
typedef void           (ALAPIENTRY *LPALSOURCEIV)( ALuint sid, ALenum param, const ALint* values );
typedef void           (ALAPIENTRY *LPALGETSOURCEF)( ALuint sid, ALenum param, ALfloat* value );
typedef void           (ALAPIENTRY *LPALGETSOURCE3F)( ALuint sid, ALenum param, ALfloat* value1, ALfloat* value2, ALfloat* value3);
typedef void           (ALAPIENTRY *LPALGETSOURCEFV)( ALuint sid, ALenum param, ALfloat* values );
typedef void           (ALAPIENTRY *LPALGETSOURCEI)( ALuint sid, ALenum param, ALint* value );
typedef void           (ALAPIENTRY *LPALGETSOURCE3I)( ALuint sid, ALenum param, ALint* value1, ALint* value2, ALint* value3);
typedef void           (ALAPIENTRY *LPALGETSOURCEIV)( ALuint sid, ALenum param, ALint* values );
typedef void           (ALAPIENTRY *LPALSOURCEPLAYV)( ALsizei ns, const ALuint *sids );
typedef void           (ALAPIENTRY *LPALSOURCESTOPV)( ALsizei ns, const ALuint *sids );
typedef void           (ALAPIENTRY *LPALSOURCEREWINDV)( ALsizei ns, const ALuint *sids );
typedef void           (ALAPIENTRY *LPALSOURCEPAUSEV)( ALsizei ns, const ALuint *sids );
typedef void           (ALAPIENTRY *LPALSOURCEPLAY)( ALuint sid );
typedef void           (ALAPIENTRY *LPALSOURCESTOP)( ALuint sid );
typedef void           (ALAPIENTRY *LPALSOURCEREWIND)( ALuint sid );
typedef void           (ALAPIENTRY *LPALSOURCEPAUSE)( ALuint sid );
typedef void           (ALAPIENTRY *LPALSOURCEQUEUEBUFFERS)(ALuint sid, ALsizei numEntries, const ALuint *bids );
typedef void           (ALAPIENTRY *LPALSOURCEUNQUEUEBUFFERS)(ALuint sid, ALsizei numEntries, ALuint *bids );
typedef void           (ALAPIENTRY *LPALGENBUFFERS)( ALsizei n, ALuint* buffers );
typedef void           (ALAPIENTRY *LPALDELETEBUFFERS)( ALsizei n, const ALuint* buffers );
typedef ALboolean      (ALAPIENTRY *LPALISBUFFER)( ALuint bid );
typedef void           (ALAPIENTRY *LPALBUFFERDATA)( ALuint bid, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq );
typedef void           (ALAPIENTRY *LPALBUFFERF)( ALuint bid, ALenum param, ALfloat value);
typedef void           (ALAPIENTRY *LPALBUFFER3F)( ALuint bid, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3 );
typedef void           (ALAPIENTRY *LPALBUFFERFV)( ALuint bid, ALenum param, const ALfloat* values );
typedef void           (ALAPIENTRY *LPALBUFFERI)( ALuint bid, ALenum param, ALint value);
typedef void           (ALAPIENTRY *LPALBUFFER3I)( ALuint bid, ALenum param, ALint value1, ALint value2, ALint value3 );
typedef void           (ALAPIENTRY *LPALBUFFERIV)( ALuint bid, ALenum param, const ALint* values );
typedef void           (ALAPIENTRY *LPALGETBUFFERF)( ALuint bid, ALenum param, ALfloat* value );
typedef void           (ALAPIENTRY *LPALGETBUFFER3F)( ALuint bid, ALenum param, ALfloat* value1, ALfloat* value2, ALfloat* value3);
typedef void           (ALAPIENTRY *LPALGETBUFFERFV)( ALuint bid, ALenum param, ALfloat* values );
typedef void           (ALAPIENTRY *LPALGETBUFFERI)( ALuint bid, ALenum param, ALint* value );
typedef void           (ALAPIENTRY *LPALGETBUFFER3I)( ALuint bid, ALenum param, ALint* value1, ALint* value2, ALint* value3);
typedef void           (ALAPIENTRY *LPALGETBUFFERIV)( ALuint bid, ALenum param, ALint* values );
typedef void           (ALAPIENTRY *LPALDOPPLERFACTOR)( ALfloat value );
typedef void           (ALAPIENTRY *LPALDOPPLERVELOCITY)( ALfloat value );
typedef void           (ALAPIENTRY *LPALSPEEDOFSOUND)( ALfloat value );
typedef void           (ALAPIENTRY *LPALDISTANCEMODEL)( ALenum distanceModel );

#endif /* AL_NO_PROTOTYPES */

#ifdef TARGET_OS_MAC
#if TARGET_OS_MAC
#pragma export off
#endif /* TARGET_OS_MAC */
#endif /* TARGET_OS_MAC */


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* __al_h_ */
