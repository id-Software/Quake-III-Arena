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
// win_main.c

#include "../client/client.h"
#include "../qcommon/qcommon.h"
#include "win_local.h"
#include "resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>

#define	CD_BASEDIR	"quake3"
#define	CD_EXE		"quake3.exe"
#define	CD_BASEDIR_LINUX	"bin\\x86\\glibc-2.1"
#define	CD_EXE_LINUX "quake3"
#define MEM_THRESHOLD 96*1024*1024

static char		sys_cmdline[MAX_STRING_CHARS];

// define this to use alternate spanking method
// I found out that the regular way doesn't work on my box for some reason
// see the associated spank.sh script
#define ALT_SPANK
#ifdef ALT_SPANK
#include <stdio.h>
#include <sys/stat.h>

int fh = 0;

void Spk_Open(char *name)
{
  fh = open( name, O_TRUNC | O_CREAT | O_WRONLY, S_IREAD | S_IWRITE );
};

void Spk_Close(void)
{
  if (!fh)
    return;

  close( fh );
  fh = 0;
}

void Spk_Printf (const char *text, ...)
{
  va_list argptr;
  char buf[32768];

  if (!fh)
    return;

  va_start (argptr,text);
  vsprintf (buf, text, argptr);
  write(fh, buf, strlen(buf));
  _commit(fh);
  va_end (argptr);

};
#endif

/*
==================
Sys_LowPhysicalMemory()
==================
*/

qboolean Sys_LowPhysicalMemory() {
	MEMORYSTATUS stat;
  GlobalMemoryStatus (&stat);
	return (stat.dwTotalPhys <= MEM_THRESHOLD) ? qtrue : qfalse;
}

/*
==================
Sys_BeginProfiling
==================
*/
void Sys_BeginProfiling( void ) {
	// this is just used on the mac build
}

/*
=============
Sys_Error

Show the early console as an error dialog
=============
*/
void QDECL Sys_Error( const char *error, ... ) {
	va_list		argptr;
	char		text[4096];
    MSG        msg;

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	Conbuf_AppendText( text );
	Conbuf_AppendText( "\n" );

	Sys_SetErrorText( text );
	Sys_ShowConsole( 1, qtrue );

	timeEndPeriod( 1 );

	IN_Shutdown();

	// wait for the user to quit
	while ( 1 ) {
		if (!GetMessage (&msg, NULL, 0, 0))
			Com_Quit_f ();
		TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	Sys_DestroyConsole();

	exit (1);
}

/*
==============
Sys_Quit
==============
*/
void Sys_Quit( void ) {
	timeEndPeriod( 1 );
	IN_Shutdown();
	Sys_DestroyConsole();

	exit (0);
}

/*
==============
Sys_Print
==============
*/
void Sys_Print( const char *msg ) {
	Conbuf_AppendText( msg );
}


/*
==============
Sys_Mkdir
==============
*/
void Sys_Mkdir( const char *path ) {
	_mkdir (path);
}

/*
==============
Sys_Cwd
==============
*/
char *Sys_Cwd( void ) {
	static char cwd[MAX_OSPATH];

	_getcwd( cwd, sizeof( cwd ) - 1 );
	cwd[MAX_OSPATH-1] = 0;

	return cwd;
}

/*
==============
Sys_DefaultCDPath
==============
*/
char *Sys_DefaultCDPath( void ) {
	return "";
}

/*
==============
Sys_DefaultBasePath
==============
*/
char *Sys_DefaultBasePath( void ) {
	return Sys_Cwd();
}

/*
==============================================================

DIRECTORY SCANNING

==============================================================
*/

#define	MAX_FOUND_FILES	0x1000

void Sys_ListFilteredFiles( const char *basedir, char *subdirs, char *filter, char **list, int *numfiles ) {
	char		search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
	char		filename[MAX_OSPATH];
	int			findhandle;
	struct _finddata_t findinfo;

	if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
		return;
	}

	if (strlen(subdirs)) {
		Com_sprintf( search, sizeof(search), "%s\\%s\\*", basedir, subdirs );
	}
	else {
		Com_sprintf( search, sizeof(search), "%s\\*", basedir );
	}

	findhandle = _findfirst (search, &findinfo);
	if (findhandle == -1) {
		return;
	}

	do {
		if (findinfo.attrib & _A_SUBDIR) {
			if (Q_stricmp(findinfo.name, ".") && Q_stricmp(findinfo.name, "..")) {
				if (strlen(subdirs)) {
					Com_sprintf( newsubdirs, sizeof(newsubdirs), "%s\\%s", subdirs, findinfo.name);
				}
				else {
					Com_sprintf( newsubdirs, sizeof(newsubdirs), "%s", findinfo.name);
				}
				Sys_ListFilteredFiles( basedir, newsubdirs, filter, list, numfiles );
			}
		}
		if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
			break;
		}
		Com_sprintf( filename, sizeof(filename), "%s\\%s", subdirs, findinfo.name );
		if (!Com_FilterPath( filter, filename, qfalse ))
			continue;
		list[ *numfiles ] = CopyString( filename );
		(*numfiles)++;
	} while ( _findnext (findhandle, &findinfo) != -1 );

	_findclose (findhandle);
}

