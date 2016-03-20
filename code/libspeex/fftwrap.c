/* Copyright (C) 2005-2006 Jean-Marc Valin 
   File: fftwrap.c

   Wrapper for various FFTs 

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
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

/*#define USE_SMALLFT*/
#define USE_KISS_FFT


#include "arch.h"
#include "os_support.h"

#define MAX_FFT_SIZE 2048

#ifdef FIXED_POINT
static int maximize_range(spx_word16_t *in, spx_word16_t *out, spx_word16_t bound, int len)
{
   int i, shift;
   spx_word16_t max_val = 0;
   for (i=0;i<len;i++)
   {
      if (in[i]>max_val)
         max_val = in[i];
      if (-in[i]>max_val)
         max_val = -in[i];
   }
   shift=0;
   while (max_val <= (bound>>1) && max_val != 0)
   {
      max_val <<= 1;
      shift++;
   }
   for (i=0;i<len;i++)
   {
      out[i] = SHL16(in[i], shift);
   }   
   return shift;
}

static void renorm_range(spx_word16_t *in, spx_word16_t *out, int shift, int len)
{
   int i;
   for (i=0;i<len;i++)
   {
      out[i] = PSHR16(in[i], shift);
   }
}
#endif

#ifdef USE_SMALLFT

#include "smallft.h"
#include <math.h>

void *spx_fft_init(int size)
{
   struct drft_lookup *table;
   table = speex_alloc(sizeof(struct drft_lookup));
   spx_drft_init((struct drft_lookup *)table, size);
   return (void*)table;
}

void spx_fft_destroy(void *table)
{
   spx_drft_clear(table);
   speex_free(table);
}

void spx_fft(void *table, float *in, float *out)
{
   if (in==out)
   {
      int i;
      float scale = 1./((struct drft_lookup *)table)->n;
      speex_warning("FFT should not be done in-place");
      for (i=0;i<((struct drft_lookup *)table)->n;i++)
         out[i] = scale*in[i];
   } else {
      int i;
      float scale = 1./((struct drft_lookup *)table)->n;
      for (i=0;i<((struct drft_lookup *)table)->n;i++)
         out[i] = scale*in[i];
   }
   spx_drft_forward((struct drft_lookup *)table, out);
}

void spx_ifft(void *table, float *in, float *out)
{
   if (in==out)
   {
      speex_warning("FFT should not be done in-place");
   } else {
      int i;
      for (i=0;i<((struct drft_lookup *)table)->n;i++)
         out[i] = in[i];
   }
   spx_drft_backward((struct drft_lookup *)table, out);
}

#elif defined(USE_KISS_FFT)

#include "kiss_fftr.h"
#include "kiss_fft.h"

struct kiss_config {
   kiss_fftr_cfg forward;
   kiss_fftr_cfg backward;
   int N;
};

void *spx_fft_init(int size)
{
   struct kiss_config *table;
   table = (struct kiss_config*)speex_alloc(sizeof(struct kiss_config));
   table->forward = kiss_fftr_alloc(size,0,NULL,NULL);
   table->backward = kiss_fftr_alloc(size,1,NULL,NULL);
   table->N = size;
   return table;
}

void spx_fft_destroy(void *table)
{
   struct kiss_config *t = (struct kiss_config *)table;
   kiss_fftr_free(t->forward);
   kiss_fftr_free(t->backward);
   speex_free(table);
}

#ifdef FIXED_POINT

void spx_fft(void *table, spx_word16_t *in, spx_word16_t *out)
{
   int shift;
   struct kiss_config *t = (struct kiss_config *)table;
   shift = maximize_range(in, in, 32000, t->N);
   kiss_fftr2(t->forward, in, out);
   renorm_range(in, in, shift, t->N);
   renorm_range(out, out, shift, t->N);
}

#else

void spx_fft(void *table, spx_word16_t *in, spx_word16_t *out)
{
   int i;
   float scale;
   struct kiss_config *t = (struct kiss_config *)table;
   scale = 1./t->N;
   kiss_fftr2(t->forward, in, out);
   for (i=0;i<t->N;i++)
      out[i] *= scale;
}
#endif

void spx_ifft(void *table, spx_word16_t *in, spx_word16_t *out)
{
   struct kiss_config *t = (struct kiss_config *)table;
   kiss_fftri2(t->backward, in, out);
}


#else

#error No other FFT implemented

#endif


#ifdef FIXED_POINT
/*#include "smallft.h"*/


void spx_fft_float(void *table, float *in, float *out)
{
   int i;
#ifdef USE_SMALLFT
   int N = ((struct drft_lookup *)table)->n;
#elif defined(USE_KISS_FFT)
   int N = ((struct kiss_config *)table)->N;
#else
#endif
#ifdef VAR_ARRAYS
   spx_word16_t _in[N];
   spx_word16_t _out[N];
#else
   spx_word16_t _in[MAX_FFT_SIZE];
   spx_word16_t _out[MAX_FFT_SIZE];
#endif
   for (i=0;i<N;i++)
      _in[i] = (int)floor(.5+in[i]);
   spx_fft(table, _in, _out);
   for (i=0;i<N;i++)
      out[i] = _out[i];
#if 0
   if (!fixed_point)
   {
      float scale;
      struct drft_lookup t;
      spx_drft_init(&t, ((struct kiss_config *)table)->N);
      scale = 1./((struct kiss_config *)table)->N;
      for (i=0;i<((struct kiss_config *)table)->N;i++)
         out[i] = scale*in[i];
      spx_drft_forward(&t, out);
      spx_drft_clear(&t);
   }
#endif
}

void spx_ifft_float(void *table, float *in, float *out)
{
   int i;
#ifdef USE_SMALLFT
   int N = ((struct drft_lookup *)table)->n;
#elif defined(USE_KISS_FFT)
   int N = ((struct kiss_config *)table)->N;
#else
#endif
#ifdef VAR_ARRAYS
   spx_word16_t _in[N];
   spx_word16_t _out[N];
#else
   spx_word16_t _in[MAX_FFT_SIZE];
   spx_word16_t _out[MAX_FFT_SIZE];
#endif
   for (i=0;i<N;i++)
      _in[i] = (int)floor(.5+in[i]);
   spx_ifft(table, _in, _out);
   for (i=0;i<N;i++)
      out[i] = _out[i];
#if 0
   if (!fixed_point)
   {
      int i;
      struct drft_lookup t;
      spx_drft_init(&t, ((struct kiss_config *)table)->N);
      for (i=0;i<((struct kiss_config *)table)->N;i++)
         out[i] = in[i];
      spx_drft_backward(&t, out);
      spx_drft_clear(&t);
   }
#endif
}

#else

void spx_fft_float(void *table, float *in, float *out)
{
   spx_fft(table, in, out);
}
void spx_ifft_float(void *table, float *in, float *out)
{
   spx_ifft(table, in, out);
}

#endif
