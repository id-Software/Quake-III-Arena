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
// Rafael particles
// cg_particles.c  

#include "cg_local.h"

//#define WOLF_PARTICLES

#define BLOODRED	2
#define EMISIVEFADE	3
#define GREY75		4

typedef struct particle_s
{
	struct particle_s	*next;

	float		time;
	float		endtime;

	vec3_t		org;
	vec3_t		vel;
	vec3_t		accel;
	int			color;
	float		colorvel;
	float		alpha;
	float		alphavel;
	int			type;
	qhandle_t	pshader;
	
	float		height;
	float		width;
				
	float		endheight;
	float		endwidth;
	
	float		start;
	float		end;

	float		startfade;
	qboolean	rotate;
	int			snum;
	
	qboolean	link;

	// Ridah
	int			shaderAnim;
	int			roll;

	int			accumroll;

} cparticle_t;

typedef enum
{
	P_NONE,
	P_WEATHER,
	P_FLAT,
	P_SMOKE,
	P_ROTATE,
	P_WEATHER_TURBULENT,
	P_ANIM,	// Ridah
	P_BAT,
	P_BLEED,
	P_FLAT_SCALEUP,
	P_FLAT_SCALEUP_FADE,
	P_WEATHER_FLURRY,
	P_SMOKE_IMPACT,
	P_BUBBLE,
	P_BUBBLE_TURBULENT,
	P_SPRITE
} particle_type_t;

#define	MAX_SHADER_ANIMS		32
#define	MAX_SHADER_ANIM_FRAMES	64

#ifndef WOLF_PARTICLES
static char *shaderAnimNames[MAX_SHADER_ANIMS] = {
	"explode1",
	NULL
};
static qhandle_t shaderAnims[MAX_SHADER_ANIMS][MAX_SHADER_ANIM_FRAMES];
static int	shaderAnimCounts[MAX_SHADER_ANIMS] = {
	23
};
static float	shaderAnimSTRatio[MAX_SHADER_ANIMS] = {
	1.0f
};
static int	numShaderAnims;
// done.
#else
static char *shaderAnimNames[MAX_SHADER_ANIMS] = {
	"explode1",
	"blacksmokeanim",
	"twiltb2",
	"expblue",
	"blacksmokeanimb",	// uses 'explode1' sequence
	"blood",
	NULL
};
static qhandle_t shaderAnims[MAX_SHADER_ANIMS][MAX_SHADER_ANIM_FRAMES];
static int	shaderAnimCounts[MAX_SHADER_ANIMS] = {
	23,
	25,
	45,
	25,
	23,
	5,
};
static float	shaderAnimSTRatio[MAX_SHADER_ANIMS] = {
	1.405f,
	1.0f,
	1.0f,
	1.0f,
	1.0f,
	1.0f,
};
#endif

#define		PARTICLE_GRAVITY	40

#ifdef WOLF_PARTICLES
#define		MAX_PARTICLES	1024 * 8
#else
#define		MAX_PARTICLES 1024
#endif

cparticle_t	*active_particles, *free_particles;
cparticle_t	particles[MAX_PARTICLES];
int		cl_numparticles = MAX_PARTICLES;

qboolean		initparticles = qfalse;
vec3_t			vforward, vright, vup;
vec3_t			rforward, rright, rup;

float			oldtime;

/*
===============
CL_ClearParticles
===============
*/
void CG_ClearParticles (void)
{
	int		i;

	memset( particles, 0, sizeof(particles) );

	free_particles = &particles[0];
	active_particles = NULL;

	for (i=0 ;i<cl_numparticles ; i++)
	{
		particles[i].next = &particles[i+1];
		particles[i].type = 0;
	}
	particles[cl_numparticles-1].next = NULL;

	oldtime = cg.time;

	// Ridah, init the shaderAnims
	for (i=0; shaderAnimNames[i]; i++) {
		int j;

		for (j=0; j<shaderAnimCounts[i]; j++) {
			shaderAnims[i][j] = trap_R_RegisterShader( va("%s%i", shaderAnimNames[i], j+1) );
		}
	}
	numShaderAnims = i;
	// done.

	initparticles = qtrue;
}


