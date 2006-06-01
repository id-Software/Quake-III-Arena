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

#include "client.h"
#include "snd_codec.h"

static snd_codec_t *codecs;

/*
=================
S_FileExtension
=================
*/
static char *S_FileExtension(const char *fni)
{
	// we should search from the ending to the last '/'

	char *fn = (char *) fni + strlen(fni) - 1;
	char *eptr = NULL;

	while(*fn != '/' && fn != fni)
	{
		if(*fn == '.')
			eptr = fn;
		fn--;
	}

	return eptr;
}

/*
=================
S_FindCodecForFile

Select an appropriate codec for a file based on its extension
=================
*/
static snd_codec_t *S_FindCodecForFile(const char *filename)
{
	char *ext = S_FileExtension(filename);
	snd_codec_t *codec = codecs;

	if(!ext)
	{
		// No extension - auto-detect
		while(codec)
		{
			char fn[MAX_QPATH];
			
			// there is no extension so we do not need to subtract 4 chars
			Q_strncpyz(fn, filename, MAX_QPATH);
			COM_DefaultExtension(fn, MAX_QPATH, codec->ext);

			// Check it exists
			if(FS_ReadFile(fn, NULL) != -1)
				return codec;

			// Nope. Next!
			codec = codec->next;
		}

		// Nothin'
		return NULL;
	}

	while(codec)
	{
		if(!Q_stricmp(ext, codec->ext))
			return codec;
		codec = codec->next;
	}

	return NULL;
}

/*
=================
S_CodecInit
=================
*/
void S_CodecInit()
{
	codecs = NULL;
	S_CodecRegister(&wav_codec);
#if USE_CODEC_VORBIS
	S_CodecRegister(&ogg_codec);
#endif
}

/*
=================
S_CodecShutdown
=================
*/
void S_CodecShutdown()
{
	codecs = NULL;
}

/*
=================
S_CodecRegister
=================
*/
void S_CodecRegister(snd_codec_t *codec)
{
	codec->next = codecs;
	codecs = codec;
}

/*
=================
S_CodecLoad
=================
*/
void *S_CodecLoad(const char *filename, snd_info_t *info)
{
	snd_codec_t *codec;
	char fn[MAX_QPATH];

	codec = S_FindCodecForFile(filename);
	if(!codec)
	{
		Com_Printf("Unknown extension for %s\n", filename);
		return NULL;
	}

	strncpy(fn, filename, sizeof(fn));
	COM_DefaultExtension(fn, sizeof(fn), codec->ext);

	return codec->load(fn, info);
}

/*
=================
S_CodecOpenStream
=================
*/
snd_stream_t *S_CodecOpenStream(const char *filename)
{
	snd_codec_t *codec;
	char fn[MAX_QPATH];

	codec = S_FindCodecForFile(filename);
	if(!codec)
	{
		Com_Printf("Unknown extension for %s\n", filename);
		return NULL;
	}

	strncpy(fn, filename, sizeof(fn));
	COM_DefaultExtension(fn, sizeof(fn), codec->ext);

	return codec->open(fn);
}

void S_CodecCloseStream(snd_stream_t *stream)
{
	stream->codec->close(stream);
}

int S_CodecReadStream(snd_stream_t *stream, int bytes, void *buffer)
{
	return stream->codec->read(stream, bytes, buffer);
}

//=======================================================================
// Util functions (used by codecs)

/*
=================
S_CodecUtilOpen
=================
*/
snd_stream_t *S_CodecUtilOpen(const char *filename, snd_codec_t *codec)
{
	snd_stream_t *stream;
	fileHandle_t hnd;
	int length;

	// Try to open the file
	length = FS_FOpenFileRead(filename, &hnd, qtrue);
	if(!hnd)
	{
		Com_Printf("Can't read sound file %s\n", filename);
		return NULL;
	}

	// Allocate a stream
	stream = Z_Malloc(sizeof(snd_stream_t));
	if(!stream)
	{
		FS_FCloseFile(hnd);
		return NULL;
	}

	// Copy over, return
	stream->codec = codec;
	stream->file = hnd;
	stream->length = length;
	return stream;
}

/*
=================
S_CodecUtilClose
=================
*/
void S_CodecUtilClose(snd_stream_t *stream)
{
	FS_FCloseFile(stream->file);
	Z_Free(stream);
}
