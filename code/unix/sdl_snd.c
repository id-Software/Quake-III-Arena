#if USE_SDL

/*
 * SDL implementation for Quake 3: Arena's GPL source release.
 *
 * This is a replacement of the Linux/OpenSoundSystem code with
 *  an SDL backend, since it allows us to trivially point just about any
 *  existing 2D audio backend known to man on any platform at the code,
 *  plus it benefits from all of SDL's tapdancing to support buggy drivers,
 *  etc, and gets us free ALSA support, too.
 *
 * This is the best idea for a direct modernization of the Linux sound code
 *  in Quake 3. However, it would be nice to replace this with true 3D
 *  positional audio, compliments of OpenAL...
 *
 * Written by Ryan C. Gordon (icculus@icculus.org). Please refer to
 *    http://icculus.org/quake3/ for the latest version of this code.
 *
 *  Patches and comments are welcome at the above address.
 *
 * I cut-and-pasted this from linux_snd.c, and moved it to SDL line-by-line.
 *  There is probably some cruft that could be removed here.
 *
 * You should define USE_SDL=1 and then add this to the makefile.
 *  USE_SDL will disable the Open Sound System target.
 */

/*
Original copyright on Q3A sources:
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

#include <stdlib.h>
#include <stdio.h>

#include "SDL.h"

#include "../game/q_shared.h"
#include "../client/snd_local.h"

int snd_inited=0;

cvar_t *sndbits;
cvar_t *sndspeed;
cvar_t *sndchannels;
cvar_t *snddevice;
cvar_t *sdldevsamps;
cvar_t *sdlmixsamps;

static qboolean use_custom_memset = qfalse;
// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=371 
void Snd_Memset (void* dest, const int val, const size_t count)
{
  int *pDest;
  int i, iterate;

  if (!use_custom_memset)
  {
    Com_Memset(dest,val,count);
    return;
  }
  iterate = count / sizeof(int);
  pDest = (int*)dest;
  for(i=0; i<iterate; i++)
  {
    pDest[i] = val;
  }
}

/* The audio callback. All the magic happens here. */
static int dmapos = 0;
static int dmasize = 0;
static void sdl_audio_callback(void *userdata, Uint8 *stream, int len)
{
    int pos = (dmapos * (dma.samplebits/8));
    if (pos >= dmasize)
        dmapos = pos = 0;

    if (!snd_inited)  /* shouldn't happen, but just in case... */
    {
        memset(stream, '\0', len);
        return;
    }
    else
    {
        int tobufend = dmasize - pos;  /* bytes to buffer's end. */
        int len1 = len;
        int len2 = 0;

        if (len1 > tobufend)
        {
            len1 = tobufend;
            len2 = len - len1;
        }
        memcpy(stream, dma.buffer + pos, len1);
        if (len2 <= 0)
            dmapos += (len1 / (dma.samplebits/8));
        else  /* wraparound? */
        {
            memcpy(stream+len1, dma.buffer, len2);
            dmapos = (len2 / (dma.samplebits/8));
        }
    }

    if (dmapos >= dmasize)
        dmapos = 0;
}

