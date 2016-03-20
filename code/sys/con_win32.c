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

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"
#include "windows.h"

#define QCONSOLE_HISTORY 32

static WORD qconsole_attrib;
static WORD qconsole_backgroundAttrib;

// saved console status
static DWORD qconsole_orig_mode;
static CONSOLE_CURSOR_INFO qconsole_orig_cursorinfo;

// cmd history
static char qconsole_history[ QCONSOLE_HISTORY ][ MAX_EDIT_LINE ];
static int qconsole_history_pos = -1;
static int qconsole_history_lines = 0;
static int qconsole_history_oldest = 0;

// current edit buffer
static char qconsole_line[ MAX_EDIT_LINE ];
static int qconsole_linelen = 0;
static qboolean qconsole_drawinput = qtrue;
static int qconsole_cursor;

static HANDLE qconsole_hout;
static HANDLE qconsole_hin;

/*
==================
CON_ColorCharToAttrib

Convert Quake color character to Windows text attrib
==================
*/
static WORD CON_ColorCharToAttrib( char color ) {
	WORD attrib;

	if ( color == COLOR_WHITE )
	{
		// use console's foreground and background colors
		attrib = qconsole_attrib;
	}
	else
	{
		float *rgba = g_color_table[ ColorIndex( color ) ];

		// set foreground color
		attrib = ( rgba[0] >= 0.5 ? FOREGROUND_RED		: 0 ) |
				( rgba[1] >= 0.5 ? FOREGROUND_GREEN		: 0 ) |
				( rgba[2] >= 0.5 ? FOREGROUND_BLUE		: 0 ) |
				( rgba[3] >= 0.5 ? FOREGROUND_INTENSITY	: 0 );

		// use console's background color
		attrib |= qconsole_backgroundAttrib;
	}

	return attrib;
}

/*
==================
CON_CtrlHandler

The Windows Console doesn't use signals for terminating the application
with Ctrl-C, logging off, window closing, etc.  Instead it uses a special
handler routine.  Fortunately, the values for Ctrl signals don't seem to
overlap with true signal codes that Windows provides, so calling
Sys_SigHandler() with those numbers should be safe for generating unique
shutdown messages.
==================
*/
static BOOL WINAPI CON_CtrlHandler( DWORD sig )
{
	Sys_SigHandler( sig );
	return TRUE;
}

/*
==================
CON_HistAdd
==================
*/
static void CON_HistAdd( void )
{
	Q_strncpyz( qconsole_history[ qconsole_history_oldest ], qconsole_line,
		sizeof( qconsole_history[ qconsole_history_oldest ] ) );

	if( qconsole_history_lines < QCONSOLE_HISTORY )
		qconsole_history_lines++;

	if( qconsole_history_oldest >= QCONSOLE_HISTORY - 1 )
		qconsole_history_oldest = 0;
	else
		qconsole_history_oldest++;

	qconsole_history_pos = qconsole_history_oldest;
}

/*
==================
CON_HistPrev
==================
*/
static void CON_HistPrev( void )
{
	int pos;

	pos = ( qconsole_history_pos < 1 ) ?
		( QCONSOLE_HISTORY - 1 ) : ( qconsole_history_pos - 1 );

	// don' t allow looping through history
	if( pos == qconsole_history_oldest || pos >= qconsole_history_lines )
		return;

	qconsole_history_pos = pos;
	Q_strncpyz( qconsole_line, qconsole_history[ qconsole_history_pos ], 
		sizeof( qconsole_line ) );
	qconsole_linelen = strlen( qconsole_line );
	qconsole_cursor = qconsole_linelen;
}

/*
==================
CON_HistNext
==================
*/
static void CON_HistNext( void )
{
	int pos;

	// don' t allow looping through history
	if( qconsole_history_pos == qconsole_history_oldest )
		return;

	pos = ( qconsole_history_pos >= QCONSOLE_HISTORY - 1 ) ?
		0 : ( qconsole_history_pos + 1 ); 

	// clear the edit buffer if they try to advance to a future command
	if( pos == qconsole_history_oldest )
	{
		qconsole_history_pos = pos;
		qconsole_line[ 0 ] = '\0';
		qconsole_linelen = 0;
		qconsole_cursor = qconsole_linelen;
		return;
	}

	qconsole_history_pos = pos;
	Q_strncpyz( qconsole_line, qconsole_history[ qconsole_history_pos ],
		sizeof( qconsole_line ) );
	qconsole_linelen = strlen( qconsole_line );
	qconsole_cursor = qconsole_linelen;
}


