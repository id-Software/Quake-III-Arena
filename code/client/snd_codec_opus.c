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

// Ogg Opus support is enabled by this define
#ifdef USE_CODEC_OPUS

// includes for the Q3 sound system
#include "client.h"
#include "snd_codec.h"

// includes for the Ogg Opus codec
#include <errno.h>
#include <opusfile.h>

// samples are 16 bit
#define OPUS_SAMPLEWIDTH 2

// Q3 Ogg Opus codec
snd_codec_t opus_codec =
{
	"opus",
	S_OggOpus_CodecLoad,
	S_OggOpus_CodecOpenStream,
	S_OggOpus_CodecReadStream,
	S_OggOpus_CodecCloseStream,
	NULL
};

// callbacks for opusfile

// fread() replacement
int S_OggOpus_Callback_read(void *datasource, unsigned char *ptr, int size )
{
	snd_stream_t *stream;
	int bytesRead = 0;

	// check if input is valid
	if(!ptr)
	{
		errno = EFAULT;
		return -1;
	}
	
	if(!size)
	{
		// It's not an error, caller just wants zero bytes!
		errno = 0;
		return 0;
	}
 
	if (size < 0)
	{
		errno = EINVAL;
		return -1;
	}

	if(!datasource)
	{
		errno = EBADF;
		return -1;
	}
	
	// we use a snd_stream_t in the generic pointer to pass around
	stream = (snd_stream_t *) datasource;

	// read it with the Q3 function FS_Read()
	bytesRead = FS_Read(ptr, size, stream->file);

	// update the file position
	stream->pos += bytesRead;

	return bytesRead;
}

