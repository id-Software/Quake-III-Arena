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
// snd_mix.c -- portable code to mix sounds for snd_dma.c

#include "client.h"
#include "snd_local.h"
#if idppc_altivec && !defined(MACOS_X)
#include <altivec.h>
#endif

static portable_samplepair_t paintbuffer[PAINTBUFFER_SIZE];
static int snd_vol;

// bk001119 - these not static, required by unix/snd_mixa.s
int*     snd_p;  
int      snd_linear_count;
short*   snd_out;

#if	!id386                                        // if configured not to use asm

void S_WriteLinearBlastStereo16 (void)
{
	int		i;
	int		val;

	for (i=0 ; i<snd_linear_count ; i+=2)
	{
		val = snd_p[i]>>8;
		if (val > 0x7fff)
			snd_out[i] = 0x7fff;
		else if (val < -32768)
			snd_out[i] = -32768;
		else
			snd_out[i] = val;

		val = snd_p[i+1]>>8;
		if (val > 0x7fff)
			snd_out[i+1] = 0x7fff;
		else if (val < -32768)
			snd_out[i+1] = -32768;
		else
			snd_out[i+1] = val;
	}
}
#elif defined(__GNUC__)
// uses snd_mixa.s
void S_WriteLinearBlastStereo16 (void);
#else

__declspec( naked ) void S_WriteLinearBlastStereo16 (void)
{
	__asm {

 push edi
 push ebx
 mov ecx,ds:dword ptr[snd_linear_count]
 mov ebx,ds:dword ptr[snd_p]
 mov edi,ds:dword ptr[snd_out]
LWLBLoopTop:
 mov eax,ds:dword ptr[-8+ebx+ecx*4]
 sar eax,8
 cmp eax,07FFFh
 jg LClampHigh
 cmp eax,0FFFF8000h
 jnl LClampDone
 mov eax,0FFFF8000h
 jmp LClampDone
LClampHigh:
 mov eax,07FFFh
LClampDone:
 mov edx,ds:dword ptr[-4+ebx+ecx*4]
 sar edx,8
 cmp edx,07FFFh
 jg LClampHigh2
 cmp edx,0FFFF8000h
 jnl LClampDone2
 mov edx,0FFFF8000h
 jmp LClampDone2
LClampHigh2:
 mov edx,07FFFh
LClampDone2:
 shl edx,16
 and eax,0FFFFh
 or edx,eax
 mov ds:dword ptr[-4+edi+ecx*2],edx
 sub ecx,2
 jnz LWLBLoopTop
 pop ebx
 pop edi
 ret
	}
}

#endif

void S_TransferStereo16 (unsigned long *pbuf, int endtime)
{
	int		lpos;
	int		ls_paintedtime;
	
	snd_p = (int *) paintbuffer;
	ls_paintedtime = s_paintedtime;

	while (ls_paintedtime < endtime)
	{
	// handle recirculating buffer issues
		lpos = ls_paintedtime & ((dma.samples>>1)-1);

		snd_out = (short *) pbuf + (lpos<<1);

		snd_linear_count = (dma.samples>>1) - lpos;
		if (ls_paintedtime + snd_linear_count > endtime)
			snd_linear_count = endtime - ls_paintedtime;

		snd_linear_count <<= 1;

	// write a linear blast of samples
		S_WriteLinearBlastStereo16 ();

		snd_p += snd_linear_count;
		ls_paintedtime += (snd_linear_count>>1);

		if( CL_VideoRecording( ) )
			CL_WriteAVIAudioFrame( (byte *)snd_out, snd_linear_count << 1 );
	}
}

