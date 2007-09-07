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
#include "windows.h"

#define QCONSOLE_WIDTH 80
#define QCONSOLE_HEIGHT 30

#define QCONSOLE_THEME FOREGROUND_RED | \
                       BACKGROUND_RED | \
                       BACKGROUND_GREEN | \
                       BACKGROUND_BLUE

#define QCONSOLE_INPUT_RECORDS 1024

static int qconsole_chars = 0;

/*
==================
CON_Hide
==================
*/
void CON_Hide( void )
{
}

/*
==================
CON_Show
==================
*/
void CON_Show( void )
{
}

/*
==================
CON_Shutdown
==================
*/
void CON_Shutdown( void )
{
}

/*
==================
CON_Init
==================
*/
void CON_Init( void )
{
  SMALL_RECT win = { 0, 0, QCONSOLE_WIDTH-1, QCONSOLE_HEIGHT-1 };
  HANDLE hout;
  COORD screen = { 0, 0 };
  DWORD written;
  CONSOLE_SCREEN_BUFFER_INFO binfo;
  SMALL_RECT rect;

  SetConsoleTitle("ioquake3 Dedicated Server Console");

  hout = GetStdHandle( STD_OUTPUT_HANDLE );

  SetConsoleWindowInfo( hout, TRUE, &win );
  SetConsoleTextAttribute( hout, QCONSOLE_THEME );
  FillConsoleOutputAttribute( hout, QCONSOLE_THEME, 63999, screen, &written ); 
  
  // adjust console scroll to match up with cursor position  
  GetConsoleScreenBufferInfo( hout, &binfo );
  rect.Top = binfo.srWindow.Top;
  rect.Left = binfo.srWindow.Left;
  rect.Bottom = binfo.srWindow.Bottom;
  rect.Right = binfo.srWindow.Right;
  rect.Top += ( binfo.dwCursorPosition.Y - binfo.srWindow.Bottom ); 
  rect.Bottom = binfo.dwCursorPosition.Y; 
  SetConsoleWindowInfo( hout, TRUE, &rect );

}

/*
==================
CON_ConsoleInput
==================
*/
char *CON_ConsoleInput( void )
{
  HANDLE hin, hout;
  INPUT_RECORD buff[ QCONSOLE_INPUT_RECORDS ];
  DWORD count = 0;
  int i;
  static char input[ 1024 ] = { "" };
  int inputlen;
  int newlinepos = -1;
  CHAR_INFO line[ QCONSOLE_WIDTH ];
  int linelen = 0;

  inputlen = 0;
  input[ 0 ] = '\0';

  hin = GetStdHandle( STD_INPUT_HANDLE );
  if( hin == INVALID_HANDLE_VALUE )
    return NULL;
  hout = GetStdHandle( STD_OUTPUT_HANDLE );
  if( hout == INVALID_HANDLE_VALUE )
    return NULL;

  if( !PeekConsoleInput( hin, buff, QCONSOLE_INPUT_RECORDS, &count ) )
    return NULL;

  // if we have overflowed, start dropping oldest input events
  if( count == QCONSOLE_INPUT_RECORDS )
  {
    ReadConsoleInput( hin, buff, 1, &count );
    return NULL;
  } 

  for( i = 0; i < count; i++ )
  {
    if( buff[ i ].EventType == KEY_EVENT &&
        buff[ i ].Event.KeyEvent.bKeyDown )
    {
      if( buff[ i ].Event.KeyEvent.wVirtualKeyCode == VK_RETURN )
      {
        newlinepos = i;
        break;
      }

      if( linelen < QCONSOLE_WIDTH &&
          buff[ i ].Event.KeyEvent.uChar.AsciiChar )
      {
        if( buff[ i ].Event.KeyEvent.wVirtualKeyCode == VK_BACK )
        {
          if( linelen > 0 )
            linelen--;
            
        }
        else
        {
          line[ linelen ].Attributes =  QCONSOLE_THEME;
          line[ linelen++ ].Char.AsciiChar =
            buff[ i ].Event.KeyEvent.uChar.AsciiChar;
        }
      }
    }
  }

  // provide visual feedback for incomplete commands
  if( linelen != qconsole_chars )
  {
    CONSOLE_SCREEN_BUFFER_INFO binfo;
    COORD writeSize = { QCONSOLE_WIDTH, 1 };
    COORD writePos = { 0, 0 };
    SMALL_RECT writeArea = { 0, 0, 0, 0 };
    int i;

    // keep track of this so we don't need to re-write to console every frame
    qconsole_chars = linelen;

    GetConsoleScreenBufferInfo( hout, &binfo );

    // adjust scrolling to cursor when typing
    if( binfo.dwCursorPosition.Y > binfo.srWindow.Bottom )
    {
      SMALL_RECT rect;

      rect.Top = binfo.srWindow.Top;
      rect.Left = binfo.srWindow.Left;
      rect.Bottom = binfo.srWindow.Bottom;
      rect.Right = binfo.srWindow.Right;

      rect.Top += ( binfo.dwCursorPosition.Y - binfo.srWindow.Bottom ); 
      rect.Bottom = binfo.dwCursorPosition.Y; 
      
      SetConsoleWindowInfo( hout, TRUE, &rect );
      GetConsoleScreenBufferInfo( hout, &binfo );
    }
    
    writeArea.Left = 0;
    writeArea.Top = binfo.srWindow.Bottom; 
    writeArea.Bottom = binfo.srWindow.Bottom; 
    writeArea.Right = QCONSOLE_WIDTH;

    // pad line with ' ' to handle VK_BACK
    for( i = linelen; i < QCONSOLE_WIDTH; i++ )
    {
      line[ i ].Char.AsciiChar = ' '; 
      line[ i ].Attributes =  QCONSOLE_THEME;
    }

    WriteConsoleOutput( hout, line, writeSize, writePos, &writeArea );

    if( binfo.dwCursorPosition.X != linelen )
    {
      COORD cursorPos = { 0, 0 };

      cursorPos.X = linelen;
      cursorPos.Y = binfo.srWindow.Bottom;
      SetConsoleCursorPosition( hout, cursorPos );
    }
  }

  // don't touch the input buffer if this is an incomplete command
  if( newlinepos < 0)
  {
    return NULL;
  }
  else
  {
    // add a newline
    COORD cursorPos = { 0, 0 };
    CONSOLE_SCREEN_BUFFER_INFO binfo;

    GetConsoleScreenBufferInfo( hout, &binfo );
    cursorPos.Y = binfo.srWindow.Bottom + 1;
    SetConsoleCursorPosition( hout, cursorPos );
  }


  if( !ReadConsoleInput( hin, buff, newlinepos+1, &count ) )
    return NULL;

  for( i = 0; i < count; i++ )
  {
    if( buff[ i ].EventType == KEY_EVENT &&
        buff[ i ].Event.KeyEvent.bKeyDown )
    {
      if( buff[ i ].Event.KeyEvent.wVirtualKeyCode == VK_BACK )
      {
        if( inputlen > 0 )
          input[ --inputlen ] = '\0';
        continue;
      }
      if( inputlen < ( sizeof( input ) - 1 ) &&
          buff[ i ].Event.KeyEvent.uChar.AsciiChar )
      {
        input[ inputlen++ ] = buff[ i ].Event.KeyEvent.uChar.AsciiChar;
        input[ inputlen ] = '\0'; 
      }
    }
  }
  if( !inputlen )
    return NULL;
  return input;
}
