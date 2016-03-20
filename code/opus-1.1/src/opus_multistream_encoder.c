/* Copyright (c) 2011 Xiph.Org Foundation
   Written by Jean-Marc Valin */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "opus_multistream.h"
#include "opus.h"
#include "opus_private.h"
#include "stack_alloc.h"
#include <stdarg.h>
#include "float_cast.h"
#include "os_support.h"
#include "mathops.h"
#include "mdct.h"
#include "modes.h"
#include "bands.h"
#include "quant_bands.h"

typedef struct {
   int nb_streams;
   int nb_coupled_streams;
   unsigned char mapping[8];
} VorbisLayout;

/* Index is nb_channel-1*/
static const VorbisLayout vorbis_mappings[8] = {
      {1, 0, {0}},                      /* 1: mono */
      {1, 1, {0, 1}},                   /* 2: stereo */
      {2, 1, {0, 2, 1}},                /* 3: 1-d surround */
      {2, 2, {0, 1, 2, 3}},             /* 4: quadraphonic surround */
      {3, 2, {0, 4, 1, 2, 3}},          /* 5: 5-channel surround */
      {4, 2, {0, 4, 1, 2, 3, 5}},       /* 6: 5.1 surround */
      {4, 3, {0, 4, 1, 2, 3, 5, 6}},    /* 7: 6.1 surround */
      {5, 3, {0, 6, 1, 2, 3, 4, 5, 7}}, /* 8: 7.1 surround */
};

typedef void (*opus_copy_channel_in_func)(
  opus_val16 *dst,
  int dst_stride,
  const void *src,
  int src_stride,
  int src_channel,
  int frame_size
);

struct OpusMSEncoder {
   ChannelLayout layout;
   int lfe_stream;
   int application;
   int variable_duration;
   int surround;
   opus_int32 bitrate_bps;
   float subframe_mem[3];
   /* Encoder states go here */
   /* then opus_val32 window_mem[channels*120]; */
   /* then opus_val32 preemph_mem[channels]; */
};

static opus_val32 *ms_get_preemph_mem(OpusMSEncoder *st)
{
   int s;
   char *ptr;
   int coupled_size, mono_size;

   coupled_size = opus_encoder_get_size(2);
   mono_size = opus_encoder_get_size(1);
   ptr = (char*)st + align(sizeof(OpusMSEncoder));
   for (s=0;s<st->layout.nb_streams;s++)
   {
      if (s < st->layout.nb_coupled_streams)
         ptr += align(coupled_size);
      else
         ptr += align(mono_size);
   }
   return (opus_val32*)(ptr+st->layout.nb_channels*120*sizeof(opus_val32));
}

static opus_val32 *ms_get_window_mem(OpusMSEncoder *st)
{
   int s;
   char *ptr;
   int coupled_size, mono_size;

   coupled_size = opus_encoder_get_size(2);
   mono_size = opus_encoder_get_size(1);
   ptr = (char*)st + align(sizeof(OpusMSEncoder));
   for (s=0;s<st->layout.nb_streams;s++)
   {
      if (s < st->layout.nb_coupled_streams)
         ptr += align(coupled_size);
      else
         ptr += align(mono_size);
   }
   return (opus_val32*)ptr;
}

static int validate_encoder_layout(const ChannelLayout *layout)
{
   int s;
   for (s=0;s<layout->nb_streams;s++)
   {
      if (s < layout->nb_coupled_streams)
      {
         if (get_left_channel(layout, s, -1)==-1)
            return 0;
         if (get_right_channel(layout, s, -1)==-1)
            return 0;
      } else {
         if (get_mono_channel(layout, s, -1)==-1)
            return 0;
      }
   }
   return 1;
}

static void channel_pos(int channels, int pos[8])
{
   /* Position in the mix: 0 don't mix, 1: left, 2: center, 3:right */
   if (channels==4)
   {
      pos[0]=1;
      pos[1]=3;
      pos[2]=1;
      pos[3]=3;
   } else if (channels==3||channels==5||channels==6)
   {
      pos[0]=1;
      pos[1]=2;
      pos[2]=3;
      pos[3]=1;
      pos[4]=3;
      pos[5]=0;
   } else if (channels==7)
   {
      pos[0]=1;
      pos[1]=2;
      pos[2]=3;
      pos[3]=1;
      pos[4]=3;
      pos[5]=2;
      pos[6]=0;
   } else if (channels==8)
   {
      pos[0]=1;
      pos[1]=2;
      pos[2]=3;
      pos[3]=1;
      pos[4]=3;
      pos[5]=1;
      pos[6]=3;
      pos[7]=0;
   }
}