/*
===================
S_TransferPaintBuffer

===================
*/
void S_TransferPaintBuffer(int endtime)
{
	int 	out_idx;
	int 	count;
	int 	out_mask;
	int 	*p;
	int 	step;
	int		val;
	unsigned long *pbuf;

	pbuf = (unsigned long *)dma.buffer;


	if ( s_testsound->integer ) {
		int		i;
		int		count;

		// write a fixed sine wave
		count = (endtime - s_paintedtime);
		for (i=0 ; i<count ; i++)
			paintbuffer[i].left = paintbuffer[i].right = sin((s_paintedtime+i)*0.1)*20000*256;
	}


	if (dma.samplebits == 16 && dma.channels == 2)
	{	// optimized case
		S_TransferStereo16 (pbuf, endtime);
	}
	else
	{	// general case
		p = (int *) paintbuffer;
		count = (endtime - s_paintedtime) * dma.channels;
		out_mask = dma.samples - 1; 
		out_idx = s_paintedtime * dma.channels & out_mask;
		step = 3 - dma.channels;

		if (dma.samplebits == 16)
		{
			short *out = (short *) pbuf;
			while (count--)
			{
				val = *p >> 8;
				p+= step;
				if (val > 0x7fff)
					val = 0x7fff;
				else if (val < -32768)
					val = -32768;
				out[out_idx] = val;
				out_idx = (out_idx + 1) & out_mask;
			}
		}
		else if (dma.samplebits == 8)
		{
			unsigned char *out = (unsigned char *) pbuf;
			while (count--)
			{
				val = *p >> 8;
				p+= step;
				if (val > 0x7fff)
					val = 0x7fff;
				else if (val < -32768)
					val = -32768;
				out[out_idx] = (val>>8) + 128;
				out_idx = (out_idx + 1) & out_mask;
			}
		}
	}
}


/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/

