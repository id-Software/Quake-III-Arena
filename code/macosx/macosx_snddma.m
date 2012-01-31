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

// mac_snddma.c
// all other sound mixing is portable

#include "../client/snd_local.h"
#include <Carbon/Carbon.h>

// For 'ri'
#include "../renderer/tr_local.h"

#import <Foundation/NSZone.h>

// TJW - Different versions of SoundManager have different DMA buffer sizes.  On MacOS X DP2,
// the buffer size is 8K.  On MacOS 9 it is much smaller.  The SoundManager guy at Apple says
// that the size of the buffer will be decreasing for final release to help get rid of latency.
//#define	MAX_MIXED_SAMPLES	(0x8000 * 64)
//#define	SUBMISSION_CHUNK	 (0x100 * 64)

// Original MacOS 9 sizes
//#define	MAX_MIXED_SAMPLES	0x8000
//#define	SUBMISSION_CHUNK	 0x100 


static unsigned int submissionChunk;
static unsigned int maxMixedSamples;

static	short		*s_mixedSamples;
static	int				 s_chunkCount;		// number of chunks submitted
static	SndChannel		*s_sndChan;
static	ExtSoundHeader	s_sndHeader;

/*
===============
S_Callback
===============
*/
void S_Callback( SndChannel *sc, SndCommand *cmd )
{
    SndCommand		mySndCmd;
    SndCommand		mySndCmd2;
    int				offset;

    offset = ( s_chunkCount * submissionChunk ) & (maxMixedSamples-1);

    // queue up another sound buffer
    memset( &s_sndHeader, 0, sizeof( s_sndHeader ) );
    s_sndHeader.samplePtr = (void *)(s_mixedSamples + offset);
    s_sndHeader.numChannels = 2;
    s_sndHeader.sampleRate = rate22khz;
    s_sndHeader.loopStart = 0;
    s_sndHeader.loopEnd = 0;
    s_sndHeader.encode = extSH;
    s_sndHeader.baseFrequency = 1;
    s_sndHeader.numFrames = submissionChunk / 2;
    s_sndHeader.markerChunk = NULL;
    s_sndHeader.instrumentChunks = NULL;
    s_sndHeader.AESRecording = NULL;
    s_sndHeader.sampleSize = 16;

    mySndCmd.cmd = bufferCmd;
    mySndCmd.param1 = 0;
    mySndCmd.param2 = (int)&s_sndHeader;
    SndDoCommand( sc, &mySndCmd, true );

    // and another callback
    mySndCmd2.cmd = callBackCmd;
    mySndCmd2.param1 = 0;
    mySndCmd2.param2 = 0;
    SndDoCommand( sc, &mySndCmd2, true );

    s_chunkCount++;		// this is the next buffer we will submit
}

/*
===============
S_MakeTestPattern
===============
*/
void S_MakeTestPattern( void ) {
	int		i;
	float	v;
	int		sample;
	
	for ( i = 0 ; i < dma.samples / 2 ; i ++ ) {
		v = sin( M_PI * 2 * i / 64 );
		sample = v * 0x4000;
		((short *)dma.buffer)[i*2] = sample;	
		((short *)dma.buffer)[i*2+1] = sample;	
	}
}

/*
===============
SNDDMA_Init
===============
*/
qboolean SNDDMA_Init(void)
{
    int		err;
    cvar_t *bufferSize;
    cvar_t *chunkSize;

    chunkSize = ri.Cvar_Get( "s_chunksize", "8192", CVAR_ARCHIVE );
    bufferSize = ri.Cvar_Get( "s_buffersize", "65536", CVAR_ARCHIVE );

    if (!chunkSize->integer) {
        ri.Error(ERR_FATAL, "snd_chunkSize must be non-zero\n");
    }

    if (!bufferSize->integer) {
        ri.Error(ERR_FATAL, "snd_bufferSize must be non-zero\n");
    }

    if (chunkSize->integer >= bufferSize->integer) {
        ri.Error(ERR_FATAL, "snd_chunkSize must be less than snd_bufferSize\n");
    }

    if (bufferSize->integer % chunkSize->integer) {
        ri.Error(ERR_FATAL, "snd_bufferSize must be an even multiple of snd_chunkSize\n");
    }

    // create a sound channel
    s_sndChan = NULL;
    err = SndNewChannel( &s_sndChan, sampledSynth, initStereo, NewSndCallBackProc(S_Callback) );
    if ( err ) {
        return false;
    }

    submissionChunk = chunkSize->integer;
    maxMixedSamples = bufferSize->integer;

    s_mixedSamples = NSZoneMalloc(NULL, sizeof(*s_mixedSamples) * maxMixedSamples);
    
    dma.channels = 2;
    dma.samples = maxMixedSamples;
    dma.submission_chunk = submissionChunk;
    dma.samplebits = 16;
    dma.speed = 22050;
    dma.buffer = (byte *)s_mixedSamples;

    // que up the first submission-chunk sized buffer
    s_chunkCount = 0;

    S_Callback( s_sndChan, NULL );

    return qtrue;
}

/*
===============
SNDDMA_GetDMAPos
===============
*/
int	SNDDMA_GetDMAPos(void) {
    return s_chunkCount * submissionChunk;
}

/*
===============
SNDDMA_Shutdown
===============
*/
void SNDDMA_Shutdown(void) {
	if ( s_sndChan ) {
		SndDisposeChannel( s_sndChan, true );
		s_sndChan = NULL;
	}
}

/*
===============
SNDDMA_BeginPainting
===============
*/
void SNDDMA_BeginPainting(void) {
}

/*
===============
SNDDMA_Submit
===============
*/
void SNDDMA_Submit(void) {
}