// fseek() replacement
int S_OggOpus_Callback_seek(void *datasource, opus_int64 offset, int whence)
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
			if(retVal < 0)
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
			if(retVal < 0)
			{
			 return retVal;
			}

			// keep track of file position
			stream->pos += (int) offset;
			break;
		}
 
		case SEEK_END :
		{
			// set the file position in the actual file with the Q3 function
			retVal = FS_Seek(stream->file, (long) offset, FS_SEEK_END);

			// something has gone wrong, so we return here
			if(retVal < 0)
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
int S_OggOpus_Callback_close(void *datasource)
{
	// we do nothing here and close all things manually in S_OggOpus_CodecCloseStream()
	return 0;
}

// ftell() replacement
opus_int64 S_OggOpus_Callback_tell(void *datasource)
{
	snd_stream_t   *stream;

	// check if input is valid
	if(!datasource)
	{
		errno = EBADF;
		return -1;
	}

	// snd_stream_t in the generic pointer
	stream = (snd_stream_t *) datasource;

	return (opus_int64) FS_FTell(stream->file);
}

// the callback structure
const OpusFileCallbacks S_OggOpus_Callbacks =
{
 &S_OggOpus_Callback_read,
 &S_OggOpus_Callback_seek,
 &S_OggOpus_Callback_tell,
 &S_OggOpus_Callback_close
};

/*
=================
S_OggOpus_CodecOpenStream
=================
*/
snd_stream_t *S_OggOpus_CodecOpenStream(const char *filename)
{
	snd_stream_t *stream;

	// Opus codec control structure
	OggOpusFile *of;

	// some variables used to get informations about the file
	const OpusHead *opusInfo;
	ogg_int64_t numSamples;

	// check if input is valid
	if(!filename)
	{
		return NULL;
	}

	// Open the stream
	stream = S_CodecUtilOpen(filename, &opus_codec);
	if(!stream)
	{
		return NULL;
	}

	// open the codec with our callbacks and stream as the generic pointer
	of = op_open_callbacks(stream, &S_OggOpus_Callbacks, NULL, 0, NULL );
	if (!of)
	{
		S_CodecUtilClose(&stream);

		return NULL;
	}

	// the stream must be seekable
	if(!op_seekable(of))
	{
		op_free(of);

		S_CodecUtilClose(&stream);

		return NULL;
	}

	// get the info about channels and rate
	opusInfo = op_head(of, -1);
	if(!opusInfo)
	{
		op_free(of);

		S_CodecUtilClose(&stream);

		return NULL;
	}

	if(opusInfo->stream_count != 1)
	{
		op_free(of);

		S_CodecUtilClose(&stream);

		Com_Printf("Only Ogg Opus files with one stream are support\n");
		return NULL;
	}

	if(opusInfo->channel_count != 1 && opusInfo->channel_count != 2)
	{
		op_free(of);

		S_CodecUtilClose(&stream);

		Com_Printf("Only mono and stereo Ogg Opus files are supported\n");
		return NULL;
	}

	// get the number of sample-frames in the file
	numSamples = op_pcm_total(of, -1);

	// fill in the info-structure in the stream
	stream->info.rate = 48000;
	stream->info.width = OPUS_SAMPLEWIDTH;
	stream->info.channels = opusInfo->channel_count;
	stream->info.samples = numSamples;
	stream->info.size = stream->info.samples * stream->info.channels * stream->info.width;
	stream->info.dataofs = 0;

	// We use stream->pos for the file pointer in the compressed ogg file
	stream->pos = 0;
	
	// We use the generic pointer in stream for the opus codec control structure
	stream->ptr = of;

	return stream;
}

/*
=================
S_OggOpus_CodecCloseStream
=================
*/
void S_OggOpus_CodecCloseStream(snd_stream_t *stream)
{
	// check if input is valid
	if(!stream)
	{
		return;
	}
	
	// let the opus codec cleanup its stuff
	op_free((OggOpusFile *) stream->ptr);

	// close the stream
	S_CodecUtilClose(&stream);
}

/*
=================
S_OggOpus_CodecReadStream
=================
*/
int S_OggOpus_CodecReadStream(snd_stream_t *stream, int bytes, void *buffer)
{
	// buffer handling
	int samplesRead, samplesLeft, c;
	opus_int16 *bufPtr;

	// check if input is valid
	if(!(stream && buffer))
	{
		return 0;
	}

	if(bytes <= 0)
	{
		return 0;
	}

	samplesRead = 0;
	samplesLeft = bytes / stream->info.channels / stream->info.width;
	bufPtr = buffer;

	if(samplesLeft <= 0)
	{
		return 0;
	}

	// cycle until we have the requested or all available bytes read
	while(-1)
	{
		// read some samples from the opus codec
		c = op_read((OggOpusFile *) stream->ptr, bufPtr + samplesRead * stream->info.channels, samplesLeft * stream->info.channels, NULL);
		
		// no more samples are left
		if(c <= 0)
		{
			break;
		}

		samplesRead += c;
		samplesLeft -= c;
  
		// we have enough samples
		if(samplesLeft <= 0)
		{
			break;
		}
	}

	return samplesRead * stream->info.channels * stream->info.width;
}

/*
=====================================================================
S_OggOpus_CodecLoad

We handle S_OggOpus_CodecLoad as a special case of the streaming functions 
where we read the whole stream at once.
======================================================================
*/
void *S_OggOpus_CodecLoad(const char *filename, snd_info_t *info)
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
	stream = S_OggOpus_CodecOpenStream(filename);
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
    	buffer = Hunk_AllocateTempMemory(info->size);
	if(!buffer)
	{
		S_OggOpus_CodecCloseStream(stream);
	
		return NULL;	
	}

	// fill the buffer
	bytesRead = S_OggOpus_CodecReadStream(stream, info->size, buffer);
	
	// we don't even have read a single byte
	if(bytesRead <= 0)
	{
		Hunk_FreeTempMemory(buffer);
		S_OggOpus_CodecCloseStream(stream);

		return NULL;	
	}

	S_OggOpus_CodecCloseStream(stream);
	
	return buffer;
}

#endif // USE_CODEC_OPUS
