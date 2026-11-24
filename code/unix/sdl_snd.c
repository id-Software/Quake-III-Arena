/*
===========================================================================
SDL2 sound backend (replaces the OSS /dev/dsp code in linux_snd.c).
Q3's mixer writes into a ring buffer (dma.buffer); an SDL audio callback
drains it and reports the read position through SNDDMA_GetDMAPos.
===========================================================================
*/
#include <SDL2/SDL.h>

#include "../game/q_shared.h"
#include "../client/snd_local.h"

static int snd_inited = 0;
static SDL_AudioDeviceID audioDev = 0;
static int dmapos = 0;          // in samples
static int dmasize = 0;         // in bytes

cvar_t *s_sdlBits;
cvar_t *s_sdlSpeed;
cvar_t *s_sdlChannels;
cvar_t *s_sdlDevSamps;

static void SNDDMA_AudioCallback( void *userdata, Uint8 *stream, int len )
{
	int bytes = dma.samplebits / 8;
	int pos = dmapos * bytes;
	int tobufend, len1, len2;

	if ( !snd_inited || !dma.buffer ) {
		memset( stream, 0, len );
		return;
	}
	if ( pos >= dmasize ) {
		dmapos = pos = 0;
	}

	tobufend = dmasize - pos;
	len1 = len;
	len2 = 0;
	if ( len1 > tobufend ) {
		len1 = tobufend;
		len2 = len - len1;
	}
	memcpy( stream, dma.buffer + pos, len1 );
	if ( len2 <= 0 ) {
		dmapos += len1 / bytes;
	} else {
		memcpy( stream + len1, dma.buffer, len2 );
		dmapos = len2 / bytes;
	}
	if ( dmapos >= dmasize / bytes ) {
		dmapos = 0;
	}
}

qboolean SNDDMA_Init( void )
{
	SDL_AudioSpec desired, obtained;
	int tmp;

	if ( snd_inited ) {
		return qtrue;
	}

	s_sdlBits = Cvar_Get( "sdlbits", "16", CVAR_ARCHIVE );
	s_sdlSpeed = Cvar_Get( "sdlspeed", "0", CVAR_ARCHIVE );
	s_sdlChannels = Cvar_Get( "sdlchannels", "2", CVAR_ARCHIVE );
	s_sdlDevSamps = Cvar_Get( "sdldevsamps", "0", CVAR_ARCHIVE );

	Com_Printf( "SDL_Init( SDL_INIT_AUDIO )... " );
	if ( SDL_InitSubSystem( SDL_INIT_AUDIO ) != 0 ) {
		Com_Printf( "FAILED (%s)\n", SDL_GetError() );
		return qfalse;
	}
	Com_Printf( "OK\n" );

	memset( &desired, 0, sizeof( desired ) );
	memset( &obtained, 0, sizeof( obtained ) );

	tmp = s_sdlBits->integer;
	if ( tmp != 16 && tmp != 8 ) tmp = 16;
	desired.format = ( tmp == 16 ) ? AUDIO_S16SYS : AUDIO_U8;

	desired.freq = s_sdlSpeed->integer;
	if ( !desired.freq ) desired.freq = 22050;

	desired.samples = s_sdlDevSamps->integer;
	if ( !desired.samples ) desired.samples = 512;

	desired.channels = s_sdlChannels->integer;
	if ( desired.channels < 1 || desired.channels > 2 ) desired.channels = 2;
	desired.callback = SNDDMA_AudioCallback;

	// allow no changes: SDL converts to whatever the device wants for us
	audioDev = SDL_OpenAudioDevice( NULL, 0, &desired, &obtained, 0 );
	if ( !audioDev ) {
		Com_Printf( "SDL_OpenAudioDevice() failed: %s\n", SDL_GetError() );
		SDL_QuitSubSystem( SDL_INIT_AUDIO );
		return qfalse;
	}

	Com_Printf( "Audio: %d Hz, %d ch, %d-bit, %d sample buffer\n", obtained.freq, obtained.channels,
	            ( obtained.format & 0xFF ), obtained.samples );

	// ring buffer: a power of two number of samples, at least ~10 device buffers
	tmp = obtained.samples * obtained.channels * 10;
	if ( tmp & ( tmp - 1 ) ) {
		int v = 1;
		while ( v < tmp ) v <<= 1;
		tmp = v;
	}

	dma.samplebits = obtained.format & 0xFF;
	dma.speed = obtained.freq;
	dma.channels = obtained.channels;
	dma.samples = tmp;
	dma.submission_chunk = 1;
	dmasize = dma.samples * ( dma.samplebits / 8 );
	dma.buffer = calloc( 1, dmasize );
	dmapos = 0;

	snd_inited = 1;
	SDL_PauseAudioDevice( audioDev, 0 );
	return qtrue;
}

int SNDDMA_GetDMAPos( void )
{
	return snd_inited ? dmapos : 0;
}

void SNDDMA_Shutdown( void )
{
	if ( audioDev ) {
		SDL_PauseAudioDevice( audioDev, 1 );
		SDL_CloseAudioDevice( audioDev );
		audioDev = 0;
	}
	if ( snd_inited ) {
		SDL_QuitSubSystem( SDL_INIT_AUDIO );
	}
	snd_inited = 0;
	if ( dma.buffer ) {
		free( dma.buffer );
		dma.buffer = NULL;
	}
}

void SNDDMA_BeginPainting( void )
{
	if ( audioDev ) SDL_LockAudioDevice( audioDev );
}

void SNDDMA_Submit( void )
{
	if ( audioDev ) SDL_UnlockAudioDevice( audioDev );
}

// declared in q_shared.h on Linux (old glibc memset workaround; not needed anymore)
void Snd_Memset( void *dest, const int val, const size_t count )
{
	Com_Memset( dest, val, count );
}