static qboolean strgtr(const char *s0, const char *s1) {
	int l0, l1, i;

	l0 = strlen(s0);
	l1 = strlen(s1);

	if (l1<l0) {
		l0 = l1;
	}

	for(i=0;i<l0;i++) {
		if (s1[i] > s0[i]) {
			return qtrue;
		}
		if (s1[i] < s0[i]) {
			return qfalse;
		}
	}
	return qfalse;
}

char **Sys_ListFiles( const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs ) {
	char		search[MAX_OSPATH];
	int			nfiles;
	char		**listCopy;
	char		*list[MAX_FOUND_FILES];
	struct _finddata_t findinfo;
	int			findhandle;
	int			flag;
	int			i;

	if (filter) {

		nfiles = 0;
		Sys_ListFilteredFiles( directory, "", filter, list, &nfiles );

		list[ nfiles ] = 0;
		*numfiles = nfiles;

		if (!nfiles)
			return NULL;

		listCopy = Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );
		for ( i = 0 ; i < nfiles ; i++ ) {
			listCopy[i] = list[i];
		}
		listCopy[i] = NULL;

		return listCopy;
	}

	if ( !extension) {
		extension = "";
	}

	// passing a slash as extension will find directories
	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		flag = 0;
	} else {
		flag = _A_SUBDIR;
	}

	Com_sprintf( search, sizeof(search), "%s\\*%s", directory, extension );

	// search
	nfiles = 0;

	findhandle = _findfirst (search, &findinfo);
	if (findhandle == -1) {
		*numfiles = 0;
		return NULL;
	}

	do {
		if ( (!wantsubs && flag ^ ( findinfo.attrib & _A_SUBDIR )) || (wantsubs && findinfo.attrib & _A_SUBDIR) ) {
			if ( nfiles == MAX_FOUND_FILES - 1 ) {
				break;
			}
			list[ nfiles ] = CopyString( findinfo.name );
			nfiles++;
		}
	} while ( _findnext (findhandle, &findinfo) != -1 );

	list[ nfiles ] = 0;

	_findclose (findhandle);

	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles ) {
		return NULL;
	}

	listCopy = Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );
	for ( i = 0 ; i < nfiles ; i++ ) {
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	do {
		flag = 0;
		for(i=1; i<nfiles; i++) {
			if (strgtr(listCopy[i-1], listCopy[i])) {
				char *temp = listCopy[i];
				listCopy[i] = listCopy[i-1];
				listCopy[i-1] = temp;
				flag = 1;
			}
		}
	} while(flag);

	return listCopy;
}

void	Sys_FreeFileList( char **list ) {
	int		i;

	if ( !list ) {
		return;
	}

	for ( i = 0 ; list[i] ; i++ ) {
		Z_Free( list[i] );
	}

	Z_Free( list );
}

//========================================================


/*
================
Sys_ScanForCD

Search all the drives to see if there is a valid CD to grab
the cddir from
================
*/
qboolean Sys_ScanForCD( void ) {
	static char	cddir[MAX_OSPATH];
	char		drive[4];
	FILE		*f;
	char		test[MAX_OSPATH];
#if 0
	// don't override a cdpath on the command line
	if ( strstr( sys_cmdline, "cdpath" ) ) {
		return;
	}
#endif

	drive[0] = 'c';
	drive[1] = ':';
	drive[2] = '\\';
	drive[3] = 0;

	// scan the drives
	for ( drive[0] = 'c' ; drive[0] <= 'z' ; drive[0]++ ) {
		if ( GetDriveType (drive) != DRIVE_CDROM ) {
			continue;
		}

		sprintf (cddir, "%s%s", drive, CD_BASEDIR);
		sprintf (test, "%s\\%s", cddir, CD_EXE);
		f = fopen( test, "r" );
		if ( f ) {
			fclose (f);
			return qtrue;
    } else {
      sprintf(cddir, "%s%s", drive, CD_BASEDIR_LINUX);
      sprintf(test, "%s\\%s", cddir, CD_EXE_LINUX);
  		f = fopen( test, "r" );
	  	if ( f ) {
		  	fclose (f);
			  return qtrue;
      }
    }
	}

	return qfalse;
}

