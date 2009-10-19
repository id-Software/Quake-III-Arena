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
 * name:		be_ai_weight.c
 *
 * desc:		fuzzy logic
 *
 * $Archive: /MissionPack/code/botlib/be_ai_weight.c $
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "l_memory.h"
#include "l_log.h"
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
#include "be_ai_weight.h"

#define MAX_INVENTORYVALUE			999999
#define EVALUATERECURSIVELY

#define MAX_WEIGHT_FILES			128
weightconfig_t	*weightFileList[MAX_WEIGHT_FILES];

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int ReadValue(source_t *source, float *value)
{
	token_t token;

	if (!PC_ExpectAnyToken(source, &token)) return qfalse;
	if (!strcmp(token.string, "-"))
	{
		SourceWarning(source, "negative value set to zero\n");

		if(!PC_ExpectAnyToken(source, &token))
		{
			SourceError(source, "Missing return value\n");
			return qfalse;
		}
	}

	if (token.type != TT_NUMBER)
	{
		SourceError(source, "invalid return value %s\n", token.string);
		return qfalse;
	}
	
	*value = token.floatvalue;
	return qtrue;
} //end of the function ReadValue
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int ReadFuzzyWeight(source_t *source, fuzzyseperator_t *fs)
{
	if (PC_CheckTokenString(source, "balance"))
	{
		fs->type = WT_BALANCE;
		if (!PC_ExpectTokenString(source, "(")) return qfalse;
		if (!ReadValue(source, &fs->weight)) return qfalse;
		if (!PC_ExpectTokenString(source, ",")) return qfalse;
		if (!ReadValue(source, &fs->minweight)) return qfalse;
		if (!PC_ExpectTokenString(source, ",")) return qfalse;
		if (!ReadValue(source, &fs->maxweight)) return qfalse;
		if (!PC_ExpectTokenString(source, ")")) return qfalse;
	} //end if
	else
	{
		fs->type = 0;
		if (!ReadValue(source, &fs->weight)) return qfalse;
		fs->minweight = fs->weight;
		fs->maxweight = fs->weight;
	} //end if
	if (!PC_ExpectTokenString(source, ";")) return qfalse;
	return qtrue;
} //end of the function ReadFuzzyWeight
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void FreeFuzzySeperators_r(fuzzyseperator_t *fs)
{
	if (!fs) return;
	if (fs->child) FreeFuzzySeperators_r(fs->child);
	if (fs->next) FreeFuzzySeperators_r(fs->next);
	FreeMemory(fs);
} //end of the function FreeFuzzySeperators
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void FreeWeightConfig2(weightconfig_t *config)
{
	int i;

	for (i = 0; i < config->numweights; i++)
	{
		FreeFuzzySeperators_r(config->weights[i].firstseperator);
		if (config->weights[i].name) FreeMemory(config->weights[i].name);
	} //end for
	FreeMemory(config);
} //end of the function FreeWeightConfig2
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void FreeWeightConfig(weightconfig_t *config)
{
	if (!LibVarGetValue("bot_reloadcharacters")) return;
	FreeWeightConfig2(config);
} //end of the function FreeWeightConfig
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
fuzzyseperator_t *ReadFuzzySeperators_r(source_t *source)
{
	int newindent, index, def, founddefault;
	token_t token;
	fuzzyseperator_t *fs, *lastfs, *firstfs;

	founddefault = qfalse;
	firstfs = NULL;
	lastfs = NULL;
	if (!PC_ExpectTokenString(source, "(")) return NULL;
	if (!PC_ExpectTokenType(source, TT_NUMBER, TT_INTEGER, &token)) return NULL;
	index = token.intvalue;
	if (!PC_ExpectTokenString(source, ")")) return NULL;
	if (!PC_ExpectTokenString(source, "{")) return NULL;
	if (!PC_ExpectAnyToken(source, &token)) return NULL;
	do
	{
		def = !strcmp(token.string, "default");
		if (def || !strcmp(token.string, "case"))
		{
			fs = (fuzzyseperator_t *) GetClearedMemory(sizeof(fuzzyseperator_t));
			fs->index = index;
			if (lastfs) lastfs->next = fs;
			else firstfs = fs;
			lastfs = fs;
			if (def)
			{
				if (founddefault)
				{
					SourceError(source, "switch already has a default\n");
					FreeFuzzySeperators_r(firstfs);
					return NULL;
				} //end if
				fs->value = MAX_INVENTORYVALUE;
				founddefault = qtrue;
			} //end if
			else
			{
				if (!PC_ExpectTokenType(source, TT_NUMBER, TT_INTEGER, &token))
				{
					FreeFuzzySeperators_r(firstfs);
					return NULL;
				} //end if
				fs->value = token.intvalue;
			} //end else
			if (!PC_ExpectTokenString(source, ":") || !PC_ExpectAnyToken(source, &token))
			{
				FreeFuzzySeperators_r(firstfs);
				return NULL;
			} //end if
			newindent = qfalse;
			if (!strcmp(token.string, "{"))
			{
				newindent = qtrue;
				if (!PC_ExpectAnyToken(source, &token))
				{
					FreeFuzzySeperators_r(firstfs);
					return NULL;
				} //end if
			} //end if
			if (!strcmp(token.string, "return"))
			{
				if (!ReadFuzzyWeight(source, fs))
				{
					FreeFuzzySeperators_r(firstfs);
					return NULL;
				} //end if
			} //end if
			else if (!strcmp(token.string, "switch"))
			{
				fs->child = ReadFuzzySeperators_r(source);
				if (!fs->child)
				{
					FreeFuzzySeperators_r(firstfs);
					return NULL;
				} //end if
			} //end else if
			else
			{
				SourceError(source, "invalid name %s\n", token.string);
				return NULL;
			} //end else
			if (newindent)
			{
				if (!PC_ExpectTokenString(source, "}"))
				{
					FreeFuzzySeperators_r(firstfs);
					return NULL;
				} //end if
			} //end if
		} //end if
		else
		{
			FreeFuzzySeperators_r(firstfs);
			SourceError(source, "invalid name %s\n", token.string);
			return NULL;
		} //end else
		if (!PC_ExpectAnyToken(source, &token))
		{
			FreeFuzzySeperators_r(firstfs);
			return NULL;
		} //end if
	} while(strcmp(token.string, "}"));
	//
	if (!founddefault)
	{
		SourceWarning(source, "switch without default\n");
		fs = (fuzzyseperator_t *) GetClearedMemory(sizeof(fuzzyseperator_t));
		fs->index = index;
		fs->value = MAX_INVENTORYVALUE;
		fs->weight = 0;
		fs->next = NULL;
		fs->child = NULL;
		if (lastfs) lastfs->next = fs;
		else firstfs = fs;
		lastfs = fs;
	} //end if
	//
	return firstfs;
} //end of the function ReadFuzzySeperators_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
weightconfig_t *ReadWeightConfig(char *filename)
{
	int newindent, avail = 0, n;
	token_t token;
	source_t *source;
	fuzzyseperator_t *fs;
	weightconfig_t *config = NULL;
#ifdef DEBUG
	int starttime;

	starttime = Sys_MilliSeconds();
#endif //DEBUG

	if (!LibVarGetValue("bot_reloadcharacters"))
	{
		avail = -1;
		for( n = 0; n < MAX_WEIGHT_FILES; n++ )
		{
			config = weightFileList[n];
			if( !config )
			{
				if( avail == -1 )
				{
					avail = n;
				} //end if
				continue;
			} //end if
			if( strcmp( filename, config->filename ) == 0 )
			{
				//botimport.Print( PRT_MESSAGE, "retained %s\n", filename );
				return config;
			} //end if
		} //end for

		if( avail == -1 )
		{
			botimport.Print( PRT_ERROR, "weightFileList was full trying to load %s\n", filename );
			return NULL;
		} //end if
	} //end if

	PC_SetBaseFolder(BOTFILESBASEFOLDER);
	source = LoadSourceFile(filename);
	if (!source)
	{
		botimport.Print(PRT_ERROR, "counldn't load %s\n", filename);
		return NULL;
	} //end if
	//
	config = (weightconfig_t *) GetClearedMemory(sizeof(weightconfig_t));
	config->numweights = 0;
	Q_strncpyz( config->filename, filename, sizeof(config->filename) );
	//parse the item config file
	while(PC_ReadToken(source, &token))
	{
		if (!strcmp(token.string, "weight"))
		{
			if (config->numweights >= MAX_WEIGHTS)
			{
				SourceWarning(source, "too many fuzzy weights\n");
				break;
			} //end if
			if (!PC_ExpectTokenType(source, TT_STRING, 0, &token))
			{
				FreeWeightConfig(config);
				FreeSource(source);
				return NULL;
			} //end if
			StripDoubleQuotes(token.string);
			config->weights[config->numweights].name = (char *) GetClearedMemory(strlen(token.string) + 1);
			strcpy(config->weights[config->numweights].name, token.string);
			if (!PC_ExpectAnyToken(source, &token))
			{
				FreeWeightConfig(config);
				FreeSource(source);
				return NULL;
			} //end if
			newindent = qfalse;
			if (!strcmp(token.string, "{"))
			{
				newindent = qtrue;
				if (!PC_ExpectAnyToken(source, &token))
				{
					FreeWeightConfig(config);
					FreeSource(source);
					return NULL;
				} //end if
			} //end if
			if (!strcmp(token.string, "switch"))
			{
				fs = ReadFuzzySeperators_r(source);
				if (!fs)
				{
					FreeWeightConfig(config);
					FreeSource(source);
					return NULL;
				} //end if
				config->weights[config->numweights].firstseperator = fs;
			} //end if
			else if (!strcmp(token.string, "return"))
			{
				fs = (fuzzyseperator_t *) GetClearedMemory(sizeof(fuzzyseperator_t));
				fs->index = 0;
				fs->value = MAX_INVENTORYVALUE;
				fs->next = NULL;
				fs->child = NULL;
				if (!ReadFuzzyWeight(source, fs))
				{
					FreeMemory(fs);
					FreeWeightConfig(config);
					FreeSource(source);
					return NULL;
				} //end if
				config->weights[config->numweights].firstseperator = fs;
			} //end else if
			else
			{
				SourceError(source, "invalid name %s\n", token.string);
				FreeWeightConfig(config);
				FreeSource(source);
				return NULL;
			} //end else
			if (newindent)
			{
				if (!PC_ExpectTokenString(source, "}"))
				{
					FreeWeightConfig(config);
					FreeSource(source);
					return NULL;
				} //end if
			} //end if
			config->numweights++;
		} //end if
		else
		{
			SourceError(source, "invalid name %s\n", token.string);
			FreeWeightConfig(config);
			FreeSource(source);
			return NULL;
		} //end else
	} //end while
	//free the source at the end of a pass
	FreeSource(source);
	//if the file was located in a pak file
	botimport.Print(PRT_MESSAGE, "loaded %s\n", filename);
#ifdef DEBUG
	if (botDeveloper)
	{
		botimport.Print(PRT_MESSAGE, "weights loaded in %d msec\n", Sys_MilliSeconds() - starttime);
	} //end if
#endif //DEBUG
	//
	if (!LibVarGetValue("bot_reloadcharacters"))
	{
		weightFileList[avail] = config;
	} //end if
	//
	return config;
} //end of the function ReadWeightConfig
#if 0
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean WriteFuzzyWeight(FILE *fp, fuzzyseperator_t *fs)
{
	if (fs->type == WT_BALANCE)
	{
		if (fprintf(fp, " return balance(") < 0) return qfalse;
		if (!WriteFloat(fp, fs->weight)) return qfalse;
		if (fprintf(fp, ",") < 0) return qfalse;
		if (!WriteFloat(fp, fs->minweight)) return qfalse;
		if (fprintf(fp, ",") < 0) return qfalse;
		if (!WriteFloat(fp, fs->maxweight)) return qfalse;
		if (fprintf(fp, ");\n") < 0) return qfalse;
	} //end if
	else
	{
		if (fprintf(fp, " return ") < 0) return qfalse;
		if (!WriteFloat(fp, fs->weight)) return qfalse;
		if (fprintf(fp, ";\n") < 0) return qfalse;
	} //end else
	return qtrue;
} //end of the function WriteFuzzyWeight
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean WriteFuzzySeperators_r(FILE *fp, fuzzyseperator_t *fs, int indent)
{
	if (!WriteIndent(fp, indent)) return qfalse;
	if (fprintf(fp, "switch(%d)\n", fs->index) < 0) return qfalse;
	if (!WriteIndent(fp, indent)) return qfalse;
	if (fprintf(fp, "{\n") < 0) return qfalse;
	indent++;
	do
	{
		if (!WriteIndent(fp, indent)) return qfalse;
		if (fs->next)
		{
			if (fprintf(fp, "case %d:", fs->value) < 0) return qfalse;
		} //end if
		else
		{
			if (fprintf(fp, "default:") < 0) return qfalse;
		} //end else
		if (fs->child)
		{
			if (fprintf(fp, "\n") < 0) return qfalse;
			if (!WriteIndent(fp, indent)) return qfalse;
			if (fprintf(fp, "{\n") < 0) return qfalse;
			if (!WriteFuzzySeperators_r(fp, fs->child, indent + 1)) return qfalse;
			if (!WriteIndent(fp, indent)) return qfalse;
			if (fs->next)
			{
				if (fprintf(fp, "} //end case\n") < 0) return qfalse;
			} //end if
			else
			{
				if (fprintf(fp, "} //end default\n") < 0) return qfalse;
			} //end else
		} //end if
		else
		{
			if (!WriteFuzzyWeight(fp, fs)) return qfalse;
		} //end else
		fs = fs->next;
	} while(fs);
	indent--;
	if (!WriteIndent(fp, indent)) return qfalse;
	if (fprintf(fp, "} //end switch\n") < 0) return qfalse;
	return qtrue;
} //end of the function WriteItemFuzzyWeights_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean WriteWeightConfig(char *filename, weightconfig_t *config)
{
	int i;
	FILE *fp;
	weight_t *ifw;

	fp = fopen(filename, "wb");
	if (!fp) return qfalse;

	for (i = 0; i < config->numweights; i++)
	{
		ifw = &config->weights[i];
		if (fprintf(fp, "\nweight \"%s\"\n", ifw->name) < 0) return qfalse;
		if (fprintf(fp, "{\n") < 0) return qfalse;
		if (ifw->firstseperator->index > 0)
		{
			if (!WriteFuzzySeperators_r(fp, ifw->firstseperator, 1)) return qfalse;
		} //end if
		else
		{
			if (!WriteIndent(fp, 1)) return qfalse;
			if (!WriteFuzzyWeight(fp, ifw->firstseperator)) return qfalse;
		} //end else
		if (fprintf(fp, "} //end weight\n") < 0) return qfalse;
	} //end for
	fclose(fp);
	return qtrue;
} //end of the function WriteWeightConfig
#endif
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int FindFuzzyWeight(weightconfig_t *wc, char *name)
{
	int i;

	for (i = 0; i < wc->numweights; i++)
	{
		if (!strcmp(wc->weights[i].name, name))
		{
			return i;
		} //end if
	} //end if
	return -1;
} //end of the function FindFuzzyWeight
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float FuzzyWeight_r(int *inventory, fuzzyseperator_t *fs)
{
	float scale, w1, w2;

	if (inventory[fs->index] < fs->value)
	{
		if (fs->child) return FuzzyWeight_r(inventory, fs->child);
		else return fs->weight;
	} //end if
	else if (fs->next)
	{
		if (inventory[fs->index] < fs->next->value)
		{
			//first weight
			if (fs->child) w1 = FuzzyWeight_r(inventory, fs->child);
			else w1 = fs->weight;
			//second weight
			if (fs->next->child) w2 = FuzzyWeight_r(inventory, fs->next->child);
			else w2 = fs->next->weight;
			//the scale factor
			if(fs->next->value == MAX_INVENTORYVALUE) // is fs->next the default case?
        		return w2;      // can't interpolate, return default weight
			else
				scale = (float) (inventory[fs->index] - fs->value) / (fs->next->value - fs->value);
			//scale between the two weights
			return (1 - scale) * w1 + scale * w2;
		} //end if
		return FuzzyWeight_r(inventory, fs->next);
	} //end else if
	return fs->weight;
} //end of the function FuzzyWeight_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float FuzzyWeightUndecided_r(int *inventory, fuzzyseperator_t *fs)
{
	float scale, w1, w2;

	if (inventory[fs->index] < fs->value)
	{
		if (fs->child) return FuzzyWeightUndecided_r(inventory, fs->child);
		else return fs->minweight + random() * (fs->maxweight - fs->minweight);
	} //end if
	else if (fs->next)
	{
		if (inventory[fs->index] < fs->next->value)
		{
			//first weight
			if (fs->child) w1 = FuzzyWeightUndecided_r(inventory, fs->child);
			else w1 = fs->minweight + random() * (fs->maxweight - fs->minweight);
			//second weight
			if (fs->next->child) w2 = FuzzyWeight_r(inventory, fs->next->child);
			else w2 = fs->next->minweight + random() * (fs->next->maxweight - fs->next->minweight);
			//the scale factor
			if(fs->next->value == MAX_INVENTORYVALUE) // is fs->next the default case?
        		return w2;      // can't interpolate, return default weight
			else
				scale = (float) (inventory[fs->index] - fs->value) / (fs->next->value - fs->value);
			//scale between the two weights
			return (1 - scale) * w1 + scale * w2;
		} //end if
		return FuzzyWeightUndecided_r(inventory, fs->next);
	} //end else if
	return fs->weight;
} //end of the function FuzzyWeightUndecided_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float FuzzyWeight(int *inventory, weightconfig_t *wc, int weightnum)
{
#ifdef EVALUATERECURSIVELY
	return FuzzyWeight_r(inventory, wc->weights[weightnum].firstseperator);
#else
	fuzzyseperator_t *s;

	s = wc->weights[weightnum].firstseperator;
	if (!s) return 0;
	while(1)
	{
		if (inventory[s->index] < s->value)
		{
			if (s->child) s = s->child;
			else return s->weight;
		} //end if
		else
		{
			if (s->next) s = s->next;
			else return s->weight;
		} //end else
	} //end if
	return 0;
#endif
} //end of the function FuzzyWeight
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float FuzzyWeightUndecided(int *inventory, weightconfig_t *wc, int weightnum)
{
#ifdef EVALUATERECURSIVELY
	return FuzzyWeightUndecided_r(inventory, wc->weights[weightnum].firstseperator);
#else
	fuzzyseperator_t *s;

	s = wc->weights[weightnum].firstseperator;
	if (!s) return 0;
	while(1)
	{
		if (inventory[s->index] < s->value)
		{
			if (s->child) s = s->child;
			else return s->minweight + random() * (s->maxweight - s->minweight);
		} //end if
		else
		{
			if (s->next) s = s->next;
			else return s->minweight + random() * (s->maxweight - s->minweight);
		} //end else
	} //end if
	return 0;
#endif
} //end of the function FuzzyWeightUndecided
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EvolveFuzzySeperator_r(fuzzyseperator_t *fs)
{
	if (fs->child)
	{
		EvolveFuzzySeperator_r(fs->child);
	} //end if
	else if (fs->type == WT_BALANCE)
	{
		//every once in a while an evolution leap occurs, mutation
		if (random() < 0.01) fs->weight += crandom() * (fs->maxweight - fs->minweight);
		else fs->weight += crandom() * (fs->maxweight - fs->minweight) * 0.5;
		//modify bounds if necesary because of mutation
		if (fs->weight < fs->minweight) fs->minweight = fs->weight;
		else if (fs->weight > fs->maxweight) fs->maxweight = fs->weight;
	} //end else if
	if (fs->next) EvolveFuzzySeperator_r(fs->next);
} //end of the function EvolveFuzzySeperator_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EvolveWeightConfig(weightconfig_t *config)
{
	int i;

	for (i = 0; i < config->numweights; i++)
	{
		EvolveFuzzySeperator_r(config->weights[i].firstseperator);
	} //end for
} //end of the function EvolveWeightConfig
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ScaleFuzzySeperator_r(fuzzyseperator_t *fs, float scale)
{
	if (fs->child)
	{
		ScaleFuzzySeperator_r(fs->child, scale);
	} //end if
	else if (fs->type == WT_BALANCE)
	{
		//
		fs->weight = (float) (fs->maxweight + fs->minweight) * scale;
		//get the weight between bounds
		if (fs->weight < fs->minweight) fs->weight = fs->minweight;
		else if (fs->weight > fs->maxweight) fs->weight = fs->maxweight;
	} //end else if
	if (fs->next) ScaleFuzzySeperator_r(fs->next, scale);
} //end of the function ScaleFuzzySeperator_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ScaleWeight(weightconfig_t *config, char *name, float scale)
{
	int i;

	if (scale < 0) scale = 0;
	else if (scale > 1) scale = 1;
	for (i = 0; i < config->numweights; i++)
	{
		if (!strcmp(name, config->weights[i].name))
		{
			ScaleFuzzySeperator_r(config->weights[i].firstseperator, scale);
			break;
		} //end if
	} //end for
} //end of the function ScaleWeight
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ScaleFuzzySeperatorBalanceRange_r(fuzzyseperator_t *fs, float scale)
{
	if (fs->child)
	{
		ScaleFuzzySeperatorBalanceRange_r(fs->child, scale);
	} //end if
	else if (fs->type == WT_BALANCE)
	{
		float mid = (fs->minweight + fs->maxweight) * 0.5;
		//get the weight between bounds
		fs->maxweight = mid + (fs->maxweight - mid) * scale;
		fs->minweight = mid + (fs->minweight - mid) * scale;
		if (fs->maxweight < fs->minweight)
		{
			fs->maxweight = fs->minweight;
		} //end if
	} //end else if
	if (fs->next) ScaleFuzzySeperatorBalanceRange_r(fs->next, scale);
} //end of the function ScaleFuzzySeperatorBalanceRange_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ScaleFuzzyBalanceRange(weightconfig_t *config, float scale)
{
	int i;

	if (scale < 0) scale = 0;
	else if (scale > 100) scale = 100;
	for (i = 0; i < config->numweights; i++)
	{
		ScaleFuzzySeperatorBalanceRange_r(config->weights[i].firstseperator, scale);
	} //end for
} //end of the function ScaleFuzzyBalanceRange
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int InterbreedFuzzySeperator_r(fuzzyseperator_t *fs1, fuzzyseperator_t *fs2,
								fuzzyseperator_t *fsout)
{
	if (fs1->child)
	{
		if (!fs2->child || !fsout->child)
		{
			botimport.Print(PRT_ERROR, "cannot interbreed weight configs, unequal child\n");
			return qfalse;
		} //end if
		if (!InterbreedFuzzySeperator_r(fs2->child, fs2->child, fsout->child))
		{
			return qfalse;
		} //end if
	} //end if
	else if (fs1->type == WT_BALANCE)
	{
		if (fs2->type != WT_BALANCE || fsout->type != WT_BALANCE)
		{
			botimport.Print(PRT_ERROR, "cannot interbreed weight configs, unequal balance\n");
			return qfalse;
		} //end if
		fsout->weight = (fs1->weight + fs2->weight) / 2;
		if (fsout->weight > fsout->maxweight) fsout->maxweight = fsout->weight;
		if (fsout->weight > fsout->minweight) fsout->minweight = fsout->weight;
	} //end else if
	if (fs1->next)
	{
		if (!fs2->next || !fsout->next)
		{
			botimport.Print(PRT_ERROR, "cannot interbreed weight configs, unequal next\n");
			return qfalse;
		} //end if
		if (!InterbreedFuzzySeperator_r(fs1->next, fs2->next, fsout->next))
		{
			return qfalse;
		} //end if
	} //end if
	return qtrue;
} //end of the function InterbreedFuzzySeperator_r
//===========================================================================
// config1 and config2 are interbreeded and stored in configout
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void InterbreedWeightConfigs(weightconfig_t *config1, weightconfig_t *config2,
								weightconfig_t *configout)
{
	int i;

	if (config1->numweights != config2->numweights ||
		config1->numweights != configout->numweights)
	{
		botimport.Print(PRT_ERROR, "cannot interbreed weight configs, unequal numweights\n");
		return;
	} //end if
	for (i = 0; i < config1->numweights; i++)
	{
		InterbreedFuzzySeperator_r(config1->weights[i].firstseperator,
									config2->weights[i].firstseperator,
									configout->weights[i].firstseperator);
	} //end for
} //end of the function InterbreedWeightConfigs
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotShutdownWeights(void)
{
	int i;

	for( i = 0; i < MAX_WEIGHT_FILES; i++ )
	{
		if (weightFileList[i])
		{
			FreeWeightConfig2(weightFileList[i]);
			weightFileList[i] = NULL;
		} //end if
	} //end for
} //end of the function BotShutdownWeights
