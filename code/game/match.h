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

// make sure this is the same character as we use in chats in g_cmd.c
#define EC	"\x19"

//match template contexts
#define MTCONTEXT_MISC					2
#define MTCONTEXT_INITIALTEAMCHAT		4
#define MTCONTEXT_TIME					8
#define MTCONTEXT_TEAMMATE				16
#define MTCONTEXT_ADDRESSEE				32
#define MTCONTEXT_PATROLKEYAREA			64
#define MTCONTEXT_REPLYCHAT				128
#define MTCONTEXT_CTF					256

//message types
#define MSG_NEWLEADER					1		//new leader
#define MSG_ENTERGAME					2		//enter game message
#define MSG_HELP						3		//help someone
#define MSG_ACCOMPANY					4		//accompany someone
#define MSG_DEFENDKEYAREA				5		//defend a key area
#define MSG_RUSHBASE					6		//everyone rush to base
#define MSG_GETFLAG						7		//get the enemy flag
#define MSG_STARTTEAMLEADERSHIP			8		//someone wants to become the team leader
#define MSG_STOPTEAMLEADERSHIP			9		//someone wants to stop being the team leader
#define MSG_WHOISTEAMLAEDER				10		//who is the team leader
#define MSG_WAIT						11		//wait for someone
#define MSG_WHATAREYOUDOING				12		//what are you doing?
#define MSG_JOINSUBTEAM					13		//join a sub-team
#define MSG_LEAVESUBTEAM				14		//leave a sub-team
#define MSG_CREATENEWFORMATION			15		//create a new formation
#define MSG_FORMATIONPOSITION			16		//tell someone his/her position in a formation
#define MSG_FORMATIONSPACE				17		//set the formation intervening space
#define MSG_DOFORMATION					18		//form a known formation
#define MSG_DISMISS						19		//dismiss commanded team mates
#define MSG_CAMP						20		//camp somewhere
#define MSG_CHECKPOINT					21		//remember a check point
#define MSG_PATROL						22		//patrol between certain keypoints
#define MSG_LEADTHEWAY					23		//lead the way
#define MSG_GETITEM						24		//get an item
#define MSG_KILL						25		//kill someone
#define MSG_WHEREAREYOU					26		//where is someone
#define MSG_RETURNFLAG					27		//return the flag
#define MSG_WHATISMYCOMMAND				28		//ask the team leader what to do
#define MSG_WHICHTEAM					29		//ask which team a bot is in
#define MSG_TASKPREFERENCE				30		//tell your teamplay task preference
#define MSG_ATTACKENEMYBASE				31		//attack the enemy base
#define MSG_HARVEST						32		//go harvest
#define MSG_SUICIDE						33		//order to suicide
//
#define MSG_ME							100
#define MSG_EVERYONE					101
#define MSG_MULTIPLENAMES				102
#define MSG_NAME						103
#define MSG_PATROLKEYAREA				104
#define MSG_MINUTES						105
#define MSG_SECONDS						106
#define MSG_FOREVER						107
#define MSG_FORALONGTIME				108
#define MSG_FORAWHILE					109
//
#define MSG_CHATALL						200
#define MSG_CHATTEAM					201
#define MSG_CHATTELL					202
//
#define MSG_CTF							300		//ctf message

//command sub types
#define ST_SOMEWHERE					0
#define ST_NEARITEM						1
#define ST_ADDRESSED					2
#define ST_METER						4
#define ST_FEET							8
#define ST_TIME							16
#define ST_HERE							32
#define ST_THERE						64
#define ST_I							128
#define ST_MORE							256
#define ST_BACK							512
#define ST_REVERSE						1024
#define ST_SOMEONE						2048
#define ST_GOTFLAG						4096
#define ST_CAPTUREDFLAG					8192
#define ST_RETURNEDFLAG					16384
#define ST_TEAM							32768
#define ST_1FCTFGOTFLAG					65535
//ctf task preferences
#define ST_DEFENDER						1
#define ST_ATTACKER						2
#define ST_ROAMER						4


//word replacement variables
#define THE_ENEMY						7
#define THE_TEAM						7
//team message variables
#define NETNAME							0
#define PLACE							1
#define FLAG							1
#define MESSAGE							2
#define ADDRESSEE						2
#define ITEM							3
#define TEAMMATE						4
#define TEAMNAME						4
#define ENEMY							4
#define KEYAREA							5
#define FORMATION						5
#define POSITION						5
#define NUMBER							5
#define TIME							6
#define NAME							6
#define MORE							6