/*
================
Sys_CheckCD

Return true if the proper CD is in the drive
================
*/
qboolean	Sys_CheckCD( void ) {
  // FIXME: mission pack
  return qtrue;
	//return Sys_ScanForCD();
}


/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData( void ) {
	char *data = NULL;
	char *cliptext;

	if ( OpenClipboard( NULL ) != 0 ) {
		HANDLE hClipboardData;

		if ( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != 0 ) {
			if ( ( cliptext = GlobalLock( hClipboardData ) ) != 0 ) {
				data = Z_Malloc( GlobalSize( hClipboardData ) + 1 );
				Q_strncpyz( data, cliptext, GlobalSize( hClipboardData ) );
				GlobalUnlock( hClipboardData );
				
				strtok( data, "\n\r\b" );
			}
		}
		CloseClipboard();
	}
	return data;
}


/*
========================================================================

LOAD/UNLOAD DLL

========================================================================
*/

/*
=================
Sys_UnloadDll

=================
*/
void Sys_UnloadDll( void *dllHandle ) {
	if ( !dllHandle ) {
		return;
	}
	if ( !FreeLibrary( dllHandle ) ) {
		Com_Error (ERR_FATAL, "Sys_UnloadDll FreeLibrary failed");
	}
}

/*
=================
Sys_LoadDll

Used to load a development dll instead of a virtual machine

TTimo: added some verbosity in debug
=================
*/
extern char		*FS_BuildOSPath( const char *base, const char *game, const char *qpath );

// fqpath param added 7/20/02 by T.Ray - Sys_LoadDll is only called in vm.c at this time
// fqpath will be empty if dll not loaded, otherwise will hold fully qualified path of dll module loaded
// fqpath buffersize must be at least MAX_QPATH+1 bytes long
void * QDECL Sys_LoadDll( const char *name, char *fqpath , intptr_t (QDECL **entryPoint)(intptr_t, ...),
				  intptr_t (QDECL *systemcalls)(intptr_t, ...) ) {
	HINSTANCE	libHandle;
	void	(QDECL *dllEntry)( intptr_t (QDECL *syscallptr)(intptr_t, ...) );
	char	*basepath;
	char	*cdpath;
	char	*gamedir;
	char	*fn;
#ifdef NDEBUG
	static int	lastWarning = 0;
	int		timestamp;
  int   ret;
#endif
	char	filename[MAX_QPATH];

	*fqpath = 0 ;		// added 7/20/02 by T.Ray

	Com_sprintf( filename, sizeof( filename ), "%sx86.dll", name );

#ifdef NDEBUG
	timestamp = Sys_Milliseconds();
	if( ((timestamp - lastWarning) > (5 * 60000)) && !Cvar_VariableIntegerValue( "dedicated" )
		&& !Cvar_VariableIntegerValue( "com_blindlyLoadDLLs" ) ) {
		if (FS_FileExists(filename)) {
			lastWarning = timestamp;
			ret = MessageBoxEx( NULL, "You are about to load a .DLL executable that\n"
				  "has not been verified for use with Quake III Arena.\n"
				  "This type of file can compromise the security of\n"
				  "your computer.\n\n"
				  "Select 'OK' if you choose to load it anyway.",
				  "Security Warning", MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2 | MB_TOPMOST | MB_SETFOREGROUND,
				  MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ) );
			if( ret != IDOK ) {
				return NULL;
			}
		}
	}
#endif