static void print_audiospec(const char *str, const SDL_AudioSpec *spec)
{
    Com_Printf("%s:\n", str);

    // I'm sorry this is nasty.
    #define PRINT_AUDIO_FMT(x) \
        if (spec->format == x) Com_Printf("Format: %s\n", #x); else
    PRINT_AUDIO_FMT(AUDIO_U8)
    PRINT_AUDIO_FMT(AUDIO_S8)
    PRINT_AUDIO_FMT(AUDIO_U16LSB)
    PRINT_AUDIO_FMT(AUDIO_S16LSB)
    PRINT_AUDIO_FMT(AUDIO_U16MSB)
    PRINT_AUDIO_FMT(AUDIO_S16MSB)
    Com_Printf("Format: UNKNOWN\n");
    #undef PRINT_AUDIO_FMT

    Com_Printf("Freq: %d\n", (int) spec->freq);
    Com_Printf("Samples: %d\n", (int) spec->samples);
    Com_Printf("Channels: %d\n", (int) spec->channels);
    Com_Printf("\n");
}

qboolean SNDDMA_Init(void)
{
    char drivername[128];
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;
	int tmp;

	if (snd_inited)
		return 1;

    Com_Printf("SDL Audio driver initializing...\n");

	if (!snddevice) {
		sndbits = Cvar_Get("sndbits", "16", CVAR_ARCHIVE);
		sndspeed = Cvar_Get("sndspeed", "0", CVAR_ARCHIVE);
		sndchannels = Cvar_Get("sndchannels", "2", CVAR_ARCHIVE);
		snddevice = Cvar_Get("snddevice", "/dev/dsp", CVAR_ARCHIVE);
		sdldevsamps = Cvar_Get("sdldevsamps", "0", CVAR_ARCHIVE);
		sdlmixsamps = Cvar_Get("sdlmixsamps", "0", CVAR_ARCHIVE);
	}

    if (!SDL_WasInit(SDL_INIT_AUDIO))
    {
        Com_Printf("Calling SDL_Init(SDL_INIT_AUDIO)...\n");
        if (SDL_Init(SDL_INIT_AUDIO) == -1)
        {
		    Com_Printf("SDL_Init(SDL_INIT_AUDIO) failed: %s\n", SDL_GetError());
            return qfalse;
        }
        Com_Printf("SDL_Init(SDL_INIT_AUDIO) passed.\n");
    }

    if (SDL_AudioDriverName(drivername, sizeof (drivername)) == NULL)
        strcpy(drivername, "(UNKNOWN)");
    Com_Printf("SDL audio driver is \"%s\"\n", drivername);

    memset(&desired, '\0', sizeof (desired));
    memset(&obtained, '\0', sizeof (obtained));

    tmp = ((int) sndbits->value);
    if ((tmp != 16) && (tmp != 8))
        tmp = 16;

    desired.freq = (int) sndspeed->value;
    if(!desired.freq) desired.freq = 22050;
    desired.format = ((tmp == 16) ? AUDIO_S16SYS : AUDIO_U8);

    // I dunno if this is the best idea, but I'll give it a try...
    //  should probably check a cvar for this...
    if (sdldevsamps->value)
        desired.samples = sdldevsamps->value;
    else
    {
        // just pick a sane default.
        if (desired.freq <= 11025)
            desired.samples = 256;
        else if (desired.freq <= 22050)
            desired.samples = 512;
        else if (desired.freq <= 44100)
            desired.samples = 1024;
        else
            desired.samples = 2048;  // (*shrug*)
    }

    desired.channels = (int) sndchannels->value;
    desired.callback = sdl_audio_callback;

    print_audiospec("Format we requested from SDL audio device", &desired);

    if (SDL_OpenAudio(&desired, &obtained) == -1)
    {
        Com_Printf("SDL_OpenAudio() failed: %s\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return qfalse;
    } // if

    print_audiospec("Format we actually got", &obtained);

    // dma.samples needs to be big, or id's mixer will just refuse to
    //  work at all; we need to keep it significantly bigger than the
    //  amount of SDL callback samples, and just copy a little each time
    //  the callback runs.
    // 32768 is what the OSS driver filled in here on my system. I don't
    //  know if it's a good value overall, but at least we know it's
    //  reasonable...this is why I let the user override.
    tmp = sdlmixsamps->value;
    if (!tmp)
        tmp = (obtained.samples * obtained.channels) * 10;

    if (tmp & (tmp - 1))  // not a power of two? Seems to confuse something.
    {
        int val = 1;
        while (val < tmp)
            val <<= 1;

        Com_Printf("WARNING: sdlmixsamps wasn't a power of two (%d),"
                   " so we made it one (%d).\n", tmp, val);
        tmp = val;
    }

    dmapos = 0;
	dma.samplebits = obtained.format & 0xFF;  // first byte of format is bits.
    dma.channels = obtained.channels;
    dma.samples = tmp;
	dma.submission_chunk = 1;
	dma.speed = obtained.freq;
    dmasize = (dma.samples * (dma.samplebits/8));
	dma.buffer = calloc(1, dmasize);

    Com_Printf("Starting SDL audio callback...\n");
    SDL_PauseAudio(0);  // start callback.

    Com_Printf("SDL audio initialized.\n");
	snd_inited = 1;
	return qtrue;
}

int SNDDMA_GetDMAPos(void)
{
    return dmapos;
}

void SNDDMA_Shutdown(void)
{
    Com_Printf("Closing SDL audio device...\n");
    SDL_PauseAudio(1);
    SDL_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    free(dma.buffer);
    dma.buffer = NULL;
    dmapos = dmasize = 0;
    snd_inited = 0;
    Com_Printf("SDL audio device shut down.\n");
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void)
{
    SDL_UnlockAudio();
}

void SNDDMA_BeginPainting (void)
{
    SDL_LockAudio();
}

#endif  // USE_SDL

// end of linux_snd_sdl.c ...