#if idppc_altivec
static void S_PaintChannelFrom16_altivec( channel_t *ch, const sfx_t *sc, int count, int sampleOffset, int bufferOffset ) {
	int						data, aoff, boff;
	int						leftvol, rightvol;
	int						i, j;
	portable_samplepair_t	*samp;
	sndBuffer				*chunk;
	short					*samples;
	float					ooff, fdata, fdiv, fleftvol, frightvol;

	samp = &paintbuffer[ bufferOffset ];

	if (ch->doppler) {
		sampleOffset = sampleOffset*ch->oldDopplerScale;
	}

	chunk = sc->soundData;
	while (sampleOffset>=SND_CHUNK_SIZE) {
		chunk = chunk->next;
		sampleOffset -= SND_CHUNK_SIZE;
		if (!chunk) {
			chunk = sc->soundData;
		}
	}

	if (!ch->doppler || ch->dopplerScale==1.0f) {
		vector signed short volume_vec;
		vector unsigned int volume_shift;
		int vectorCount, samplesLeft, chunkSamplesLeft;
		leftvol = ch->leftvol*snd_vol;
		rightvol = ch->rightvol*snd_vol;
		samples = chunk->sndChunk;
		((short *)&volume_vec)[0] = leftvol;
		((short *)&volume_vec)[1] = leftvol;
		((short *)&volume_vec)[4] = leftvol;
		((short *)&volume_vec)[5] = leftvol;
		((short *)&volume_vec)[2] = rightvol;
		((short *)&volume_vec)[3] = rightvol;
		((short *)&volume_vec)[6] = rightvol;
		((short *)&volume_vec)[7] = rightvol;
		volume_shift = vec_splat_u32(8);
		i = 0;

		while(i < count) {
			/* Try to align destination to 16-byte boundary */
			while(i < count && (((unsigned long)&samp[i] & 0x1f) || ((count-i) < 8) || ((SND_CHUNK_SIZE - sampleOffset) < 8))) {
				data  = samples[sampleOffset++];
				samp[i].left += (data * leftvol)>>8;
				samp[i].right += (data * rightvol)>>8;
	
				if (sampleOffset == SND_CHUNK_SIZE) {
					chunk = chunk->next;
					samples = chunk->sndChunk;
					sampleOffset = 0;
				}
				i++;
			}
			/* Destination is now aligned.  Process as many 8-sample 
			   chunks as we can before we run out of room from the current
			   sound chunk.  We do 8 per loop to avoid extra source data reads. */
			samplesLeft = count - i;
			chunkSamplesLeft = SND_CHUNK_SIZE - sampleOffset;
			if(samplesLeft > chunkSamplesLeft)
				samplesLeft = chunkSamplesLeft;
			
			vectorCount = samplesLeft / 8;
			
			if(vectorCount)
			{
				vector unsigned char tmp;
				vector short s0, s1, sampleData0, sampleData1;
				vector signed int merge0, merge1;
				vector signed int d0, d1, d2, d3;				
				vector unsigned char samplePermute0 =
					VECCONST_UINT8(0, 1, 4, 5, 0, 1, 4, 5, 2, 3, 6, 7, 2, 3, 6, 7);
				vector unsigned char samplePermute1 = 
					VECCONST_UINT8(8, 9, 12, 13, 8, 9, 12, 13, 10, 11, 14, 15, 10, 11, 14, 15);
				vector unsigned char loadPermute0, loadPermute1;
				
				// Rather than permute the vectors after we load them to do the sample
				// replication and rearrangement, we permute the alignment vector so
				// we do everything in one step below and avoid data shuffling.
				tmp = vec_lvsl(0,&samples[sampleOffset]);								
				loadPermute0 = vec_perm(tmp,tmp,samplePermute0);
				loadPermute1 = vec_perm(tmp,tmp,samplePermute1);
				
				s0 = *(vector short *)&samples[sampleOffset];
				while(vectorCount)
				{
					/* Load up source (16-bit) sample data */
					s1 = *(vector short *)&samples[sampleOffset+7];
					
					/* Load up destination sample data */
					d0 = *(vector signed int *)&samp[i];
					d1 = *(vector signed int *)&samp[i+2];
					d2 = *(vector signed int *)&samp[i+4];
					d3 = *(vector signed int *)&samp[i+6];

					sampleData0 = vec_perm(s0,s1,loadPermute0);
					sampleData1 = vec_perm(s0,s1,loadPermute1);
					
					merge0 = vec_mule(sampleData0,volume_vec);
					merge0 = vec_sra(merge0,volume_shift);	/* Shift down to proper range */
					
					merge1 = vec_mulo(sampleData0,volume_vec);
					merge1 = vec_sra(merge1,volume_shift);
					
					d0 = vec_add(merge0,d0);
					d1 = vec_add(merge1,d1);
					
					merge0 = vec_mule(sampleData1,volume_vec);
					merge0 = vec_sra(merge0,volume_shift);	/* Shift down to proper range */
					
					merge1 = vec_mulo(sampleData1,volume_vec);
					merge1 = vec_sra(merge1,volume_shift);					

					d2 = vec_add(merge0,d2);
					d3 = vec_add(merge1,d3);

					/* Store destination sample data */
					*(vector signed int *)&samp[i] = d0;
					*(vector signed int *)&samp[i+2] = d1;
					*(vector signed int *)&samp[i+4] = d2;
					*(vector signed int *)&samp[i+6] = d3;

					i += 8;
					vectorCount--;
					s0 = s1;
					sampleOffset += 8;
				}
				if (sampleOffset == SND_CHUNK_SIZE) {
					chunk = chunk->next;
					samples = chunk->sndChunk;
					sampleOffset = 0;
				}
			}
		}
	} else {
		fleftvol = ch->leftvol*snd_vol;
		frightvol = ch->rightvol*snd_vol;

		ooff = sampleOffset;
		samples = chunk->sndChunk;
		
		for ( i=0 ; i<count ; i++ ) {

			aoff = ooff;
			ooff = ooff + ch->dopplerScale;
			boff = ooff;
			fdata = 0;
			for (j=aoff; j<boff; j++) {
				if (j == SND_CHUNK_SIZE) {
					chunk = chunk->next;
					if (!chunk) {
						chunk = sc->soundData;
					}
					samples = chunk->sndChunk;
					ooff -= SND_CHUNK_SIZE;
				}
				fdata  += samples[j&(SND_CHUNK_SIZE-1)];
			}
			fdiv = 256 * (boff-aoff);
			samp[i].left += (fdata * fleftvol)/fdiv;
			samp[i].right += (fdata * frightvol)/fdiv;
		}
	}
}
#endif