#ifndef NDEBUG
	libHandle = LoadLibrary( filename );
  if (libHandle)
    Com_Printf("LoadLibrary '%s' ok\n", filename);
  else
    Com_Printf("LoadLibrary '%s' failed\n", filename);
	if ( !libHandle ) {
#endif
	basepath = Cvar_VariableString( "fs_basepath" );
	cdpath = Cvar_VariableString( "fs_cdpath" );
	gamedir = Cvar_VariableString( "fs_game" );

	fn = FS_BuildOSPath( basepath, gamedir, filename );
	libHandle = LoadLibrary( fn );
#ifndef NDEBUG
  if (libHandle)
    Com_Printf("LoadLibrary '%s' ok\n", fn);
  else
    Com_Printf("LoadLibrary '%s' failed\n", fn);
#endif

	if ( !libHandle ) {
		if( cdpath[0] ) {
			fn = FS_BuildOSPath( cdpath, gamedir, filename );
			libHandle = LoadLibrary( fn );
#ifndef NDEBUG
      if (libHandle)
        Com_Printf("LoadLibrary '%s' ok\n", fn);
      else
        Com_Printf("LoadLibrary '%s' failed\n", fn);
#endif
		}

		if ( !libHandle ) {
			return NULL;
		}
	}
#ifndef NDEBUG
	}
#endif

	dllEntry = ( void (QDECL *)(intptr_t (QDECL *)( intptr_t, ... ) ) )GetProcAddress( libHandle, "dllEntry" ); 
	*entryPoint = (intptr_t (QDECL *)(intptr_t,...))GetProcAddress( libHandle, "vmMain" );
	if ( !*entryPoint || !dllEntry ) {
		FreeLibrary( libHandle );
		return NULL;
	}
	dllEntry( systemcalls );

	if ( libHandle ) Q_strncpyz ( fqpath , filename , MAX_QPATH ) ;		// added 7/20/02 by T.Ray
	return libHandle;
}


/*
========================================================================

BACKGROUND FILE STREAMING

========================================================================
*/

#if 1

void Sys_InitStreamThread( void ) {
}

void Sys_ShutdownStreamThread( void ) {
}

void Sys_BeginStreamedFile( fileHandle_t f, int readAhead ) {
}

void Sys_EndStreamedFile( fileHandle_t f ) {
}

int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f ) {
   return FS_Read( buffer, size * count, f );
}

void Sys_StreamSeek( fileHandle_t f, int offset, int origin ) {
   FS_Seek( f, offset, origin );
}


#else

typedef struct {
	fileHandle_t	file;
	byte	*buffer;
	qboolean	eof;
	qboolean	active;
	int		bufferSize;
	int		streamPosition;	// next byte to be returned by Sys_StreamRead
	int		threadPosition;	// next byte to be read from file
} streamsIO_t;

typedef struct {
	HANDLE				threadHandle;
	int					threadId;
	CRITICAL_SECTION	crit;
	streamsIO_t			sIO[MAX_FILE_HANDLES];
} streamState_t;

streamState_t	stream;

/*
===============
Sys_StreamThread

A thread will be sitting in this loop forever
================
*/
void Sys_StreamThread( void ) {
	int		buffer;
	int		count;
	int		readCount;
	int		bufferPoint;
	int		r, i;

	while (1) {
		Sleep( 10 );
//		EnterCriticalSection (&stream.crit);

		for (i=1;i<MAX_FILE_HANDLES;i++) {
			// if there is any space left in the buffer, fill it up
			if ( stream.sIO[i].active  && !stream.sIO[i].eof ) {
				count = stream.sIO[i].bufferSize - (stream.sIO[i].threadPosition - stream.sIO[i].streamPosition);
				if ( !count ) {
					continue;
				}

				bufferPoint = stream.sIO[i].threadPosition % stream.sIO[i].bufferSize;
				buffer = stream.sIO[i].bufferSize - bufferPoint;
				readCount = buffer < count ? buffer : count;

				r = FS_Read( stream.sIO[i].buffer + bufferPoint, readCount, stream.sIO[i].file );
				stream.sIO[i].threadPosition += r;

				if ( r != readCount ) {
					stream.sIO[i].eof = qtrue;
				}
			}
		}
//		LeaveCriticalSection (&stream.crit);
	}
}

/*
===============
Sys_InitStreamThread

================
*/
void Sys_InitStreamThread( void ) {
	int i;

	InitializeCriticalSection ( &stream.crit );

	// don't leave the critical section until there is a
	// valid file to stream, which will cause the StreamThread
	// to sleep without any overhead
//	EnterCriticalSection( &stream.crit );

	stream.threadHandle = CreateThread(
	   NULL,	// LPSECURITY_ATTRIBUTES lpsa,
	   0,		// DWORD cbStack,
	   (LPTHREAD_START_ROUTINE)Sys_StreamThread,	// LPTHREAD_START_ROUTINE lpStartAddr,
	   0,			// LPVOID lpvThreadParm,
	   0,			//   DWORD fdwCreate,
	   &stream.threadId);
	for(i=0;i<MAX_FILE_HANDLES;i++) {
		stream.sIO[i].active = qfalse;
	}
}