#if 1
/* Computes a rough approximation of log2(2^a + 2^b) */
static opus_val16 logSum(opus_val16 a, opus_val16 b)
{
   opus_val16 max;
   opus_val32 diff;
   opus_val16 frac;
   static const opus_val16 diff_table[17] = {
         QCONST16(0.5000000f, DB_SHIFT), QCONST16(0.2924813f, DB_SHIFT), QCONST16(0.1609640f, DB_SHIFT), QCONST16(0.0849625f, DB_SHIFT),
         QCONST16(0.0437314f, DB_SHIFT), QCONST16(0.0221971f, DB_SHIFT), QCONST16(0.0111839f, DB_SHIFT), QCONST16(0.0056136f, DB_SHIFT),
         QCONST16(0.0028123f, DB_SHIFT)
   };
   int low;
   if (a>b)
   {
      max = a;
      diff = SUB32(EXTEND32(a),EXTEND32(b));
   } else {
      max = b;
      diff = SUB32(EXTEND32(b),EXTEND32(a));
   }
   if (diff >= QCONST16(8.f, DB_SHIFT))
      return max;
#ifdef FIXED_POINT
   low = SHR32(diff, DB_SHIFT-1);
   frac = SHL16(diff - SHL16(low, DB_SHIFT-1), 16-DB_SHIFT);
#else
   low = (int)floor(2*diff);
   frac = 2*diff - low;
#endif
   return max + diff_table[low] + MULT16_16_Q15(frac, SUB16(diff_table[low+1], diff_table[low]));
}
#else
opus_val16 logSum(opus_val16 a, opus_val16 b)
{
   return log2(pow(4, a)+ pow(4, b))/2;
}
#endif

void surround_analysis(const CELTMode *celt_mode, const void *pcm, opus_val16 *bandLogE, opus_val32 *mem, opus_val32 *preemph_mem,
      int len, int overlap, int channels, int rate, opus_copy_channel_in_func copy_channel_in
)
{
   int c;
   int i;
   int LM;
   int pos[8] = {0};
   int upsample;
   int frame_size;
   opus_val16 channel_offset;
   opus_val32 bandE[21];
   opus_val16 maskLogE[3][21];
   VARDECL(opus_val32, in);
   VARDECL(opus_val16, x);
   VARDECL(opus_val32, freq);
   SAVE_STACK;

   upsample = resampling_factor(rate);
   frame_size = len*upsample;

   for (LM=0;LM<celt_mode->maxLM;LM++)
      if (celt_mode->shortMdctSize<<LM==frame_size)
         break;

   ALLOC(in, frame_size+overlap, opus_val32);
   ALLOC(x, len, opus_val16);
   ALLOC(freq, frame_size, opus_val32);

   channel_pos(channels, pos);

   for (c=0;c<3;c++)
      for (i=0;i<21;i++)
         maskLogE[c][i] = -QCONST16(28.f, DB_SHIFT);

   for (c=0;c<channels;c++)
   {
      OPUS_COPY(in, mem+c*overlap, overlap);
      (*copy_channel_in)(x, 1, pcm, channels, c, len);
      celt_preemphasis(x, in+overlap, frame_size, 1, upsample, celt_mode->preemph, preemph_mem+c, 0);
      clt_mdct_forward(&celt_mode->mdct, in, freq, celt_mode->window, overlap, celt_mode->maxLM-LM, 1);
      if (upsample != 1)
      {
         int bound = len;
         for (i=0;i<bound;i++)
            freq[i] *= upsample;
         for (;i<frame_size;i++)
            freq[i] = 0;
      }

      compute_band_energies(celt_mode, freq, bandE, 21, 1, 1<<LM);
      amp2Log2(celt_mode, 21, 21, bandE, bandLogE+21*c, 1);
      /* Apply spreading function with -6 dB/band going up and -12 dB/band going down. */
      for (i=1;i<21;i++)
         bandLogE[21*c+i] = MAX16(bandLogE[21*c+i], bandLogE[21*c+i-1]-QCONST16(1.f, DB_SHIFT));
      for (i=19;i>=0;i--)
         bandLogE[21*c+i] = MAX16(bandLogE[21*c+i], bandLogE[21*c+i+1]-QCONST16(2.f, DB_SHIFT));
      if (pos[c]==1)
      {
         for (i=0;i<21;i++)
            maskLogE[0][i] = logSum(maskLogE[0][i], bandLogE[21*c+i]);
      } else if (pos[c]==3)
      {
         for (i=0;i<21;i++)
            maskLogE[2][i] = logSum(maskLogE[2][i], bandLogE[21*c+i]);
      } else if (pos[c]==2)
      {
         for (i=0;i<21;i++)
         {
            maskLogE[0][i] = logSum(maskLogE[0][i], bandLogE[21*c+i]-QCONST16(.5f, DB_SHIFT));
            maskLogE[2][i] = logSum(maskLogE[2][i], bandLogE[21*c+i]-QCONST16(.5f, DB_SHIFT));
         }
      }
#if 0
      for (i=0;i<21;i++)
         printf("%f ", bandLogE[21*c+i]);
      float sum=0;
      for (i=0;i<21;i++)
         sum += bandLogE[21*c+i];
      printf("%f ", sum/21);
#endif
      OPUS_COPY(mem+c*overlap, in+frame_size, overlap);
   }
   for (i=0;i<21;i++)
      maskLogE[1][i] = MIN32(maskLogE[0][i],maskLogE[2][i]);
   channel_offset = HALF16(celt_log2(QCONST32(2.f,14)/(channels-1)));
   for (c=0;c<3;c++)
      for (i=0;i<21;i++)
         maskLogE[c][i] += channel_offset;
#if 0
   for (c=0;c<3;c++)
   {
      for (i=0;i<21;i++)
         printf("%f ", maskLogE[c][i]);
   }
#endif
   for (c=0;c<channels;c++)
   {
      opus_val16 *mask;
      if (pos[c]!=0)
      {
         mask = &maskLogE[pos[c]-1][0];
         for (i=0;i<21;i++)
            bandLogE[21*c+i] = bandLogE[21*c+i] - mask[i];
      } else {
         for (i=0;i<21;i++)
            bandLogE[21*c+i] = 0;
      }
#if 0
      for (i=0;i<21;i++)
         printf("%f ", bandLogE[21*c+i]);
      printf("\n");
#endif
#if 0
      float sum=0;
      for (i=0;i<21;i++)
         sum += bandLogE[21*c+i];
      printf("%f ", sum/(float)QCONST32(21.f, DB_SHIFT));
      printf("\n");
#endif
   }
   RESTORE_STACK;
}