static void S_PaintChannelFrom16_scalar( channel_t *ch, const sfx_t *sc, int count, int sampleOffset, int bufferOffset ) {
	int						data, aoff, boff;
	int						leftvol, rightvol;
	int						i, j;
	portable_samplepair_t	*samp;
	sndBuffer				*chunk;
	short					*samples;
	float					ooff, fdata, fdiv, fleftvol, frightvol;

	samp = &paintbuffer[ bufferOffset ];

	if (ch->doppler) {
		sampleOffset = sampleOffset*ch->oldDopplerScale;
	}

	chunk = sc->soundData;
	while (sampleOffset>=SND_CHUNK_SIZE) {
		chunk = chunk->next;
		sampleOffset -= SND_CHUNK_SIZE;
		if (!chunk) {
			chunk = sc->soundData;
		}
	}

	if (!ch->doppler || ch->dopplerScale==1.0f) {
		leftvol = ch->leftvol*snd_vol;
		rightvol = ch->rightvol*snd_vol;
		samples = chunk->sndChunk;
		for ( i=0 ; i<count ; i++ ) {
			data  = samples[sampleOffset++];
			samp[i].left += (data * leftvol)>>8;
			samp[i].right += (data * rightvol)>>8;

			if (sampleOffset == SND_CHUNK_SIZE) {
				chunk = chunk->next;
				samples = chunk->sndChunk;
				sampleOffset = 0;
			}
		}
	} else {
		fleftvol = ch->leftvol*snd_vol;
		frightvol = ch->rightvol*snd_vol;

		ooff = sampleOffset;
		samples = chunk->sndChunk;
		



		for ( i=0 ; i<count ; i++ ) {

			aoff = ooff;
			ooff = ooff + ch->dopplerScale;
			boff = ooff;
			fdata = 0;
			for (j=aoff; j<boff; j++) {
				if (j == SND_CHUNK_SIZE) {
					chunk = chunk->next;
					if (!chunk) {
						chunk = sc->soundData;
					}
					samples = chunk->sndChunk;
					ooff -= SND_CHUNK_SIZE;
				}
				fdata  += samples[j&(SND_CHUNK_SIZE-1)];
			}
			fdiv = 256 * (boff-aoff);
			samp[i].left += (fdata * fleftvol)/fdiv;
			samp[i].right += (fdata * frightvol)/fdiv;
		}
	}
}

static void S_PaintChannelFrom16( channel_t *ch, const sfx_t *sc, int count, int sampleOffset, int bufferOffset ) {
#if idppc_altivec
	if (com_altivec->integer) {
		// must be in a seperate function or G3 systems will crash.
		S_PaintChannelFrom16_altivec( ch, sc, count, sampleOffset, bufferOffset );
		return;
	}
#endif
	S_PaintChannelFrom16_scalar( ch, sc, count, sampleOffset, bufferOffset );
}

void S_PaintChannelFromWavelet( channel_t *ch, sfx_t *sc, int count, int sampleOffset, int bufferOffset ) {
	int						data;
	int						leftvol, rightvol;
	int						i;
	portable_samplepair_t	*samp;
	sndBuffer				*chunk;
	short					*samples;

	leftvol = ch->leftvol*snd_vol;
	rightvol = ch->rightvol*snd_vol;

	i = 0;
	samp = &paintbuffer[ bufferOffset ];
	chunk = sc->soundData;
	while (sampleOffset>=(SND_CHUNK_SIZE_FLOAT*4)) {
		chunk = chunk->next;
		sampleOffset -= (SND_CHUNK_SIZE_FLOAT*4);
		i++;
	}

	if (i!=sfxScratchIndex || sfxScratchPointer != sc) {
		S_AdpcmGetSamples( chunk, sfxScratchBuffer );
		sfxScratchIndex = i;
		sfxScratchPointer = sc;
	}

	samples = sfxScratchBuffer;

	for ( i=0 ; i<count ; i++ ) {
		data  = samples[sampleOffset++];
		samp[i].left += (data * leftvol)>>8;
		samp[i].right += (data * rightvol)>>8;

		if (sampleOffset == SND_CHUNK_SIZE*2) {
			chunk = chunk->next;
			decodeWavelet(chunk, sfxScratchBuffer);
			sfxScratchIndex++;
			sampleOffset = 0;
		}
	}
}

