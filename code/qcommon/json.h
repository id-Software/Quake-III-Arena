/*
===========================================================================
Copyright (C) 2016 James Canete

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
===========================================================================
*/

#ifndef JSON_H
#define JSON_H

enum
{
	JSONTYPE_STRING, // string
	JSONTYPE_OBJECT, // object
	JSONTYPE_ARRAY,  // array
	JSONTYPE_VALUE,  // number, true, false, or null
	JSONTYPE_ERROR   // out of data
};

// --------------------------------------------------------------------------
//   Array Functions
// --------------------------------------------------------------------------

// Get pointer to first value in array
// When given pointer to an array, returns pointer to the first
// returns NULL if array is empty or not an array.
const char *JSON_ArrayGetFirstValue(const char *json, const char *jsonEnd);

// Get pointer to next value in array
// When given pointer to a value, returns pointer to the next value
// returns NULL when no next value.
const char *JSON_ArrayGetNextValue(const char *json, const char *jsonEnd);

// Get pointers to values in an array
// returns 0 if not an array, array is empty, or out of data
// returns number of values in the array and copies into index if successful
unsigned int JSON_ArrayGetIndex(const char *json, const char *jsonEnd, const char **indexes, unsigned int numIndexes);

// Get pointer to indexed value from array
// returns NULL if not an array, no index, or out of data
const char *JSON_ArrayGetValue(const char *json, const char *jsonEnd, unsigned int index);

// --------------------------------------------------------------------------
//   Object Functions
// --------------------------------------------------------------------------

// Get pointer to named value from object
// returns NULL if not an object, name not found, or out of data
const char *JSON_ObjectGetNamedValue(const char *json, const char *jsonEnd, const char *name);

// --------------------------------------------------------------------------
//   Value Functions
// --------------------------------------------------------------------------

// Get type of value
// returns JSONTYPE_ERROR if out of data
unsigned int JSON_ValueGetType(const char *json, const char *jsonEnd);

// Get value as string
// returns 0 if out of data
// returns length and copies into string if successful, including terminating nul.
// string values are stripped of enclosing quotes but not escaped
unsigned int JSON_ValueGetString(const char *json, const char *jsonEnd, char *outString, unsigned int stringLen);

// Get value as appropriate type
// returns 0 if value is false, value is null, or out of data
// returns 1 if value is true
// returns value otherwise
double JSON_ValueGetDouble(const char *json, const char *jsonEnd);
float JSON_ValueGetFloat(const char *json, const char *jsonEnd);
int JSON_ValueGetInt(const char *json, const char *jsonEnd);

#endif

#ifdef JSON_IMPLEMENTATION
#include <stdio.h>

// --------------------------------------------------------------------------
//   Internal Functions
// --------------------------------------------------------------------------

static const char *JSON_SkipSeparators(const char *json, const char *jsonEnd);
static const char *JSON_SkipString(const char *json, const char *jsonEnd);
static const char *JSON_SkipStruct(const char *json, const char *jsonEnd);
static const char *JSON_SkipValue(const char *json, const char *jsonEnd);
static const char *JSON_SkipValueAndSeparators(const char *json, const char *jsonEnd);

#define IS_SEPARATOR(x)    ((x) == ' ' || (x) == '\t' || (x) == '\n' || (x) == '\r' || (x) == ',' || (x) == ':')
#define IS_STRUCT_OPEN(x)  ((x) == '{' || (x) == '[')
#define IS_STRUCT_CLOSE(x) ((x) == '}' || (x) == ']')

static const char *JSON_SkipSeparators(const char *json, const char *jsonEnd)
{
	while (json < jsonEnd && IS_SEPARATOR(*json))
		json++;

	return json;
}

static const char *JSON_SkipString(const char *json, const char *jsonEnd)
{
	for (json++; json < jsonEnd && *json != '"'; json++)
		if (*json == '\\')
			json++;

	return (json + 1 > jsonEnd) ? jsonEnd : json + 1;
}

static const char *JSON_SkipStruct(const char *json, const char *jsonEnd)
{
	json = JSON_SkipSeparators(json + 1, jsonEnd);
	while (json < jsonEnd && !IS_STRUCT_CLOSE(*json))
		json = JSON_SkipValueAndSeparators(json, jsonEnd);

	return (json + 1 > jsonEnd) ? jsonEnd : json + 1;
}

static const char *JSON_SkipValue(const char *json, const char *jsonEnd)
{
	if (json >= jsonEnd)
		return jsonEnd;
	else if (*json == '"')
		json = JSON_SkipString(json, jsonEnd);
	else if (IS_STRUCT_OPEN(*json))
		json = JSON_SkipStruct(json, jsonEnd);
	else
	{
		while (json < jsonEnd && !IS_SEPARATOR(*json) && !IS_STRUCT_CLOSE(*json))
			json++;
	}

	return json;
}

static const char *JSON_SkipValueAndSeparators(const char *json, const char *jsonEnd)
{
	json = JSON_SkipValue(json, jsonEnd);
	return JSON_SkipSeparators(json, jsonEnd);
}

// returns 0 if value requires more parsing, 1 if no more data/false/null, 2 if true
static unsigned int JSON_NoParse(const char *json, const char *jsonEnd)
{
	if (!json || json >= jsonEnd || *json == 'f' || *json == 'n')
		return 1;

	if (*json == 't')
		return 2;

	return 0;
}