opus_int32 opus_multistream_encoder_get_size(int nb_streams, int nb_coupled_streams)
{
   int coupled_size;
   int mono_size;

   if(nb_streams<1||nb_coupled_streams>nb_streams||nb_coupled_streams<0)return 0;
   coupled_size = opus_encoder_get_size(2);
   mono_size = opus_encoder_get_size(1);
   return align(sizeof(OpusMSEncoder))
        + nb_coupled_streams * align(coupled_size)
        + (nb_streams-nb_coupled_streams) * align(mono_size);
}

opus_int32 opus_multistream_surround_encoder_get_size(int channels, int mapping_family)
{
   int nb_streams;
   int nb_coupled_streams;
   opus_int32 size;

   if (mapping_family==0)
   {
      if (channels==1)
      {
         nb_streams=1;
         nb_coupled_streams=0;
      } else if (channels==2)
      {
         nb_streams=1;
         nb_coupled_streams=1;
      } else
         return 0;
   } else if (mapping_family==1 && channels<=8 && channels>=1)
   {
      nb_streams=vorbis_mappings[channels-1].nb_streams;
      nb_coupled_streams=vorbis_mappings[channels-1].nb_coupled_streams;
   } else if (mapping_family==255)
   {
      nb_streams=channels;
      nb_coupled_streams=0;
   } else
      return 0;
   size = opus_multistream_encoder_get_size(nb_streams, nb_coupled_streams);
   if (channels>2)
   {
      size += channels*(120*sizeof(opus_val32) + sizeof(opus_val32));
   }
   return size;
}


static int opus_multistream_encoder_init_impl(
      OpusMSEncoder *st,
      opus_int32 Fs,
      int channels,
      int streams,
      int coupled_streams,
      const unsigned char *mapping,
      int application,
      int surround
)
{
   int coupled_size;
   int mono_size;
   int i, ret;
   char *ptr;

   if ((channels>255) || (channels<1) || (coupled_streams>streams) ||
       (coupled_streams+streams>255) || (streams<1) || (coupled_streams<0))
      return OPUS_BAD_ARG;

   st->layout.nb_channels = channels;
   st->layout.nb_streams = streams;
   st->layout.nb_coupled_streams = coupled_streams;
   st->subframe_mem[0]=st->subframe_mem[1]=st->subframe_mem[2]=0;
   if (!surround)
      st->lfe_stream = -1;
   st->bitrate_bps = OPUS_AUTO;
   st->application = application;
   st->variable_duration = OPUS_FRAMESIZE_ARG;
   for (i=0;i<st->layout.nb_channels;i++)
      st->layout.mapping[i] = mapping[i];
   if (!validate_layout(&st->layout) || !validate_encoder_layout(&st->layout))
      return OPUS_BAD_ARG;
   ptr = (char*)st + align(sizeof(OpusMSEncoder));
   coupled_size = opus_encoder_get_size(2);
   mono_size = opus_encoder_get_size(1);

   for (i=0;i<st->layout.nb_coupled_streams;i++)
   {
      ret = opus_encoder_init((OpusEncoder*)ptr, Fs, 2, application);
      if(ret!=OPUS_OK)return ret;
      if (i==st->lfe_stream)
         opus_encoder_ctl((OpusEncoder*)ptr, OPUS_SET_LFE(1));
      ptr += align(coupled_size);
   }
   for (;i<st->layout.nb_streams;i++)
   {
      ret = opus_encoder_init((OpusEncoder*)ptr, Fs, 1, application);
      if (i==st->lfe_stream)
         opus_encoder_ctl((OpusEncoder*)ptr, OPUS_SET_LFE(1));
      if(ret!=OPUS_OK)return ret;
      ptr += align(mono_size);
   }
   if (surround)
   {
      OPUS_CLEAR(ms_get_preemph_mem(st), channels);
      OPUS_CLEAR(ms_get_window_mem(st), channels*120);
   }
   st->surround = surround;
   return OPUS_OK;
}

