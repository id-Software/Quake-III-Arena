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

/*****************************************************************************
 * name:		be_ai_char.c
 *
 * desc:		bot characters
 *
 * $Archive: /MissionPack/code/botlib/be_ai_char.c $
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "l_log.h"
#include "l_memory.h"
#include "l_utils.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_libvar.h"
#include "aasfile.h"
#include "botlib.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_ai_char.h"

#define MAX_CHARACTERISTICS		80

#define CT_INTEGER				1
#define CT_FLOAT				2
#define CT_STRING				3

#define DEFAULT_CHARACTER		"bots/default_c.c"

//characteristic value
union cvalue
{
	int integer;
	float _float;
	char *string;
};
//a characteristic
typedef struct bot_characteristic_s
{
	char type;						//characteristic type
	union cvalue value;				//characteristic value
} bot_characteristic_t;

//a bot character
typedef struct bot_character_s
{
	char filename[MAX_QPATH];
	float skill;
	bot_characteristic_t c[1];		//variable sized
} bot_character_t;

bot_character_t *botcharacters[MAX_CLIENTS + 1];

//========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//========================================================================
bot_character_t *BotCharacterFromHandle(int handle)
{
	if (handle <= 0 || handle > MAX_CLIENTS)
	{
		botimport.Print(PRT_FATAL, "character handle %d out of range\n", handle);
		return NULL;
	} //end if
	if (!botcharacters[handle])
	{
		botimport.Print(PRT_FATAL, "invalid character %d\n", handle);
		return NULL;
	} //end if
	return botcharacters[handle];
} //end of the function BotCharacterFromHandle
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotDumpCharacter(bot_character_t *ch)
{
	int i;

	Log_Write("%s\n", ch->filename);
	Log_Write("skill %.1f\n", ch->skill);
	Log_Write("{\n");
	for (i = 0; i < MAX_CHARACTERISTICS; i++)
	{
		switch(ch->c[i].type)
		{
			case CT_INTEGER: Log_Write(" %4d %d\n", i, ch->c[i].value.integer); break;
			case CT_FLOAT: Log_Write(" %4d %f\n", i, ch->c[i].value._float); break;
			case CT_STRING: Log_Write(" %4d %s\n", i, ch->c[i].value.string); break;
		} //end case
	} //end for
	Log_Write("}\n");
} //end of the function BotDumpCharacter
//========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//========================================================================
void BotFreeCharacterStrings(bot_character_t *ch)
{
	int i;

	for (i = 0; i < MAX_CHARACTERISTICS; i++)
	{
		if (ch->c[i].type == CT_STRING)
		{
			FreeMemory(ch->c[i].value.string);
		} //end if
	} //end for
} //end of the function BotFreeCharacterStrings
//========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//========================================================================
void BotFreeCharacter2(int handle)
{
	if (handle <= 0 || handle > MAX_CLIENTS)
	{
		botimport.Print(PRT_FATAL, "character handle %d out of range\n", handle);
		return;
	} //end if
	if (!botcharacters[handle])
	{
		botimport.Print(PRT_FATAL, "invalid character %d\n", handle);
		return;
	} //end if
	BotFreeCharacterStrings(botcharacters[handle]);
	FreeMemory(botcharacters[handle]);
	botcharacters[handle] = NULL;
} //end of the function BotFreeCharacter2
//========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//========================================================================
void BotFreeCharacter(int handle)
{
	if (!LibVarGetValue("bot_reloadcharacters")) return;
	BotFreeCharacter2(handle);
} //end of the function BotFreeCharacter
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotDefaultCharacteristics(bot_character_t *ch, bot_character_t *defaultch)
{
	int i;

	for (i = 0; i < MAX_CHARACTERISTICS; i++)
	{
		if (ch->c[i].type) continue;
		//
		if (defaultch->c[i].type == CT_FLOAT)
		{
			ch->c[i].type = CT_FLOAT;
			ch->c[i].value._float = defaultch->c[i].value._float;
		} //end if
		else if (defaultch->c[i].type == CT_INTEGER)
		{
			ch->c[i].type = CT_INTEGER;
			ch->c[i].value.integer = defaultch->c[i].value.integer;
		} //end else if
		else if (defaultch->c[i].type == CT_STRING)
		{
			ch->c[i].type = CT_STRING;
			ch->c[i].value.string = (char *) GetMemory(strlen(defaultch->c[i].value.string)+1);
			strcpy(ch->c[i].value.string, defaultch->c[i].value.string);
		} //end else if
	} //end for
} //end of the function BotDefaultCharacteristics
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
bot_character_t *BotLoadCharacterFromFile(char *charfile, int skill)
{
	int indent, index, foundcharacter;
	bot_character_t *ch;
	source_t *source;
	token_t token;

	foundcharacter = qfalse;
	//a bot character is parsed in two phases
	PC_SetBaseFolder(BOTFILESBASEFOLDER);
	source = LoadSourceFile(charfile);
	if (!source)
	{
		botimport.Print(PRT_ERROR, "counldn't load %s\n", charfile);
		return NULL;
	} //end if
	ch = (bot_character_t *) GetClearedMemory(sizeof(bot_character_t) +
					MAX_CHARACTERISTICS * sizeof(bot_characteristic_t));
	strcpy(ch->filename, charfile);
	while(PC_ReadToken(source, &token))
	{
		if (!strcmp(token.string, "skill"))
		{
			if (!PC_ExpectTokenType(source, TT_NUMBER, 0, &token))
			{
				FreeSource(source);
				BotFreeCharacterStrings(ch);
				FreeMemory(ch);
				return NULL;
			} //end if
			if (!PC_ExpectTokenString(source, "{"))
			{
				FreeSource(source);
				BotFreeCharacterStrings(ch);
				FreeMemory(ch);
				return NULL;
			} //end if
			//if it's the correct skill
			if (skill < 0 || token.intvalue == skill)
			{
				foundcharacter = qtrue;
				ch->skill = token.intvalue;
				while(PC_ExpectAnyToken(source, &token))
				{
					if (!strcmp(token.string, "}")) break;
					if (token.type != TT_NUMBER || !(token.subtype & TT_INTEGER))
					{
						SourceError(source, "expected integer index, found %s", token.string);
						FreeSource(source);
						BotFreeCharacterStrings(ch);
						FreeMemory(ch);
						return NULL;
					} //end if
					index = token.intvalue;
					if (index < 0 || index > MAX_CHARACTERISTICS)
					{
						SourceError(source, "characteristic index out of range [0, %d]", MAX_CHARACTERISTICS);
						FreeSource(source);
						BotFreeCharacterStrings(ch);
						FreeMemory(ch);
						return NULL;
					} //end if
					if (ch->c[index].type)
					{
						SourceError(source, "characteristic %d already initialized", index);
						FreeSource(source);
						BotFreeCharacterStrings(ch);
						FreeMemory(ch);
						return NULL;
					} //end if
					if (!PC_ExpectAnyToken(source, &token))
					{
						FreeSource(source);
						BotFreeCharacterStrings(ch);
						FreeMemory(ch);
						return NULL;
					} //end if
					if (token.type == TT_NUMBER)
					{
						if (token.subtype & TT_FLOAT)
						{
							ch->c[index].value._float = token.floatvalue;
							ch->c[index].type = CT_FLOAT;
						} //end if
						else
						{
							ch->c[index].value.integer = token.intvalue;
							ch->c[index].type = CT_INTEGER;
						} //end else
					} //end if
					else if (token.type == TT_STRING)
					{
						StripDoubleQuotes(token.string);
						ch->c[index].value.string = GetMemory(strlen(token.string)+1);
						strcpy(ch->c[index].value.string, token.string);
						ch->c[index].type = CT_STRING;
					} //end else if
					else
					{
						SourceError(source, "expected integer, float or string, found %s", token.string);
						FreeSource(source);
						BotFreeCharacterStrings(ch);
						FreeMemory(ch);
						return NULL;
					} //end else
				} //end if
				break;
			} //end if
			else
			{
				indent = 1;
				while(indent)
				{
					if (!PC_ExpectAnyToken(source, &token))
					{
						FreeSource(source);
						BotFreeCharacterStrings(ch);
						FreeMemory(ch);
						return NULL;
					} //end if
					if (!strcmp(token.string, "{")) indent++;
					else if (!strcmp(token.string, "}")) indent--;
				} //end while
			} //end else
		} //end if
		else
		{
			SourceError(source, "unknown definition %s", token.string);
			FreeSource(source);
			BotFreeCharacterStrings(ch);
			FreeMemory(ch);
			return NULL;
		} //end else
	} //end while
	FreeSource(source);
	//
	if (!foundcharacter)
	{
		BotFreeCharacterStrings(ch);
		FreeMemory(ch);
		return NULL;
	} //end if
	return ch;
} //end of the function BotLoadCharacterFromFile
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotFindCachedCharacter(char *charfile, float skill)
{
	int handle;

	for (handle = 1; handle <= MAX_CLIENTS; handle++)
	{
		if ( !botcharacters[handle] ) continue;
		if ( strcmp( botcharacters[handle]->filename, charfile ) == 0 &&
			(skill < 0 || fabs(botcharacters[handle]->skill - skill) < 0.01) )
		{
			return handle;
		} //end if
	} //end for
	return 0;
} //end of the function BotFindCachedCharacter
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotLoadCachedCharacter(char *charfile, float skill, int reload)
{
	int handle, cachedhandle, intskill;
	bot_character_t *ch = NULL;
#ifdef DEBUG
	int starttime;

	starttime = Sys_MilliSeconds();
#endif //DEBUG

	//find a free spot for a character
	for (handle = 1; handle <= MAX_CLIENTS; handle++)
	{
		if (!botcharacters[handle]) break;
	} //end for
	if (handle > MAX_CLIENTS) return 0;
	//try to load a cached character with the given skill
	if (!reload)
	{
		cachedhandle = BotFindCachedCharacter(charfile, skill);
		if (cachedhandle)
		{
			botimport.Print(PRT_MESSAGE, "loaded cached skill %f from %s\n", skill, charfile);
			return cachedhandle;
		} //end if
	} //end else
	//
	intskill = (int) (skill + 0.5);
	//try to load the character with the given skill
	ch = BotLoadCharacterFromFile(charfile, intskill);
	if (ch)
	{
		botcharacters[handle] = ch;
		//
		botimport.Print(PRT_MESSAGE, "loaded skill %d from %s\n", intskill, charfile);
#ifdef DEBUG
		if (botDeveloper)
		{
			botimport.Print(PRT_MESSAGE, "skill %d loaded in %d msec from %s\n", intskill, Sys_MilliSeconds() - starttime, charfile);
		} //end if
#endif //DEBUG
		return handle;
	} //end if
	//
	botimport.Print(PRT_WARNING, "couldn't find skill %d in %s\n", intskill, charfile);
	//
	if (!reload)
	{
		//try to load a cached default character with the given skill
		cachedhandle = BotFindCachedCharacter(DEFAULT_CHARACTER, skill);
		if (cachedhandle)
		{
			botimport.Print(PRT_MESSAGE, "loaded cached default skill %d from %s\n", intskill, charfile);
			return cachedhandle;
		} //end if
	} //end if
	//try to load the default character with the given skill
	ch = BotLoadCharacterFromFile(DEFAULT_CHARACTER, intskill);
	if (ch)
	{
		botcharacters[handle] = ch;
		botimport.Print(PRT_MESSAGE, "loaded default skill %d from %s\n", intskill, charfile);
		return handle;
	} //end if
	//
	if (!reload)
	{
		//try to load a cached character with any skill
		cachedhandle = BotFindCachedCharacter(charfile, -1);
		if (cachedhandle)
		{
			botimport.Print(PRT_MESSAGE, "loaded cached skill %f from %s\n", botcharacters[cachedhandle]->skill, charfile);
			return cachedhandle;
		} //end if
	} //end if
	//try to load a character with any skill
	ch = BotLoadCharacterFromFile(charfile, -1);
	if (ch)
	{
		botcharacters[handle] = ch;
		botimport.Print(PRT_MESSAGE, "loaded skill %f from %s\n", ch->skill, charfile);
		return handle;
	} //end if
	//
	if (!reload)
	{
		//try to load a cached character with any skill
		cachedhandle = BotFindCachedCharacter(DEFAULT_CHARACTER, -1);
		if (cachedhandle)
		{
			botimport.Print(PRT_MESSAGE, "loaded cached default skill %f from %s\n", botcharacters[cachedhandle]->skill, charfile);
			return cachedhandle;
		} //end if
	} //end if
	//try to load a character with any skill
	ch = BotLoadCharacterFromFile(DEFAULT_CHARACTER, -1);
	if (ch)
	{
		botcharacters[handle] = ch;
		botimport.Print(PRT_MESSAGE, "loaded default skill %f from %s\n", ch->skill, charfile);
		return handle;
	} //end if
	//
	botimport.Print(PRT_WARNING, "couldn't load any skill from %s\n", charfile);
	//couldn't load any character
	return 0;
} //end of the function BotLoadCachedCharacter
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotLoadCharacterSkill(char *charfile, float skill)
{
	int ch, defaultch;

	defaultch = BotLoadCachedCharacter(DEFAULT_CHARACTER, skill, qfalse);
	ch = BotLoadCachedCharacter(charfile, skill, LibVarGetValue("bot_reloadcharacters"));

	if (defaultch && ch)
	{
		BotDefaultCharacteristics(botcharacters[ch], botcharacters[defaultch]);
	} //end if

	return ch;
} //end of the function BotLoadCharacterSkill
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotInterpolateCharacters(int handle1, int handle2, float desiredskill)
{
	bot_character_t *ch1, *ch2, *out;
	int i, handle;
	float scale;

	ch1 = BotCharacterFromHandle(handle1);
	ch2 = BotCharacterFromHandle(handle2);
	if (!ch1 || !ch2)
		return 0;
	//find a free spot for a character
	for (handle = 1; handle <= MAX_CLIENTS; handle++)
	{
		if (!botcharacters[handle]) break;
	} //end for
	if (handle > MAX_CLIENTS) return 0;
	out = (bot_character_t *) GetClearedMemory(sizeof(bot_character_t) +
					MAX_CHARACTERISTICS * sizeof(bot_characteristic_t));
	out->skill = desiredskill;
	strcpy(out->filename, ch1->filename);
	botcharacters[handle] = out;

	scale = (float) (desiredskill - ch1->skill) / (ch2->skill - ch1->skill);
	for (i = 0; i < MAX_CHARACTERISTICS; i++)
	{
		//
		if (ch1->c[i].type == CT_FLOAT && ch2->c[i].type == CT_FLOAT)
		{
			out->c[i].type = CT_FLOAT;
			out->c[i].value._float = ch1->c[i].value._float +
								(ch2->c[i].value._float - ch1->c[i].value._float) * scale;
		} //end if
		else if (ch1->c[i].type == CT_INTEGER)
		{
			out->c[i].type = CT_INTEGER;
			out->c[i].value.integer = ch1->c[i].value.integer;
		} //end else if
		else if (ch1->c[i].type == CT_STRING)
		{
			out->c[i].type = CT_STRING;
			out->c[i].value.string = (char *) GetMemory(strlen(ch1->c[i].value.string)+1);
			strcpy(out->c[i].value.string, ch1->c[i].value.string);
		} //end else if
	} //end for
	return handle;
} //end of the function BotInterpolateCharacters
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotLoadCharacter(char *charfile, float skill)
{
	int firstskill, secondskill, handle;

	//make sure the skill is in the valid range
	if (skill < 1.0) skill = 1.0;
	else if (skill > 5.0) skill = 5.0;
	//skill 1, 4 and 5 should be available in the character files
	if (skill == 1.0 || skill == 4.0 || skill == 5.0)
	{
		return BotLoadCharacterSkill(charfile, skill);
	} //end if
	//check if there's a cached skill
	handle = BotFindCachedCharacter(charfile, skill);
	if (handle)
	{
		botimport.Print(PRT_MESSAGE, "loaded cached skill %f from %s\n", skill, charfile);
		return handle;
	} //end if
	if (skill < 4.0)
	{
		//load skill 1 and 4
		firstskill = BotLoadCharacterSkill(charfile, 1);
		if (!firstskill) return 0;
		secondskill = BotLoadCharacterSkill(charfile, 4);
		if (!secondskill) return firstskill;
	} //end if
	else
	{
		//load skill 4 and 5
		firstskill = BotLoadCharacterSkill(charfile, 4);
		if (!firstskill) return 0;
		secondskill = BotLoadCharacterSkill(charfile, 5);
		if (!secondskill) return firstskill;
	} //end else
	//interpolate between the two skills
	handle = BotInterpolateCharacters(firstskill, secondskill, skill);
	if (!handle) return 0;
	//write the character to the log file
	BotDumpCharacter(botcharacters[handle]);
	//
	return handle;
} //end of the function BotLoadCharacter
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int CheckCharacteristicIndex(int character, int index)
{
	bot_character_t *ch;

	ch = BotCharacterFromHandle(character);
	if (!ch) return qfalse;
	if (index < 0 || index >= MAX_CHARACTERISTICS)
	{
		botimport.Print(PRT_ERROR, "characteristic %d does not exist\n", index);
		return qfalse;
	} //end if
	if (!ch->c[index].type)
	{
		botimport.Print(PRT_ERROR, "characteristic %d is not initialized\n", index);
		return qfalse;
	} //end if
	return qtrue;
} //end of the function CheckCharacteristicIndex
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
float Characteristic_Float(int character, int index)
{
	bot_character_t *ch;

	ch = BotCharacterFromHandle(character);
	if (!ch) return 0;
	//check if the index is in range
	if (!CheckCharacteristicIndex(character, index)) return 0;
	//an integer will be converted to a float
	if (ch->c[index].type == CT_INTEGER)
	{
		return (float) ch->c[index].value.integer;
	} //end if
	//floats are just returned
	else if (ch->c[index].type == CT_FLOAT)
	{
		return ch->c[index].value._float;
	} //end else if
	//cannot convert a string pointer to a float
	else
	{
		botimport.Print(PRT_ERROR, "characteristic %d is not a float\n", index);
		return 0;
	} //end else if
//	return 0;
} //end of the function Characteristic_Float
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float Characteristic_BFloat(int character, int index, float min, float max)
{
	float value;
	bot_character_t *ch;

	ch = BotCharacterFromHandle(character);
	if (!ch) return 0;
	if (min > max)
	{
		botimport.Print(PRT_ERROR, "cannot bound characteristic %d between %f and %f\n", index, min, max);
		return 0;
	} //end if
	value = Characteristic_Float(character, index);
	if (value < min) return min;
	if (value > max) return max;
	return value;
} //end of the function Characteristic_BFloat
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int Characteristic_Integer(int character, int index)
{
	bot_character_t *ch;

	ch = BotCharacterFromHandle(character);
	if (!ch) return 0;
	//check if the index is in range
	if (!CheckCharacteristicIndex(character, index)) return 0;
	//an integer will just be returned
	if (ch->c[index].type == CT_INTEGER)
	{
		return ch->c[index].value.integer;
	} //end if
	//floats are casted to integers
	else if (ch->c[index].type == CT_FLOAT)
	{
		return (int) ch->c[index].value._float;
	} //end else if
	else
	{
		botimport.Print(PRT_ERROR, "characteristic %d is not an integer\n", index);
		return 0;
	} //end else if
//	return 0;
} //end of the function Characteristic_Integer
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int Characteristic_BInteger(int character, int index, int min, int max)
{
	int value;
	bot_character_t *ch;

	ch = BotCharacterFromHandle(character);
	if (!ch) return 0;
	if (min > max)
	{
		botimport.Print(PRT_ERROR, "cannot bound characteristic %d between %d and %d\n", index, min, max);
		return 0;
	} //end if
	value = Characteristic_Integer(character, index);
	if (value < min) return min;
	if (value > max) return max;
	return value;
} //end of the function Characteristic_BInteger
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void Characteristic_String(int character, int index, char *buf, int size)
{
	bot_character_t *ch;

	ch = BotCharacterFromHandle(character);
	if (!ch) return;
	//check if the index is in range
	if (!CheckCharacteristicIndex(character, index)) return;
	//an integer will be converted to a float
	if (ch->c[index].type == CT_STRING)
	{
		strncpy(buf, ch->c[index].value.string, size-1);
		buf[size-1] = '\0';
		return;
	} //end if
	else
	{
		botimport.Print(PRT_ERROR, "characteristic %d is not a string\n", index);
		return;
	} //end else if
	return;
} //end of the function Characteristic_String
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotShutdownCharacters(void)
{
	int handle;

	for (handle = 1; handle <= MAX_CLIENTS; handle++)
	{
		if (botcharacters[handle])
		{
			BotFreeCharacter2(handle);
		} //end if
	} //end for
} //end of the function BotShutdownCharacters