/*
==================
CON_Show
==================
*/
static void CON_Show( void )
{
	CONSOLE_SCREEN_BUFFER_INFO binfo;
	COORD writeSize = { MAX_EDIT_LINE, 1 };
	COORD writePos = { 0, 0 };
	SMALL_RECT writeArea = { 0, 0, 0, 0 };
	COORD cursorPos;
	int i;
	CHAR_INFO line[ MAX_EDIT_LINE ];
	WORD attrib;

	GetConsoleScreenBufferInfo( qconsole_hout, &binfo );

	// if we're in the middle of printf, don't bother writing the buffer
	if( !qconsole_drawinput )
		return;

	writeArea.Left = 0;
	writeArea.Top = binfo.dwCursorPosition.Y; 
	writeArea.Bottom = binfo.dwCursorPosition.Y; 
	writeArea.Right = MAX_EDIT_LINE;

	// set color to white
	attrib = CON_ColorCharToAttrib( COLOR_WHITE );

	// build a space-padded CHAR_INFO array
	for( i = 0; i < MAX_EDIT_LINE; i++ )
	{
		if( i < qconsole_linelen )
		{
			if( i + 1 < qconsole_linelen && Q_IsColorString( qconsole_line + i ) )
				attrib = CON_ColorCharToAttrib( *( qconsole_line + i + 1 ) );

			line[ i ].Char.AsciiChar = qconsole_line[ i ];
		}
		else
			line[ i ].Char.AsciiChar = ' ';

		line[ i ].Attributes = attrib;
	}

	if( qconsole_linelen > binfo.srWindow.Right )
	{
		WriteConsoleOutput( qconsole_hout, 
			line + (qconsole_linelen - binfo.srWindow.Right ),
			writeSize, writePos, &writeArea );
	}
	else
	{
		WriteConsoleOutput( qconsole_hout, line, writeSize,
			writePos, &writeArea );
	}

	// set curor position
	cursorPos.Y = binfo.dwCursorPosition.Y;
	cursorPos.X = qconsole_cursor < qconsole_linelen
					? qconsole_cursor
					: qconsole_linelen > binfo.srWindow.Right
						? binfo.srWindow.Right
						: qconsole_linelen;

	SetConsoleCursorPosition( qconsole_hout, cursorPos );
}

/*
==================
CON_Hide
==================
*/
static void CON_Hide( void )
{
	int realLen;

	realLen = qconsole_linelen;

	// remove input line from console output buffer
	qconsole_linelen = 0;
	CON_Show( );

	qconsole_linelen = realLen;
}


/*
==================
CON_Shutdown
==================
*/
void CON_Shutdown( void )
{
	CON_Hide( );
	SetConsoleMode( qconsole_hin, qconsole_orig_mode );
	SetConsoleCursorInfo( qconsole_hout, &qconsole_orig_cursorinfo );
	SetConsoleTextAttribute( qconsole_hout, qconsole_attrib );
	CloseHandle( qconsole_hout );
	CloseHandle( qconsole_hin );
}

/*
==================
CON_Init
==================
*/
void CON_Init( void )
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	int i;

	// handle Ctrl-C or other console termination
	SetConsoleCtrlHandler( CON_CtrlHandler, TRUE );

	qconsole_hin = GetStdHandle( STD_INPUT_HANDLE );
	if( qconsole_hin == INVALID_HANDLE_VALUE )
		return;

	qconsole_hout = GetStdHandle( STD_OUTPUT_HANDLE );
	if( qconsole_hout == INVALID_HANDLE_VALUE )
		return;

	GetConsoleMode( qconsole_hin, &qconsole_orig_mode );

	// allow mouse wheel scrolling
	SetConsoleMode( qconsole_hin,
		qconsole_orig_mode & ~ENABLE_MOUSE_INPUT );

	FlushConsoleInputBuffer( qconsole_hin ); 

	GetConsoleScreenBufferInfo( qconsole_hout, &info );
	qconsole_attrib = info.wAttributes;
	qconsole_backgroundAttrib = qconsole_attrib & (BACKGROUND_BLUE|BACKGROUND_GREEN|BACKGROUND_RED|BACKGROUND_INTENSITY);

	SetConsoleTitle(CLIENT_WINDOW_TITLE " Dedicated Server Console");

	// initialize history
	for( i = 0; i < QCONSOLE_HISTORY; i++ )
		qconsole_history[ i ][ 0 ] = '\0';

	// set text color to white
	SetConsoleTextAttribute( qconsole_hout, CON_ColorCharToAttrib( COLOR_WHITE ) );
}

