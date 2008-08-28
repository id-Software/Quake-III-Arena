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
#ifndef __KEYCODES_H__
#define __KEYCODES_H__

//
// these are the key numbers that should be passed to KeyEvent
//

// normal keys should be passed as lowercased ascii

typedef enum {
	K_TAB = 9,
	K_ENTER = 13,
	K_ESCAPE = 27,
	K_SPACE = 32,

	K_BACKSPACE = 127,

	K_COMMAND = 128,
	K_CAPSLOCK,
	K_POWER,
	K_PAUSE,

	K_UPARROW,
	K_DOWNARROW,
	K_LEFTARROW,
	K_RIGHTARROW,

	K_ALT,
	K_CTRL,
	K_SHIFT,
	K_INS,
	K_DEL,
	K_PGDN,
	K_PGUP,
	K_HOME,
	K_END,

	K_F1,
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	K_F11,
	K_F12,
	K_F13,
	K_F14,
	K_F15,

	K_KP_HOME,
	K_KP_UPARROW,
	K_KP_PGUP,
	K_KP_LEFTARROW,
	K_KP_5,
	K_KP_RIGHTARROW,
	K_KP_END,
	K_KP_DOWNARROW,
	K_KP_PGDN,
	K_KP_ENTER,
	K_KP_INS,
	K_KP_DEL,
	K_KP_SLASH,
	K_KP_MINUS,
	K_KP_PLUS,
	K_KP_NUMLOCK,
	K_KP_STAR,
	K_KP_EQUALS,

	K_MOUSE1,
	K_MOUSE2,
	K_MOUSE3,
	K_MOUSE4,
	K_MOUSE5,

	K_MWHEELDOWN,
	K_MWHEELUP,

	K_JOY1,
	K_JOY2,
	K_JOY3,
	K_JOY4,
	K_JOY5,
	K_JOY6,
	K_JOY7,
	K_JOY8,
	K_JOY9,
	K_JOY10,
	K_JOY11,
	K_JOY12,
	K_JOY13,
	K_JOY14,
	K_JOY15,
	K_JOY16,
	K_JOY17,
	K_JOY18,
	K_JOY19,
	K_JOY20,
	K_JOY21,
	K_JOY22,
	K_JOY23,
	K_JOY24,
	K_JOY25,
	K_JOY26,
	K_JOY27,
	K_JOY28,
	K_JOY29,
	K_JOY30,
	K_JOY31,
	K_JOY32,

	K_AUX1,
	K_AUX2,
	K_AUX3,
	K_AUX4,
	K_AUX5,
	K_AUX6,
	K_AUX7,
	K_AUX8,
	K_AUX9,
	K_AUX10,
	K_AUX11,
	K_AUX12,
	K_AUX13,
	K_AUX14,
	K_AUX15,
	K_AUX16,

	K_WORLD_0,
	K_WORLD_1,
	K_WORLD_2,
	K_WORLD_3,
	K_WORLD_4,
	K_WORLD_5,
	K_WORLD_6,
	K_WORLD_7,
	K_WORLD_8,
	K_WORLD_9,
	K_WORLD_10,
	K_WORLD_11,
	K_WORLD_12,
	K_WORLD_13,
	K_WORLD_14,
	K_WORLD_15,
	K_WORLD_16,
	K_WORLD_17,
	K_WORLD_18,
	K_WORLD_19,
	K_WORLD_20,
	K_WORLD_21,
	K_WORLD_22,
	K_WORLD_23,
	K_WORLD_24,
	K_WORLD_25,
	K_WORLD_26,
	K_WORLD_27,
	K_WORLD_28,
	K_WORLD_29,
	K_WORLD_30,
	K_WORLD_31,
	K_WORLD_32,
	K_WORLD_33,
	K_WORLD_34,
	K_WORLD_35,
	K_WORLD_36,
	K_WORLD_37,
	K_WORLD_38,
	K_WORLD_39,
	K_WORLD_40,
	K_WORLD_41,
	K_WORLD_42,
	K_WORLD_43,
	K_WORLD_44,
	K_WORLD_45,
	K_WORLD_46,
	K_WORLD_47,
	K_WORLD_48,
	K_WORLD_49,
	K_WORLD_50,
	K_WORLD_51,
	K_WORLD_52,
	K_WORLD_53,
	K_WORLD_54,
	K_WORLD_55,
	K_WORLD_56,
	K_WORLD_57,
	K_WORLD_58,
	K_WORLD_59,
	K_WORLD_60,
	K_WORLD_61,
	K_WORLD_62,
	K_WORLD_63,
	K_WORLD_64,
	K_WORLD_65,
	K_WORLD_66,
	K_WORLD_67,
	K_WORLD_68,
	K_WORLD_69,
	K_WORLD_70,
	K_WORLD_71,
	K_WORLD_72,
	K_WORLD_73,
	K_WORLD_74,
	K_WORLD_75,
	K_WORLD_76,
	K_WORLD_77,
	K_WORLD_78,
	K_WORLD_79,
	K_WORLD_80,
	K_WORLD_81,
	K_WORLD_82,
	K_WORLD_83,
	K_WORLD_84,
	K_WORLD_85,
	K_WORLD_86,
	K_WORLD_87,
	K_WORLD_88,
	K_WORLD_89,
	K_WORLD_90,
	K_WORLD_91,
	K_WORLD_92,
	K_WORLD_93,
	K_WORLD_94,
	K_WORLD_95,

	K_SUPER,
	K_COMPOSE,
	K_MODE,
	K_HELP,
	K_PRINT,
	K_SYSREQ,
	K_SCROLLOCK,
	K_BREAK,
	K_MENU,
	K_EURO,
	K_UNDO,

	// Pseudo-key that brings the console down
	K_CONSOLE,

	MAX_KEYS
} keyNum_t;

// MAX_KEYS replaces K_LAST_KEY, however some mods may have used K_LAST_KEY
// in detecting binds, so we leave it defined to the old hardcoded value
// of maxiumum keys to prevent mods from crashing older versions of the engine
#define K_LAST_KEY              256

// The menu code needs to get both key and char events, but
// to avoid duplicating the paths, the char events are just
// distinguished by or'ing in K_CHAR_FLAG (ugly)
#define	K_CHAR_FLAG		1024

#endif
