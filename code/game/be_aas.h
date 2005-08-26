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

/*****************************************************************************
 * name:		be_aas.h
 *
 * desc:		Area Awareness System, stuff exported to the AI
 *
 * $Archive: /source/code/botlib/be_aas.h $
 *
 *****************************************************************************/

#ifndef MAX_STRINGFIELD
#define MAX_STRINGFIELD				80
#endif

//travel flags
#define TFL_INVALID				0x00000001	//traveling temporary not possible
#define TFL_WALK				0x00000002	//walking
#define TFL_CROUCH				0x00000004	//crouching
#define TFL_BARRIERJUMP			0x00000008	//jumping onto a barrier
#define TFL_JUMP				0x00000010	//jumping
#define TFL_LADDER				0x00000020	//climbing a ladder
#define TFL_WALKOFFLEDGE		0x00000080	//walking of a ledge
#define TFL_SWIM				0x00000100	//swimming
#define TFL_WATERJUMP			0x00000200	//jumping out of the water
#define TFL_TELEPORT			0x00000400	//teleporting
#define TFL_ELEVATOR			0x00000800	//elevator
#define TFL_ROCKETJUMP			0x00001000	//rocket jumping
#define TFL_BFGJUMP				0x00002000	//bfg jumping
#define TFL_GRAPPLEHOOK			0x00004000	//grappling hook
#define TFL_DOUBLEJUMP			0x00008000	//double jump
#define TFL_RAMPJUMP			0x00010000	//ramp jump
#define TFL_STRAFEJUMP			0x00020000	//strafe jump
#define TFL_JUMPPAD				0x00040000	//jump pad
#define TFL_AIR					0x00080000	//travel through air
#define TFL_WATER				0x00100000	//travel through water
#define TFL_SLIME				0x00200000	//travel through slime
#define TFL_LAVA				0x00400000	//travel through lava
#define TFL_DONOTENTER			0x00800000	//travel through donotenter area
#define TFL_FUNCBOB				0x01000000	//func bobbing
#define TFL_FLIGHT				0x02000000	//flight
#define TFL_BRIDGE				0x04000000	//move over a bridge
//
#define TFL_NOTTEAM1			0x08000000	//not team 1
#define TFL_NOTTEAM2			0x10000000	//not team 2

//default travel flags
#define TFL_DEFAULT	TFL_WALK|TFL_CROUCH|TFL_BARRIERJUMP|\
	TFL_JUMP|TFL_LADDER|\
	TFL_WALKOFFLEDGE|TFL_SWIM|TFL_WATERJUMP|\
	TFL_TELEPORT|TFL_ELEVATOR|\
	TFL_AIR|TFL_WATER|TFL_JUMPPAD|TFL_FUNCBOB

typedef enum
{
	SOLID_NOT,			// no interaction with other objects
	SOLID_TRIGGER,		// only touch when inside, after moving
	SOLID_BBOX,			// touch on edge
	SOLID_BSP			// bsp clip, touch on edge
} solid_t;

//a trace is returned when a box is swept through the AAS world
typedef struct aas_trace_s
{
	qboolean	startsolid;	// if true, the initial point was in a solid area
	float		fraction;	// time completed, 1.0 = didn't hit anything
	vec3_t		endpos;		// final position
	int			ent;		// entity blocking the trace
	int			lastarea;	// last area the trace was in (zero if none)
	int			area;		// area blocking the trace (zero if none)
	int			planenum;	// number of the plane that was hit
} aas_trace_t;

/* Defined in botlib.h

//bsp_trace_t hit surface
typedef struct bsp_surface_s
{
	char name[16];
	int flags;
	int value;
} bsp_surface_t;

//a trace is returned when a box is swept through the BSP world
typedef struct bsp_trace_s
{
	qboolean		allsolid;	// if true, plane is not valid
	qboolean		startsolid;	// if true, the initial point was in a solid area
	float			fraction;	// time completed, 1.0 = didn't hit anything
	vec3_t			endpos;		// final position
	cplane_t		plane;		// surface normal at impact
	float			exp_dist;	// expanded plane distance
	int				sidenum;	// number of the brush side hit
	bsp_surface_t	surface;	// hit surface
	int				contents;	// contents on other side of surface hit
	int				ent;		// number of entity hit
} bsp_trace_t;
//
*/