int opus_multistream_encoder_init(
      OpusMSEncoder *st,
      opus_int32 Fs,
      int channels,
      int streams,
      int coupled_streams,
      const unsigned char *mapping,
      int application
)
{
   return opus_multistream_encoder_init_impl(st, Fs, channels, streams, coupled_streams, mapping, application, 0);
}

int opus_multistream_surround_encoder_init(
      OpusMSEncoder *st,
      opus_int32 Fs,
      int channels,
      int mapping_family,
      int *streams,
      int *coupled_streams,
      unsigned char *mapping,
      int application
)
{
   if ((channels>255) || (channels<1))
      return OPUS_BAD_ARG;
   st->lfe_stream = -1;
   if (mapping_family==0)
   {
      if (channels==1)
      {
         *streams=1;
         *coupled_streams=0;
         mapping[0]=0;
      } else if (channels==2)
      {
         *streams=1;
         *coupled_streams=1;
         mapping[0]=0;
         mapping[1]=1;
      } else
         return OPUS_UNIMPLEMENTED;
   } else if (mapping_family==1 && channels<=8 && channels>=1)
   {
      int i;
      *streams=vorbis_mappings[channels-1].nb_streams;
      *coupled_streams=vorbis_mappings[channels-1].nb_coupled_streams;
      for (i=0;i<channels;i++)
         mapping[i] = vorbis_mappings[channels-1].mapping[i];
      if (channels>=6)
         st->lfe_stream = *streams-1;
   } else if (mapping_family==255)
   {
      int i;
      *streams=channels;
      *coupled_streams=0;
      for(i=0;i<channels;i++)
         mapping[i] = i;
   } else
      return OPUS_UNIMPLEMENTED;
   return opus_multistream_encoder_init_impl(st, Fs, channels, *streams, *coupled_streams,
         mapping, application, channels>2&&mapping_family==1);
}

OpusMSEncoder *opus_multistream_encoder_create(
      opus_int32 Fs,
      int channels,
      int streams,
      int coupled_streams,
      const unsigned char *mapping,
      int application,
      int *error
)
{
   int ret;
   OpusMSEncoder *st;
   if ((channels>255) || (channels<1) || (coupled_streams>streams) ||
       (coupled_streams+streams>255) || (streams<1) || (coupled_streams<0))
   {
      if (error)
         *error = OPUS_BAD_ARG;
      return NULL;
   }
   st = (OpusMSEncoder *)opus_alloc(opus_multistream_encoder_get_size(streams, coupled_streams));
   if (st==NULL)
   {
      if (error)
         *error = OPUS_ALLOC_FAIL;
      return NULL;
   }
   ret = opus_multistream_encoder_init(st, Fs, channels, streams, coupled_streams, mapping, application);
   if (ret != OPUS_OK)
   {
      opus_free(st);
      st = NULL;
   }
   if (error)
      *error = ret;
   return st;
}

OpusMSEncoder *opus_multistream_surround_encoder_create(
      opus_int32 Fs,
      int channels,
      int mapping_family,
      int *streams,
      int *coupled_streams,
      unsigned char *mapping,
      int application,
      int *error
)
{
   int ret;
   OpusMSEncoder *st;
   if ((channels>255) || (channels<1))
   {
      if (error)
         *error = OPUS_BAD_ARG;
      return NULL;
   }
   st = (OpusMSEncoder *)opus_alloc(opus_multistream_surround_encoder_get_size(channels, mapping_family));
   if (st==NULL)
   {
      if (error)
         *error = OPUS_ALLOC_FAIL;
      return NULL;
   }
   ret = opus_multistream_surround_encoder_init(st, Fs, channels, mapping_family, streams, coupled_streams, mapping, application);
   if (ret != OPUS_OK)
   {
      opus_free(st);
      st = NULL;
   }
   if (error)
      *error = ret;
   return st;
}

