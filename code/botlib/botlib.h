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
//
/*****************************************************************************
 * name:		botlib.h
 *
 * desc:		bot AI library
 *
 * $Archive: /source/code/game/botai.h $
 *
 *****************************************************************************/

#define	BOTLIB_API_VERSION		2

struct aas_clientmove_s;
struct aas_entityinfo_s;
struct aas_areainfo_s;
struct aas_altroutegoal_s;
struct aas_predictroute_s;
struct bot_consolemessage_s;
struct bot_match_s;
struct bot_goal_s;
struct bot_moveresult_s;
struct bot_initmove_s;
struct weaponinfo_s;

#define BOTFILESBASEFOLDER		"botfiles"
//debug line colors
#define LINECOLOR_NONE			-1
#define LINECOLOR_RED			1//0xf2f2f0f0L
#define LINECOLOR_GREEN			2//0xd0d1d2d3L
#define LINECOLOR_BLUE			3//0xf3f3f1f1L
#define LINECOLOR_YELLOW		4//0xdcdddedfL
#define LINECOLOR_ORANGE		5//0xe0e1e2e3L

//Print types
#define PRT_MESSAGE				1
#define PRT_WARNING				2
#define PRT_ERROR				3
#define PRT_FATAL				4
#define PRT_EXIT				5

//console message types
#define CMS_NORMAL				0
#define CMS_CHAT				1

//botlib error codes
#define BLERR_NOERROR					0	//no error
#define BLERR_LIBRARYNOTSETUP			1	//library not setup
#define BLERR_INVALIDENTITYNUMBER		2	//invalid entity number
#define BLERR_NOAASFILE					3	//no AAS file available
#define BLERR_CANNOTOPENAASFILE			4	//cannot open AAS file
#define BLERR_WRONGAASFILEID			5	//incorrect AAS file id
#define BLERR_WRONGAASFILEVERSION		6	//incorrect AAS file version
#define BLERR_CANNOTREADAASLUMP			7	//cannot read AAS file lump
#define BLERR_CANNOTLOADICHAT			8	//cannot load initial chats
#define BLERR_CANNOTLOADITEMWEIGHTS		9	//cannot load item weights
#define BLERR_CANNOTLOADITEMCONFIG		10	//cannot load item config
#define BLERR_CANNOTLOADWEAPONWEIGHTS	11	//cannot load weapon weights
#define BLERR_CANNOTLOADWEAPONCONFIG	12	//cannot load weapon config

//action flags
#define ACTION_ATTACK			0x00000001
#define ACTION_USE			0x00000002
#define ACTION_RESPAWN			0x00000008
#define ACTION_JUMP			0x00000010
#define ACTION_MOVEUP			0x00000020
#define ACTION_CROUCH			0x00000080
#define ACTION_MOVEDOWN			0x00000100
#define ACTION_MOVEFORWARD		0x00000200
#define ACTION_MOVEBACK			0x00000800
#define ACTION_MOVELEFT			0x00001000
#define ACTION_MOVERIGHT		0x00002000
#define ACTION_DELAYEDJUMP		0x00008000
#define ACTION_TALK			0x00010000
#define ACTION_GESTURE			0x00020000
#define ACTION_WALK			0x00080000
#define ACTION_AFFIRMATIVE		0x00100000
#define ACTION_NEGATIVE			0x00200000
#define ACTION_GETFLAG			0x00800000
#define ACTION_GUARDBASE		0x01000000
#define ACTION_PATROL			0x02000000
#define ACTION_FOLLOWME			0x08000000
#define ACTION_JUMPEDLASTFRAME		0x10000000

//the bot input, will be converted to an usercmd_t
typedef struct bot_input_s
{
	float thinktime;		//time since last output (in seconds)
	vec3_t dir;				//movement direction
	float speed;			//speed in the range [0, 400]
	vec3_t viewangles;		//the view angles
	int actionflags;		//one of the ACTION_? flags
	int weapon;				//weapon to use
} bot_input_t;

#ifndef BSPTRACE

#define BSPTRACE

//bsp_trace_t hit surface
typedef struct bsp_surface_s
{
	char name[16];
	int flags;
	int value;
} bsp_surface_t;