//entity info
typedef struct aas_entityinfo_s
{
	int		valid;			// true if updated this frame
	int		type;			// entity type
	int		flags;			// entity flags
	float	ltime;			// local time
	float	update_time;	// time between last and current update
	int		number;			// number of the entity
	vec3_t	origin;			// origin of the entity
	vec3_t	angles;			// angles of the model
	vec3_t	old_origin;		// for lerping
	vec3_t	lastvisorigin;	// last visible origin
	vec3_t	mins;			// bounding box minimums
	vec3_t	maxs;			// bounding box maximums
	int		groundent;		// ground entity
	int		solid;			// solid type
	int		modelindex;		// model used
	int		modelindex2;	// weapons, CTF flags, etc
	int		frame;			// model frame number
	int		event;			// impulse events -- muzzle flashes, footsteps, etc
	int		eventParm;		// even parameter
	int		powerups;		// bit flags
	int		weapon;			// determines weapon and flash model, etc
	int		legsAnim;		// mask off ANIM_TOGGLEBIT
	int		torsoAnim;		// mask off ANIM_TOGGLEBIT
} aas_entityinfo_t;

// area info
typedef struct aas_areainfo_s
{
	int contents;
	int flags;
	int presencetype;
	int cluster;
	vec3_t mins;
	vec3_t maxs;
	vec3_t center;
} aas_areainfo_t;

// client movement prediction stop events, stop as soon as:
#define SE_NONE					0
#define SE_HITGROUND			1		// the ground is hit
#define SE_LEAVEGROUND			2		// there's no ground
#define SE_ENTERWATER			4		// water is entered
#define SE_ENTERSLIME			8		// slime is entered
#define SE_ENTERLAVA			16		// lava is entered
#define SE_HITGROUNDDAMAGE		32		// the ground is hit with damage
#define SE_GAP					64		// there's a gap
#define SE_TOUCHJUMPPAD			128		// touching a jump pad area
#define SE_TOUCHTELEPORTER		256		// touching teleporter
#define SE_ENTERAREA			512		// the given stoparea is entered
#define SE_HITGROUNDAREA		1024	// a ground face in the area is hit
#define SE_HITBOUNDINGBOX		2048	// hit the specified bounding box
#define SE_TOUCHCLUSTERPORTAL	4096	// touching a cluster portal

typedef struct aas_clientmove_s
{
	vec3_t endpos;			//position at the end of movement prediction
	int endarea;			//area at end of movement prediction
	vec3_t velocity;		//velocity at the end of movement prediction
	aas_trace_t trace;		//last trace
	int presencetype;		//presence type at end of movement prediction
	int stopevent;			//event that made the prediction stop
	int endcontents;		//contents at the end of movement prediction
	float time;				//time predicted ahead
	int frames;				//number of frames predicted ahead
} aas_clientmove_t;

// alternate route goals
#define ALTROUTEGOAL_ALL				1
#define ALTROUTEGOAL_CLUSTERPORTALS		2
#define ALTROUTEGOAL_VIEWPORTALS		4

typedef struct aas_altroutegoal_s
{
	vec3_t origin;
	int areanum;
	unsigned short starttraveltime;
	unsigned short goaltraveltime;
	unsigned short extratraveltime;
} aas_altroutegoal_t;

// route prediction stop events
#define RSE_NONE				0
#define RSE_NOROUTE				1	//no route to goal
#define RSE_USETRAVELTYPE		2	//stop as soon as on of the given travel types is used
#define RSE_ENTERCONTENTS		4	//stop when entering the given contents
#define RSE_ENTERAREA			8	//stop when entering the given area

typedef struct aas_predictroute_s
{
	vec3_t endpos;			//position at the end of movement prediction
	int endarea;			//area at end of movement prediction
	int stopevent;			//event that made the prediction stop
	int endcontents;		//contents at the end of movement prediction
	int endtravelflags;		//end travel flags
	int numareas;			//number of areas predicted ahead
	int time;				//time predicted ahead (in hundreth of a sec)
} aas_predictroute_t;
