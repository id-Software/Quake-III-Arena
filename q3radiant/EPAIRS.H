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
#ifndef	EPAIRS_H_
#define	EPAIRS_H_

typedef struct
{
	char  *key;
	char  *value;
} userEpair_t;

userEpair_t eclassStr[] =
{
	//////////////////////////////
	// Worldspawn
	//////////////////////////////
	{"classname",   "worldspawn"},
	{"color",       "0 0 0"},
	{"rem",         "Only used for the world entity."},
	{"rem",			"cdtrack - number of CD track to play when level starts"},
	{"rem",			"fog_value - level of fog for this map"},
	
	//////////////////////////////
	// Light
	//////////////////////////////
	{"classname",	"light"},
	{"size",		"-8 -8 -8 8 8 8"},
	{"color",		"0 1 0"},
	{"flag",		"START_OFF"},

	{"rem",			"style - number of style to use, 0-63"},
	{"rem",			"nelnosmama - string to define lightstyle"},

	//////////////////////////////
	// light_walltorch
	//////////////////////////////
	{"classname",	"light_walltorch"},
	{"size",		"-8 -8 -8 8 8 8"},
	{"color",		"0 1 0"},
	{"flag",		"START_ON"},

	{"rem",			"style - number of style to use, 0-63"},
	{"rem",			"nelnosmama - string to define lightstyle"},

	//////////////////////////////
	// light_spot
	//////////////////////////////
	{"classname",	"light_spot"},
	{"size",		"-8 -8 -8 8 8 8"},
	{"color",		"0 1 0"},
	{"flag",		"START_ON"},

	{"rem",			"Used to make a spotlight.  If it is targeted at another entity,"},
	{"rem",			"the spotlight will point directly at it, otherwise it will point"},
	{"rem",			"in the direction of its 'angle' field"},
	{"rem",			""},
	{"rem",			"style - number of style to use, 0-63"},
	{"rem",			"nelnosmama - string to define lightstyle"},

	//////////////////////////////
	// light_strobe
	//////////////////////////////
	{"classname",	"light_strobe"},
	{"size",		"-8 -8 -8 8 8 8"},
	{"color",		"0 1 0"},

	{"flag",		"START_ON"},
	{"rem",			"style - number of style to use, 0-63"},
	{"rem",			"nelnosmama - string to define lightstyle"},

	//////////////////////////////
	// effect_fog
	//////////////////////////////
	{"classname",	"effect_fog"},
	{"color",		"0 1 0"},
	{"rem",			"Duh, fog."},
	{"rem",			""},

	//////////////////////////////
	// effect_snow
	//////////////////////////////
	{"classname",	"effect_snow"},
	{"color",		"1 1 1"},
	{"height",		"10"},
	{"rem",			"Snow!"},
	{"rem",			"Do you realize the entire street value of this mountain?"},
	
	//////////////////////////////
	// effect_rain
	//////////////////////////////
	{"classname",	"effect_rain"},
	{"height",		"10"},
	{"color",		"0 0.5 0.8"},
	{"rem",			"Rain!"},
	{"rem",			"Yellow rain?"},

	//////////////////////////////
	// func_door
	//////////////////////////////
	{"classname",    "func_door"},
	{"color",        "0 0.5 0.8"},
	{"health",		 "0"},
	{"speed",		 "100"},
	{"wait",		 "3"},
	{"lip",			 "8"},
	{"dmg",			 "2"},
	{"angle",		 "0 0 0"},
	{"message",		 "you suck"},
	{"targetname",	 ""},
	{"flag",         "START_OPEN"},
	{"flag",         "REVERSE"},
	{"flag",         "DOOR_DONT_LINK"},
	{"flag",         "TOGGLE"},
	{"flag",		 "AUTO_OPEN"},
	{"flag",		 "USE_TO_CLOSE"},

	{"rem", "Doors that touch are linked together to operate as one."},
	{"rem", "  "},
	{"rem", "message - printed when the door is touched if it is a trigger door and it hasn't been fired yet"},
	{"rem", "angle - determines the opening direction"},
	{"rem", "targetname - if set, no touch field will be spawned and a remote button or trigger field activates the door."},
	{"rem", "health - if set, door must be shot open"},
	{"rem",	"speed - movement speed (100 default)"},
	{"rem", "wait - time to wait before returning (3 default, -1 = never return)"},
	{"rem",	"lip - amount of door visible remaining at end of move (8 default)"},
	{"rem",	"dmg - damage to inflict when blocked (2 default)"},
	{"rem", "sound_opening - name of the sound to play during opening, ie. doors/creek.wav"},
	{"rem",	"sound_open_finish - name of the sound to play when opening completes, ie. doors/slam.wav"},
	{"rem", "sound_closing - name of the sound to play when closing starts, ie. doors/creek.wav"},
	{"rem",	"sound_close_finish - name of the sound to play when closing completes, ie. doors/slam.wav"},
	{"rem", "  "},
	{"rem", "Spawnflags:"},
    {"rem", "TOGGLE causes the door to wait in both the start and end states for a"},
    {"rem", "trigger event."},
    {"rem", "  "},
    {"rem", "START_OPEN causes the door to move to its destination when spawned, and"},
    {"rem", "operate in reverse.  It is used to temporarily or permanently close off an"},
    {"rem", "area when triggered (not usefull for touch or takedamage doors)."},
	{"rem",	"AUTO_OPEN will spawn a trigger field around the door that can open it without"},
	{"rem", "its being used."},
	{"rem",	"DOOR_DONT_LINK will stop a door from being automatically linked to other doors"},
	{"rem", "that it touches."},

	//////////////////////////////
	// func_plat
	//////////////////////////////
	{"classname",    "func_plat"},
	{"color",        "0 0.5 0.8"},
	{"flag",         "PLAT_START_UP"},
	{"flag",         ""},
	{"flag",         ""},
	{"flag",         "PLAT_TOGGLE"},

	{"rem", "Plats should be drawn in the up position to spawn correctly"},
	{"rem",	" "},
	{"rem", "message - printed when the door is touched if it is a trigger door and it hasn't been fired yet"},
	{"rem", "angle - determines the opening direction"},
	{"rem", "targetname - if set, no touch field will be spawned and a remote button or trigger field activates the door."},
	{"rem", "health - if set, door must be shot open"},
	{"rem",	"speed - movement speed (100 default)"},
	{"rem", "wait - time to wait before returning (3 default, -1 = never return)"},
	{"rem",	"height - number of units to move the platform up from spawn position.  If height is"},
	{"rem", "not specified, then the movement distance is determined based on the vertical size of the platform."},
	{"rem",	"dmg - damage to inflict when blocked (2 default)"},
	{"rem", "sound_up - name of the sound to play when going up, ie. doors/creek.wav"},
	{"rem",	"sound_top - name of the sound to play when plat hits top, ie. doors/slam.wav"},
	{"rem", "sound_down - name of the sound to play when going down, ie. doors/creek.wav"},
	{"rem",	"sound_bottom - name of the sound to play when plat hits bottom, ie. doors/slam.wav"},
	{"rem",	" "},
	{"rem", "Spawnflags:"},
	{"rem",	"PLAT_START_UP starts the platform in the up (drawn) position"},
	{"rem",	"(height added to drawn position)"},
    {"rem", "TOGGLE causes the platform to wait in both the start and end states for a"},
    {"rem", "trigger event."},

	//////////////////////////////
	// func_door_rotate
	//////////////////////////////
	{"classname",    "func_door_rotate"},
	{"distance",	 "90.0"},
	{"color",        "0 0.5 0.8"},
	{"message",		 "they suck"},
	{"flag",         "START_OPEN"},
	{"flag",         "REVERSE"},
	{"flag",         "DOOR_DONT_LINK"},
	{"flag",         "TOGGLE"},
	{"flag",         "X_AXIS"},
	{"flag",         "Y_AXIS"},
	{"flag",		 "AUTO_OPEN"},
	{"flag",		 "USE_TO_CLOSE"},
//	{"flag",         "X_AXIS"},
//	{"flag",         "Y_AXIS"},

	{"rem", "if two doors touch, they are assumed to be connected and operate as a unit."},
	{"rem", "  "},
    {"rem", "TOGGLE causes the door to wait in both the start and end states for a"},
    {"rem", "trigger event."},
    {"rem", "  "},
    {"rem", "START_OPEN causes the door to move to its destination when spawned, and"},
    {"rem", "operate in reverse.  It is used to temporarily or permanently close off an"},
    {"rem", "area when triggered (not usefull for touch or takedamage doors)."},
    {"rem", "  "},
    {"rem", "Key doors are allways wait -1."},
    {"rem", "  "},
    {"rem", "You need to have an origin brush as part of this entity.  The center of"},
    {"rem", "that brush will be"},
    {"rem", "the point around which it is rotated. It will rotate around the Z axis by"},
    {"rem", "default.  You can"},
    {"rem", "check either the X_AXIS or Y_AXIS box to change that."},
    {"rem", "  "},
    {"rem", "'distance'	is how many degrees the door will be rotated."},
    {"rem", "'speed'	determines how fast the door moves; default value is 100."},
    {"rem", "  "},
    {"rem", "REVERSE will cause the door to rotate in the opposite direction."},
    {"rem", "  "},
    {"rem", "'message'	is printed when the door is touched if it is a trigger door and"},
    {"rem", "		it hasn't been fired yet."},
    {"rem", "'targetname'	if set, no touch field will be spawned and a remote button or"},
    {"rem", "			trigger field activates the door."},
    {"rem", "'health'	if set, door must be shot open"},
    {"rem", "'wait'	wait before returning (3 default, -1 = never return)"},
    {"rem", "'dmg'	damage to inflict when blocked (2 default)"},
	{"rem", "sound_opening - name of the sound to play during opening, ie. doors/creek.wav"},
	{"rem",	"sound_open_finish - name of the sound to play when opening completes, ie. doors/slam.wav"},
	{"rem", "sound_closing - name of the sound to play when closing starts, ie. doors/creek.wav"},
	{"rem",	"sound_close_finish - name of the sound to play when closing completes, ie. doors/slam.wav"},
	{"rem",	"AUTO_OPEN will spawn a trigger field around the door that can open it without"},
	{"rem", "its being used."},
	{"rem",	"DOOR_DONT_LINK will stop a door from being automatically linked to other doors"},
	{"rem", "that it touches."},

	//////////////////////////////
	// func_rotate
	//////////////////////////////
	{"classname",    "func_rotate"},
	{"color",        "0.0 0.5 0.8"},
	{"flag",         "START_ON"},
	{"flag",         "REVERSE"},
	{"flag",         "X_AXIS"},
	{"flag",         "Y_AXIS"},

    {"rem", "You need to have an origin brush as part of this entity."},
	{"rem", "The center of that brush will be"},
    {"rem", "the point around which it is rotated. It will rotate around the Z axis by"},
	{"rem", "default.  You can"},
    {"rem", "check either the X_AXIS or Y_AXIS box to change that."},
    {"rem", "  "},
    {"rem", "'speed'	determines how fast it moves; default value is 100."},
    {"rem", "'dmg'	damage to inflict when blocked (2 default)"},
    {"rem", "  "},
    {"rem", "REVERSE will cause the it to rotate in the opposite direction."},

	//////////////////////////////
	// trigger_multiple
	//////////////////////////////
	{"classname",	"trigger_multiple"},
	{"color",		"0.5 0.5 0.5"},
	{"health",		"0"},
	{"delay",		"0"},
	{"wait",		"0.2"},
	{"sound",		""},
	{"targetname",	""},
	{"target",		""},
	{"killtarget",	""},
	{"message",		""},
	{"flag",		"NOTOUCH"},

	{"rem",			"a repeatable trigger, targetted at the entity with the name"},
	{"rem",			"targetname."},
	{"rem",			""},
	{"rem",			"health - if set the trigger must be killed to activate"},
	{"rem",			"delay - time to wait after activation before firing target"},
	{"rem",			"wait - time to wait between retriggering (default = 0.2 seconds)"},
	{"rem",			""},
	{"rem",			"Spawnflags:"},
	{"rem",			"NOTOUCH - if set the trigger is only fired by other entities and"},
	{"rem",			"not by touching."},

	//////////////////////////////
	// trigger_console
	//////////////////////////////
	{"classname",	"trigger_console"},
	{"color",		"0.5 0.5 0.5"},
	{"wait",		"0.2"},
	{"command",		""},
	{"rem",			""},
	{"rem",			"command - the command to send to the console when triggered"},

	//////////////////////////////
	// trigger_once
	//////////////////////////////
	{"classname",	"trigger_once"},
	{"color",		"0.5 0.5 0.5"},
	{"angle",		 "0 0 0"},
	{"health",		"0"},
	{"delay",		"0"},
	{"sound",		""},
	{"targetname",	""},
	{"killtarget",	""},
	{"message",		""},
	{"flag",		"NOTOUCH"},

	{"rem",			"triggers once, then removes itself"},
	{"rem",			" "},
	{"rem",			"health - if set the trigger must be killed to activate"},
	{"rem",			"delay - time to wait after activation before firing target"},
	{"rem",			"wait - time to wait between retriggering (default = 0.2 seconds)"},
	{"rem",			"sound - name of sound to play upon firing"},
	{"rem",			" "},
	{"rem",			"Spawnflags:"},
	{"rem",			"NOTOUCH - if set the trigger is only fired by other entities and"},
	{"rem",			"not by touching."},

	//////////////////////////////
	// trigger_relay
	//////////////////////////////
	{"classname",	"trigger_relay"},
	{"color",		"0.5 0.5 0.5"},
	{"delay",		"0"},
	{"sound",		""},
	{"targetname",	""},
	{"killtarget",	""},
	{"message",		""},

	{"rem",			"sound - name of sound to play upon firing"},

	//////////////////////////////
	// trigger_teleport
	//////////////////////////////
	{"classname",	"trigger_teleport"},
	{"color",		"0.5 0.5 0.5"},
	{"sound",		""},
	{"targetname",	""},
	{"killtarget",	""},
	{"message",		""},
	{"flag",		"PLAYER_ONLY"},
	{"flag",		"NO_FLASH"},
	{"flag",		"NO_ANGLE_ADJUST"},
	
	{"rem",			"sound - name of sound to play upon firing, if not specified, then no"},
	{"rem",			"sound will be played"},
	{"rem",			"fog_value - sets fog_value to this when a teleporter is used"},
	{"rem",			""},
	{"rem",			"Spawnflags:"},
	{"rem",			"PLAYER_ONLY - will only teleport players (NOT bots)"},
	{"rem",			"NO_FLASH - no spawn fog will be generated when an object teleports"},
	{"rem",			"NO_ANGLE_ADJUST - the object's angle will not be adjusted when it is"},
	{"rem",			"teleported."},

	//////////////////////////////
	// info_teleport_destination
	//////////////////////////////
	{"classname",	"info_teleport_destination"},
	{"color",		"0.5 0.5 0.5"},
	{"size",		"-8.0 -8.0 -8.0 8.0 8.0 8.0"},

	//////////////////////////////
	// trigger_warp
	//////////////////////////////
	{"classname",	"trigger_warp"},
	{"color",		"0.5 0.5 0.5"},
	{"sound",		""},
	{"targetname",	""},
	{"killtarget",	""},
	{"message",		""},
	{"flag",		"PLAYER_ONLY"},
	{"flag",		"NO_FLASH"},
	{"flag",		"NO_ANGLE_ADJUST"},

	{"rem",			"sound - name of sound to play upon firing, if not specified, then no"},
	{"rem",			"sound will be played"},
	{"rem",			"fog_value - sets fog_value to this when a warp is used"},
	{"rem",			"speed - speed from this path point to the next"},
	{"rem",			""},
	{"rem",			"Spawnflags:"},
	{"rem",			"PLAYER_ONLY - will only teleport players (NOT bots)"},
	{"rem",			"NO_FLASH - no spawn fog will be generated when an object teleports"},
	{"rem",			"NO_ANGLE_ADJUST - the object's angle will not be adjusted when it is"},
	{"rem",			"teleported."},

	//////////////////////////////
	//	tele_cylinder
	//////////////////////////////

	{"classname",	"warp_cylinder"},
	{"color",		"0.5 0.5 0.5"},
	{"color",        "1 0 1"},
	{"size",         "-16 -16 -24 16 16 32"},
	{"rem",          "Location player starts in deathmatch"},

	//////////////////////////////
	// info_warp_destination
	//////////////////////////////
	{"classname",	"info_warp_destination"},
	{"color",		"0.5 0.5 0.5"},
	{"size",		"-8.0 -8.0 -8.0 8.0 8.0 8.0"},

	{"rem",			"target - next target to warp to"},
	{"rem",			"speed - speed from this path point to the next"},

	//////////////////////////////
	// trigger_onlyregistered
	//////////////////////////////
	{"classname",	"trigger_onlyregistered"},
	{"color",		"0.5 0.5 0.5"},
	{"sound",		""},
	{"wait",		"2.0"},
	{"message",		""},

	{"rem",			"sound - name of sound to play upon firing"},
	{"rem",			"wait - the number of seconds between triggerings and"},
	{"rem",			"the length of time the message will be displayed"},

	//////////////////////////////
	// trigger_hurt
	//////////////////////////////
	{"classname",	"trigger_hurt"},
	{"color",		"0.5 0.5 0.5"},
	{"sound",		""},
	{"dmg",			"2.0"},
	{"wait",		"2.0"},
	{"message",		""},

	{"rem",			"sound - name of sound to play upon firing"},
	{"rem",			"dmg - the amount of damage the trigger will do to an object"},
	{"rem",			"wait - the number of seconds between triggerings and"},

	//////////////////////////////
	// trigger_push
	//////////////////////////////
	{"classname",	"trigger_push"},
	{"color",		"0.5 0.5 0.5"},
	{"sound",		""},
	{"message",		""},
	{"speed",		"1000.0"},
	{"flag",		"PUSH_ONCE"},
	{"rem",			"speed - the velocity to give the object"},
	{"rem",			"speed - the velocity to give the object"},

	//////////////////////////////
	// trigger_counter
	//////////////////////////////
	{"classname",	"trigger_counter"},
	{"color",		"0.5 0.5 0.5"},
	{"sound",		""},
	{"flag",		"NO_MESSAGE"},

	{"rem",			"sound - name of sound to play upon firing"},
	{"rem",			"message - message to display upon last triggering"},
	{"rem",			"when NO_MESSAGE is set, no messages are displayed upon triggering"},

	//////////////////////////////
	// trigger_changelevel
	//////////////////////////////
	{"classname",	"trigger_changelevel"},
	{"color",		"0.5 0.5 0.5"},
	{"sound",		""},
	{"map",			""},
	{"flag",		"NO_INTERMISSION"},

	{"rem",			"sound - name of sound to play upon firing"},
	{"rem",			"message - message to display upon last triggering"},

	//////////////////////////////
	// func_wall
	//////////////////////////////
	{"classname",	"func_wall"},
	{"color",		"0 0.5 0.8"},
	{"health",		"0"},
	{"message",		""},
	{"targetname",	""},
	{"target",		""},
	{"killtarget",	""},

	{"rem",			"targetname - the name of this wall if it is a target"},
	{"rem",			"target - the next entity to trigger when this one is triggered"},
	{"rem",			"killtarget - the targetname of the entity to remove when triggered"},

	//////////////////////////////
	// func_button
	//////////////////////////////
	{"classname",	"func_button"},
	{"color",		"0 0.5 0.8"},
	{"health",		"0"},
	{"targetname",	""},
	{"target",		""},
	{"killtarget",	""},
	{"speed",		""},
	{"wait",		""},
	{"angle",		""},
	{"lip",			""},
	{"flag",		"PUSH_TOUCH"},

	{"rem",			"targetname - the name of this wall if it is a target"},
	{"rem",			"target - the next entity to trigger when this one is triggered"},
	{"rem",			"killtarget - the targetname of the entity to remove when triggered"},
	{"rem",			"sound_use - the sound to play when the button is used (defaults to none)"},
	{"rem",			"sound_return - the sound to play when the button returns (defaults to none)"},
	{"rem",			"speed - rate of travel when button moves"},
	{"rem",			"wait - seconds to wait befor returning to useable (-1 = never return)"},
	{"rem",			"angle - direction of travel"},
	{"rem",			"lip - amount of button left sticking out after being pushed (default 4)"},
	{"rem",			"health - when > 0 the button must be killed in order to fire"},
	{"rem",			"PUSH_TOUCH will allow the button to be pushed by running into it, Quake style"},

	//////////////////////////////
	// func_multi_button
	//////////////////////////////
	{"classname",	"func_multi_button"},
	{"color",		"0 0.5 0.8"},
	{"health",		"0"},
	{"targetname",	""},
	{"target",		""},
	{"killtarget",	""},
	{"speed",		""},
	{"wait",		""},
	{"angle",		""},
	{"lip",			""},
	{"flag",		"PUSH_TOUCH"},
	{"flag",		"CYCLE"},

	{"rem",			"targetname - the name of this wall if it is a target"},
	{"rem",			"target - the next entity to trigger when this one is triggered"},
	{"rem",			"killtarget - the targetname of the entity to remove when triggered"},
	{"rem",			"sound_use - the sound to play when the button is used (defaults to none)"},
	{"rem",			"sound_return - the sound to play when the button returns (defaults to none)"},
	{"rem",			"speed - rate of travel when button moves"},
	{"rem",			"wait - seconds to wait befor returning to useable (-1 = never return)"},
	{"rem",			"angle - direction of travel"},
	{"rem",			"health - when > 0 the button must be killed in order to fire"},
	{"rem",			"distance - distance button travels on each push"},
	{"rem",			"count - number of positions this button has"},
	{"rem",			"PUSH_TOUCH will allow the button to be pushed by running into it, Quake style"},
	{"rem",			"CYCLE - button will not return to top from last position, but will go back through all positions"},

	//////////////////////////////
	// func_train
	//////////////////////////////
	{"classname",    "func_train"},
	{"distance",	 "90.0"},
	{"color",        "0 0.5 0.8"},
	{"rem",			"targetname - the name of this train"},
	{"rem",			"target - the path_corner that the train will spawn at"},
	{"rem",			"killtarget - the targetname of the entity to remove when triggered"},

	//////////////////////////////
	// func_train
	//////////////////////////////
	{"classname",    "func_train2"},
	{"distance",	 "90.0"},
	{"color",        "0 0.5 0.8"},
	{"rem",			"targetname - the name of this train"},
	{"rem",			"target - the path_corner that the train will spawn at"},
	{"rem",			"killtarget - the targetname of the entity to remove when triggered"},
	
	//////////////////////////////
	// path_corner_train
	//////////////////////////////
	{"classname",    "path_corner_train"},
	{"distance",	 "90.0"},
	{"color",        "0.5 0.3 0"},
	{"size",		 "-8 -8 -8 8 8 8"},
	{"flag",		"X_AXIS"},
	{"flag",		"Y_AXIS"},
	{"flag",		"Z_AXIS"},
	{"flag",		"TRIGWAIT"},
	{"rem",			"killtarget - the targetname of the entity to remove when this"},
	{"rem",			"path_corner is reached"},
	{"rem",			"speed - rate of travel from this path_corner to the next"},
	{"rem",			"wait - seconds to wait after the actions on this path_corner are complete"},
	{"rem",			"sound - sound to play at this path corner"},
	{"rem",			"x_distance - distance in degrees to rotate around x axis"},
	{"rem",			"y_distance - distance in degrees to rotate around y axis"},
	{"rem",			"z_distance - distance in degrees to rotate around z axis"},
	{"rem",			"x_speed - speed to rotate along x axis in degrees per second"},
	{"rem",			"y_speed - speed to rotate along y axis in degrees per second"},
	{"rem",			"z_speed - speed to rotate along z axis in degrees per second"},
	{"rem",			"health - if health is set, the train will wait at this path corner"},
	{"rem",			"until it is killed."},
	{"rem",			"Spawnflags:"},
	{"rem",			"X_AXIS - when checked train will rotate continually around "},
	{"rem",			"x axis at x_speed degrees per second"},
	{"rem",			"Y_AXIS - when checked train will rotate continually around "},
	{"rem",			"y axis at y_speed degrees per second"},
	{"rem",			"Z_AXIS - when checked train will rotate continually around "},
	{"rem",			"z axis at z_speed degrees per second"},
	{"rem",			"TRIGWAIT - wait here until triggered"},

	//////////////////////////////
	// info_player_start
	//////////////////////////////
	{"classname",    "info_player_start"},
	{"color",        "1 0 0"},
	{"size",         "-16 -16 -24 16 16 32"},
	{"rem",          "Location player starts in single play."},

	//////////////////////////////
	// info_mikiko_start
	//////////////////////////////
	{"classname",    "info_mikiko_start"},
	{"color",        "1 0 0"},
	{"size",         "-16 -16 -24 16 16 32"},
	{"rem",          "Location Mikiko starts in single play."},

	//////////////////////////////
	// info_superfly_start
	//////////////////////////////
	{"classname",    "info_superfly_start"},
	{"color",        "1 0 0"},
	{"size",         "-16 -16 -24 16 16 32"},
	{"rem",          "Location Superfly starts in single play."},

	//////////////////////////////
	// info_null
	//////////////////////////////
	{"classname",    "info_null"},
	{"color",        "1 0 0"},
	{"size",         "-8 -8 -8 8 8 8"},
	{"rem",          "You can point anything to this as a target"},

	//////////////////////////////
	// info_player_deathmatch
	//////////////////////////////
	{"classname",    "info_player_deathmatch"},
	{"color",        "1 0 1"},
	{"size",         "-16 -16 -24 16 16 32"},
	{"rem",          "Location player starts in deathmatch"},

	//////////////////////////////
	// info_player_coop
	//////////////////////////////
	{"classname",    "info_player_coop"},
	{"color",        "1 0 1"},
	{"size",         "-16 -16 -24 16 16 32"},
	{"rem",          "Location player starts in coop"},

	//////////////////////////////
	// func_door_secret
	//////////////////////////////
	{"classname",    "func_door_secret"},
	{"color",        "0 0.5 0.8"},
	{"health",		 "0"},
	{"speed",		 "100"},
	{"wait",		 "3"},
	{"lip",			 "8"},
	{"dmg",			 "2"},
	{"angle",		 "0 0 0"},
	{"message",		 "we all suck"},
	{"targetname",	 ""},
	{"flag",         "OPEN_ONCE"},
	{"flag",         "1ST_LEFT"},
	{"flag",         "1ST_DOWN"},
	{"flag",         "NO_SHOOT"},
	{"flag",		 "YES_SHOOT"},

	{"rem", "message - printed when the door is touched if it is a trigger door and it hasn't been fired yet"},
	{"rem", "angle - determines the opening direction"},
	{"rem", "targetname - if set, no touch field will be spawned and a remote button or trigger field activates the door."},
	{"rem", "health - if set, door must be shot open"},
	{"rem",	"speed - movement speed (100 default)"},
	{"rem", "wait - time to wait before returning (3 default, -1 = never return)"},
	{"rem",	"dmg - damage to inflict when blocked (2 default)"},
	{"rem", "sound_opening - name of the sound to play during opening, ie. doors/creek.wav"},
	{"rem",	"sound_open_finish - name of the sound to play when opening completes, ie. doors/slam.wav"},
	{"rem", "sound_closing - name of the sound to play when closing starts, ie. doors/creek.wav"},
	{"rem",	"sound_close_finish - name of the sound to play when closing completes, ie. doors/slam.wav"},
	{"rem", "  "},
	{"rem", "Spawnflags:"},
    {"rem", "SECRET_OPEN_ONCE - door stays open (imagine that...)"},
    {"rem", "SECRET_1ST_LEFT - first move is left of move direction"},
    {"rem", "SECRET_1ST_DOWN - first move is down from move direction"},
    {"rem", "SECRET_NO_SHOOT - only opened by a trigger"},
    {"rem", "SECRET_YES_SHOOT - shootable even if targeted"},

	//////////////////////////////
	// func_wall_explode
	//////////////////////////////
	{"classname",	"func_wall_explode"},
	{"color",		"0 0.5 0.8"},
	{"health",		"0"},
	{"message",		""},
	{"targetname",	""},
	{"target",		""},
	{"killtarget",	""},
	{"model_1",		""},
	{"model_2",		""},
	{"model_3",		""},
	{"flag",        "ROCK_CHUNKS"},
	{"flag",        "WOOD_CHUNKS"},
	{"flag",        "EXTRA_CHUNKS"},
	{"flag",        "EXTRA_VELOCITY"},
	{"flag",		"NO_CHUNKS"},
	{"flag",		"NO_SOUND"},

	{"rem",			"targetname - the name of this wall if it is a target"},
	{"rem",			"target - the next entity to trigger when this one is triggered"},
	{"rem",			"killtarget - the targetname of the entity to remove when triggered"},
	{"rem",			"health - ummm... this would be the health of the wall, if it is 0 then"},
	{"rem",			"the wall can only be exploded by targetting it"},
	{"rem",			"message - this prints out when wall go boom"},
	{"rem",			"model_1 - the specific pathname of the first model to throw when killed"},
	{"rem",			"if model_1 is set then spawnflags ROCK_CHUNKS and WOOD_CHUNKS are overridden"},
	{"rem",			"model_2 - the pathname of the second model"},
	{"rem",			"model_3 - I wouldn't bet on it, but this is probably the name of the 3rd model"},
	{"rem",			" "},
	{"rem", "Spawnflags:"},
    {"rem", "ROCK_CHUNKS - makes rock chunk wall-gibs fly"},
	{"rem",	"WOOD_CHUNKS - makes wood chunk wall-gibs fly"},
	{"rem",	"EXTRA_CHUNKS - makes up to 3 chunks per explosion, instead of just one"},
	{"rem",	"EXTRA_VELOCITY - gives chunks a higher velocity (good for underwater)"},
	{"rem", "NO_CHUNKS - um, no chunks"},
	{"rem", "NO_SOUND - no sound, use when lots of func walls are activated simultaneously to keep the"},
	{"rem", "Quake engine from choking with too many sounds at once."},

	//////////////////////////////
	// func_anim
	//////////////////////////////
	{"classname",	"func_anim"},
	{"color",		"0.0 1.0 0.1"},
	{"flag",        "VISIBLE"},
	{"rem", "Spawnflags:"},
    {"rem", "VISIBLE - start visible"},

	//////////////////////////////
	// func_floater
	//////////////////////////////
	{"classname",	"func_floater"},
	{"color",		"0 0.5 0.8"},
	{"size",		"-16 -16 -16 16 16 16"},
	{"model",		""},

	{"rem",			"model - pathname to model (ie. models/floater.mdl)"},
	{"rem",			"velocity_cap - maximum up/down velocity"},
	{"rem",			"dissipate - how fast velocity degrades to velocity_cap (default = 0.99)"},
	{"rem",			"object_mass - mass of object (mass / volume = density)"},
	{"rem",			"object_volume - volume of object (mass / volume = density)"},

	//////////////////////////////
	// func_debris
	//////////////////////////////

	{"classname",	"func_debris"},
	{"color",		"0 0.5 0.8"},
	{"target",		""},
	{"flag",		"GO_TO_ACTIVATOR"},
	{"flag",		"NO_ROTATE"},
	{"flag",		"MOMENTUM_DAMAGE"},
	{"flag",		"NO_ROTATION_ADJUST"},
	{"flag",		"DROP_ONLY"},
	{"flag",		"QUARTER_SIZE"},

	{"rem",			"target - debris will fly towards targeted entity"},
	{"rem",			"fly_sound - sound to play while the entity flies through the air"},
	{"rem",			"hit_sound - sound to play when the entity hits something"},
	{"rem",			"damage - amount of damage to do when hitting another object"},
	{"rem",			"if MOMENTUM_DAMAGE is selected, then damage will be based on the"},
	{"rem",			"speed of the debris when it impacts the object and damage becomes"},
	{"rem",			"the divisor, so if speed at impact = 300 and damge = 3, then damage done"},
	{"rem",			"while 300 / 3 = 100 points.  If damage is not set, then momentum damage"},
	{"rem",			"will be the default with a damage divisor of 3."},
	{"rem",			"Spawnflags:"},
	{"rem",			"GO_TO_ACTIVATOR - debris will fly at whoever activated it"},
	{"rem",			"NO_ROTATE - don't give this debris any random rotation"},
	{"rem",			"MOMENTUM_DAMAGE - damage based on velocity"},
	{"rem",			"DROP_ONLY - no upward velocity, debris just falls"},
	{"rem",			"QUARTER_SIZE - shrink bounding box by 1/4"},
	{"rem",			"Notes:"},
	{"rem",			"A func debris entity must have an origin brush contained in it,"},
	{"rem",			"otherwise it will rotate around the center of the level, not"},
	{"rem",			"its own center.  That is bad."},

	//////////////////////////////
	// func_debris_visible
	//////////////////////////////

	{"classname",	"func_debris_visible"},
	{"color",		"0 0.5 0.8"},
	{"target",		""},
	{"flag",		"GO_TO_ACTIVATOR"},
	{"flag",		"NO_ROTATE"},
	{"flag",		"MOMENTUM_DAMAGE"},
	{"flag",		"NO_ROTATION_ADJUST"},
	{"flag",		"DROP_ONLY"},
	{"flag",		"QUARTER_SIZE"},

	{"rem",			"SAME AS FUNC_DEBRIS EXCEPT DEBRIS IS VISIBLE BEFORE TARGETTING"},
	{"rem",			"target - debris will fly towards targeted entity"},
	{"rem",			"fly_sound - sound to play while the entity flies through the air"},
	{"rem",			"hit_sound - sound to play when the entity hits something"},
	{"rem",			"damage - amount of damage to do when hitting another object"},
	{"rem",			"if MOMENTUM_DAMAGE is selected, then damage will be based on the"},
	{"rem",			"speed of the debris when it impacts the object and damage becomes"},
	{"rem",			"the divisor, so if speed at impact = 300 and damge = 3, then damage done"},
	{"rem",			"while 300 / 3 = 100 points.  If damage is not set, then momentum damage"},
	{"rem",			"will be the default with a damage divisor of 3."},
	{"rem",			"Spawnflags:"},
	{"rem",			"GO_TO_ACTIVATOR - debris will fly at whoever activated it"},
	{"rem",			"NO_ROTATE - don't give this debris any random rotation"},
	{"rem",			"MOMENTUM_DAMAGE - damage based on velocity"},
	{"rem",			"DROP_ONLY - no upward velocity, debris just falls"},
	{"rem",			"QUARTER_SIZE - shrink bounding box by 1/4"},
	{"rem",			"Notes:"},
	{"rem",			"A func debris entity must have an origin brush contained in it,"},
	{"rem",			"otherwise it will rotate around the center of the level, not"},
	{"rem",			"its own center.  That is bad."},

	//////////////////////////////
	// func_particlefield
	//////////////////////////////
	{"classname",	"func_particlefield"},
	{"color",		"0 0.5 0.8"},
	{"flag",		"USE_COUNT"},
	{"rem",			"count - number of times to trigger before activated"},
	{"rem",			"color - color of particles use:"},
	{"rem",			"black, blue, green, cyan, red, "},
	{"rem",			"purple, brown, ltgray, dkgray, "},
	{"rem",			"ltblue, ltgreen, ltcyan, ltpurple,"},
	{"rem",			"yellow, white"},

	//////////////////////////////
	// func_monitor
	//////////////////////////////
	{"classname",	"func_monitor"},
	{"color",		"0 0.5 0.8"},
	{"rem",			"target - the info_camera that the view will be from"},
	{"rem",			"fov - the field of view when looking through this camera"},

	//////////////////////////////
	// info_camera
	//////////////////////////////
	{"classname",    "info_camera"},
	{"color",        "1 0 0"},
	{"size",         "-8 -8 -8 8 8 8"},

	//////////////////////////////
	// misc_lavaball_drop
	//////////////////////////////
	{"classname",	"misc_lavaball_drop"},
	{"size",		"-8 -8 -8 8 8 8"},
	{"color",		"1.0 0.5 0.0"},

	//////////////////////////////
	// misc_lavaball_toss
	//////////////////////////////
	{"classname",	"misc_lavaball_toss"},
	{"size",		"-8 -8 -8 8 8 8"},
	{"color",		"1.0 0.5 0.0"},

	{"rem",			"target = can be triggered"},
	{"rem",			"mintime = minimum time between tosses (default = 4.0 seconds)"},
	{"rem",			"maxtime = maximum time between tosses (default = 12.0 seconds)"},
	{"rem",			"damage = damage to do when an hitting something"},
	{"rem",			"upmin = minimum upward velocity (default = 200)"},
	{"rem",			"upmax = maximum upward velocity (default = 800)"},

	//////////////////////////////
	// sound_ambient
	//////////////////////////////
	
	{"classname",	"sound_ambient"},
	{"size",		"-8 -8 -8 8 8 8"},
	{"color",		"1 0 1"},

	{"rem",			"sound - path to ambient sound (ie. ambience/sound.wav."},
	{"rem",			"fade - distance multiplier to make sounds fade in further away from source."},
	{"rem",			"1.0 is normal, 2.0 is twice as far."},
	{"rem",			"volume - 0 through 255.  255 is max."},

	//////////////////////////////
	// sound_ambient
	//////////////////////////////
	
	{"classname",	"trigger_fog_value"},
	{"color",		"0 1 0"},

	{"rem",			"fog_value - the value for fog density 0 - 4.  0 is off, ie. r_drawfog 0"},

	//////////////////////////////
	// node_node
	//////////////////////////////
	
	{"classname",	"node_node"},
	{"color",		"0.5 0.5 1"},
	{"size",		"-8 -8 -8 8 8 8"},
	{"flag",		"NODE_DOOR"},

	{"rem",			"number - the number of this node"},
	{"rem",			"link - the number of the node linked to (can be up to four link fields)"},

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// Monsters
	// 
	//////////////////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////
	// Episode 1 monsters
	////////////////////////////////////////

    {"classname",    "monster_froginator"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A light and refreshing monster."},
	{"rem",          "One day I will read the spec and know what this"},
	{"rem",          "monster does."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_crox"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A realistic representation of your mother"},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_slaughterskeet"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_thunderskeet"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_venomvermin"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_tentaclor"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_sludgeminion"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"flag",		 "PATHFOLLOW"},
	{"rem",          "A big thing with a big beak."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_prisoner"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "gib him."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "func_prisoner_respawn"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"rem",			 "time - seconds until a new prisoner is spawned"},

    {"classname",    "monster_inmater"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-32 -32 -24 32 32 64"},
	{"flag",		 "WANDER"},
	{"flag",		 "PATHFOLLOW"},
	{"rem",          "A big thing with a big beak."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},
	{"rem",			 "PATH_FOLLOW - begin following a monster path"},

    {"classname",    "monster_squid"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_trackattack"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"rem",          "Track turrets need to have a target"},
	{"rem",          "which is the path_corner_track at which"},
	{"rem",          "they will start."},
	{"rem",          "Spawn about 16 units below the track to"},

    {"classname",    "monster_trackdaddy"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"rem",          "Track turrets need to have a target"},
	{"rem",          "which is the path_corner_track at which"},
	{"rem",          "they will start."},
	
    {"classname",    "monster_lasergat"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"rem",          "Track turrets need to have a target"},
	{"rem",          "which is the path_corner_track at which"},
	{"rem",          "they will start."},

    {"classname",    "monster_psyclaw"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big dog without a head and a"},
	{"rem",			 "brain for a back.  Cool, eh?"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_labworker"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "Lab worker.  Your momma."},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_battleboar"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A motorized, cybernetic pig, of course."},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

	//////////////////////////////
	//	path_corner_track
	//	
	//	path corners for tracked turrets only!
	//////////////////////////////
	
	{"classname",    "path_corner_track"},
	{"distance",	 "90.0"},
	{"color",        "0.5 0.3 0"},
	{"size",		 "-8 -8 -8 8 8 8"},

	////////////////////////////////////////
	// Episode 2 monsters
	////////////////////////////////////////

    {"classname",    "monster_skeleton"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "Where's he at?  Ahh!  He's in me!"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_spider"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-32 -32 -24 32 32 32"},
	{"flag",		 "WANDER"},
	{"rem",          "Dumbass."},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_tarantula"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "Dumbass."},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_griffon"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_harpy"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_centurion"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_siren"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_ferryman"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_satyr"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-32 -32 -24 32 32 64"},
	{"flag",		 "WANDER"},
	{"rem",          "This thing will beat you down."},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_column"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-32 -32 -24 32 32 64"},
	{"flag",		 "WANDER"},
	{"rem",          "Look at the tits on that statue!!"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

	////////////////////////////////////////
	// Episode 3 monsters
	////////////////////////////////////////

    {"classname",    "monster_plague_rat"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"rem",          "I broke you!"},
	{"flag",		 "WANDER"},
	{"rem",          "I broke you!"},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_rotworm"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "I beat the dictionary out of your filthy mouth!"},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_buboid"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "Dumbass."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_priest"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_doombat"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_lycanthir"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_fletcher"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_dwarf"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_dragonegg"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_babydragon"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

	////////////////////////////////////////
	// Episode 4 monsters
	////////////////////////////////////////

    {"classname",    "monster_gang1"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "Less than half a fucking man."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_gang2"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "You're fucking dumb!  Suck it down."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_blackprisoner"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_whiteprisoner"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_femgang"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_rocketdude"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_chaingang"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "A big thing with a big beak."},
	{"rem",          "radius - wander radius"},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monster_labmonkey"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-16 -16 -24 16 16 32"},
	{"flag",		 "WANDER"},
	{"rem",          "Lab monkey.  This thing will crack your skull."},
	{"rem",			 "Spawnflags:"},
	{"rem",			 "WANDER - monster will wander around"},

    {"classname",    "monkey_message"},
	{"color",        "1.0 0.0 0.0"},
    {"size",         "-8 -8 -8 8 8 8"},
	{"rem",          "Target this from the monkey switch to display current monkey state."},

	//////////////////////////////////////////////////////////////////
	// monster AI markers
	//////////////////////////////////////////////////////////////////

	{"classname",	"monster_path_corner"},
	{"distance",	"90.0"},
	{"color",		"0.5 0.3 0"},
	{"size",		"-8 -8 -8 8 8 8"},
	{"rem",			"A path corner for a monster to travel along"},
	{"rem"			"target1 - target4: "},
	{"rem",			"Can have up to 4 targets for the monster"},
	{"rem",			"to travel to.  If there is more than one target"},
	{"rem",			"the monster will randomly choose the next target"},
	{"rem",			"from those available."},
	{"rem",			"action1 - action4: "},
	{"rem",			"Use the action field to make a monster perform a"},
	{"rem",			"specific action at the path corner.  For example"},
	{"rem",			"'action scoop' will make a sludge minion scoop"},
	{"rem",			"sludge at that path corner (other monsters will"},
	{"rem",			"ignore the scoop command.  You can have multiple"},
	{"rem",			"actions on a path corner, for instance:"},
	{"rem",			"	action1	scoop"},
	{"rem",			"	action2	interrogate"},
	{"rem",			"Sludge minions reaching this path corner will scoop"},
	{"rem",			"while Inmaters will search out nearest prisoner"},


	//////////////////////////////////////////////////////////////////
	// decorations
	//////////////////////////////////////////////////////////////////

    {"classname",	"deco_e1"},
	{"color",		"1.0 0.0 0.0"},
    {"size",		"-16 -16 -24 16 16 0"},
	{"flag",		"DECO_EXPLODE"},
	{"flag",		"DECO_NO_EXPLODE"},
	{"flag",		"DECO_PUSHABLE"},
	{"rem",			"model - choose model # -- see list."},

    {"classname",	"deco_e2"},
	{"color",		"1.0 0.0 0.0"},
    {"size",		"-16 -16 -24 16 16 0"},
	{"flag",		"DECO_EXPLODE"},
	{"flag",		"DECO_NO_EXPLODE"},
	{"flag",		"DECO_PUSHABLE"},
	{"rem",			"model - choose model # -- see list."},

    {"classname",	"deco_e3"},
	{"color",		"1.0 0.0 0.0"},
    {"size",		"-16 -16 -24 16 16 0"},
	{"flag",		"DECO_EXPLODE"},
	{"flag",		"DECO_NO_EXPLODE"},
	{"flag",		"DECO_PUSHABLE"},
	{"rem",			"model - choose model # -- see list."},

    {"classname",	"deco_e4"},
	{"color",		"1.0 0.0 0.0"},
    {"size",		"-16 -16 -24 16 16 0"},
	{"flag",		"DECO_EXPLODE"},
	{"flag",		"DECO_NO_EXPLODE"},
	{"flag",		"DECO_PUSHABLE"},
	{"rem",			"model - choose model # -- see list."},

	{NULL,            NULL},
};

#endif