static void surround_rate_allocation(
      OpusMSEncoder *st,
      opus_int32 *rate,
      int frame_size
      )
{
   int i;
   opus_int32 channel_rate;
   opus_int32 Fs;
   char *ptr;
   int stream_offset;
   int lfe_offset;
   int coupled_ratio; /* Q8 */
   int lfe_ratio;     /* Q8 */

   ptr = (char*)st + align(sizeof(OpusMSEncoder));
   opus_encoder_ctl((OpusEncoder*)ptr, OPUS_GET_SAMPLE_RATE(&Fs));

   if (st->bitrate_bps > st->layout.nb_channels*40000)
      stream_offset = 20000;
   else
      stream_offset = st->bitrate_bps/st->layout.nb_channels/2;
   stream_offset += 60*(Fs/frame_size-50);
   /* We start by giving each stream (coupled or uncoupled) the same bitrate.
      This models the main saving of coupled channels over uncoupled. */
   /* The LFE stream is an exception to the above and gets fewer bits. */
   lfe_offset = 3500 + 60*(Fs/frame_size-50);
   /* Coupled streams get twice the mono rate after the first 20 kb/s. */
   coupled_ratio = 512;
   /* Should depend on the bitrate, for now we assume LFE gets 1/8 the bits of mono */
   lfe_ratio = 32;

   /* Compute bitrate allocation between streams */
   if (st->bitrate_bps==OPUS_AUTO)
   {
      channel_rate = Fs+60*Fs/frame_size;
   } else if (st->bitrate_bps==OPUS_BITRATE_MAX)
   {
      channel_rate = 300000;
   } else {
      int nb_lfe;
      int nb_uncoupled;
      int nb_coupled;
      int total;
      nb_lfe = (st->lfe_stream!=-1);
      nb_coupled = st->layout.nb_coupled_streams;
      nb_uncoupled = st->layout.nb_streams-nb_coupled-nb_lfe;
      total = (nb_uncoupled<<8)         /* mono */
            + coupled_ratio*nb_coupled /* stereo */
            + nb_lfe*lfe_ratio;
      channel_rate = 256*(st->bitrate_bps-lfe_offset*nb_lfe-stream_offset*(nb_coupled+nb_uncoupled))/total;
   }
#ifndef FIXED_POINT
   if (st->variable_duration==OPUS_FRAMESIZE_VARIABLE && frame_size != Fs/50)
   {
      opus_int32 bonus;
      bonus = 60*(Fs/frame_size-50);
      channel_rate += bonus;
   }
#endif

   for (i=0;i<st->layout.nb_streams;i++)
   {
      if (i<st->layout.nb_coupled_streams)
         rate[i] = stream_offset+(channel_rate*coupled_ratio>>8);
      else if (i!=st->lfe_stream)
         rate[i] = stream_offset+channel_rate;
      else
         rate[i] = lfe_offset+(channel_rate*lfe_ratio>>8);
   }
}

