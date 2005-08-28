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
//
// snd_mixa.s
// x86 assembly-language sound code
//

#include "qasm.h"

#if	id386

	.text

#if 0 
//----------------------------------------------------------------------
// 8-bit sound-mixing code
//----------------------------------------------------------------------

#define ch		4+16
#define sc		8+16
#define count	12+16

.globl C(S_PaintChannelFrom8)
C(S_PaintChannelFrom8):
	pushl	%esi				// preserve register variables
	pushl	%edi
	pushl	%ebx
	pushl	%ebp

//	int 	data;
//	short	*lscale, *rscale;
//	unsigned char *sfx;
//	int		i;

	movl	ch(%esp),%ebx
	movl	sc(%esp),%esi

//	if (ch->leftvol > 255)
//		ch->leftvol = 255;
//	if (ch->rightvol > 255)
//		ch->rightvol = 255;
	movl	ch_leftvol(%ebx),%eax
	movl	ch_rightvol(%ebx),%edx
	cmpl	$255,%eax
	jna		LLeftSet
	movl	$255,%eax
LLeftSet:
	cmpl	$255,%edx
	jna		LRightSet
	movl	$255,%edx
LRightSet:

//	lscale = snd_scaletable[ch->leftvol >> 3];
//	rscale = snd_scaletable[ch->rightvol >> 3];
//	sfx = (signed char *)sc->data + ch->pos;
//	ch->pos += count;
	andl	$0xF8,%eax
	addl	$20,%esi
	movl	(%esi),%esi
	andl	$0xF8,%edx
	movl	ch_pos(%ebx),%edi
	movl	count(%esp),%ecx
	addl	%edi,%esi
	shll	$7,%eax
	addl	%ecx,%edi
	shll	$7,%edx
	movl	%edi,ch_pos(%ebx)
	addl	$(C(snd_scaletable)),%eax
	addl	$(C(snd_scaletable)),%edx
	subl	%ebx,%ebx
	movb	-1(%esi,%ecx,1),%bl

	testl	$1,%ecx
	jz		LMix8Loop

	movl	(%eax,%ebx,4),%edi
	movl	(%edx,%ebx,4),%ebp
	addl	C(paintbuffer)+psp_left-psp_size(,%ecx,psp_size),%edi
	addl	C(paintbuffer)+psp_right-psp_size(,%ecx,psp_size),%ebp
	movl	%edi,C(paintbuffer)+psp_left-psp_size(,%ecx,psp_size)
	movl	%ebp,C(paintbuffer)+psp_right-psp_size(,%ecx,psp_size)
	movb	-2(%esi,%ecx,1),%bl

	decl	%ecx
	jz		LDone

//	for (i=0 ; i<count ; i++)
//	{
LMix8Loop:

//		data = sfx[i];
//		paintbuffer[i].left += lscale[data];
//		paintbuffer[i].right += rscale[data];
	movl	(%eax,%ebx,4),%edi
	movl	(%edx,%ebx,4),%ebp
	addl	C(paintbuffer)+psp_left-psp_size(,%ecx,psp_size),%edi
	addl	C(paintbuffer)+psp_right-psp_size(,%ecx,psp_size),%ebp
	movb	-2(%esi,%ecx,1),%bl
	movl	%edi,C(paintbuffer)+psp_left-psp_size(,%ecx,psp_size)
	movl	%ebp,C(paintbuffer)+psp_right-psp_size(,%ecx,psp_size)

	movl	(%eax,%ebx,4),%edi
	movl	(%edx,%ebx,4),%ebp
	movb	-3(%esi,%ecx,1),%bl
	addl	C(paintbuffer)+psp_left-psp_size*2(,%ecx,psp_size),%edi
	addl	C(paintbuffer)+psp_right-psp_size*2(,%ecx,psp_size),%ebp
	movl	%edi,C(paintbuffer)+psp_left-psp_size*2(,%ecx,psp_size)
	movl	%ebp,C(paintbuffer)+psp_right-psp_size*2(,%ecx,psp_size)

//	}
	subl	$2,%ecx
	jnz		LMix8Loop

LDone:
	popl	%ebp
	popl	%ebx
	popl	%edi
	popl	%esi

	ret


#endif

//----------------------------------------------------------------------
// Transfer of stereo buffer to 16-bit DMA buffer code
//----------------------------------------------------------------------

.globl C(S_WriteLinearBlastStereo16)
C(S_WriteLinearBlastStereo16):
	pushl	%edi
	pushl	%ebx

//	int		i;
//	int		val;
	movl	C(snd_linear_count),%ecx
	movl	C(snd_p),%ebx
	movl	C(snd_out),%edi

//	for (i=0 ; i<snd_linear_count ; i+=2)
//	{
LWLBLoopTop:

//		val = (snd_p[i]*snd_vol)>>8;
//		if (val > 0x7fff)
//			snd_out[i] = 0x7fff;
//		else if (val < (short)0x8000)
//			snd_out[i] = (short)0x8000;
//		else
//			snd_out[i] = val;
	movl	-8(%ebx,%ecx,4),%eax
	sarl	$8,%eax
	cmpl	$0x7FFF,%eax
	jg		LClampHigh
	cmpl	$0xFFFF8000,%eax
	jnl		LClampDone
	movl	$0xFFFF8000,%eax
	jmp		LClampDone
LClampHigh:
	movl	$0x7FFF,%eax
LClampDone:

//		val = (snd_p[i+1]*snd_vol)>>8;
//		if (val > 0x7fff)
//			snd_out[i+1] = 0x7fff;
//		else if (val < (short)0x8000)
//			snd_out[i+1] = (short)0x8000;
//		else
//			snd_out[i+1] = val;
	movl	-4(%ebx,%ecx,4),%edx
	sarl	$8,%edx
	cmpl	$0x7FFF,%edx
	jg		LClampHigh2
	cmpl	$0xFFFF8000,%edx
	jnl		LClampDone2
	movl	$0xFFFF8000,%edx
	jmp		LClampDone2
LClampHigh2:
	movl	$0x7FFF,%edx
LClampDone2:
	shll	$16,%edx
	andl	$0xFFFF,%eax
	orl		%eax,%edx
	movl	%edx,-4(%edi,%ecx,2)

//	}
	subl	$2,%ecx
	jnz		LWLBLoopTop

//	snd_p += snd_linear_count;

	popl	%ebx
	popl	%edi

	ret

#endif	// id386