/*
=====================
CG_AddParticleToScene
=====================
*/
void CG_AddParticleToScene (cparticle_t *p, vec3_t org, float alpha)
{

	vec3_t		point;
	polyVert_t	verts[4];
	float		width;
	float		height;
	float		time, time2;
	float		ratio;
	float		invratio;
	vec3_t		color;
	polyVert_t	TRIverts[3];
	vec3_t		rright2, rup2;

	if (p->type == P_WEATHER || p->type == P_WEATHER_TURBULENT || p->type == P_WEATHER_FLURRY
		|| p->type == P_BUBBLE || p->type == P_BUBBLE_TURBULENT)
	{// create a front facing polygon
			
		if (p->type != P_WEATHER_FLURRY)
		{
			if (p->type == P_BUBBLE || p->type == P_BUBBLE_TURBULENT)
			{
				if (org[2] > p->end)			
				{	
					p->time = cg.time;	
					VectorCopy (org, p->org); // Ridah, fixes rare snow flakes that flicker on the ground
									
					p->org[2] = ( p->start + crandom () * 4 );
					
					
					if (p->type == P_BUBBLE_TURBULENT)
					{
						p->vel[0] = crandom() * 4;
						p->vel[1] = crandom() * 4;
					}
				
				}
			}
			else
			{
				if (org[2] < p->end)			
				{	
					p->time = cg.time;	
					VectorCopy (org, p->org); // Ridah, fixes rare snow flakes that flicker on the ground
									
					while (p->org[2] < p->end) 
					{
						p->org[2] += (p->start - p->end); 
					}
					
					
					if (p->type == P_WEATHER_TURBULENT)
					{
						p->vel[0] = crandom() * 16;
						p->vel[1] = crandom() * 16;
					}
				
				}
			}
			

			// Rafael snow pvs check
			if (!p->link)
				return;

			p->alpha = 1;
		}
		
		// Ridah, had to do this or MAX_POLYS is being exceeded in village1.bsp
		if (Distance( cg.snap->ps.origin, org ) > 1024) {
			return;
		}
		// done.
	
		if (p->type == P_BUBBLE || p->type == P_BUBBLE_TURBULENT)
		{
			VectorMA (org, -p->height, vup, point);	
			VectorMA (point, -p->width, vright, point);	
			VectorCopy (point, verts[0].xyz);	
			verts[0].st[0] = 0;	
			verts[0].st[1] = 0;	
			verts[0].modulate[0] = 255;	
			verts[0].modulate[1] = 255;	
			verts[0].modulate[2] = 255;	
			verts[0].modulate[3] = 255 * p->alpha;	

			VectorMA (org, -p->height, vup, point);	
			VectorMA (point, p->width, vright, point);	
			VectorCopy (point, verts[1].xyz);	
			verts[1].st[0] = 0;	
			verts[1].st[1] = 1;	
			verts[1].modulate[0] = 255;	
			verts[1].modulate[1] = 255;	
			verts[1].modulate[2] = 255;	
			verts[1].modulate[3] = 255 * p->alpha;	

			VectorMA (org, p->height, vup, point);	
			VectorMA (point, p->width, vright, point);	
			VectorCopy (point, verts[2].xyz);	
			verts[2].st[0] = 1;	
			verts[2].st[1] = 1;	
			verts[2].modulate[0] = 255;	
			verts[2].modulate[1] = 255;	
			verts[2].modulate[2] = 255;	
			verts[2].modulate[3] = 255 * p->alpha;	

			VectorMA (org, p->height, vup, point);	
			VectorMA (point, -p->width, vright, point);	
			VectorCopy (point, verts[3].xyz);	
			verts[3].st[0] = 1;	
			verts[3].st[1] = 0;	
			verts[3].modulate[0] = 255;	
			verts[3].modulate[1] = 255;	
			verts[3].modulate[2] = 255;	
			verts[3].modulate[3] = 255 * p->alpha;	
		}
		else
		{
			VectorMA (org, -p->height, vup, point);	
			VectorMA (point, -p->width, vright, point);	
			VectorCopy( point, TRIverts[0].xyz );
			TRIverts[0].st[0] = 1;
			TRIverts[0].st[1] = 0;
			TRIverts[0].modulate[0] = 255;
			TRIverts[0].modulate[1] = 255;
			TRIverts[0].modulate[2] = 255;
			TRIverts[0].modulate[3] = 255 * p->alpha;	

			VectorMA (org, p->height, vup, point);	
			VectorMA (point, -p->width, vright, point);	
			VectorCopy (point, TRIverts[1].xyz);	
			TRIverts[1].st[0] = 0;
			TRIverts[1].st[1] = 0;
			TRIverts[1].modulate[0] = 255;
			TRIverts[1].modulate[1] = 255;
			TRIverts[1].modulate[2] = 255;
			TRIverts[1].modulate[3] = 255 * p->alpha;	

			VectorMA (org, p->height, vup, point);	
			VectorMA (point, p->width, vright, point);	
			VectorCopy (point, TRIverts[2].xyz);	
			TRIverts[2].st[0] = 0;
			TRIverts[2].st[1] = 1;
			TRIverts[2].modulate[0] = 255;
			TRIverts[2].modulate[1] = 255;
			TRIverts[2].modulate[2] = 255;
			TRIverts[2].modulate[3] = 255 * p->alpha;	
		}
	
	}
	else if (p->type == P_SPRITE)
	{
		vec3_t	rr, ru;
		vec3_t	rotate_ang;

#ifdef WOLF_PARTICLES
		VectorSet (color, 1.0, 1.0, 1.0);
#else
		VectorSet (color, 1.0, 1.0, 0.5);
#endif
		time = cg.time - p->time;
		time2 = p->endtime - p->time;
		ratio = time / time2;

		width = p->width + ( ratio * ( p->endwidth - p->width) );
		height = p->height + ( ratio * ( p->endheight - p->height) );

		if (p->roll) {
			vectoangles( cg.refdef.viewaxis[0], rotate_ang );
			rotate_ang[ROLL] += p->roll;
			AngleVectors ( rotate_ang, NULL, rr, ru);
		}

		if (p->roll) {
			VectorMA (org, -height, ru, point);	
			VectorMA (point, -width, rr, point);	
		} else {
			VectorMA (org, -height, vup, point);	
			VectorMA (point, -width, vright, point);	
		}
		VectorCopy (point, verts[0].xyz);	
		verts[0].st[0] = 0;	
		verts[0].st[1] = 0;	
		verts[0].modulate[0] = 255;	
		verts[0].modulate[1] = 255;	
		verts[0].modulate[2] = 255;	
		verts[0].modulate[3] = 255;

		if (p->roll) {
			VectorMA (point, 2*height, ru, point);	
		} else {
			VectorMA (point, 2*height, vup, point);	
		}
		VectorCopy (point, verts[1].xyz);	
		verts[1].st[0] = 0;	
		verts[1].st[1] = 1;	
		verts[1].modulate[0] = 255;	
		verts[1].modulate[1] = 255;	
		verts[1].modulate[2] = 255;	
		verts[1].modulate[3] = 255;	

		if (p->roll) {
			VectorMA (point, 2*width, rr, point);	
		} else {
			VectorMA (point, 2*width, vright, point);	
		}
		VectorCopy (point, verts[2].xyz);	
		verts[2].st[0] = 1;	
		verts[2].st[1] = 1;	
		verts[2].modulate[0] = 255;	
		verts[2].modulate[1] = 255;	
		verts[2].modulate[2] = 255;	
		verts[2].modulate[3] = 255;	

		if (p->roll) {
			VectorMA (point, -2*height, ru, point);	
		} else {
			VectorMA (point, -2*height, vup, point);	
		}
		VectorCopy (point, verts[3].xyz);	
		verts[3].st[0] = 1;	
		verts[3].st[1] = 0;	
		verts[3].modulate[0] = 255;	
		verts[3].modulate[1] = 255;	
		verts[3].modulate[2] = 255;	
		verts[3].modulate[3] = 255;	
	}
	else if (p->type == P_SMOKE || p->type == P_SMOKE_IMPACT)
	{// create a front rotating facing polygon

		if ( p->type == P_SMOKE_IMPACT && Distance( cg.snap->ps.origin, org ) > 1024) {
			return;
		}

		if (p->color == BLOODRED)
			VectorSet (color, 0.22f, 0.0f, 0.0f);
		else if (p->color == GREY75)
		{
			float	len;
			float	greyit;
			float	val;
			len = Distance (cg.snap->ps.origin, org);
			if (!len)
				len = 1;

			val = 4096/len;
			greyit = 0.25 * val;
			if (greyit > 0.5)
				greyit = 0.5;

			VectorSet (color, greyit, greyit, greyit);
		}
		else
			VectorSet (color, 1.0, 1.0, 1.0);

		time = cg.time - p->time;
		time2 = p->endtime - p->time;
		ratio = time / time2;
		
		if (cg.time > p->startfade)
		{
			invratio = 1 - ( (cg.time - p->startfade) / (p->endtime - p->startfade) );

			if (p->color == EMISIVEFADE)
			{
				float fval;
				fval = (invratio * invratio);
				if (fval < 0)
					fval = 0;
				VectorSet (color, fval , fval , fval );
			}
			invratio *= p->alpha;
		}
		else 
			invratio = 1 * p->alpha;

		if ( cgs.glconfig.hardwareType == GLHW_RAGEPRO )
			invratio = 1;

		if (invratio > 1)
			invratio = 1;
	
		width = p->width + ( ratio * ( p->endwidth - p->width) );
		height = p->height + ( ratio * ( p->endheight - p->height) );

		if (p->type != P_SMOKE_IMPACT)
		{
			vec3_t temp;

			vectoangles (rforward, temp);
			p->accumroll += p->roll;
			temp[ROLL] += p->accumroll * 0.1;
			AngleVectors ( temp, NULL, rright2, rup2);
		}
		else
		{
			VectorCopy (rright, rright2);
			VectorCopy (rup, rup2);
		}
		
		if (p->rotate)
		{
			VectorMA (org, -height, rup2, point);	
			VectorMA (point, -width, rright2, point);	
		}
		else
		{
			VectorMA (org, -p->height, vup, point);	
			VectorMA (point, -p->width, vright, point);	
		}
		VectorCopy (point, verts[0].xyz);	
		verts[0].st[0] = 0;	
		verts[0].st[1] = 0;	
		verts[0].modulate[0] = 255 * color[0];	
		verts[0].modulate[1] = 255 * color[1];	
		verts[0].modulate[2] = 255 * color[2];	
		verts[0].modulate[3] = 255 * invratio;	

		if (p->rotate)
		{
			VectorMA (org, -height, rup2, point);	
			VectorMA (point, width, rright2, point);	
		}
		else
		{
			VectorMA (org, -p->height, vup, point);	
			VectorMA (point, p->width, vright, point);	
		}
		VectorCopy (point, verts[1].xyz);	
		verts[1].st[0] = 0;	
		verts[1].st[1] = 1;	
		verts[1].modulate[0] = 255 * color[0];	
		verts[1].modulate[1] = 255 * color[1];	
		verts[1].modulate[2] = 255 * color[2];	
		verts[1].modulate[3] = 255 * invratio;	

		if (p->rotate)
		{
			VectorMA (org, height, rup2, point);	
			VectorMA (point, width, rright2, point);	
		}
		else
		{
			VectorMA (org, p->height, vup, point);	
			VectorMA (point, p->width, vright, point);	
		}
		VectorCopy (point, verts[2].xyz);	
		verts[2].st[0] = 1;	
		verts[2].st[1] = 1;	
		verts[2].modulate[0] = 255 * color[0];	
		verts[2].modulate[1] = 255 * color[1];	
		verts[2].modulate[2] = 255 * color[2];	
		verts[2].modulate[3] = 255 * invratio;	

		if (p->rotate)
		{
			VectorMA (org, height, rup2, point);	
			VectorMA (point, -width, rright2, point);	
		}
		else
		{
			VectorMA (org, p->height, vup, point);	
			VectorMA (point, -p->width, vright, point);	
		}
		VectorCopy (point, verts[3].xyz);	
		verts[3].st[0] = 1;	
		verts[3].st[1] = 0;	
		verts[3].modulate[0] = 255 * color[0];	
		verts[3].modulate[1] = 255 * color[1];	
		verts[3].modulate[2] = 255 * color[2];	
		verts[3].modulate[3] = 255  * invratio;	
		
	}
	else if (p->type == P_BLEED)
	{
		vec3_t	rr, ru;
		vec3_t	rotate_ang;
		float	alpha;

		alpha = p->alpha;
		
		if ( cgs.glconfig.hardwareType == GLHW_RAGEPRO )
			alpha = 1;

		if (p->roll) 
		{
			vectoangles( cg.refdef.viewaxis[0], rotate_ang );
			rotate_ang[ROLL] += p->roll;
			AngleVectors ( rotate_ang, NULL, rr, ru);
		}
		else
		{
			VectorCopy (vup, ru);
			VectorCopy (vright, rr);
		}

		VectorMA (org, -p->height, ru, point);	
		VectorMA (point, -p->width, rr, point);	
		VectorCopy (point, verts[0].xyz);	
		verts[0].st[0] = 0;	
		verts[0].st[1] = 0;	
		verts[0].modulate[0] = 111;	
		verts[0].modulate[1] = 19;	
		verts[0].modulate[2] = 9;	
		verts[0].modulate[3] = 255 * alpha;	

		VectorMA (org, -p->height, ru, point);	
		VectorMA (point, p->width, rr, point);	
		VectorCopy (point, verts[1].xyz);	
		verts[1].st[0] = 0;	
		verts[1].st[1] = 1;	
		verts[1].modulate[0] = 111;	
		verts[1].modulate[1] = 19;	
		verts[1].modulate[2] = 9;	
		verts[1].modulate[3] = 255 * alpha;	

		VectorMA (org, p->height, ru, point);	
		VectorMA (point, p->width, rr, point);	
		VectorCopy (point, verts[2].xyz);	
		verts[2].st[0] = 1;	
		verts[2].st[1] = 1;	
		verts[2].modulate[0] = 111;	
		verts[2].modulate[1] = 19;	
		verts[2].modulate[2] = 9;	
		verts[2].modulate[3] = 255 * alpha;	

		VectorMA (org, p->height, ru, point);	
		VectorMA (point, -p->width, rr, point);	
		VectorCopy (point, verts[3].xyz);	
		verts[3].st[0] = 1;	
		verts[3].st[1] = 0;	
		verts[3].modulate[0] = 111;	
		verts[3].modulate[1] = 19;	
		verts[3].modulate[2] = 9;	
		verts[3].modulate[3] = 255 * alpha;	

	}
	else if (p->type == P_FLAT_SCALEUP)
	{
		float width, height;
		float sinR, cosR;

		if (p->color == BLOODRED)
			VectorSet (color, 1, 1, 1);
		else
			VectorSet (color, 0.5, 0.5, 0.5);
		
		time = cg.time - p->time;
		time2 = p->endtime - p->time;
		ratio = time / time2;

		width = p->width + ( ratio * ( p->endwidth - p->width) );
		height = p->height + ( ratio * ( p->endheight - p->height) );

		if (width > p->endwidth)
			width = p->endwidth;

		if (height > p->endheight)
			height = p->endheight;

		sinR = height * sin(DEG2RAD(p->roll)) * sqrt(2);
		cosR = width * cos(DEG2RAD(p->roll)) * sqrt(2);

		VectorCopy (org, verts[0].xyz);	
		verts[0].xyz[0] -= sinR;
		verts[0].xyz[1] -= cosR;
		verts[0].st[0] = 0;	
		verts[0].st[1] = 0;	
		verts[0].modulate[0] = 255 * color[0];	
		verts[0].modulate[1] = 255 * color[1];	
		verts[0].modulate[2] = 255 * color[2];	
		verts[0].modulate[3] = 255;	

		VectorCopy (org, verts[1].xyz);	
		verts[1].xyz[0] -= cosR;	
		verts[1].xyz[1] += sinR;	
		verts[1].st[0] = 0;	
		verts[1].st[1] = 1;	
		verts[1].modulate[0] = 255 * color[0];	
		verts[1].modulate[1] = 255 * color[1];	
		verts[1].modulate[2] = 255 * color[2];	
		verts[1].modulate[3] = 255;	

		VectorCopy (org, verts[2].xyz);	
		verts[2].xyz[0] += sinR;	
		verts[2].xyz[1] += cosR;	
		verts[2].st[0] = 1;	
		verts[2].st[1] = 1;	
		verts[2].modulate[0] = 255 * color[0];	
		verts[2].modulate[1] = 255 * color[1];	
		verts[2].modulate[2] = 255 * color[2];	
		verts[2].modulate[3] = 255;	

		VectorCopy (org, verts[3].xyz);	
		verts[3].xyz[0] += cosR;	
		verts[3].xyz[1] -= sinR;	
		verts[3].st[0] = 1;	
		verts[3].st[1] = 0;	
		verts[3].modulate[0] = 255 * color[0];	
		verts[3].modulate[1] = 255 * color[1];	
		verts[3].modulate[2] = 255 * color[2];	
		verts[3].modulate[3] = 255;		
	}
	else if (p->type == P_FLAT)
	{

		VectorCopy (org, verts[0].xyz);	
		verts[0].xyz[0] -= p->height;	
		verts[0].xyz[1] -= p->width;	
		verts[0].st[0] = 0;	
		verts[0].st[1] = 0;	
		verts[0].modulate[0] = 255;	
		verts[0].modulate[1] = 255;	
		verts[0].modulate[2] = 255;	
		verts[0].modulate[3] = 255;	

		VectorCopy (org, verts[1].xyz);	
		verts[1].xyz[0] -= p->height;	
		verts[1].xyz[1] += p->width;	
		verts[1].st[0] = 0;	
		verts[1].st[1] = 1;	
		verts[1].modulate[0] = 255;	
		verts[1].modulate[1] = 255;	
		verts[1].modulate[2] = 255;	
		verts[1].modulate[3] = 255;	

		VectorCopy (org, verts[2].xyz);	
		verts[2].xyz[0] += p->height;	
		verts[2].xyz[1] += p->width;	
		verts[2].st[0] = 1;	
		verts[2].st[1] = 1;	
		verts[2].modulate[0] = 255;	
		verts[2].modulate[1] = 255;	
		verts[2].modulate[2] = 255;	
		verts[2].modulate[3] = 255;	

		VectorCopy (org, verts[3].xyz);	
		verts[3].xyz[0] += p->height;	
		verts[3].xyz[1] -= p->width;	
		verts[3].st[0] = 1;	
		verts[3].st[1] = 0;	
		verts[3].modulate[0] = 255;	
		verts[3].modulate[1] = 255;	
		verts[3].modulate[2] = 255;	
		verts[3].modulate[3] = 255;	

	}
	// Ridah
	else if (p->type == P_ANIM) {
		vec3_t	rr, ru;
		vec3_t	rotate_ang;
		int i, j;

		time = cg.time - p->time;
		time2 = p->endtime - p->time;
		ratio = time / time2;
		if (ratio >= 1.0f) {
			ratio = 0.9999f;
		}

		width = p->width + ( ratio * ( p->endwidth - p->width) );
		height = p->height + ( ratio * ( p->endheight - p->height) );

		// if we are "inside" this sprite, don't draw
		if (Distance( cg.snap->ps.origin, org ) < width/1.5) {
			return;
		}

		i = p->shaderAnim;
		j = (int)floor(ratio * shaderAnimCounts[p->shaderAnim]);
		p->pshader = shaderAnims[i][j];

		if (p->roll) {
			vectoangles( cg.refdef.viewaxis[0], rotate_ang );
			rotate_ang[ROLL] += p->roll;
			AngleVectors ( rotate_ang, NULL, rr, ru);
		}

		if (p->roll) {
			VectorMA (org, -height, ru, point);	
			VectorMA (point, -width, rr, point);	
		} else {
			VectorMA (org, -height, vup, point);	
			VectorMA (point, -width, vright, point);	
		}
		VectorCopy (point, verts[0].xyz);	
		verts[0].st[0] = 0;	
		verts[0].st[1] = 0;	
		verts[0].modulate[0] = 255;	
		verts[0].modulate[1] = 255;	
		verts[0].modulate[2] = 255;	
		verts[0].modulate[3] = 255;

		if (p->roll) {
			VectorMA (point, 2*height, ru, point);	
		} else {
			VectorMA (point, 2*height, vup, point);	
		}
		VectorCopy (point, verts[1].xyz);	
		verts[1].st[0] = 0;	
		verts[1].st[1] = 1;	
		verts[1].modulate[0] = 255;	
		verts[1].modulate[1] = 255;	
		verts[1].modulate[2] = 255;	
		verts[1].modulate[3] = 255;	

		if (p->roll) {
			VectorMA (point, 2*width, rr, point);	
		} else {
			VectorMA (point, 2*width, vright, point);	
		}
		VectorCopy (point, verts[2].xyz);	
		verts[2].st[0] = 1;	
		verts[2].st[1] = 1;	
		verts[2].modulate[0] = 255;	
		verts[2].modulate[1] = 255;	
		verts[2].modulate[2] = 255;	
		verts[2].modulate[3] = 255;	

		if (p->roll) {
			VectorMA (point, -2*height, ru, point);	
		} else {
			VectorMA (point, -2*height, vup, point);	
		}
		VectorCopy (point, verts[3].xyz);	
		verts[3].st[0] = 1;	
		verts[3].st[1] = 0;	
		verts[3].modulate[0] = 255;	
		verts[3].modulate[1] = 255;	
		verts[3].modulate[2] = 255;	
		verts[3].modulate[3] = 255;	
	}
	// done.
	
	if (!p->pshader) {
// (SA) temp commented out for DM
//		CG_Printf ("CG_AddParticleToScene type %d p->pshader == ZERO\n", p->type);
		return;
	}

	if (p->type == P_WEATHER || p->type == P_WEATHER_TURBULENT || p->type == P_WEATHER_FLURRY)
		trap_R_AddPolyToScene( p->pshader, 3, TRIverts );
	else
		trap_R_AddPolyToScene( p->pshader, 4, verts );

}

