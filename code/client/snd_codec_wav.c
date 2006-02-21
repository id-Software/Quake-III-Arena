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

/*
=================
FGetLittleLong
=================
*/
static int FGetLittleLong( fileHandle_t f ) {
	int		v;

	FS_Read( &v, sizeof(v), f );

	return LittleLong( v);
}

/*
=================
FGetLittleShort
=================
*/
static short FGetLittleShort( fileHandle_t f ) {
	short	v;

	FS_Read( &v, sizeof(v), f );

	return LittleShort( v);
}

/*
=================
S_ReadChunkInfo
=================
*/
static int S_ReadChunkInfo(fileHandle_t f, char *name)
{
	int len, r;

	name[4] = 0;

	r = FS_Read(name, 4, f);
	if(r != 4)
		return -1;

	len = FGetLittleLong(f);
	if( len < 0 ) {
		Com_Printf( S_COLOR_YELLOW "WARNING: Negative chunk length\n" );
		return -1;
	}

	return len;
}

/*
=================
S_FindRIFFChunk

Returns the length of the data in the chunk, or -1 if not found
=================
*/
static int S_FindRIFFChunk( fileHandle_t f, char *chunk ) {
	char	name[5];
	int		len;

	while( ( len = S_ReadChunkInfo(f, name) ) >= 0 )
	{
		// If this is the right chunk, return
		if( !Q_strncmp( name, chunk, 4 ) )
			return len;

		len = PAD( len, 2 );

		// Not the right chunk - skip it
		FS_Seek( f, len, FS_SEEK_CUR );
	}

	return -1;
}

/*
=================
S_ByteSwapRawSamples
=================
*/
static void S_ByteSwapRawSamples( int samples, int width, int s_channels, const byte *data ) {
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
=================
S_ReadRIFFHeader
=================
*/
static qboolean S_ReadRIFFHeader(fileHandle_t file, snd_info_t *info)
{
	char dump[16];
	int wav_format;
	int bits;
	int fmtlen = 0;

	// skip the riff wav header
	FS_Read(dump, 12, file);

	// Scan for the format chunk
	if((fmtlen = S_FindRIFFChunk(file, "fmt ")) < 0)
	{
		Com_Printf( S_COLOR_RED "ERROR: Couldn't find \"fmt\" chunk\n");
		return qfalse;
	}

	// Save the parameters
	wav_format = FGetLittleShort(file);
	info->channels = FGetLittleShort(file);
	info->rate = FGetLittleLong(file);
	FGetLittleLong(file);
	FGetLittleShort(file);
	bits = FGetLittleShort(file);

	if( bits < 8 )
	{
	  Com_Printf( S_COLOR_RED "ERROR: Less than 8 bit sound is not supported\n");
	  return qfalse;
	}

	info->width = bits / 8;
	info->dataofs = 0;

	// Skip the rest of the format chunk if required
	if(fmtlen > 16)
	{
		fmtlen -= 16;
		FS_Seek( file, fmtlen, FS_SEEK_CUR );
	}

	// Scan for the data chunk
	if( (info->size = S_FindRIFFChunk(file, "data")) < 0)
	{
		Com_Printf( S_COLOR_RED "ERROR: Couldn't find \"data\" chunk\n");
		return qfalse;
	}
	info->samples = (info->size / info->width) / info->channels;

	return qtrue;
}

// WAV codec
snd_codec_t wav_codec =
{
	".wav",
	S_WAV_CodecLoad,
	S_WAV_CodecOpenStream,
	S_WAV_CodecReadStream,
	S_WAV_CodecCloseStream,
	NULL
};

/*
=================
S_WAV_CodecLoad
=================
*/
void *S_WAV_CodecLoad(const char *filename, snd_info_t *info)
{
	fileHandle_t file;
	void *buffer;

	// Try to open the file
	FS_FOpenFileRead(filename, &file, qtrue);
	if(!file)
	{
		Com_Printf( S_COLOR_RED "ERROR: Could not open \"%s\"\n",
				filename);
		return NULL;
	}

	// Read the RIFF header
	if(!S_ReadRIFFHeader(file, info))
	{
		FS_FCloseFile(file);
		Com_Printf( S_COLOR_RED "ERROR: Incorrect/unsupported format in \"%s\"\n",
				filename);
		return NULL;
	}

	// Allocate some memory
	buffer = Z_Malloc(info->size);
	if(!buffer)
	{
		FS_FCloseFile(file);
		Com_Printf( S_COLOR_RED "ERROR: Out of memory reading \"%s\"\n",
				filename);
		return NULL;
	}

	// Read, byteswap
	FS_Read(buffer, info->size, file);
	S_ByteSwapRawSamples(info->samples, info->width, info->channels, (byte *)buffer);

	// Close and return
	FS_FCloseFile(file);
	return buffer;
}

/*
=================
S_WAV_CodecOpenStream
=================
*/
snd_stream_t *S_WAV_CodecOpenStream(const char *filename)
{
	snd_stream_t *rv;

	// Open
	rv = S_CodecUtilOpen(filename, &wav_codec);
	if(!rv)
		return NULL;

	// Read the RIFF header
	if(!S_ReadRIFFHeader(rv->file, &rv->info))
	{
		S_CodecUtilClose(rv);
		return NULL;
	}

	return rv;
}

/*
=================
S_WAV_CodecCloseStream
=================
*/
void S_WAV_CodecCloseStream(snd_stream_t *stream)
{
	S_CodecUtilClose(stream);
}

/*
=================
S_WAV_CodecReadStream
=================
*/
int S_WAV_CodecReadStream(snd_stream_t *stream, int bytes, void *buffer)
{
	int remaining = stream->info.size - stream->pos;
	int samples;

	if(remaining <= 0)
		return 0;
	if(bytes > remaining)
		bytes = remaining;
	stream->pos += bytes;
	samples = (bytes / stream->info.width) / stream->info.channels;
	FS_Read(buffer, bytes, stream->file);
	S_ByteSwapRawSamples(samples, stream->info.width, stream->info.channels, buffer);
	return bytes;
}
