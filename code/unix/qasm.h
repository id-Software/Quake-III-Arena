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
#ifndef __ASM_I386__
#define __ASM_I386__

#include "../qcommon/q_platform.h"

#ifdef __MINGW32__
#undef ELF
#endif

#ifdef ELF
#define C(label) label
#else
#define C(label) _##label
#endif

	.extern C(snd_scaletable)
	.extern C(paintbuffer)
	.extern C(snd_linear_count)
	.extern C(snd_p)
	.extern C(snd_vol)
	.extern C(snd_out)
	.extern C(vright)
	.extern C(vup)
	.extern C(vpn)
	.extern C(BOPS_Error)

//
// !!! note that this file must match the corresponding C structures at all
// times !!!
//

// plane_t structure
// !!! if this is changed, it must be changed in model.h too !!!
// !!! if the size of this is changed, the array lookup in SV_HullPointContents
//     must be changed too !!!
#define pl_normal	0
#define pl_dist		12
#define pl_type		16
#define pl_signbits	17
#define pl_pad		18
#define pl_size		20

// hull_t structure
// !!! if this is changed, it must be changed in model.h too !!!
#define	hu_clipnodes		0
#define	hu_planes			4
#define	hu_firstclipnode	8
#define	hu_lastclipnode		12
#define	hu_clip_mins		16
#define	hu_clip_maxs		28
#define hu_size  			40

// dnode_t structure
// !!! if this is changed, it must be changed in bspfile.h too !!!
#define	nd_planenum		0
#define	nd_children		4
#define	nd_mins			8
#define	nd_maxs			20
#define	nd_firstface	32
#define	nd_numfaces		36
#define nd_size			40

// sfxcache_t structure
// !!! if this is changed, it much be changed in sound.h too !!!
#define sfxc_length		0
#define sfxc_loopstart	4
#define sfxc_speed		8
#define sfxc_width		12
#define sfxc_stereo		16
#define sfxc_data		20

// channel_t structure
// !!! if this is changed, it much be changed in sound.h too !!!
#define ch_sfx			0
#define ch_leftvol		4
#define ch_rightvol		8
#define ch_end			12
#define ch_pos			16
#define ch_looping		20
#define ch_entnum		24
#define ch_entchannel	28
#define ch_origin		32
#define ch_dist_mult	44
#define ch_master_vol	48
#define ch_size			52

// portable_samplepair_t structure
// !!! if this is changed, it much be changed in sound.h too !!!
#define psp_left		0
#define psp_right		4
#define psp_size		8


//
// !!! note that this file must match the corresponding C structures at all
// times !!!
//

// espan_t structure
// !!! if this is changed, it must be changed in r_shared.h too !!!
#define espan_t_u    	0
#define espan_t_v	    4
#define espan_t_count   8
#define espan_t_pnext	12
#define espan_t_size    16

// sspan_t structure
// !!! if this is changed, it must be changed in d_local.h too !!!
#define sspan_t_u    	0
#define sspan_t_v	    4
#define sspan_t_count   8
#define sspan_t_size    12

// spanpackage_t structure
// !!! if this is changed, it must be changed in d_polyset.c too !!!
#define spanpackage_t_pdest				0
#define spanpackage_t_pz				4
#define spanpackage_t_count				8
#define spanpackage_t_ptex				12
#define spanpackage_t_sfrac				16
#define spanpackage_t_tfrac				20
#define spanpackage_t_light				24
#define spanpackage_t_zi				28
#define spanpackage_t_size				32 

// edge_t structure
// !!! if this is changed, it must be changed in r_shared.h too !!!
#define et_u			0
#define et_u_step		4
#define et_prev			8
#define et_next			12
#define et_surfs		16
#define et_nextremove	20
#define et_nearzi		24
#define et_owner		28
#define et_size			32

// surf_t structure
// !!! if this is changed, it must be changed in r_shared.h too !!!
#define SURF_T_SHIFT	6
#define st_next			0
#define st_prev			4
#define st_spans		8
#define st_key			12
#define st_last_u		16
#define st_spanstate	20
#define st_flags		24
#define st_data			28
#define st_entity		32
#define st_nearzi		36
#define st_insubmodel	40
#define st_d_ziorigin	44
#define st_d_zistepu	48
#define st_d_zistepv	52
#define st_pad			56
#define st_size			64

// clipplane_t structure
// !!! if this is changed, it must be changed in r_local.h too !!!
#define cp_normal		0
#define cp_dist			12
#define cp_next			16
#define cp_leftedge		20
#define cp_rightedge	21
#define cp_reserved		22
#define cp_size			24

// medge_t structure
// !!! if this is changed, it must be changed in model.h too !!!
#define me_v				0
#define me_cachededgeoffset	4
#define me_size				8

// mvertex_t structure
// !!! if this is changed, it must be changed in model.h too !!!
#define mv_position		0
#define mv_size			12

// refdef_t structure
// !!! if this is changed, it must be changed in render.h too !!!
#define rd_vrect					0
#define rd_aliasvrect				20
#define rd_vrectright				40
#define rd_vrectbottom				44
#define rd_aliasvrectright			48
#define rd_aliasvrectbottom			52
#define rd_vrectrightedge			56
#define rd_fvrectx					60
#define rd_fvrecty					64
#define rd_fvrectx_adj				68
#define rd_fvrecty_adj				72
#define rd_vrect_x_adj_shift20		76
#define rd_vrectright_adj_shift20	80
#define rd_fvrectright_adj			84
#define rd_fvrectbottom_adj			88
#define rd_fvrectright				92
#define rd_fvrectbottom				96
#define rd_horizontalFieldOfView	100
#define rd_xOrigin					104
#define rd_yOrigin					108
#define rd_vieworg					112
#define rd_viewangles				124
#define rd_ambientlight				136
#define rd_size						140

// mtriangle_t structure
// !!! if this is changed, it must be changed in model.h too !!!
#define mtri_facesfront		0
#define mtri_vertindex		4
#define mtri_size			16	// !!! if this changes, array indexing in !!!
								// !!! d_polysa.s must be changed to match !!!
#define mtri_shift			4

#endif