// Ridah, made this static so it doesn't interfere with other files
static float roll = 0.0;

/*
===============
CG_AddParticles
===============
*/
void CG_AddParticles (void)
{
	cparticle_t		*p, *next;
	float			alpha;
	float			time, time2;
	vec3_t			org;
	cparticle_t		*active, *tail;
	vec3_t			rotate_ang;

	if (!initparticles)
		CG_ClearParticles ();

	VectorCopy( cg.refdef.viewaxis[0], vforward );
	VectorCopy( cg.refdef.viewaxis[1], vright );
	VectorCopy( cg.refdef.viewaxis[2], vup );

	vectoangles( cg.refdef.viewaxis[0], rotate_ang );
	roll += ((cg.time - oldtime) * 0.1) ;
	rotate_ang[ROLL] += (roll*0.9);
	AngleVectors ( rotate_ang, rforward, rright, rup);
	
	oldtime = cg.time;

	active = NULL;
	tail = NULL;

	for (p=active_particles ; p ; p=next)
	{

		next = p->next;

		time = (cg.time - p->time)*0.001;

		alpha = p->alpha + time*p->alphavel;
		if (alpha <= 0)
		{	// faded out
			p->next = free_particles;
			free_particles = p;
			p->type = 0;
			p->color = 0;
			p->alpha = 0;
			continue;
		}

		if (p->type == P_SMOKE || p->type == P_ANIM || p->type == P_BLEED || p->type == P_SMOKE_IMPACT)
		{
			if (cg.time > p->endtime)
			{
				p->next = free_particles;
				free_particles = p;
				p->type = 0;
				p->color = 0;
				p->alpha = 0;
			
				continue;
			}

		}

		if (p->type == P_WEATHER_FLURRY)
		{
			if (cg.time > p->endtime)
			{
				p->next = free_particles;
				free_particles = p;
				p->type = 0;
				p->color = 0;
				p->alpha = 0;
			
				continue;
			}
		}


		if (p->type == P_FLAT_SCALEUP_FADE)
		{
			if (cg.time > p->endtime)
			{
				p->next = free_particles;
				free_particles = p;
				p->type = 0;
				p->color = 0;
				p->alpha = 0;
				continue;
			}

		}

		if ((p->type == P_BAT || p->type == P_SPRITE) && p->endtime < 0) {
			// temporary sprite
			CG_AddParticleToScene (p, p->org, alpha);
			p->next = free_particles;
			free_particles = p;
			p->type = 0;
			p->color = 0;
			p->alpha = 0;
			continue;
		}

		p->next = NULL;
		if (!tail)
			active = tail = p;
		else
		{
			tail->next = p;
			tail = p;
		}

		if (alpha > 1.0)
			alpha = 1;

		time2 = time*time;

		org[0] = p->org[0] + p->vel[0]*time + p->accel[0]*time2;
		org[1] = p->org[1] + p->vel[1]*time + p->accel[1]*time2;
		org[2] = p->org[2] + p->vel[2]*time + p->accel[2]*time2;

		CG_AddParticleToScene (p, org, alpha);
	}

	active_particles = active;
}