void S_PaintChannelFromADPCM( channel_t *ch, sfx_t *sc, int count, int sampleOffset, int bufferOffset ) {
	int						data;
	int						leftvol, rightvol;
	int						i;
	portable_samplepair_t	*samp;
	sndBuffer				*chunk;
	short					*samples;

	leftvol = ch->leftvol*snd_vol;
	rightvol = ch->rightvol*snd_vol;

	i = 0;
	samp = &paintbuffer[ bufferOffset ];
	chunk = sc->soundData;

	if (ch->doppler) {
		sampleOffset = sampleOffset*ch->oldDopplerScale;
	}

	while (sampleOffset>=(SND_CHUNK_SIZE*4)) {
		chunk = chunk->next;
		sampleOffset -= (SND_CHUNK_SIZE*4);
		i++;
	}

	if (i!=sfxScratchIndex || sfxScratchPointer != sc) {
		S_AdpcmGetSamples( chunk, sfxScratchBuffer );
		sfxScratchIndex = i;
		sfxScratchPointer = sc;
	}

	samples = sfxScratchBuffer;

	for ( i=0 ; i<count ; i++ ) {
		data  = samples[sampleOffset++];
		samp[i].left += (data * leftvol)>>8;
		samp[i].right += (data * rightvol)>>8;

		if (sampleOffset == SND_CHUNK_SIZE*4) {
			chunk = chunk->next;
			S_AdpcmGetSamples( chunk, sfxScratchBuffer);
			sampleOffset = 0;
			sfxScratchIndex++;
		}
	}
}

void S_PaintChannelFromMuLaw( channel_t *ch, sfx_t *sc, int count, int sampleOffset, int bufferOffset ) {
	int						data;
	int						leftvol, rightvol;
	int						i;
	portable_samplepair_t	*samp;
	sndBuffer				*chunk;
	byte					*samples;
	float					ooff;

	leftvol = ch->leftvol*snd_vol;
	rightvol = ch->rightvol*snd_vol;

	samp = &paintbuffer[ bufferOffset ];
	chunk = sc->soundData;
	while (sampleOffset>=(SND_CHUNK_SIZE*2)) {
		chunk = chunk->next;
		sampleOffset -= (SND_CHUNK_SIZE*2);
		if (!chunk) {
			chunk = sc->soundData;
		}
	}

	if (!ch->doppler) {
		samples = (byte *)chunk->sndChunk + sampleOffset;
		for ( i=0 ; i<count ; i++ ) {
			data  = mulawToShort[*samples];
			samp[i].left += (data * leftvol)>>8;
			samp[i].right += (data * rightvol)>>8;
			samples++;
			if (samples == (byte *)chunk->sndChunk+(SND_CHUNK_SIZE*2)) {
				chunk = chunk->next;
				samples = (byte *)chunk->sndChunk;
			}
		}
	} else {
		ooff = sampleOffset;
		samples = (byte *)chunk->sndChunk;
		for ( i=0 ; i<count ; i++ ) {
			data  = mulawToShort[samples[(int)(ooff)]];
			ooff = ooff + ch->dopplerScale;
			samp[i].left += (data * leftvol)>>8;
			samp[i].right += (data * rightvol)>>8;
			if (ooff >= SND_CHUNK_SIZE*2) {
				chunk = chunk->next;
				if (!chunk) {
					chunk = sc->soundData;
				}
				samples = (byte *)chunk->sndChunk;
				ooff = 0.0;
			}
		}
	}
}

