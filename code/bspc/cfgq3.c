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
//===========================================================================
// BSPC configuration file
// Quake3
//===========================================================================

#define PRESENCE_NONE				1
#define PRESENCE_NORMAL				2
#define PRESENCE_CROUCH				4

bbox	//30x30x56
{
	presencetype	PRESENCE_NORMAL
	flags			0x0000
	mins			{-15, -15, -24}
	maxs			{15, 15, 32}
} //end bbox

bbox	//30x30x40
{
	presencetype	PRESENCE_CROUCH
	flags			0x0001
	mins			{-15, -15, -24}
	maxs			{15, 15, 16}
} //end bbox

settings
{
	phys_gravitydirection		{0, 0, -1}
	phys_friction				6
	phys_stopspeed				100
	phys_gravity				800
	phys_waterfriction			1
	phys_watergravity			400
	phys_maxvelocity			320
	phys_maxwalkvelocity		320
	phys_maxcrouchvelocity		100
	phys_maxswimvelocity		150
	phys_maxacceleration		2200
	phys_airaccelerate			0
	phys_maxstep				18
	phys_maxsteepness			0.7
	phys_maxwaterjump			19
	phys_maxbarrier				33
	phys_jumpvel				270
	phys_falldelta5				40
	phys_falldelta10			60
	rs_waterjump				400
	rs_teleport					50
	rs_barrierjump				100
	rs_startcrouch				300
	rs_startgrapple				500
	rs_startwalkoffledge		70
	rs_startjump				300
	rs_rocketjump				500
	rs_bfgjump					500
	rs_jumppad					250
	rs_aircontrolledjumppad		300
	rs_funcbob					300
	rs_startelevator			50
	rs_falldamage5				300
	rs_falldamage10				500
	rs_maxjumpfallheight		450
} //end settings
