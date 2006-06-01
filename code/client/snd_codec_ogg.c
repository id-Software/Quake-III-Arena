/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2005 Stuart Dalton (badcdev@gmail.com)
Copyright (C) 2005-2006 Joerg Dietrich <dietrich_joerg@gmx.de>

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

// OGG support is enabled by this define
#if USE_CODEC_VORBIS

// includes for the Q3 sound system
#include "client.h"
#include "snd_codec.h"

// includes for the OGG codec
#include <errno.h>
#include <vorbis/vorbisfile.h>

// The OGG codec can return the samples in a number of different formats,
// we use the standard signed short format.
#define OGG_SAMPLEWIDTH 2

// Q3 OGG codec
snd_codec_t ogg_codec =
{
	".ogg",
	S_OGG_CodecLoad,
	S_OGG_CodecOpenStream,
	S_OGG_CodecReadStream,
	S_OGG_CodecCloseStream,
	NULL
};

// callbacks for vobisfile

// fread() replacement
size_t S_OGG_Callback_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	snd_stream_t *stream;
	int byteSize = 0;
	int bytesRead = 0;
	size_t nMembRead = 0;

	// check if input is valid
	if(!ptr)
	{
		errno = EFAULT; 
		return 0;
	}
	
	if(!(size && nmemb))
	{
		// It's not an error, caller just wants zero bytes!
		errno = 0;
		return 0;
	}
 
	if(!datasource)
	{
		errno = EBADF; 
		return 0;
	}
	
	// we use a snd_stream_t in the generic pointer to pass around
	stream = (snd_stream_t *) datasource;

	// FS_Read does not support multi-byte elements
	byteSize = nmemb * size;

	// read it with the Q3 function FS_Read()
	bytesRead = FS_Read(ptr, byteSize, stream->file);

	// update the file position
	stream->pos += bytesRead;

	// this function returns the number of elements read not the number of bytes
	nMembRead = bytesRead / size;

	// even if the last member is only read partially
	// it is counted as a whole in the return value	
	if(bytesRead % size)
	{
		nMembRead++;
	}
	
	return nMembRead;
}

// fseek() replacement
int S_OGG_Callback_seek(void *datasource, ogg_int64_t offset, int whence)
{
	snd_stream_t *stream;
	int retVal = 0;

	// check if input is valid
	if(!datasource)
	{
		errno = EBADF; 
		return -1;
	}

	// snd_stream_t in the generic pointer
	stream = (snd_stream_t *) datasource;

	// we must map the whence to its Q3 counterpart
	switch(whence)
	{
		case SEEK_SET :
		{
			// set the file position in the actual file with the Q3 function
			retVal = FS_Seek(stream->file, (long) offset, FS_SEEK_SET);

			// something has gone wrong, so we return here
			if(!(retVal == 0))
			{
			 return retVal;
			}

			// keep track of file position
			stream->pos = (int) offset;
			break;
		}
  
		case SEEK_CUR :
		{
			// set the file position in the actual file with the Q3 function
			retVal = FS_Seek(stream->file, (long) offset, FS_SEEK_CUR);

			// something has gone wrong, so we return here
			if(!(retVal == 0))
			{
			 return retVal;
			}

			// keep track of file position
			stream->pos += (int) offset;
			break;
		}
 
		case SEEK_END :
		{
			// Quake 3 seems to have trouble with FS_SEEK_END 
			// so we use the file length and FS_SEEK_SET

			// set the file position in the actual file with the Q3 function
			retVal = FS_Seek(stream->file, (long) stream->length + (long) offset, FS_SEEK_SET);

			// something has gone wrong, so we return here
			if(!(retVal == 0))
			{
			 return retVal;
			}

			// keep track of file position
			stream->pos = stream->length + (int) offset;
			break;
		}
  
		default :
		{
			// unknown whence, so we return an error
			errno = EINVAL;
			return -1;
		}
	}

	// stream->pos shouldn't be smaller than zero or bigger than the filesize
	stream->pos = (stream->pos < 0) ? 0 : stream->pos;
	stream->pos = (stream->pos > stream->length) ? stream->length : stream->pos;

	return 0;
}

// fclose() replacement
int S_OGG_Callback_close(void *datasource)
{
	// we do nothing here and close all things manually in S_OGG_CodecCloseStream()
	return 0;
}

// ftell() replacement
long S_OGG_Callback_tell(void *datasource)
{
	// check if input is valid
	if(!datasource)
	{
		errno = EBADF; 
		return -1;
	}

	// we keep track of the file position in stream->pos
	return (long) (((snd_stream_t *) datasource) -> pos);
}

// the callback structure
const ov_callbacks S_OGG_Callbacks =
{
 &S_OGG_Callback_read,
 &S_OGG_Callback_seek,
 &S_OGG_Callback_close,
 &S_OGG_Callback_tell
};