/*
===================
S_PaintChannels
===================
*/
void S_PaintChannels( int endtime ) {
	int 	i;
	int 	end;
	channel_t *ch;
	sfx_t	*sc;
	int		ltime, count;
	int		sampleOffset;


	snd_vol = s_volume->value*255;

//Com_Printf ("%i to %i\n", s_paintedtime, endtime);
	while ( s_paintedtime < endtime ) {
		// if paintbuffer is smaller than DMA buffer
		// we may need to fill it multiple times
		end = endtime;
		if ( endtime - s_paintedtime > PAINTBUFFER_SIZE ) {
			end = s_paintedtime + PAINTBUFFER_SIZE;
		}

		// clear the paint buffer to either music or zeros
		if ( s_rawend < s_paintedtime ) {
			if ( s_rawend ) {
				//Com_DPrintf ("background sound underrun\n");
			}
			Com_Memset(paintbuffer, 0, (end - s_paintedtime) * sizeof(portable_samplepair_t));
		} else {
			// copy from the streaming sound source
			int		s;
			int		stop;

			stop = (end < s_rawend) ? end : s_rawend;

			for ( i = s_paintedtime ; i < stop ; i++ ) {
				s = i&(MAX_RAW_SAMPLES-1);
				paintbuffer[i-s_paintedtime] = s_rawsamples[s];
			}
//		if (i != end)
//			Com_Printf ("partial stream\n");
//		else
//			Com_Printf ("full stream\n");
			for ( ; i < end ; i++ ) {
				paintbuffer[i-s_paintedtime].left =
				paintbuffer[i-s_paintedtime].right = 0;
			}
		}

		// paint in the channels.
		ch = s_channels;
		for ( i = 0; i < MAX_CHANNELS ; i++, ch++ ) {		
			if ( !ch->thesfx || (ch->leftvol<0.25 && ch->rightvol<0.25 )) {
				continue;
			}

			ltime = s_paintedtime;
			sc = ch->thesfx;

			sampleOffset = ltime - ch->startSample;
			count = end - ltime;
			if ( sampleOffset + count > sc->soundLength ) {
				count = sc->soundLength - sampleOffset;
			}

			if ( count > 0 ) {	
				if( sc->soundCompressionMethod == 1) {
					S_PaintChannelFromADPCM		(ch, sc, count, sampleOffset, ltime - s_paintedtime);
				} else if( sc->soundCompressionMethod == 2) {
					S_PaintChannelFromWavelet	(ch, sc, count, sampleOffset, ltime - s_paintedtime);
				} else if( sc->soundCompressionMethod == 3) {
					S_PaintChannelFromMuLaw	(ch, sc, count, sampleOffset, ltime - s_paintedtime);
				} else {
					S_PaintChannelFrom16		(ch, sc, count, sampleOffset, ltime - s_paintedtime);
				}
			}
		}

		// paint in the looped channels.
		ch = loop_channels;
		for ( i = 0; i < numLoopChannels ; i++, ch++ ) {		
			if ( !ch->thesfx || (!ch->leftvol && !ch->rightvol )) {
				continue;
			}

			ltime = s_paintedtime;
			sc = ch->thesfx;

			if (sc->soundData==NULL || sc->soundLength==0) {
				continue;
			}
			// we might have to make two passes if it
			// is a looping sound effect and the end of
			// the sample is hit
			do {
				sampleOffset = (ltime % sc->soundLength);

				count = end - ltime;
				if ( sampleOffset + count > sc->soundLength ) {
					count = sc->soundLength - sampleOffset;
				}

				if ( count > 0 ) {	
					if( sc->soundCompressionMethod == 1) {
						S_PaintChannelFromADPCM		(ch, sc, count, sampleOffset, ltime - s_paintedtime);
					} else if( sc->soundCompressionMethod == 2) {
						S_PaintChannelFromWavelet	(ch, sc, count, sampleOffset, ltime - s_paintedtime);
					} else if( sc->soundCompressionMethod == 3) {
						S_PaintChannelFromMuLaw		(ch, sc, count, sampleOffset, ltime - s_paintedtime);
					} else {
						S_PaintChannelFrom16		(ch, sc, count, sampleOffset, ltime - s_paintedtime);
					}
					ltime += count;
				}
			} while ( ltime < end);
		}

		// transfer out according to DMA format
		S_TransferPaintBuffer( end );
		s_paintedtime = end;
	}
}