//remove the bsp_trace_s structure definition l8r on
//a trace is returned when a box is swept through the world
typedef struct bsp_trace_s
{
	qboolean		allsolid;	// if true, plane is not valid
	qboolean		startsolid;	// if true, the initial point was in a solid area
	float			fraction;	// time completed, 1.0 = didn't hit anything
	vec3_t			endpos;		// final position
	cplane_t		plane;		// surface normal at impact
	float			exp_dist;	// expanded plane distance
	int				sidenum;	// number of the brush side hit
	bsp_surface_t	surface;	// the hit point surface
	int				contents;	// contents on other side of surface hit
	int				ent;		// number of entity hit
} bsp_trace_t;

#endif	// BSPTRACE

//entity state
typedef struct bot_entitystate_s
{
	int		type;			// entity type
	int		flags;			// entity flags
	vec3_t	origin;			// origin of the entity
	vec3_t	angles;			// angles of the model
	vec3_t	old_origin;		// for lerping
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
} bot_entitystate_t;

//bot AI library exported functions
typedef struct botlib_import_s
{
	//print messages from the bot library
	void		(QDECL *Print)(int type, char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
	//trace a bbox through the world
	void		(*Trace)(bsp_trace_t *trace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask);
	//trace a bbox against a specific entity
	void		(*EntityTrace)(bsp_trace_t *trace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int entnum, int contentmask);
	//retrieve the contents at the given point
	int			(*PointContents)(vec3_t point);
	//check if the point is in potential visible sight
	int			(*inPVS)(vec3_t p1, vec3_t p2);
	//retrieve the BSP entity data lump
	char		*(*BSPEntityData)(void);
	//
	void		(*BSPModelMinsMaxsOrigin)(int modelnum, vec3_t angles, vec3_t mins, vec3_t maxs, vec3_t origin);
	//send a bot client command
	void		(*BotClientCommand)(int client, char *command);
	//memory allocation
	void		*(*GetMemory)(int size);		// allocate from Zone
	void		(*FreeMemory)(void *ptr);		// free memory from Zone
	int			(*AvailableMemory)(void);		// available Zone memory
	void		*(*HunkAlloc)(int size);		// allocate from hunk
	//file system access
	int			(*FS_FOpenFile)( const char *qpath, fileHandle_t *file, fsMode_t mode );
	int			(*FS_Read)( void *buffer, int len, fileHandle_t f );
	int			(*FS_Write)( const void *buffer, int len, fileHandle_t f );
	void		(*FS_FCloseFile)( fileHandle_t f );
	int			(*FS_Seek)( fileHandle_t f, long offset, int origin );
	//debug visualisation stuff
	int			(*DebugLineCreate)(void);
	void		(*DebugLineDelete)(int line);
	void		(*DebugLineShow)(int line, vec3_t start, vec3_t end, int color);
	//
	int			(*DebugPolygonCreate)(int color, int numPoints, vec3_t *points);
	void		(*DebugPolygonDelete)(int id);
} botlib_import_t;

typedef struct aas_export_s
{
	//-----------------------------------
	// be_aas_entity.h
	//-----------------------------------
	void		(*AAS_EntityInfo)(int entnum, struct aas_entityinfo_s *info);
	//-----------------------------------
	// be_aas_main.h
	//-----------------------------------
	int			(*AAS_Initialized)(void);
	void		(*AAS_PresenceTypeBoundingBox)(int presencetype, vec3_t mins, vec3_t maxs);
	float		(*AAS_Time)(void);
	//--------------------------------------------
	// be_aas_sample.c
	//--------------------------------------------
	int			(*AAS_PointAreaNum)(vec3_t point);
	int			(*AAS_PointReachabilityAreaIndex)( vec3_t point );
	int			(*AAS_TraceAreas)(vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas);
	int			(*AAS_BBoxAreas)(vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas);
	int			(*AAS_AreaInfo)( int areanum, struct aas_areainfo_s *info );
	//--------------------------------------------
	// be_aas_bspq3.c
	//--------------------------------------------
	int			(*AAS_PointContents)(vec3_t point);
	int			(*AAS_NextBSPEntity)(int ent);
	int			(*AAS_ValueForBSPEpairKey)(int ent, char *key, char *value, int size);
	int			(*AAS_VectorForBSPEpairKey)(int ent, char *key, vec3_t v);
	int			(*AAS_FloatForBSPEpairKey)(int ent, char *key, float *value);
	int			(*AAS_IntForBSPEpairKey)(int ent, char *key, int *value);
	//--------------------------------------------
	// be_aas_reach.c
	//--------------------------------------------
	int			(*AAS_AreaReachability)(int areanum);
	//--------------------------------------------
	// be_aas_route.c
	//--------------------------------------------
	int			(*AAS_AreaTravelTimeToGoalArea)(int areanum, vec3_t origin, int goalareanum, int travelflags);
	int			(*AAS_EnableRoutingArea)(int areanum, int enable);
	int			(*AAS_PredictRoute)(struct aas_predictroute_s *route, int areanum, vec3_t origin,
							int goalareanum, int travelflags, int maxareas, int maxtime,
							int stopevent, int stopcontents, int stoptfl, int stopareanum);
	//--------------------------------------------
	// be_aas_altroute.c
	//--------------------------------------------
	int			(*AAS_AlternativeRouteGoals)(vec3_t start, int startareanum, vec3_t goal, int goalareanum, int travelflags,
										struct aas_altroutegoal_s *altroutegoals, int maxaltroutegoals,
										int type);
	//--------------------------------------------
	// be_aas_move.c
	//--------------------------------------------
	int			(*AAS_Swimming)(vec3_t origin);
	int			(*AAS_PredictClientMovement)(struct aas_clientmove_s *move,
											int entnum, vec3_t origin,
											int presencetype, int onground,
											vec3_t velocity, vec3_t cmdmove,
											int cmdframes,
											int maxframes, float frametime,
											int stopevent, int stopareanum, int visualize);
} aas_export_t;

typedef struct ea_export_s
{
	//ClientCommand elementary actions
	void	(*EA_Command)(int client, char *command );
	void	(*EA_Say)(int client, char *str);
	void	(*EA_SayTeam)(int client, char *str);
	//
	void	(*EA_Action)(int client, int action);
	void	(*EA_Gesture)(int client);
	void	(*EA_Talk)(int client);
	void	(*EA_Attack)(int client);
	void	(*EA_Use)(int client);
	void	(*EA_Respawn)(int client);
	void	(*EA_MoveUp)(int client);
	void	(*EA_MoveDown)(int client);
	void	(*EA_MoveForward)(int client);
	void	(*EA_MoveBack)(int client);
	void	(*EA_MoveLeft)(int client);
	void	(*EA_MoveRight)(int client);
	void	(*EA_Crouch)(int client);

	void	(*EA_SelectWeapon)(int client, int weapon);
	void	(*EA_Jump)(int client);
	void	(*EA_DelayedJump)(int client);
	void	(*EA_Move)(int client, vec3_t dir, float speed);
	void	(*EA_View)(int client, vec3_t viewangles);
	//send regular input to the server
	void	(*EA_EndRegular)(int client, float thinktime);
	void	(*EA_GetInput)(int client, float thinktime, bot_input_t *input);
	void	(*EA_ResetInput)(int client);
} ea_export_t;

typedef struct ai_export_s
{
	//-----------------------------------
	// be_ai_char.h
	//-----------------------------------
	int		(*BotLoadCharacter)(char *charfile, float skill);
	void	(*BotFreeCharacter)(int character);
	float	(*Characteristic_Float)(int character, int index);
	float	(*Characteristic_BFloat)(int character, int index, float min, float max);
	int		(*Characteristic_Integer)(int character, int index);
	int		(*Characteristic_BInteger)(int character, int index, int min, int max);
	void	(*Characteristic_String)(int character, int index, char *buf, int size);
	//-----------------------------------
	// be_ai_chat.h
	//-----------------------------------
	int		(*BotAllocChatState)(void);
	void	(*BotFreeChatState)(int handle);
	void	(*BotQueueConsoleMessage)(int chatstate, int type, char *message);
	void	(*BotRemoveConsoleMessage)(int chatstate, int handle);
	int		(*BotNextConsoleMessage)(int chatstate, struct bot_consolemessage_s *cm);
	int		(*BotNumConsoleMessages)(int chatstate);
	void	(*BotInitialChat)(int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7);
	int		(*BotNumInitialChats)(int chatstate, char *type);
	int		(*BotReplyChat)(int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7);
	int		(*BotChatLength)(int chatstate);
	void	(*BotEnterChat)(int chatstate, int client, int sendto);
	void	(*BotGetChatMessage)(int chatstate, char *buf, int size);
	int		(*StringContains)(char *str1, char *str2, int casesensitive);
	int		(*BotFindMatch)(char *str, struct bot_match_s *match, unsigned long int context);
	void	(*BotMatchVariable)(struct bot_match_s *match, int variable, char *buf, int size);
	void	(*UnifyWhiteSpaces)(char *string);
	void	(*BotReplaceSynonyms)(char *string, unsigned long int context);
	int		(*BotLoadChatFile)(int chatstate, char *chatfile, char *chatname);
	void	(*BotSetChatGender)(int chatstate, int gender);
	void	(*BotSetChatName)(int chatstate, char *name, int client);
	//-----------------------------------
	// be_ai_goal.h
	//-----------------------------------
	void	(*BotResetGoalState)(int goalstate);
	void	(*BotResetAvoidGoals)(int goalstate);
	void	(*BotRemoveFromAvoidGoals)(int goalstate, int number);
	void	(*BotPushGoal)(int goalstate, struct bot_goal_s *goal);
	void	(*BotPopGoal)(int goalstate);
	void	(*BotEmptyGoalStack)(int goalstate);
	void	(*BotDumpAvoidGoals)(int goalstate);
	void	(*BotDumpGoalStack)(int goalstate);
	void	(*BotGoalName)(int number, char *name, int size);
	int		(*BotGetTopGoal)(int goalstate, struct bot_goal_s *goal);
	int		(*BotGetSecondGoal)(int goalstate, struct bot_goal_s *goal);
	int		(*BotChooseLTGItem)(int goalstate, vec3_t origin, int *inventory, int travelflags);
	int		(*BotChooseNBGItem)(int goalstate, vec3_t origin, int *inventory, int travelflags,
								struct bot_goal_s *ltg, float maxtime);
	int		(*BotTouchingGoal)(vec3_t origin, struct bot_goal_s *goal);
	int		(*BotItemGoalInVisButNotVisible)(int viewer, vec3_t eye, vec3_t viewangles, struct bot_goal_s *goal);
	int		(*BotGetLevelItemGoal)(int index, char *classname, struct bot_goal_s *goal);
	int		(*BotGetNextCampSpotGoal)(int num, struct bot_goal_s *goal);
	int		(*BotGetMapLocationGoal)(char *name, struct bot_goal_s *goal);
	float	(*BotAvoidGoalTime)(int goalstate, int number);
	void	(*BotSetAvoidGoalTime)(int goalstate, int number, float avoidtime);
	void	(*BotInitLevelItems)(void);
	void	(*BotUpdateEntityItems)(void);
	int		(*BotLoadItemWeights)(int goalstate, char *filename);
	void	(*BotFreeItemWeights)(int goalstate);
	void	(*BotInterbreedGoalFuzzyLogic)(int parent1, int parent2, int child);
	void	(*BotSaveGoalFuzzyLogic)(int goalstate, char *filename);
	void	(*BotMutateGoalFuzzyLogic)(int goalstate, float range);
	int		(*BotAllocGoalState)(int client);
	void	(*BotFreeGoalState)(int handle);
	//-----------------------------------
	// be_ai_move.h
	//-----------------------------------
	void	(*BotResetMoveState)(int movestate);
	void	(*BotMoveToGoal)(struct bot_moveresult_s *result, int movestate, struct bot_goal_s *goal, int travelflags);
	int		(*BotMoveInDirection)(int movestate, vec3_t dir, float speed, int type);
	void	(*BotResetAvoidReach)(int movestate);
	void	(*BotResetLastAvoidReach)(int movestate);
	int		(*BotReachabilityArea)(vec3_t origin, int testground);
	int		(*BotMovementViewTarget)(int movestate, struct bot_goal_s *goal, int travelflags, float lookahead, vec3_t target);
	int		(*BotPredictVisiblePosition)(vec3_t origin, int areanum, struct bot_goal_s *goal, int travelflags, vec3_t target);
	int		(*BotAllocMoveState)(void);
	void	(*BotFreeMoveState)(int handle);
	void	(*BotInitMoveState)(int handle, struct bot_initmove_s *initmove);
	void	(*BotAddAvoidSpot)(int movestate, vec3_t origin, float radius, int type);
	//-----------------------------------
	// be_ai_weap.h
	//-----------------------------------
	int		(*BotChooseBestFightWeapon)(int weaponstate, int *inventory);
	void	(*BotGetWeaponInfo)(int weaponstate, int weapon, struct weaponinfo_s *weaponinfo);
	int		(*BotLoadWeaponWeights)(int weaponstate, char *filename);
	int		(*BotAllocWeaponState)(void);
	void	(*BotFreeWeaponState)(int weaponstate);
	void	(*BotResetWeaponState)(int weaponstate);
	//-----------------------------------
	// be_ai_gen.h
	//-----------------------------------
	int		(*GeneticParentsAndChildSelection)(int numranks, float *ranks, int *parent1, int *parent2, int *child);
} ai_export_t;

//bot AI library imported functions
typedef struct botlib_export_s
{
	//Area Awareness System functions
	aas_export_t aas;
	//Elementary Action functions
	ea_export_t ea;
	//AI functions
	ai_export_t ai;
	//setup the bot library, returns BLERR_
	int (*BotLibSetup)(void);
	//shutdown the bot library, returns BLERR_
	int (*BotLibShutdown)(void);
	//sets a library variable returns BLERR_
	int (*BotLibVarSet)(char *var_name, char *value);
	//gets a library variable returns BLERR_
	int (*BotLibVarGet)(char *var_name, char *value, int size);

	//sets a C-like define returns BLERR_
	int (*PC_AddGlobalDefine)(char *string);
	int (*PC_LoadSourceHandle)(const char *filename);
	int (*PC_FreeSourceHandle)(int handle);
	int (*PC_ReadTokenHandle)(int handle, pc_token_t *pc_token);
	int (*PC_SourceFileAndLine)(int handle, char *filename, int *line);

	//start a frame in the bot library
	int (*BotLibStartFrame)(float time);
	//load a new map in the bot library
	int (*BotLibLoadMap)(const char *mapname);
	//entity updates
	int (*BotLibUpdateEntity)(int ent, bot_entitystate_t *state);
	//just for testing
	int (*Test)(int parm0, char *parm1, vec3_t parm2, vec3_t parm3);
} botlib_export_t;

//linking of bot library
botlib_export_t *GetBotLibAPI( int apiVersion, botlib_import_t *import );

/* Library variables:

name:						default:			module(s):			description:

"basedir"					""					l_utils.c			base directory
"gamedir"					""					l_utils.c			game directory
"cddir"						""					l_utils.c			CD directory

"log"						"0"					l_log.c				enable/disable creating a log file
"maxclients"				"4"					be_interface.c		maximum number of clients
"maxentities"				"1024"				be_interface.c		maximum number of entities
"bot_developer"				"0"					be_interface.c		bot developer mode (it's "botDeveloper" in C to prevent symbol clash).

"phys_friction"				"6"					be_aas_move.c		ground friction
"phys_stopspeed"			"100"				be_aas_move.c		stop speed
"phys_gravity"				"800"				be_aas_move.c		gravity value
"phys_waterfriction"		"1"					be_aas_move.c		water friction
"phys_watergravity"			"400"				be_aas_move.c		gravity in water
"phys_maxvelocity"			"320"				be_aas_move.c		maximum velocity
"phys_maxwalkvelocity"		"320"				be_aas_move.c		maximum walk velocity
"phys_maxcrouchvelocity"	"100"				be_aas_move.c		maximum crouch velocity
"phys_maxswimvelocity"		"150"				be_aas_move.c		maximum swim velocity
"phys_walkaccelerate"		"10"				be_aas_move.c		walk acceleration
"phys_airaccelerate"		"1"					be_aas_move.c		air acceleration
"phys_swimaccelerate"		"4"					be_aas_move.c		swim acceleration
"phys_maxstep"				"18"				be_aas_move.c		maximum step height
"phys_maxsteepness"			"0.7"				be_aas_move.c		maximum floor steepness
"phys_maxbarrier"			"32"				be_aas_move.c		maximum barrier height
"phys_maxwaterjump"			"19"				be_aas_move.c		maximum waterjump height
"phys_jumpvel"				"270"				be_aas_move.c		jump z velocity
"phys_falldelta5"			"40"				be_aas_move.c
"phys_falldelta10"			"60"				be_aas_move.c
"rs_waterjump"				"400"				be_aas_move.c
"rs_teleport"				"50"				be_aas_move.c
"rs_barrierjump"			"100"				be_aas_move.c
"rs_startcrouch"			"300"				be_aas_move.c
"rs_startgrapple"			"500"				be_aas_move.c
"rs_startwalkoffledge"		"70"				be_aas_move.c
"rs_startjump"				"300"				be_aas_move.c
"rs_rocketjump"				"500"				be_aas_move.c
"rs_bfgjump"				"500"				be_aas_move.c
"rs_jumppad"				"250"				be_aas_move.c
"rs_aircontrolledjumppad"	"300"				be_aas_move.c
"rs_funcbob"				"300"				be_aas_move.c
"rs_startelevator"			"50"				be_aas_move.c
"rs_falldamage5"			"300"				be_aas_move.c
"rs_falldamage10"			"500"				be_aas_move.c
"rs_maxjumpfallheight"		"450"				be_aas_move.c

"max_aaslinks"				"4096"				be_aas_sample.c		maximum links in the AAS
"max_routingcache"			"4096"				be_aas_route.c		maximum routing cache size in KB
"forceclustering"			"0"					be_aas_main.c		force recalculation of clusters
"forcereachability"			"0"					be_aas_main.c		force recalculation of reachabilities
"forcewrite"				"0"					be_aas_main.c		force writing of aas file
"aasoptimize"				"0"					be_aas_main.c		enable aas optimization
"sv_mapChecksum"			"0"					be_aas_main.c		BSP file checksum
"bot_visualizejumppads"		"0"					be_aas_reach.c		visualize jump pads

"bot_reloadcharacters"		"0"					-					reload bot character files
"ai_gametype"				"0"					be_ai_goal.c		game type
"droppedweight"				"1000"				be_ai_goal.c		additional dropped item weight
"weapindex_rocketlauncher"	"5"					be_ai_move.c		rl weapon index for rocket jumping
"weapindex_bfg10k"			"9"					be_ai_move.c		bfg weapon index for bfg jumping
"weapindex_grapple"			"10"				be_ai_move.c		grapple weapon index for grappling
"entitytypemissile"			"3"					be_ai_move.c		ET_MISSILE
"offhandgrapple"			"0"					be_ai_move.c		enable off hand grapple hook
"cmd_grappleon"				"grappleon"			be_ai_move.c		command to activate off hand grapple
"cmd_grappleoff"			"grappleoff"		be_ai_move.c		command to deactivate off hand grapple
"itemconfig"				"items.c"			be_ai_goal.c		item configuration file
"weaponconfig"				"weapons.c"			be_ai_weap.c		weapon configuration file
"synfile"					"syn.c"				be_ai_chat.c		file with synonyms
"rndfile"					"rnd.c"				be_ai_chat.c		file with random strings
"matchfile"					"match.c"			be_ai_chat.c		file with match strings
"nochat"					"0"					be_ai_chat.c		disable chats
"max_messages"				"1024"				be_ai_chat.c		console message heap size
"max_weaponinfo"			"32"				be_ai_weap.c		maximum number of weapon info
"max_projectileinfo"		"32"				be_ai_weap.c		maximum number of projectile info
"max_iteminfo"				"256"				be_ai_goal.c		maximum number of item info
"max_levelitems"			"256"				be_ai_goal.c		maximum number of level items

*/