/* Max size in case the encoder decides to return three frames */
#define MS_FRAME_TMP (3*1275+7)
static int opus_multistream_encode_native
(
    OpusMSEncoder *st,
    opus_copy_channel_in_func copy_channel_in,
    const void *pcm,
    int analysis_frame_size,
    unsigned char *data,
    opus_int32 max_data_bytes,
    int lsb_depth,
    downmix_func downmix
)
{
   opus_int32 Fs;
   int coupled_size;
   int mono_size;
   int s;
   char *ptr;
   int tot_size;
   VARDECL(opus_val16, buf);
   VARDECL(opus_val16, bandSMR);
   unsigned char tmp_data[MS_FRAME_TMP];
   OpusRepacketizer rp;
   opus_int32 vbr;
   const CELTMode *celt_mode;
   opus_int32 bitrates[256];
   opus_val16 bandLogE[42];
   opus_val32 *mem = NULL;
   opus_val32 *preemph_mem=NULL;
   int frame_size;
   ALLOC_STACK;

   if (st->surround)
   {
      preemph_mem = ms_get_preemph_mem(st);
      mem = ms_get_window_mem(st);
   }

   ptr = (char*)st + align(sizeof(OpusMSEncoder));
   opus_encoder_ctl((OpusEncoder*)ptr, OPUS_GET_SAMPLE_RATE(&Fs));
   opus_encoder_ctl((OpusEncoder*)ptr, OPUS_GET_VBR(&vbr));
   opus_encoder_ctl((OpusEncoder*)ptr, CELT_GET_MODE(&celt_mode));

   {
      opus_int32 delay_compensation;
      int channels;

      channels = st->layout.nb_streams + st->layout.nb_coupled_streams;
      opus_encoder_ctl((OpusEncoder*)ptr, OPUS_GET_LOOKAHEAD(&delay_compensation));
      delay_compensation -= Fs/400;
      frame_size = compute_frame_size(pcm, analysis_frame_size,
            st->variable_duration, channels, Fs, st->bitrate_bps,
            delay_compensation, downmix
#ifndef DISABLE_FLOAT_API
            , st->subframe_mem
#endif
            );
   }

   if (400*frame_size < Fs)
   {
      RESTORE_STACK;
      return OPUS_BAD_ARG;
   }
   /* Validate frame_size before using it to allocate stack space.
      This mirrors the checks in opus_encode[_float](). */
   if (400*frame_size != Fs && 200*frame_size != Fs &&
       100*frame_size != Fs &&  50*frame_size != Fs &&
        25*frame_size != Fs &&  50*frame_size != 3*Fs)
   {
      RESTORE_STACK;
      return OPUS_BAD_ARG;
   }
   ALLOC(buf, 2*frame_size, opus_val16);
   coupled_size = opus_encoder_get_size(2);
   mono_size = opus_encoder_get_size(1);

   ALLOC(bandSMR, 21*st->layout.nb_channels, opus_val16);
   if (st->surround)
   {
      surround_analysis(celt_mode, pcm, bandSMR, mem, preemph_mem, frame_size, 120, st->layout.nb_channels, Fs, copy_channel_in);
   }

   if (max_data_bytes < 4*st->layout.nb_streams-1)
   {
      RESTORE_STACK;
      return OPUS_BUFFER_TOO_SMALL;
   }

   /* Compute bitrate allocation between streams (this could be a lot better) */
   surround_rate_allocation(st, bitrates, frame_size);

   if (!vbr)
      max_data_bytes = IMIN(max_data_bytes, 3*st->bitrate_bps/(3*8*Fs/frame_size));

   ptr = (char*)st + align(sizeof(OpusMSEncoder));
   for (s=0;s<st->layout.nb_streams;s++)
   {
      OpusEncoder *enc;
      enc = (OpusEncoder*)ptr;
      if (s < st->layout.nb_coupled_streams)
         ptr += align(coupled_size);
      else
         ptr += align(mono_size);
      opus_encoder_ctl(enc, OPUS_SET_BITRATE(bitrates[s]));
      if (st->surround)
      {
         opus_int32 equiv_rate;
         equiv_rate = st->bitrate_bps;
         if (frame_size*50 < Fs)
            equiv_rate -= 60*(Fs/frame_size - 50)*st->layout.nb_channels;
         if (equiv_rate > 10000*st->layout.nb_channels)
            opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND));
         else if (equiv_rate > 7000*st->layout.nb_channels)
            opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_SUPERWIDEBAND));
         else if (equiv_rate > 5000*st->layout.nb_channels)
            opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_WIDEBAND));
         else
            opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_NARROWBAND));
         if (s < st->layout.nb_coupled_streams)
         {
            /* To preserve the spatial image, force stereo CELT on coupled streams */
            opus_encoder_ctl(enc, OPUS_SET_FORCE_MODE(MODE_CELT_ONLY));
            opus_encoder_ctl(enc, OPUS_SET_FORCE_CHANNELS(2));
         }
      }
   }

   ptr = (char*)st + align(sizeof(OpusMSEncoder));
   /* Counting ToC */
   tot_size = 0;
   for (s=0;s<st->layout.nb_streams;s++)
   {
      OpusEncoder *enc;
      int len;
      int curr_max;
      int c1, c2;

      opus_repacketizer_init(&rp);
      enc = (OpusEncoder*)ptr;
      if (s < st->layout.nb_coupled_streams)
      {
         int i;
         int left, right;
         left = get_left_channel(&st->layout, s, -1);
         right = get_right_channel(&st->layout, s, -1);
         (*copy_channel_in)(buf, 2,
            pcm, st->layout.nb_channels, left, frame_size);
         (*copy_channel_in)(buf+1, 2,
            pcm, st->layout.nb_channels, right, frame_size);
         ptr += align(coupled_size);
         if (st->surround)
         {
            for (i=0;i<21;i++)
            {
               bandLogE[i] = bandSMR[21*left+i];
               bandLogE[21+i] = bandSMR[21*right+i];
            }
         }
         c1 = left;
         c2 = right;
      } else {
         int i;
         int chan = get_mono_channel(&st->layout, s, -1);
         (*copy_channel_in)(buf, 1,
            pcm, st->layout.nb_channels, chan, frame_size);
         ptr += align(mono_size);
         if (st->surround)
         {
            for (i=0;i<21;i++)
               bandLogE[i] = bandSMR[21*chan+i];
         }
         c1 = chan;
         c2 = -1;
      }
      if (st->surround)
         opus_encoder_ctl(enc, OPUS_SET_ENERGY_MASK(bandLogE));
      /* number of bytes left (+Toc) */
      curr_max = max_data_bytes - tot_size;
      /* Reserve three bytes for the last stream and four for the others */
      curr_max -= IMAX(0,4*(st->layout.nb_streams-s-1)-1);
      curr_max = IMIN(curr_max,MS_FRAME_TMP);
      if (!vbr && s == st->layout.nb_streams-1)
         opus_encoder_ctl(enc, OPUS_SET_BITRATE(curr_max*(8*Fs/frame_size)));
      len = opus_encode_native(enc, buf, frame_size, tmp_data, curr_max, lsb_depth,
            pcm, analysis_frame_size, c1, c2, st->layout.nb_channels, downmix);
      if (len<0)
      {
         RESTORE_STACK;
         return len;
      }
      /* We need to use the repacketizer to add the self-delimiting lengths
         while taking into account the fact that the encoder can now return
         more than one frame at a time (e.g. 60 ms CELT-only) */
      opus_repacketizer_cat(&rp, tmp_data, len);
      len = opus_repacketizer_out_range_impl(&rp, 0, opus_repacketizer_get_nb_frames(&rp),
            data, max_data_bytes-tot_size, s != st->layout.nb_streams-1, !vbr && s == st->layout.nb_streams-1);
      data += len;
      tot_size += len;
   }
   /*printf("\n");*/
   RESTORE_STACK;
   return tot_size;
}