/*
===============
Sys_ShutdownStreamThread

================
*/
void Sys_ShutdownStreamThread( void ) {
}


/*
===============
Sys_BeginStreamedFile

================
*/
void Sys_BeginStreamedFile( fileHandle_t f, int readAhead ) {
	if ( stream.sIO[f].file ) {
		Sys_EndStreamedFile( stream.sIO[f].file );
	}

	stream.sIO[f].file = f;
	stream.sIO[f].buffer = Z_Malloc( readAhead );
	stream.sIO[f].bufferSize = readAhead;
	stream.sIO[f].streamPosition = 0;
	stream.sIO[f].threadPosition = 0;
	stream.sIO[f].eof = qfalse;
	stream.sIO[f].active = qtrue;

	// let the thread start running
//	LeaveCriticalSection( &stream.crit );
}

/*
===============
Sys_EndStreamedFile

================
*/
void Sys_EndStreamedFile( fileHandle_t f ) {
	if ( f != stream.sIO[f].file ) {
		Com_Error( ERR_FATAL, "Sys_EndStreamedFile: wrong file");
	}
	// don't leave critical section until another stream is started
	EnterCriticalSection( &stream.crit );

	stream.sIO[f].file = 0;
	stream.sIO[f].active = qfalse;

	Z_Free( stream.sIO[f].buffer );

	LeaveCriticalSection( &stream.crit );
}


/*
===============
Sys_StreamedRead

================
*/
int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f ) {
	int		available;
	int		remaining;
	int		sleepCount;
	int		copy;
	int		bufferCount;
	int		bufferPoint;
	byte	*dest;

	if (stream.sIO[f].active == qfalse) {
		Com_Error( ERR_FATAL, "Streamed read with non-streaming file" );
	}

	dest = (byte *)buffer;
	remaining = size * count;

	if ( remaining <= 0 ) {
		Com_Error( ERR_FATAL, "Streamed read with non-positive size" );
	}

	sleepCount = 0;
	while ( remaining > 0 ) {
		available = stream.sIO[f].threadPosition - stream.sIO[f].streamPosition;
		if ( !available ) {
			if ( stream.sIO[f].eof ) {
				break;
			}
			if ( sleepCount == 1 ) {
				Com_DPrintf( "Sys_StreamedRead: waiting\n" );
			}
			if ( ++sleepCount > 100 ) {
				Com_Error( ERR_FATAL, "Sys_StreamedRead: thread has died");
			}
			Sleep( 10 );
			continue;
		}

		EnterCriticalSection( &stream.crit );

		bufferPoint = stream.sIO[f].streamPosition % stream.sIO[f].bufferSize;
		bufferCount = stream.sIO[f].bufferSize - bufferPoint;

		copy = available < bufferCount ? available : bufferCount;
		if ( copy > remaining ) {
			copy = remaining;
		}
		memcpy( dest, stream.sIO[f].buffer + bufferPoint, copy );
		stream.sIO[f].streamPosition += copy;
		dest += copy;
		remaining -= copy;

		LeaveCriticalSection( &stream.crit );
	}

	return (count * size - remaining) / size;
}

/*
===============
Sys_StreamSeek

================
*/
void Sys_StreamSeek( fileHandle_t f, int offset, int origin ) {

	// halt the thread
	EnterCriticalSection( &stream.crit );

	// clear to that point
	FS_Seek( f, offset, origin );
	stream.sIO[f].streamPosition = 0;
	stream.sIO[f].threadPosition = 0;
	stream.sIO[f].eof = qfalse;

	// let the thread start running at the new position
	LeaveCriticalSection( &stream.crit );
}

#endif

/*
========================================================================

EVENT LOOP

========================================================================
*/

#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

sysEvent_t	eventQue[MAX_QUED_EVENTS];
int			eventHead, eventTail;
byte		sys_packetReceived[MAX_MSGLEN];

