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
 * name:		be_ai_chat.h
 *
 * desc:		char AI
 *
 * $Archive: /source/code/botlib/be_ai_chat.h $
 *
 *****************************************************************************/

#define MAX_MESSAGE_SIZE		256
#define MAX_CHATTYPE_NAME		32
#define MAX_MATCHVARIABLES		8

#define CHAT_GENDERLESS			0
#define CHAT_GENDERFEMALE		1
#define CHAT_GENDERMALE			2

#define CHAT_ALL					0
#define CHAT_TEAM					1
#define CHAT_TELL					2

//a console message
typedef struct bot_consolemessage_s
{
	int handle;
	float time;									//message time
	int type;									//message type
	char message[MAX_MESSAGE_SIZE];				//message
	struct bot_consolemessage_s *prev, *next;	//prev and next in list
} bot_consolemessage_t;

//match variable
typedef struct bot_matchvariable_s
{
	char offset;
	int length;
} bot_matchvariable_t;
//returned to AI when a match is found
typedef struct bot_match_s
{
	char string[MAX_MESSAGE_SIZE];
	int type;
	int subtype;
	bot_matchvariable_t variables[MAX_MATCHVARIABLES];
} bot_match_t;

//setup the chat AI
int BotSetupChatAI(void);
//shutdown the chat AI
void BotShutdownChatAI(void);
//returns the handle to a newly allocated chat state
int BotAllocChatState(void);
//frees the chatstate
void BotFreeChatState(int handle);
//adds a console message to the chat state
void BotQueueConsoleMessage(int chatstate, int type, char *message);
//removes the console message from the chat state
void BotRemoveConsoleMessage(int chatstate, int handle);
//returns the next console message from the state
int BotNextConsoleMessage(int chatstate, bot_consolemessage_t *cm);
//returns the number of console messages currently stored in the state
int BotNumConsoleMessages(int chatstate);
//selects a chat message of the given type
void BotInitialChat(int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7);
//returns the number of initial chat messages of the given type
int BotNumInitialChats(int chatstate, char *type);
//find and select a reply for the given message
int BotReplyChat(int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7);
//returns the length of the currently selected chat message
int BotChatLength(int chatstate);
//enters the selected chat message
void BotEnterChat(int chatstate, int clientto, int sendto);
//get the chat message ready to be output
void BotGetChatMessage(int chatstate, char *buf, int size);
//checks if the first string contains the second one, returns index into first string or -1 if not found
int StringContains(char *str1, char *str2, int casesensitive);
//finds a match for the given string using the match templates
int BotFindMatch(char *str, bot_match_t *match, unsigned long int context);
//returns a variable from a match
void BotMatchVariable(bot_match_t *match, int variable, char *buf, int size);
//unify all the white spaces in the string
void UnifyWhiteSpaces(char *string);
//replace all the context related synonyms in the string
void BotReplaceSynonyms(char *string, unsigned long int context);
//loads a chat file for the chat state
int BotLoadChatFile(int chatstate, char *chatfile, char *chatname);
//store the gender of the bot in the chat state
void BotSetChatGender(int chatstate, int gender);
//store the bot name in the chat state
void BotSetChatName(int chatstate, char *name, int client);