#if !defined(DISABLE_FLOAT_API)
static void opus_copy_channel_in_float(
  opus_val16 *dst,
  int dst_stride,
  const void *src,
  int src_stride,
  int src_channel,
  int frame_size
)
{
   const float *float_src;
   opus_int32 i;
   float_src = (const float *)src;
   for (i=0;i<frame_size;i++)
#if defined(FIXED_POINT)
      dst[i*dst_stride] = FLOAT2INT16(float_src[i*src_stride+src_channel]);
#else
      dst[i*dst_stride] = float_src[i*src_stride+src_channel];
#endif
}
#endif

static void opus_copy_channel_in_short(
  opus_val16 *dst,
  int dst_stride,
  const void *src,
  int src_stride,
  int src_channel,
  int frame_size
)
{
   const opus_int16 *short_src;
   opus_int32 i;
   short_src = (const opus_int16 *)src;
   for (i=0;i<frame_size;i++)
#if defined(FIXED_POINT)
      dst[i*dst_stride] = short_src[i*src_stride+src_channel];
#else
      dst[i*dst_stride] = (1/32768.f)*short_src[i*src_stride+src_channel];
#endif
}


#ifdef FIXED_POINT
int opus_multistream_encode(
    OpusMSEncoder *st,
    const opus_val16 *pcm,
    int frame_size,
    unsigned char *data,
    opus_int32 max_data_bytes
)
{
   return opus_multistream_encode_native(st, opus_copy_channel_in_short,
      pcm, frame_size, data, max_data_bytes, 16, downmix_int);
}

#ifndef DISABLE_FLOAT_API
int opus_multistream_encode_float(
    OpusMSEncoder *st,
    const float *pcm,
    int frame_size,
    unsigned char *data,
    opus_int32 max_data_bytes
)
{
   return opus_multistream_encode_native(st, opus_copy_channel_in_float,
      pcm, frame_size, data, max_data_bytes, 16, downmix_float);
}
#endif

#else

int opus_multistream_encode_float
(
    OpusMSEncoder *st,
    const opus_val16 *pcm,
    int frame_size,
    unsigned char *data,
    opus_int32 max_data_bytes
)
{
   return opus_multistream_encode_native(st, opus_copy_channel_in_float,
      pcm, frame_size, data, max_data_bytes, 24, downmix_float);
}

int opus_multistream_encode(
    OpusMSEncoder *st,
    const opus_int16 *pcm,
    int frame_size,
    unsigned char *data,
    opus_int32 max_data_bytes
)
{
   return opus_multistream_encode_native(st, opus_copy_channel_in_short,
      pcm, frame_size, data, max_data_bytes, 16, downmix_int);
}
#endif