/*
================
Sys_QueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr ) {
	sysEvent_t	*ev;

	ev = &eventQue[ eventHead & MASK_QUED_EVENTS ];
	if ( eventHead - eventTail >= MAX_QUED_EVENTS ) {
		Com_Printf("Sys_QueEvent: overflow\n");
		// we are discarding an event, but don't leak memory
		if ( ev->evPtr ) {
			Z_Free( ev->evPtr );
		}
		eventTail++;
	}

	eventHead++;

	if ( time == 0 ) {
		time = Sys_Milliseconds();
	}

	ev->evTime = time;
	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}

/*
================
Sys_GetEvent

================
*/
sysEvent_t Sys_GetEvent( void ) {
    MSG			msg;
	sysEvent_t	ev;
	char		*s;
	msg_t		netmsg;
	netadr_t	adr;

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// pump the message loop
	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)) {
		if ( !GetMessage (&msg, NULL, 0, 0) ) {
			Com_Quit_f();
		}

		// save the msg time, because wndprocs don't have access to the timestamp
		g_wv.sysMsgTime = msg.time;

		TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	// check for console commands
	s = Sys_ConsoleInput();
	if ( s ) {
		char	*b;
		int		len;

		len = strlen( s ) + 1;
		b = Z_Malloc( len );
		Q_strncpyz( b, s, len-1 );
		Sys_QueEvent( 0, SE_CONSOLE, 0, 0, len, b );
	}

	// check for network packets
	MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
	if ( Sys_GetPacket ( &adr, &netmsg ) ) {
		netadr_t		*buf;
		int				len;

		// copy out to a seperate buffer for qeueing
		// the readcount stepahead is for SOCKS support
		len = sizeof( netadr_t ) + netmsg.cursize - netmsg.readcount;
		buf = Z_Malloc( len );
		*buf = adr;
		memcpy( buf+1, &netmsg.data[netmsg.readcount], netmsg.cursize - netmsg.readcount );
		Sys_QueEvent( 0, SE_PACKET, 0, 0, len, buf );
	}

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// create an empty event to return

	memset( &ev, 0, sizeof( ev ) );
	ev.evTime = timeGetTime();

	return ev;
}

//================================================================

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
void Sys_In_Restart_f( void ) {
	IN_Shutdown();
	IN_Init();
}


/*
=================
Sys_Net_Restart_f

Restart the network subsystem
=================
*/
void Sys_Net_Restart_f( void ) {
	NET_Restart();
}


/*
================
Sys_Init

Called after the common systems (cvars, files, etc)
are initialized
================
*/
#define OSR2_BUILD_NUMBER 1111
#define WIN98_BUILD_NUMBER 1998