/*
======================
CG_AddParticles
======================
*/
void CG_ParticleSnowFlurry (qhandle_t pshader, centity_t *cent)
{
	cparticle_t	*p;
	qboolean turb = qtrue;

	if (!pshader)
		CG_Printf ("CG_ParticleSnowFlurry pshader == ZERO!\n");

	if (!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	p->color = 0;
	p->alpha = 0.90f;
	p->alphavel = 0;

	p->start = cent->currentState.origin2[0];
	p->end = cent->currentState.origin2[1];
	
	p->endtime = cg.time + cent->currentState.time;
	p->startfade = cg.time + cent->currentState.time2;
	
	p->pshader = pshader;
	
	if (rand()%100 > 90)
	{
		p->height = 32;
		p->width = 32;
		p->alpha = 0.10f;
	}
	else
	{
		p->height = 1;
		p->width = 1;
	}

	p->vel[2] = -20;

	p->type = P_WEATHER_FLURRY;
	
	if (turb)
		p->vel[2] = -10;
	
	VectorCopy(cent->currentState.origin, p->org);

	p->org[0] = p->org[0];
	p->org[1] = p->org[1];
	p->org[2] = p->org[2];

	p->vel[0] = p->vel[1] = 0;
	
	p->accel[0] = p->accel[1] = p->accel[2] = 0;

	p->vel[0] += cent->currentState.angles[0] * 32 + (crandom() * 16);
	p->vel[1] += cent->currentState.angles[1] * 32 + (crandom() * 16);
	p->vel[2] += cent->currentState.angles[2];

	if (turb)
	{
		p->accel[0] = crandom () * 16;
		p->accel[1] = crandom () * 16;
	}

}

void CG_ParticleSnow (qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum)
{
	cparticle_t	*p;

	if (!pshader)
		CG_Printf ("CG_ParticleSnow pshader == ZERO!\n");

	if (!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	p->color = 0;
	p->alpha = 0.40f;
	p->alphavel = 0;
	p->start = origin[2];
	p->end = origin2[2];
	p->pshader = pshader;
	p->height = 1;
	p->width = 1;
	
	p->vel[2] = -50;

	if (turb)
	{
		p->type = P_WEATHER_TURBULENT;
		p->vel[2] = -50 * 1.3;
	}
	else
	{
		p->type = P_WEATHER;
	}
	
	VectorCopy(origin, p->org);

	p->org[0] = p->org[0] + ( crandom() * range);
	p->org[1] = p->org[1] + ( crandom() * range);
	p->org[2] = p->org[2] + ( crandom() * (p->start - p->end)); 

	p->vel[0] = p->vel[1] = 0;
	
	p->accel[0] = p->accel[1] = p->accel[2] = 0;

	if (turb)
	{
		p->vel[0] = crandom() * 16;
		p->vel[1] = crandom() * 16;
	}

	// Rafael snow pvs check
	p->snum = snum;
	p->link = qtrue;

}

void CG_ParticleBubble (qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum)
{
	cparticle_t	*p;
	float		randsize;

	if (!pshader)
		CG_Printf ("CG_ParticleSnow pshader == ZERO!\n");

	if (!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	p->color = 0;
	p->alpha = 0.40f;
	p->alphavel = 0;
	p->start = origin[2];
	p->end = origin2[2];
	p->pshader = pshader;
	
	randsize = 1 + (crandom() * 0.5);
	
	p->height = randsize;
	p->width = randsize;
	
	p->vel[2] = 50 + ( crandom() * 10 );

	if (turb)
	{
		p->type = P_BUBBLE_TURBULENT;
		p->vel[2] = 50 * 1.3;
	}
	else
	{
		p->type = P_BUBBLE;
	}
	
	VectorCopy(origin, p->org);

	p->org[0] = p->org[0] + ( crandom() * range);
	p->org[1] = p->org[1] + ( crandom() * range);
	p->org[2] = p->org[2] + ( crandom() * (p->start - p->end)); 

	p->vel[0] = p->vel[1] = 0;
	
	p->accel[0] = p->accel[1] = p->accel[2] = 0;

	if (turb)
	{
		p->vel[0] = crandom() * 4;
		p->vel[1] = crandom() * 4;
	}

	// Rafael snow pvs check
	p->snum = snum;
	p->link = qtrue;

}

void CG_ParticleSmoke (qhandle_t pshader, centity_t *cent)
{

	// using cent->density = enttime
	//		 cent->frame = startfade
	cparticle_t	*p;

	if (!pshader)
		CG_Printf ("CG_ParticleSmoke == ZERO!\n");

	if (!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	
	p->endtime = cg.time + cent->currentState.time;
	p->startfade = cg.time + cent->currentState.time2;
	
	p->color = 0;
	p->alpha = 1.0;
	p->alphavel = 0;
	p->start = cent->currentState.origin[2];
	p->end = cent->currentState.origin2[2];
	p->pshader = pshader;
	p->rotate = qfalse;
	p->height = 8;
	p->width = 8;
	p->endheight = 32;
	p->endwidth = 32;
	p->type = P_SMOKE;
	
	VectorCopy(cent->currentState.origin, p->org);

	p->vel[0] = p->vel[1] = 0;
	p->accel[0] = p->accel[1] = p->accel[2] = 0;

	p->vel[2] = 5;

	if (cent->currentState.frame == 1)// reverse gravity	
		p->vel[2] *= -1;

	p->roll = 8 + (crandom() * 4);
}


void CG_ParticleBulletDebris (vec3_t org, vec3_t vel, int duration)
{

	cparticle_t	*p;

	if (!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	
	p->endtime = cg.time + duration;
	p->startfade = cg.time + duration/2;
	
	p->color = EMISIVEFADE;
	p->alpha = 1.0;
	p->alphavel = 0;

	p->height = 0.5;
	p->width = 0.5;
	p->endheight = 0.5;
	p->endwidth = 0.5;

	p->pshader = cgs.media.tracerShader;

	p->type = P_SMOKE;
	
	VectorCopy(org, p->org);

	p->vel[0] = vel[0];
	p->vel[1] = vel[1];
	p->vel[2] = vel[2];
	p->accel[0] = p->accel[1] = p->accel[2] = 0;

	p->accel[2] = -60;
	p->vel[2] += -20;
	
}

/*
======================
CG_ParticleExplosion
======================
*/

void CG_ParticleExplosion (char *animStr, vec3_t origin, vec3_t vel, int duration, int sizeStart, int sizeEnd)
{
	cparticle_t	*p;
	int anim;

	if (animStr < (char *)10)
		CG_Error( "CG_ParticleExplosion: animStr is probably an index rather than a string" );

	// find the animation string
	for (anim=0; shaderAnimNames[anim]; anim++) {
		if (!Q_stricmp( animStr, shaderAnimNames[anim] ))
			break;
	}
	if (!shaderAnimNames[anim]) {
		CG_Error("CG_ParticleExplosion: unknown animation string: %s\n", animStr);
		return;
	}

	if (!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
#ifdef WOLF_PARTICLES
	p->alpha = 1.0;
#else
	p->alpha = 0.5;
#endif
	p->alphavel = 0;

	if (duration < 0) {
		duration *= -1;
		p->roll = 0;
	} else {
		p->roll = crandom()*179;
	}

	p->shaderAnim = anim;

	p->width = sizeStart;
	p->height = sizeStart*shaderAnimSTRatio[anim];	// for sprites that are stretch in either direction

	p->endheight = sizeEnd;
	p->endwidth = sizeEnd*shaderAnimSTRatio[anim];

	p->endtime = cg.time + duration;

	p->type = P_ANIM;

	VectorCopy( origin, p->org );
	VectorCopy( vel, p->vel );
	VectorClear( p->accel );

}

// Rafael Shrapnel
void CG_AddParticleShrapnel (localEntity_t *le)
{
	return;
}
// done.

int CG_NewParticleArea (int num)
{
	// const char *str;
	char *str;
	char *token;
	int type;
	vec3_t origin, origin2;
	int		i;
	float range = 0;
	int turb;
	int	numparticles;
	int	snum;
	
	str = (char *) CG_ConfigString (num);
	if (!str[0])
		return (0);
	
	// returns type 128 64 or 32
	token = COM_Parse (&str);
	type = atoi (token);
	
	if (type == 1)
		range = 128;
	else if (type == 2)
		range = 64;
	else if (type == 3)
		range = 32;
	else if (type == 0)
		range = 256;
	else if (type == 4)
		range = 8;
	else if (type == 5)
		range = 16;
	else if (type == 6)
		range = 32;
	else if (type == 7)
		range = 64;


	for (i=0; i<3; i++)
	{
		token = COM_Parse (&str);
		origin[i] = atof (token);
	}

	for (i=0; i<3; i++)
	{
		token = COM_Parse (&str);
		origin2[i] = atof (token);
	}
		
	token = COM_Parse (&str);
	numparticles = atoi (token);
	
	token = COM_Parse (&str);
	turb = atoi (token);

	token = COM_Parse (&str);
	snum = atoi (token);
	
	for (i=0; i<numparticles; i++)
	{
		if (type >= 4)
			CG_ParticleBubble (cgs.media.waterBubbleShader, origin, origin2, turb, range, snum);
		else
			CG_ParticleSnow (cgs.media.waterBubbleShader, origin, origin2, turb, range, snum);
	}

	return (1);
}

void	CG_SnowLink (centity_t *cent, qboolean particleOn)
{
	cparticle_t		*p, *next;
	int id;

	id = cent->currentState.frame;

	for (p=active_particles ; p ; p=next)
	{
		next = p->next;
		
		if (p->type == P_WEATHER || p->type == P_WEATHER_TURBULENT)
		{
			if (p->snum == id)
			{
				if (particleOn)
					p->link = qtrue;
				else
					p->link = qfalse;
			}
		}

	}
}

void CG_ParticleImpactSmokePuff (qhandle_t pshader, vec3_t origin)
{
	cparticle_t	*p;

	if (!pshader)
		CG_Printf ("CG_ParticleImpactSmokePuff pshader == ZERO!\n");

	if (!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	p->alpha = 0.25;
	p->alphavel = 0;
	p->roll = crandom()*179;

	p->pshader = pshader;

	p->endtime = cg.time + 1000;
	p->startfade = cg.time + 100;

	p->width = rand()%4 + 8;
	p->height = rand()%4 + 8;

	p->endheight = p->height *2;
	p->endwidth = p->width * 2;

	p->endtime = cg.time + 500;

	p->type = P_SMOKE_IMPACT;

	VectorCopy( origin, p->org );
	VectorSet(p->vel, 0, 0, 20);
	VectorSet(p->accel, 0, 0, 20);

	p->rotate = qtrue;
}

void CG_Particle_Bleed (qhandle_t pshader, vec3_t start, vec3_t dir, int fleshEntityNum, int duration)
{
	cparticle_t	*p;

	if (!pshader)
		CG_Printf ("CG_Particle_Bleed pshader == ZERO!\n");

	if (!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	p->alpha = 1.0;
	p->alphavel = 0;
	p->roll = 0;

	p->pshader = pshader;

	p->endtime = cg.time + duration;
	
	if (fleshEntityNum)
		p->startfade = cg.time;
	else
		p->startfade = cg.time + 100;

	p->width = 4;
	p->height = 4;

	p->endheight = 4+rand()%3;
	p->endwidth = p->endheight;

	p->type = P_SMOKE;

	VectorCopy( start, p->org );
	p->vel[0] = 0;
	p->vel[1] = 0;
	p->vel[2] = -20;
	VectorClear( p->accel );

	p->rotate = qfalse;

	p->roll = rand()%179;
	
	p->color = BLOODRED;
	p->alpha = 0.75;

}

void CG_Particle_OilParticle (qhandle_t pshader, centity_t *cent)
{
	cparticle_t	*p;

	int			time;
	int			time2;
	float		ratio;

	float	duration = 1500;

	time = cg.time;
	time2 = cg.time + cent->currentState.time;

	ratio =(float)1 - ((float)time / (float)time2);

	if (!pshader)
		CG_Printf ("CG_Particle_OilParticle == ZERO!\n");

	if (!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	p->alpha = 1.0;
	p->alphavel = 0;
	p->roll = 0;

	p->pshader = pshader;

	p->endtime = cg.time + duration;
	
	p->startfade = p->endtime;

	p->width = 1;
	p->height = 3;

	p->endheight = 3;
	p->endwidth = 1;

	p->type = P_SMOKE;

	VectorCopy(cent->currentState.origin, p->org );	
	
	p->vel[0] = (cent->currentState.origin2[0] * (16 * ratio));
	p->vel[1] = (cent->currentState.origin2[1] * (16 * ratio));
	p->vel[2] = (cent->currentState.origin2[2]);

	p->snum = 1.0f;

	VectorClear( p->accel );

	p->accel[2] = -20;

	p->rotate = qfalse;

	p->roll = rand()%179;
	
	p->alpha = 0.75;

}


void CG_Particle_OilSlick (qhandle_t pshader, centity_t *cent)
{
	cparticle_t	*p;
	
  	if (!pshader)
		CG_Printf ("CG_Particle_OilSlick == ZERO!\n");

	if (!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	
	if (cent->currentState.angles2[2])
		p->endtime = cg.time + cent->currentState.angles2[2];
	else
		p->endtime = cg.time + 60000;

	p->startfade = p->endtime;

	p->alpha = 1.0;
	p->alphavel = 0;
	p->roll = 0;

	p->pshader = pshader;

	if (cent->currentState.angles2[0] || cent->currentState.angles2[1])
	{
		p->width = cent->currentState.angles2[0];
		p->height = cent->currentState.angles2[0];

		p->endheight = cent->currentState.angles2[1];
		p->endwidth = cent->currentState.angles2[1];
	}
	else
	{
		p->width = 8;
		p->height = 8;

		p->endheight = 16;
		p->endwidth = 16;
	}

	p->type = P_FLAT_SCALEUP;

	p->snum = 1.0;

	VectorCopy(cent->currentState.origin, p->org );
	
	p->org[2]+= 0.55 + (crandom() * 0.5);

	p->vel[0] = 0;
	p->vel[1] = 0;
	p->vel[2] = 0;
	VectorClear( p->accel );

	p->rotate = qfalse;

	p->roll = rand()%179;
	
	p->alpha = 0.75;

}

void CG_OilSlickRemove (centity_t *cent)
{
	cparticle_t		*p, *next;
	int				id;

	id = 1.0f;

	if (!id)
		CG_Printf ("CG_OilSlickRevove NULL id\n");

	for (p=active_particles ; p ; p=next)
	{
		next = p->next;
		
		if (p->type == P_FLAT_SCALEUP)
		{
			if (p->snum == id)
			{
				p->endtime = cg.time + 100;
				p->startfade = p->endtime;
				p->type = P_FLAT_SCALEUP_FADE;

			}
		}

	}
}

qboolean ValidBloodPool (vec3_t start)
{
#define EXTRUDE_DIST	0.5

	vec3_t	angles;
	vec3_t	right, up;
	vec3_t	this_pos, x_pos, center_pos, end_pos;
	int		x, y;
	int		fwidth, fheight;
	trace_t	trace;
	vec3_t	normal;

	fwidth = 16;
	fheight = 16;

	VectorSet (normal, 0, 0, 1);

	vectoangles (normal, angles);
	AngleVectors (angles, NULL, right, up);

	VectorMA (start, EXTRUDE_DIST, normal, center_pos);

	for (x= -fwidth/2; x<fwidth; x+= fwidth)
	{
		VectorMA (center_pos, x, right, x_pos);

		for (y= -fheight/2; y<fheight; y+= fheight)
		{
			VectorMA (x_pos, y, up, this_pos);
			VectorMA (this_pos, -EXTRUDE_DIST*2, normal, end_pos);
			
			CG_Trace (&trace, this_pos, NULL, NULL, end_pos, -1, CONTENTS_SOLID);

			
			if (trace.entityNum < ENTITYNUM_WORLD) // may only land on world
				return qfalse;

			if (!(!trace.startsolid && trace.fraction < 1))
				return qfalse;
		
		}
	}

	return qtrue;
}

void CG_BloodPool (localEntity_t *le, qhandle_t pshader, trace_t *tr)
{	
	cparticle_t	*p;
	qboolean	legit;
	vec3_t		start;
	float		rndSize;
	
	if (!pshader)
		CG_Printf ("CG_BloodPool pshader == ZERO!\n");

	if (!free_particles)
		return;
	
	VectorCopy (tr->endpos, start);
	legit = ValidBloodPool (start);

	if (!legit) 
		return;

	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	
	p->endtime = cg.time + 3000;
	p->startfade = p->endtime;

	p->alpha = 1.0;
	p->alphavel = 0;
	p->roll = 0;

	p->pshader = pshader;

	rndSize = 0.4 + random()*0.6;

	p->width = 8*rndSize;
	p->height = 8*rndSize;

	p->endheight = 16*rndSize;
	p->endwidth = 16*rndSize;
	
	p->type = P_FLAT_SCALEUP;

	VectorCopy(start, p->org );
	
	p->vel[0] = 0;
	p->vel[1] = 0;
	p->vel[2] = 0;
	VectorClear( p->accel );

	p->rotate = qfalse;

	p->roll = rand()%179;
	
	p->alpha = 0.75;
	
	p->color = BLOODRED;
}

#define NORMALSIZE	16
#define LARGESIZE	32

void CG_ParticleBloodCloud (centity_t *cent, vec3_t origin, vec3_t dir)
{
	float	length;
	float	dist;
	float	crittersize;
	vec3_t	angles, forward;
	vec3_t	point;
	cparticle_t	*p;
	int		i;
	
	dist = 0;

	length = VectorLength (dir);
	vectoangles (dir, angles);
	AngleVectors (angles, forward, NULL, NULL);

	crittersize = LARGESIZE;

	if (length)
		dist = length / crittersize;

	if (dist < 1)
		dist = 1;

	VectorCopy (origin, point);

	for (i=0; i<dist; i++)
	{
		VectorMA (point, crittersize, forward, point);	
		
		if (!free_particles)
			return;

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cg.time;
		p->alpha = 1.0;
		p->alphavel = 0;
		p->roll = 0;

		p->pshader = cgs.media.smokePuffShader;

		p->endtime = cg.time + 350 + (crandom() * 100);
		
		p->startfade = cg.time;
		
		p->width = LARGESIZE;
		p->height = LARGESIZE;
		p->endheight = LARGESIZE;
		p->endwidth = LARGESIZE;

		p->type = P_SMOKE;

		VectorCopy( origin, p->org );
		
		p->vel[0] = 0;
		p->vel[1] = 0;
		p->vel[2] = -1;
		
		VectorClear( p->accel );

		p->rotate = qfalse;

		p->roll = rand()%179;
		
		p->color = BLOODRED;
		
		p->alpha = 0.75;
		
	}

	
}

void CG_ParticleSparks (vec3_t org, vec3_t vel, int duration, float x, float y, float speed)
{
	cparticle_t	*p;

	if (!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	
	p->endtime = cg.time + duration;
	p->startfade = cg.time + duration/2;
	
	p->color = EMISIVEFADE;
	p->alpha = 0.4f;
	p->alphavel = 0;

	p->height = 0.5;
	p->width = 0.5;
	p->endheight = 0.5;
	p->endwidth = 0.5;

	p->pshader = cgs.media.tracerShader;

	p->type = P_SMOKE;
	
	VectorCopy(org, p->org);

	p->org[0] += (crandom() * x);
	p->org[1] += (crandom() * y);

	p->vel[0] = vel[0];
	p->vel[1] = vel[1];
	p->vel[2] = vel[2];

	p->accel[0] = p->accel[1] = p->accel[2] = 0;

	p->vel[0] += (crandom() * 4);
	p->vel[1] += (crandom() * 4);
	p->vel[2] += (20 + (crandom() * 10)) * speed;	

	p->accel[0] = crandom () * 4;
	p->accel[1] = crandom () * 4;
	
}

void CG_ParticleDust (centity_t *cent, vec3_t origin, vec3_t dir)
{
	float	length;
	float	dist;
	float	crittersize;
	vec3_t	angles, forward;
	vec3_t	point;
	cparticle_t	*p;
	int		i;
	
	dist = 0;

	VectorNegate (dir, dir);
	length = VectorLength (dir);
	vectoangles (dir, angles);
	AngleVectors (angles, forward, NULL, NULL);

	crittersize = LARGESIZE;

	if (length)
		dist = length / crittersize;

	if (dist < 1)
		dist = 1;

	VectorCopy (origin, point);

	for (i=0; i<dist; i++)
	{
		VectorMA (point, crittersize, forward, point);	
				
		if (!free_particles)
			return;

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cg.time;
		p->alpha = 5.0;
		p->alphavel = 0;
		p->roll = 0;

		p->pshader = cgs.media.smokePuffShader;

		// RF, stay around for long enough to expand and dissipate naturally
		if (length)
			p->endtime = cg.time + 4500 + (crandom() * 3500);
		else
			p->endtime = cg.time + 750 + (crandom() * 500);
		
		p->startfade = cg.time;
		
		p->width = LARGESIZE;
		p->height = LARGESIZE;

		// RF, expand while falling
		p->endheight = LARGESIZE*3.0;
		p->endwidth = LARGESIZE*3.0;

		if (!length)
		{
			p->width *= 0.2f;
			p->height *= 0.2f;

			p->endheight = NORMALSIZE;
			p->endwidth = NORMALSIZE;
		}

		p->type = P_SMOKE;

		VectorCopy( point, p->org );
		
		p->vel[0] = crandom()*6;
		p->vel[1] = crandom()*6;
		p->vel[2] = random()*20;

		// RF, add some gravity/randomness
		p->accel[0] = crandom()*3;
		p->accel[1] = crandom()*3;
		p->accel[2] = -PARTICLE_GRAVITY*0.4;

		VectorClear( p->accel );

		p->rotate = qfalse;

		p->roll = rand()%179;
		
		p->alpha = 0.75;
		
	}

	
}

void CG_ParticleMisc (qhandle_t pshader, vec3_t origin, int size, int duration, float alpha)
{
	cparticle_t	*p;

	if (!pshader)
		CG_Printf ("CG_ParticleImpactSmokePuff pshader == ZERO!\n");

	if (!free_particles)
		return;

	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	p->alpha = 1.0;
	p->alphavel = 0;
	p->roll = rand()%179;

	p->pshader = pshader;

	if (duration > 0)
		p->endtime = cg.time + duration;
	else
		p->endtime = duration;

	p->startfade = cg.time;

	p->width = size;
	p->height = size;

	p->endheight = size;
	p->endwidth = size;

	p->type = P_SPRITE;

	VectorCopy( origin, p->org );

	p->rotate = qfalse;
}