int opus_multistream_encoder_ctl(OpusMSEncoder *st, int request, ...)
{
   va_list ap;
   int coupled_size, mono_size;
   char *ptr;
   int ret = OPUS_OK;

   va_start(ap, request);

   coupled_size = opus_encoder_get_size(2);
   mono_size = opus_encoder_get_size(1);
   ptr = (char*)st + align(sizeof(OpusMSEncoder));
   switch (request)
   {
   case OPUS_SET_BITRATE_REQUEST:
   {
      opus_int32 value = va_arg(ap, opus_int32);
      if (value<0 && value!=OPUS_AUTO && value!=OPUS_BITRATE_MAX)
      {
         goto bad_arg;
      }
      st->bitrate_bps = value;
   }
   break;
   case OPUS_GET_BITRATE_REQUEST:
   {
      int s;
      opus_int32 *value = va_arg(ap, opus_int32*);
      if (!value)
      {
         goto bad_arg;
      }
      *value = 0;
      for (s=0;s<st->layout.nb_streams;s++)
      {
         opus_int32 rate;
         OpusEncoder *enc;
         enc = (OpusEncoder*)ptr;
         if (s < st->layout.nb_coupled_streams)
            ptr += align(coupled_size);
         else
            ptr += align(mono_size);
         opus_encoder_ctl(enc, request, &rate);
         *value += rate;
      }
   }
   break;
   case OPUS_GET_LSB_DEPTH_REQUEST:
   case OPUS_GET_VBR_REQUEST:
   case OPUS_GET_APPLICATION_REQUEST:
   case OPUS_GET_BANDWIDTH_REQUEST:
   case OPUS_GET_COMPLEXITY_REQUEST:
   case OPUS_GET_PACKET_LOSS_PERC_REQUEST:
   case OPUS_GET_DTX_REQUEST:
   case OPUS_GET_VOICE_RATIO_REQUEST:
   case OPUS_GET_VBR_CONSTRAINT_REQUEST:
   case OPUS_GET_SIGNAL_REQUEST:
   case OPUS_GET_LOOKAHEAD_REQUEST:
   case OPUS_GET_SAMPLE_RATE_REQUEST:
   case OPUS_GET_INBAND_FEC_REQUEST:
   case OPUS_GET_FORCE_CHANNELS_REQUEST:
   case OPUS_GET_PREDICTION_DISABLED_REQUEST:
   {
      OpusEncoder *enc;
      /* For int32* GET params, just query the first stream */
      opus_int32 *value = va_arg(ap, opus_int32*);
      enc = (OpusEncoder*)ptr;
      ret = opus_encoder_ctl(enc, request, value);
   }
   break;
   case OPUS_GET_FINAL_RANGE_REQUEST:
   {
      int s;
      opus_uint32 *value = va_arg(ap, opus_uint32*);
      opus_uint32 tmp;
      if (!value)
      {
         goto bad_arg;
      }
      *value=0;
      for (s=0;s<st->layout.nb_streams;s++)
      {
         OpusEncoder *enc;
         enc = (OpusEncoder*)ptr;
         if (s < st->layout.nb_coupled_streams)
            ptr += align(coupled_size);
         else
            ptr += align(mono_size);
         ret = opus_encoder_ctl(enc, request, &tmp);
         if (ret != OPUS_OK) break;
         *value ^= tmp;
      }
   }
   break;
   case OPUS_SET_LSB_DEPTH_REQUEST:
   case OPUS_SET_COMPLEXITY_REQUEST:
   case OPUS_SET_VBR_REQUEST:
   case OPUS_SET_VBR_CONSTRAINT_REQUEST:
   case OPUS_SET_MAX_BANDWIDTH_REQUEST:
   case OPUS_SET_BANDWIDTH_REQUEST:
   case OPUS_SET_SIGNAL_REQUEST:
   case OPUS_SET_APPLICATION_REQUEST:
   case OPUS_SET_INBAND_FEC_REQUEST:
   case OPUS_SET_PACKET_LOSS_PERC_REQUEST:
   case OPUS_SET_DTX_REQUEST:
   case OPUS_SET_FORCE_MODE_REQUEST:
   case OPUS_SET_FORCE_CHANNELS_REQUEST:
   case OPUS_SET_PREDICTION_DISABLED_REQUEST:
   {
      int s;
      /* This works for int32 params */
      opus_int32 value = va_arg(ap, opus_int32);
      for (s=0;s<st->layout.nb_streams;s++)
      {
         OpusEncoder *enc;

         enc = (OpusEncoder*)ptr;
         if (s < st->layout.nb_coupled_streams)
            ptr += align(coupled_size);
         else
            ptr += align(mono_size);
         ret = opus_encoder_ctl(enc, request, value);
         if (ret != OPUS_OK)
            break;
      }
   }
   break;
   case OPUS_MULTISTREAM_GET_ENCODER_STATE_REQUEST:
   {
      int s;
      opus_int32 stream_id;
      OpusEncoder **value;
      stream_id = va_arg(ap, opus_int32);
      if (stream_id<0 || stream_id >= st->layout.nb_streams)
         ret = OPUS_BAD_ARG;
      value = va_arg(ap, OpusEncoder**);
      if (!value)
      {
         goto bad_arg;
      }
      for (s=0;s<stream_id;s++)
      {
         if (s < st->layout.nb_coupled_streams)
            ptr += align(coupled_size);
         else
            ptr += align(mono_size);
      }
      *value = (OpusEncoder*)ptr;
   }
   break;
   case OPUS_SET_EXPERT_FRAME_DURATION_REQUEST:
   {
       opus_int32 value = va_arg(ap, opus_int32);
       st->variable_duration = value;
   }
   break;
   case OPUS_GET_EXPERT_FRAME_DURATION_REQUEST:
   {
       opus_int32 *value = va_arg(ap, opus_int32*);
       if (!value)
       {
          goto bad_arg;
       }
       *value = st->variable_duration;
   }
   break;
   case OPUS_RESET_STATE:
   {
      int s;
      st->subframe_mem[0] = st->subframe_mem[1] = st->subframe_mem[2] = 0;
      if (st->surround)
      {
         OPUS_CLEAR(ms_get_preemph_mem(st), st->layout.nb_channels);
         OPUS_CLEAR(ms_get_window_mem(st), st->layout.nb_channels*120);
      }
      for (s=0;s<st->layout.nb_streams;s++)
      {
         OpusEncoder *enc;
         enc = (OpusEncoder*)ptr;
         if (s < st->layout.nb_coupled_streams)
            ptr += align(coupled_size);
         else
            ptr += align(mono_size);
         ret = opus_encoder_ctl(enc, OPUS_RESET_STATE);
         if (ret != OPUS_OK)
            break;
      }
   }
   break;
   default:
      ret = OPUS_UNIMPLEMENTED;
      break;
   }

   va_end(ap);
   return ret;
bad_arg:
   va_end(ap);
   return OPUS_BAD_ARG;
}

void opus_multistream_encoder_destroy(OpusMSEncoder *st)
{
    opus_free(st);
}
