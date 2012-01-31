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
#ifndef __ASM_I386__
#define __ASM_I386__

#ifdef ELF
#define C(label) label
#else
#define C(label) _##label
#endif


//#define GLQUAKE	1

#if defined(_WIN32) && !defined(WINDED)

#if defined(_M_IX86)
#define __i386__	1
#endif

#endif

#ifdef __i386__
#define id386	1
#else
#define id386	0
#endif

// !!! must be kept the same as in d_iface.h !!!
#define TRANSPARENT_COLOR	255

#ifndef GLQUAKE
	.extern C(d_zistepu)
	.extern C(d_pzbuffer)
	.extern C(d_zistepv)
	.extern C(d_zrowbytes)
	.extern C(d_ziorigin)
	.extern C(r_turb_s)
	.extern C(r_turb_t)
	.extern C(r_turb_pdest)
	.extern C(r_turb_spancount)
	.extern C(r_turb_turb)
	.extern C(r_turb_pbase)
	.extern C(r_turb_sstep)
	.extern C(r_turb_tstep)
	.extern	C(r_bmodelactive)
	.extern	C(d_sdivzstepu)
	.extern	C(d_tdivzstepu)
	.extern	C(d_sdivzstepv)
	.extern	C(d_tdivzstepv)
	.extern	C(d_sdivzorigin)
	.extern	C(d_tdivzorigin)
	.extern	C(sadjust)
	.extern	C(tadjust)
	.extern	C(bbextents)
	.extern	C(bbextentt)
	.extern	C(cacheblock)
	.extern	C(d_viewbuffer)
	.extern	C(cachewidth)
	.extern	C(d_pzbuffer)
	.extern	C(d_zrowbytes)
	.extern	C(d_zwidth)
	.extern C(d_scantable)
	.extern C(r_lightptr)
	.extern C(r_numvblocks)
	.extern C(prowdestbase)
	.extern C(pbasesource)
	.extern C(r_lightwidth)
	.extern C(lightright)
	.extern C(lightrightstep)
	.extern C(lightdeltastep)
	.extern C(lightdelta)
	.extern C(lightright)
	.extern C(lightdelta)
	.extern C(sourcetstep)
	.extern C(surfrowbytes)
	.extern C(lightrightstep)
	.extern C(lightdeltastep)
	.extern C(r_sourcemax)
	.extern C(r_stepback)
	.extern C(colormap)
	.extern C(blocksize)
	.extern C(sourcesstep)
	.extern C(lightleft)
	.extern C(blockdivshift)
	.extern C(blockdivmask)
	.extern C(lightleftstep)
	.extern C(r_origin)
	.extern C(r_ppn)
	.extern C(r_pup)
	.extern C(r_pright)
	.extern C(ycenter)
	.extern C(xcenter)
	.extern C(d_vrectbottom_particle)
	.extern C(d_vrectright_particle)
	.extern C(d_vrecty)
	.extern C(d_vrectx)
	.extern C(d_pix_shift)
	.extern C(d_pix_min)
	.extern C(d_pix_max)
	.extern C(d_y_aspect_shift)
	.extern C(screenwidth)
	.extern C(r_leftclipped)
	.extern C(r_leftenter)
	.extern C(r_rightclipped)
	.extern C(r_rightenter)
	.extern C(modelorg)
	.extern C(xscale)
	.extern C(r_refdef)
	.extern C(yscale)
	.extern C(r_leftexit)
	.extern C(r_rightexit)
	.extern C(r_lastvertvalid)
	.extern C(cacheoffset)
	.extern C(newedges)
	.extern C(removeedges)
	.extern C(r_pedge)
	.extern C(r_framecount)
	.extern C(r_u1)
	.extern C(r_emitted)
	.extern C(edge_p)
	.extern C(surface_p)
	.extern C(surfaces)
	.extern C(r_lzi1)
	.extern C(r_v1)
	.extern C(r_ceilv1)
	.extern C(r_nearzi)
	.extern C(r_nearzionly)
	.extern C(edge_aftertail)
	.extern C(edge_tail)
	.extern C(current_iv)
	.extern C(edge_head_u_shift20)
	.extern C(span_p)
	.extern C(edge_head)
	.extern C(fv)
	.extern C(edge_tail_u_shift20)
	.extern C(r_apverts)
	.extern C(r_anumverts)
	.extern C(aliastransform)
	.extern C(r_avertexnormals)
	.extern C(r_plightvec)
	.extern C(r_ambientlight)
	.extern C(r_shadelight)
	.extern C(aliasxcenter)
	.extern C(aliasycenter)
	.extern C(a_sstepxfrac)
	.extern C(r_affinetridesc)
	.extern C(acolormap)
	.extern C(d_pcolormap)
	.extern C(r_affinetridesc)
	.extern C(d_sfrac)
	.extern C(d_ptex)
	.extern C(d_pedgespanpackage)
	.extern C(d_tfrac)
	.extern C(d_light)
	.extern C(d_zi)
	.extern C(d_pdest)
	.extern C(d_pz)
	.extern C(d_aspancount)
	.extern C(erroradjustup)
	.extern C(errorterm)
	.extern C(d_xdenom)
	.extern C(r_p0)
	.extern C(r_p1)
	.extern C(r_p2)
	.extern C(a_tstepxfrac)
	.extern C(r_sstepx)
	.extern C(r_tstepx)
	.extern C(a_ststepxwhole)
	.extern C(zspantable)
	.extern C(skintable)
	.extern C(r_zistepx)
	.extern C(erroradjustdown)
	.extern C(d_countextrastep)
	.extern C(ubasestep)
	.extern C(a_ststepxwhole)
	.extern C(a_tstepxfrac)
	.extern C(r_lstepx)
	.extern C(a_spans)
	.extern C(erroradjustdown)
	.extern C(d_pdestextrastep)
	.extern C(d_pzextrastep)
	.extern C(d_sfracextrastep)
	.extern C(d_ptexextrastep)
	.extern C(d_countextrastep)
	.extern C(d_tfracextrastep)
	.extern C(d_lightextrastep)
	.extern C(d_ziextrastep)
	.extern C(d_pdestbasestep)
	.extern C(d_pzbasestep)
	.extern C(d_sfracbasestep)
	.extern C(d_ptexbasestep)
	.extern C(ubasestep)
	.extern C(d_tfracbasestep)
	.extern C(d_lightbasestep)
	.extern C(d_zibasestep)
	.extern C(zspantable)
	.extern C(r_lstepy)
	.extern C(r_sstepy)
	.extern C(r_tstepy)
	.extern C(r_zistepy)
	.extern C(D_PolysetSetEdgeTable)
	.extern C(D_RasterizeAliasPolySmooth)

	.extern float_point5
	.extern Float2ToThe31nd
	.extern izistep
	.extern izi
	.extern FloatMinus2ToThe31nd
	.extern float_1
	.extern float_particle_z_clip
	.extern float_minus_1
	.extern float_0
	.extern fp_16
	.extern fp_64k
	.extern fp_1m
	.extern fp_1m_minus_1
	.extern fp_8 
	.extern entryvec_table
	.extern advancetable
	.extern sstep
	.extern tstep
	.extern pspantemp
	.extern counttemp
	.extern jumptemp
	.extern reciprocal_table
	.extern DP_Count
	.extern DP_u
	.extern DP_v
	.extern DP_32768
	.extern DP_Color
	.extern DP_Pix
	.extern DP_EntryTable
	.extern	pbase
	.extern s
	.extern t
	.extern sfracf
	.extern tfracf
	.extern snext
	.extern tnext
	.extern	spancountminus1
	.extern zi16stepu
	.extern sdivz16stepu
	.extern tdivz16stepu
	.extern	zi8stepu
	.extern sdivz8stepu
	.extern tdivz8stepu
	.extern reciprocal_table_16
	.extern entryvec_table_16
	.extern ceil_cw
	.extern single_cw
	.extern fp_64kx64k
	.extern pz
	.extern spr8entryvec_table
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

// !!! if this is changed, it must be changed in r_local.h too !!!
#define	NEAR_CLIP	0.01

// !!! if this is changed, it must be changed in r_local.h too !!!
#define	CYCLE	128

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