/*
==================
CON_Input
==================
*/
char *CON_Input( void )
{
	INPUT_RECORD buff[ MAX_EDIT_LINE ];
	DWORD count = 0, events = 0;
	WORD key = 0;
	int i;
	int newlinepos = -1;

	if( !GetNumberOfConsoleInputEvents( qconsole_hin, &events ) )
		return NULL;

	if( events < 1 )
		return NULL;
  
	// if we have overflowed, start dropping oldest input events
	if( events >= MAX_EDIT_LINE )
	{
		ReadConsoleInput( qconsole_hin, buff, 1, &events );
		return NULL;
	} 

	if( !ReadConsoleInput( qconsole_hin, buff, events, &count ) )
		return NULL;

	FlushConsoleInputBuffer( qconsole_hin );

	for( i = 0; i < count; i++ )
	{
		if( buff[ i ].EventType != KEY_EVENT )
			continue;
		if( !buff[ i ].Event.KeyEvent.bKeyDown ) 
			continue;

		key = buff[ i ].Event.KeyEvent.wVirtualKeyCode;

		if( key == VK_RETURN )
		{
			newlinepos = i;
			qconsole_cursor = 0;
			break;
		}
		else if( key == VK_UP )
		{
			CON_HistPrev();
			break;
		}
		else if( key == VK_DOWN )
		{
			CON_HistNext();
			break;
		}
		else if( key == VK_LEFT )
		{
			qconsole_cursor--;
			if ( qconsole_cursor < 0 )
			{
				qconsole_cursor = 0;
			}
			break;
		}
		else if( key == VK_RIGHT )
		{
			qconsole_cursor++;
			if ( qconsole_cursor > qconsole_linelen )
			{
				qconsole_cursor = qconsole_linelen;
			}
			break;
		}
		else if( key == VK_HOME )
		{
			qconsole_cursor = 0;
			break;
		}
		else if( key == VK_END )
		{
			qconsole_cursor = qconsole_linelen;
			break;
		}
		else if( key == VK_TAB )
		{
			field_t f;

			Q_strncpyz( f.buffer, qconsole_line,
				sizeof( f.buffer ) );
			Field_AutoComplete( &f );
			Q_strncpyz( qconsole_line, f.buffer,
				sizeof( qconsole_line ) );
			qconsole_linelen = strlen( qconsole_line );
			qconsole_cursor = qconsole_linelen;
			break;
		}

		if( qconsole_linelen < sizeof( qconsole_line ) - 1 )
		{
			char c = buff[ i ].Event.KeyEvent.uChar.AsciiChar;

			if( key == VK_BACK )
			{
				if ( qconsole_cursor > 0 )
				{
					int newlen = ( qconsole_linelen > 0 ) ? qconsole_linelen - 1 : 0;
					if ( qconsole_cursor < qconsole_linelen )
					{
						memmove( qconsole_line + qconsole_cursor - 1,
									qconsole_line + qconsole_cursor,
									qconsole_linelen - qconsole_cursor );
					}

					qconsole_line[ newlen ] = '\0';
					qconsole_linelen = newlen;
					qconsole_cursor--;
				}
			}
			else if( c )
			{
				if ( qconsole_linelen > qconsole_cursor )
				{
					memmove( qconsole_line + qconsole_cursor + 1,
								qconsole_line + qconsole_cursor,
								qconsole_linelen - qconsole_cursor );
				}

				qconsole_line[ qconsole_cursor++ ] = c;

				qconsole_linelen++;
				qconsole_line[ qconsole_linelen ] = '\0'; 
			}
		}
	}

	if( newlinepos < 0) {
		CON_Show();
		return NULL;
	}

	if( !qconsole_linelen )
	{
		CON_Show();
		Com_Printf( "\n" );
		return NULL;
	}

	qconsole_linelen = 0;
	CON_Show();

	CON_HistAdd();
	Com_Printf( "%s\n", qconsole_line );

	return qconsole_line;
}

/*
=================
CON_WindowsColorPrint

Set text colors based on Q3 color codes
=================
*/
void CON_WindowsColorPrint( const char *msg )
{
	static char buffer[ MAXPRINTMSG ];
	int         length = 0;

	while( *msg )
	{
		qconsole_drawinput = ( *msg == '\n' );

		if( Q_IsColorString( msg ) || *msg == '\n' )
		{
			// First empty the buffer
			if( length > 0 )
			{
				buffer[ length ] = '\0';
				fputs( buffer, stderr );
				length = 0;
			}

			if( *msg == '\n' )
			{
				// Reset color and then add the newline
				SetConsoleTextAttribute( qconsole_hout, CON_ColorCharToAttrib( COLOR_WHITE ) );
				fputs( "\n", stderr );
				msg++;
			}
			else
			{
				// Set the color
				SetConsoleTextAttribute( qconsole_hout, CON_ColorCharToAttrib( *( msg + 1 ) ) );
				msg += 2;
			}
		}
		else
		{
			if( length >= MAXPRINTMSG - 1 )
				break;

			buffer[ length ] = *msg;
			length++;
			msg++;
		}
	}

	// Empty anything still left in the buffer
	if( length > 0 )
	{
		buffer[ length ] = '\0';
		fputs( buffer, stderr );
	}
}

/*
==================
CON_Print
==================
*/
void CON_Print( const char *msg )
{
	CON_Hide( );

	CON_WindowsColorPrint( msg );

	CON_Show( );
}