void Sys_Init( void ) {
	int cpuid;

	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
	timeBeginPeriod( 1 );

	Cmd_AddCommand ("in_restart", Sys_In_Restart_f);
	Cmd_AddCommand ("net_restart", Sys_Net_Restart_f);

	g_wv.osversion.dwOSVersionInfoSize = sizeof( g_wv.osversion );

	if (!GetVersionEx (&g_wv.osversion))
		Sys_Error ("Couldn't get OS info");

	if (g_wv.osversion.dwMajorVersion < 4)
		Sys_Error ("Quake3 requires Windows version 4 or greater");
	if (g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32s)
		Sys_Error ("Quake3 doesn't run on Win32s");

	if ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32_NT )
	{
		Cvar_Set( "arch", "winnt" );
	}
	else if ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
	{
		if ( LOWORD( g_wv.osversion.dwBuildNumber ) >= WIN98_BUILD_NUMBER )
		{
			Cvar_Set( "arch", "win98" );
		}
		else if ( LOWORD( g_wv.osversion.dwBuildNumber ) >= OSR2_BUILD_NUMBER )
		{
			Cvar_Set( "arch", "win95 osr2.x" );
		}
		else
		{
			Cvar_Set( "arch", "win95" );
		}
	}
	else
	{
		Cvar_Set( "arch", "unknown Windows variant" );
	}

	// save out a couple things in rom cvars for the renderer to access
	Cvar_Get( "win_hinstance", va("%i", (int)g_wv.hInstance), CVAR_ROM );
	Cvar_Get( "win_wndproc", va("%i", (int)MainWndProc), CVAR_ROM );

	//
	// figure out our CPU
	//
	Cvar_Get( "sys_cpustring", "detect", 0 );
	if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring"), "detect" ) )
	{
		Com_Printf( "...detecting CPU, found " );

#ifndef __MINGW32__
		cpuid = Sys_GetProcessorId();
#else // See comments in win_shared.c
		cpuid = CPUID_GENERIC;
#endif

		switch ( cpuid )
		{
		case CPUID_GENERIC:
			Cvar_Set( "sys_cpustring", "generic" );
			break;
		case CPUID_INTEL_UNSUPPORTED:
			Cvar_Set( "sys_cpustring", "x86 (pre-Pentium)" );
			break;
		case CPUID_INTEL_PENTIUM:
			Cvar_Set( "sys_cpustring", "x86 (P5/PPro, non-MMX)" );
			break;
		case CPUID_INTEL_MMX:
			Cvar_Set( "sys_cpustring", "x86 (P5/Pentium2, MMX)" );
			break;
		case CPUID_INTEL_KATMAI:
			Cvar_Set( "sys_cpustring", "Intel Pentium III" );
			break;
		case CPUID_AMD_3DNOW:
			Cvar_Set( "sys_cpustring", "AMD w/ 3DNow!" );
			break;
		case CPUID_AXP:
			Cvar_Set( "sys_cpustring", "Alpha AXP" );
			break;
		default:
			Com_Error( ERR_FATAL, "Unknown cpu type %d\n", cpuid );
			break;
		}
	}
	else
	{
		Com_Printf( "...forcing CPU type to " );
		if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "generic" ) )
		{
			cpuid = CPUID_GENERIC;
		}
		else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "x87" ) )
		{
			cpuid = CPUID_INTEL_PENTIUM;
		}
		else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "mmx" ) )
		{
			cpuid = CPUID_INTEL_MMX;
		}
		else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "3dnow" ) )
		{
			cpuid = CPUID_AMD_3DNOW;
		}
		else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "PentiumIII" ) )
		{
			cpuid = CPUID_INTEL_KATMAI;
		}
		else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "axp" ) )
		{
			cpuid = CPUID_AXP;
		}
		else
		{
			Com_Printf( "WARNING: unknown sys_cpustring '%s'\n", Cvar_VariableString( "sys_cpustring" ) );
			cpuid = CPUID_GENERIC;
		}
	}
	Cvar_SetValue( "sys_cpuid", cpuid );
	Com_Printf( "%s\n", Cvar_VariableString( "sys_cpustring" ) );

	Cvar_Set( "username", Sys_GetCurrentUser() );

	IN_Init();		// FIXME: not in dedicated?
}


qboolean Sys_DetectAltivec( void )
{
    return qfalse;  // never altivec on Windows...at least for now.  :)
}



//=======================================================================

int	totalMsec, countMsec;

/*
==================
WinMain

==================
*/
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	char		cwd[MAX_OSPATH];
	int			startTime, endTime;

    // should never get a previous instance in Win32
    if ( hPrevInstance ) {
        return 0;
	}

	g_wv.hInstance = hInstance;
	Q_strncpyz( sys_cmdline, lpCmdLine, sizeof( sys_cmdline ) );

	// done before Com/Sys_Init since we need this for error output
	Sys_CreateConsole();

	// no abort/retry/fail errors
	SetErrorMode( SEM_FAILCRITICALERRORS );

	// get the initial time base
	Sys_Milliseconds();
#if 0
	// if we find the CD, add a +set cddir xxx command line
	Sys_ScanForCD();
#endif

	Sys_InitStreamThread();

	Com_Init( sys_cmdline );
	NET_Init();

	_getcwd (cwd, sizeof(cwd));
	Com_Printf("Working directory: %s\n", cwd);

	// hide the early console since we've reached the point where we
	// have a working graphics subsystems
	if ( !com_dedicated->integer && !com_viewlog->integer ) {
		Sys_ShowConsole( 0, qfalse );
	}

    // main game loop
	while( 1 ) {
		// if not running as a game client, sleep a bit
		if ( g_wv.isMinimized || ( com_dedicated && com_dedicated->integer ) ) {
			Sleep( 5 );
		}

		// set low precision every frame, because some system calls
		// reset it arbitrarily
//		_controlfp( _PC_24, _MCW_PC );
//    _controlfp( -1, _MCW_EM  ); // no exceptions, even if some crappy
                                // syscall turns them back on!

		startTime = Sys_Milliseconds();

		// make sure mouse and joystick are only called once a frame
		IN_Frame();

		// run the game
		Com_Frame();

		endTime = Sys_Milliseconds();
		totalMsec += endTime - startTime;
		countMsec++;
	}

	// never gets here
}