// --------------------------------------------------------------------------
//   Array Functions
// --------------------------------------------------------------------------

const char *JSON_ArrayGetFirstValue(const char *json, const char *jsonEnd)
{
	if (!json || json >= jsonEnd || !IS_STRUCT_OPEN(*json))
		return NULL;

	json = JSON_SkipSeparators(json + 1, jsonEnd);

	return (json >= jsonEnd || IS_STRUCT_CLOSE(*json)) ? NULL : json;
}

const char *JSON_ArrayGetNextValue(const char *json, const char *jsonEnd)
{
	if (!json || json >= jsonEnd || IS_STRUCT_CLOSE(*json))
		return NULL;

	json = JSON_SkipValueAndSeparators(json, jsonEnd);

	return (json >= jsonEnd || IS_STRUCT_CLOSE(*json)) ? NULL : json;
}

unsigned int JSON_ArrayGetIndex(const char *json, const char *jsonEnd, const char **indexes, unsigned int numIndexes)
{
	unsigned int length = 0;

	for (json = JSON_ArrayGetFirstValue(json, jsonEnd); json; json = JSON_ArrayGetNextValue(json, jsonEnd))
	{
		if (indexes && numIndexes)
		{
			*indexes++ = json;
			numIndexes--;
		}
		length++;
	}

	return length;
}

const char *JSON_ArrayGetValue(const char *json, const char *jsonEnd, unsigned int index)
{
	for (json = JSON_ArrayGetFirstValue(json, jsonEnd); json && index; json = JSON_ArrayGetNextValue(json, jsonEnd))
		index--;

	return json;
}

// --------------------------------------------------------------------------
//   Object Functions
// --------------------------------------------------------------------------

const char *JSON_ObjectGetNamedValue(const char *json, const char *jsonEnd, const char *name)
{
	unsigned int nameLen = strlen(name);

	for (json = JSON_ArrayGetFirstValue(json, jsonEnd); json; json = JSON_ArrayGetNextValue(json, jsonEnd))
	{
		if (*json == '"')
		{
			const char *thisNameStart, *thisNameEnd;

			thisNameStart = json + 1;
			json = JSON_SkipString(json, jsonEnd);
			thisNameEnd = json - 1;
			json = JSON_SkipSeparators(json, jsonEnd);

			if ((unsigned int)(thisNameEnd - thisNameStart) == nameLen)
				if (strncmp(thisNameStart, name, nameLen) == 0)
					return json;
		}
	}

	return NULL;
}

// --------------------------------------------------------------------------
//   Value Functions
// --------------------------------------------------------------------------

unsigned int JSON_ValueGetType(const char *json, const char *jsonEnd)
{
	if (!json || json >= jsonEnd)
		return JSONTYPE_ERROR;
	else if (*json == '"')
		return JSONTYPE_STRING;
	else if (*json == '{')
		return JSONTYPE_OBJECT;
	else if (*json == '[')
		return JSONTYPE_ARRAY;

	return JSONTYPE_VALUE;
}

unsigned int JSON_ValueGetString(const char *json, const char *jsonEnd, char *outString, unsigned int stringLen)
{
	const char *stringEnd, *stringStart;

	if (!json)
	{
		*outString = '\0';
		return 0;
	}

	stringStart = json;
	stringEnd = JSON_SkipValue(stringStart, jsonEnd);
	if (stringEnd >= jsonEnd)
	{
		*outString = '\0';
		return 0;
	}

	// skip enclosing quotes if they exist
	if (*stringStart == '"')
		stringStart++;

	if (*(stringEnd - 1) == '"')
		stringEnd--;

	stringLen--;
	if (stringLen > stringEnd - stringStart)
		stringLen = stringEnd - stringStart;

	json = stringStart;
	while (stringLen--)
		*outString++ = *json++;
	*outString = '\0';

	return stringEnd - stringStart;
}

double JSON_ValueGetDouble(const char *json, const char *jsonEnd)
{
	char cValue[256];
	double dValue = 0.0;
	unsigned int np = JSON_NoParse(json, jsonEnd);

	if (np)
		return (double)(np - 1);

	if (!JSON_ValueGetString(json, jsonEnd, cValue, 256))
		return 0.0;

	sscanf(cValue, "%lf", &dValue);

	return dValue;
}

float JSON_ValueGetFloat(const char *json, const char *jsonEnd)
{
	char cValue[256];
	float fValue = 0.0f;
	unsigned int np = JSON_NoParse(json, jsonEnd);

	if (np)
		return (float)(np - 1);

	if (!JSON_ValueGetString(json, jsonEnd, cValue, 256))
		return 0.0f;

	sscanf(cValue, "%f", &fValue);

	return fValue;
}

int JSON_ValueGetInt(const char *json, const char *jsonEnd)
{
	char cValue[256];
	int iValue = 0;
	unsigned int np = JSON_NoParse(json, jsonEnd);

	if (np)
		return np - 1;

	if (!JSON_ValueGetString(json, jsonEnd, cValue, 256))
		return 0;

	sscanf(cValue, "%d", &iValue);

	return iValue;
}

#undef IS_SEPARATOR
#undef IS_STRUCT_OPEN
#undef IS_STRUCT_CLOSE

#endif