/*
=================
S_OGG_CodecOpenStream
=================
*/
snd_stream_t *S_OGG_CodecOpenStream(const char *filename)
{
	snd_stream_t *stream;

	// OGG codec control structure
	OggVorbis_File *vf;

	// some variables used to get informations about the OGG 
	vorbis_info *OGGInfo;
	ogg_int64_t numSamples;

	// check if input is valid
	if(!filename)
	{
		return NULL;
	}

	// Open the stream
	stream = S_CodecUtilOpen(filename, &ogg_codec);
	if(!stream)
	{
		return NULL;
	}

	// alloctate the OggVorbis_File
	vf = Z_Malloc(sizeof(OggVorbis_File));
	if(!vf)
	{
		S_CodecUtilClose(stream);

		return NULL;
	}

	// open the codec with our callbacks and stream as the generic pointer
	if(ov_open_callbacks(stream, vf, NULL, 0, S_OGG_Callbacks) != 0)
	{
		Z_Free(vf);

		S_CodecUtilClose(stream);

		return NULL;
	}

	// the stream must be seekable
	if(!ov_seekable(vf))
	{
		ov_clear(vf);

		Z_Free(vf);

		S_CodecUtilClose(stream);

		return NULL;
	}
 
	// we only support OGGs with one substream
	if(ov_streams(vf) != 1)
	{
		ov_clear(vf);

		Z_Free(vf);

		S_CodecUtilClose(stream);

		return NULL;  
	}

	// get the info about channels and rate
	OGGInfo = ov_info(vf, 0);
	if(!OGGInfo)
	{
		ov_clear(vf);

		Z_Free(vf);

		S_CodecUtilClose(stream);

		return NULL;  
	}

	// get the number of sample-frames in the OGG
	numSamples = ov_pcm_total(vf, 0);

	// fill in the info-structure in the stream
	stream->info.rate = OGGInfo->rate;
	stream->info.width = OGG_SAMPLEWIDTH;
	stream->info.channels = OGGInfo->channels;
	stream->info.samples = numSamples;
	stream->info.size = stream->info.samples * stream->info.channels * stream->info.width;
	stream->info.dataofs = 0;

	// We use stream->pos for the file pointer in the compressed ogg file 
	stream->pos = 0;
	
	// We use the generic pointer in stream for the OGG codec control structure
	stream->ptr = vf;

	return stream;
}

/*
=================
S_OGG_CodecCloseStream
=================
*/
void S_OGG_CodecCloseStream(snd_stream_t *stream)
{
	// check if input is valid
	if(!stream)
	{
		return;
	}
	
	// let the OGG codec cleanup its stuff
	ov_clear((OggVorbis_File *) stream->ptr);

	// free the OGG codec control struct
	Z_Free(stream->ptr);

	// close the stream
	S_CodecUtilClose(stream);
}

/*
=================
S_OGG_CodecReadStream
=================
*/
int S_OGG_CodecReadStream(snd_stream_t *stream, int bytes, void *buffer)
{
	// buffer handling
	int bytesRead, bytesLeft, c;
	char *bufPtr;
	
	// Bitstream for the decoder
	int BS = 0;

	// big endian machines want their samples in big endian order
	int IsBigEndian = 0;

#	ifdef Q3_BIG_ENDIAN
	IsBigEndian = 1;
#	endif // Q3_BIG_ENDIAN

	// check if input is valid
	if(!(stream && buffer))
	{
		return 0;
	}

	if(bytes <= 0)
	{
		return 0;
	}

	bytesRead = 0;
	bytesLeft = bytes;
	bufPtr = buffer;

	// cycle until we have the requested or all available bytes read
	while(-1)
	{
		// read some bytes from the OGG codec
		c = ov_read((OggVorbis_File *) stream->ptr, bufPtr, bytesLeft, IsBigEndian, OGG_SAMPLEWIDTH, 1, &BS);
		
		// no more bytes are left
		if(c <= 0)
		{
			break;
		}

		bytesRead += c;
		bytesLeft -= c;
		bufPtr += c;
  
		// we have enough bytes
		if(bytesLeft <= 0)
		{
			break;
		}
	}

	return bytesRead;
}

/*
=====================================================================
S_OGG_CodecLoad

We handle S_OGG_CodecLoad as a special case of the streaming functions 
where we read the whole stream at once.
======================================================================
*/
void *S_OGG_CodecLoad(const char *filename, snd_info_t *info)
{
	snd_stream_t *stream;
	byte *buffer;
	int bytesRead;
	
	// check if input is valid
	if(!(filename && info))
	{
		return NULL;
	}
	
	// open the file as a stream
	stream = S_OGG_CodecOpenStream(filename);
	if(!stream)
	{
		return NULL;
	}
	
	// copy over the info
	info->rate = stream->info.rate;
	info->width = stream->info.width;
	info->channels = stream->info.channels;
	info->samples = stream->info.samples;
	info->size = stream->info.size;
	info->dataofs = stream->info.dataofs;

	// allocate a buffer
	// this buffer must be free-ed by the caller of this function
    	buffer = Z_Malloc(info->size);
	if(!buffer)
	{
		S_OGG_CodecCloseStream(stream);
	
		return NULL;	
	}

	// fill the buffer
	bytesRead = S_OGG_CodecReadStream(stream, info->size, buffer);
	
	// we don't even have read a single byte
	if(bytesRead <= 0)
	{
		Z_Free(buffer);
		S_OGG_CodecCloseStream(stream);

		return NULL;	
	}

	S_OGG_CodecCloseStream(stream);
	
	return buffer;
}

#endif // USE_CODEC_VORBIS
