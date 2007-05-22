/*
===========================================================================
Copyright (C) 2006 Tony J. White (tjw@tjw.org)

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

#if USE_CURL
#include "client.h"
cvar_t *cl_cURLLib;

#if USE_CURL_DLOPEN

#if USE_SDL_VIDEO
#include "SDL.h"
#include "SDL_loadso.h"
#define OBJTYPE void *
#define OBJLOAD(x) SDL_LoadObject(x)
#define SYMLOAD(x,y) SDL_LoadFunction(x,y)
#define OBJFREE(x) SDL_UnloadObject(x)

#elif defined _WIN32
#include <windows.h>
#define OBJTYPE HMODULE
#define OBJLOAD(x) LoadLibrary(x)
#define SYMLOAD(x,y) GetProcAddress(x,y)
#define OBJFREE(x) FreeLibrary(x)

#elif defined __linux__ || defined __FreeBSD__ || defined MACOS_X || defined __sun
#include <dlfcn.h>
#define OBJTYPE void *
#define OBJLOAD(x) dlopen(x, RTLD_LAZY | RTLD_GLOBAL)
#define SYMLOAD(x,y) dlsym(x,y)
#define OBJFREE(x) dlclose(x)
#else

#error "Your platform has no lib loading code or it is disabled"
#endif

#if defined __linux__ || defined __FreeBSD__ || defined MACOS_X
#include <unistd.h>
#include <sys/types.h>
#endif

char* (*qcurl_version)(void);

CURL* (*qcurl_easy_init)(void);
CURLcode (*qcurl_easy_setopt)(CURL *curl, CURLoption option, ...);
CURLcode (*qcurl_easy_perform)(CURL *curl);
void (*qcurl_easy_cleanup)(CURL *curl);
CURLcode (*qcurl_easy_getinfo)(CURL *curl, CURLINFO info, ...);
CURL* (*qcurl_easy_duphandle)(CURL *curl);
void (*qcurl_easy_reset)(CURL *curl);
const char *(*qcurl_easy_strerror)(CURLcode);

CURLM* (*qcurl_multi_init)(void);
CURLMcode (*qcurl_multi_add_handle)(CURLM *multi_handle,
                                                CURL *curl_handle);
CURLMcode (*qcurl_multi_remove_handle)(CURLM *multi_handle,
                                                CURL *curl_handle);
CURLMcode (*qcurl_multi_fdset)(CURLM *multi_handle,
                                                fd_set *read_fd_set,
                                                fd_set *write_fd_set,
                                                fd_set *exc_fd_set,
                                                int *max_fd);
CURLMcode (*qcurl_multi_perform)(CURLM *multi_handle,
                                                int *running_handles);
CURLMcode (*qcurl_multi_cleanup)(CURLM *multi_handle);
CURLMsg *(*qcurl_multi_info_read)(CURLM *multi_handle,
                                                int *msgs_in_queue);
const char *(*qcurl_multi_strerror)(CURLMcode);

static OBJTYPE cURLLib = NULL;

/*
=================
GPA
=================
*/
static void *GPA(char *str)
{
	void *rv;

	rv = SYMLOAD(cURLLib, str);
	if(!rv)
	{
		Com_Printf("Can't load symbol %s\n", str);
		clc.cURLEnabled = qfalse;
		return NULL;
	}
	else
	{
		Com_DPrintf("Loaded symbol %s (0x%p)\n", str, rv);
        return rv;
	}
}
#endif /* USE_CURL_DLOPEN */

/*
=================
CL_cURL_Init
=================
*/
qboolean CL_cURL_Init()
{
#if USE_CURL_DLOPEN
	if(cURLLib)
		return qtrue;


	Com_Printf("Loading \"%s\"...", cl_cURLLib->string);
	if( (cURLLib = OBJLOAD(cl_cURLLib->string)) == 0 )
	{
#ifdef _WIN32
		return qfalse;
#else
		char fn[1024];
		getcwd(fn, sizeof(fn));
		strncat(fn, "/", sizeof(fn)-strlen(fn)-1);
		strncat(fn, cl_cURLLib->string, sizeof(fn)-strlen(fn)-1);

		if( (cURLLib = OBJLOAD(fn)) == 0 )
		{
			return qfalse;
		}
#endif /* _WIN32 */
	}

	clc.cURLEnabled = qtrue;

	qcurl_version = GPA("curl_version");

	qcurl_easy_init = GPA("curl_easy_init");
	qcurl_easy_setopt = GPA("curl_easy_setopt");
	qcurl_easy_perform = GPA("curl_easy_perform");
	qcurl_easy_cleanup = GPA("curl_easy_cleanup");
	qcurl_easy_getinfo = GPA("curl_easy_getinfo");
	qcurl_easy_duphandle = GPA("curl_easy_duphandle");
	qcurl_easy_reset = GPA("curl_easy_reset");
	qcurl_easy_strerror = GPA("curl_easy_strerror");
	
	qcurl_multi_init = GPA("curl_multi_init");
	qcurl_multi_add_handle = GPA("curl_multi_add_handle");
	qcurl_multi_remove_handle = GPA("curl_multi_remove_handle");
	qcurl_multi_fdset = GPA("curl_multi_fdset");
	qcurl_multi_perform = GPA("curl_multi_perform");
	qcurl_multi_cleanup = GPA("curl_multi_cleanup");
	qcurl_multi_info_read = GPA("curl_multi_info_read");
	qcurl_multi_strerror = GPA("curl_multi_strerror");

	if(!clc.cURLEnabled)
	{
		CL_cURL_Shutdown();
		Com_Printf("FAIL One or more symbols not found\n");
		return qfalse;
	}
	Com_Printf("OK\n");

	return qtrue;
#else
	clc.cURLEnabled = qtrue;
	return qtrue;
#endif /* USE_CURL_DLOPEN */
}

/*
=================
CL_cURL_Shutdown
=================
*/
void CL_cURL_Shutdown( void )
{
	CL_cURL_Cleanup();
#if USE_CURL_DLOPEN
	if(cURLLib)
	{
		OBJFREE(cURLLib);
		cURLLib = NULL;
	}
	qcurl_easy_init = NULL;
	qcurl_easy_setopt = NULL;
	qcurl_easy_perform = NULL;
	qcurl_easy_cleanup = NULL;
	qcurl_easy_getinfo = NULL;
	qcurl_easy_duphandle = NULL;
	qcurl_easy_reset = NULL;

	qcurl_multi_init = NULL;
	qcurl_multi_add_handle = NULL;
	qcurl_multi_remove_handle = NULL;
	qcurl_multi_fdset = NULL;
	qcurl_multi_perform = NULL;
	qcurl_multi_cleanup = NULL;
	qcurl_multi_info_read = NULL;
	qcurl_multi_strerror = NULL;
#endif /* USE_CURL_DLOPEN */
}

void CL_cURL_Cleanup(void)
{
	if(clc.downloadCURLM) {
		if(clc.downloadCURL) {
			qcurl_multi_remove_handle(clc.downloadCURLM,
				clc.downloadCURL);
			qcurl_easy_cleanup(clc.downloadCURL);
		}
		qcurl_multi_cleanup(clc.downloadCURLM);
		clc.downloadCURLM = NULL;
		clc.downloadCURL = NULL;
	}
	else if(clc.downloadCURL) {
		qcurl_easy_cleanup(clc.downloadCURL);
		clc.downloadCURL = NULL;
	}
}

static int CL_cURL_CallbackProgress( void *dummy, double dltotal, double dlnow,
	double ultotal, double ulnow )
{
	clc.downloadSize = (int)dltotal;
	Cvar_SetValue( "cl_downloadSize", clc.downloadSize );
	clc.downloadCount = (int)dlnow;
	Cvar_SetValue( "cl_downloadCount", clc.downloadCount );
	return 0;
}

static int CL_cURL_CallbackWrite(void *buffer, size_t size, size_t nmemb,
	void *stream)
{
	FS_Write( buffer, size*nmemb, ((fileHandle_t*)stream)[0] );
	return size*nmemb;
}

void CL_cURL_BeginDownload( const char *localName, const char *remoteURL )
{
	clc.cURLUsed = qtrue;
	Com_Printf("URL: %s\n", remoteURL);
	Com_DPrintf("***** CL_cURL_BeginDownload *****\n"
		"Localname: %s\n"
		"RemoteURL: %s\n"
		"****************************\n", localName, remoteURL);
	CL_cURL_Cleanup();
	Q_strncpyz(clc.downloadURL, remoteURL, sizeof(clc.downloadURL));
	Q_strncpyz(clc.downloadName, localName, sizeof(clc.downloadName));
	Com_sprintf(clc.downloadTempName, sizeof(clc.downloadTempName),
		"%s.tmp", localName);

	// Set so UI gets access to it
	Cvar_Set("cl_downloadName", localName);
	Cvar_Set("cl_downloadSize", "0");
	Cvar_Set("cl_downloadCount", "0");
	Cvar_SetValue("cl_downloadTime", cls.realtime);

	clc.downloadBlock = 0; // Starting new file
	clc.downloadCount = 0;

	clc.downloadCURL = qcurl_easy_init();
	if(!clc.downloadCURL) {
		Com_Error(ERR_DROP, "CL_cURL_BeginDownload: qcurl_easy_init() "
			"failed\n");
		return;
	}
	clc.download = FS_SV_FOpenFileWrite(clc.downloadTempName);
	if(!clc.download) {
		Com_Error(ERR_DROP, "CL_cURL_BeginDownload: failed to open "
			"%s for writing\n", clc.downloadTempName);
		return;
	}
	qcurl_easy_setopt(clc.downloadCURL, CURLOPT_WRITEDATA, clc.download);
	if(com_developer->integer)
		qcurl_easy_setopt(clc.downloadCURL, CURLOPT_VERBOSE, 1);
	qcurl_easy_setopt(clc.downloadCURL, CURLOPT_URL, clc.downloadURL);
	qcurl_easy_setopt(clc.downloadCURL, CURLOPT_TRANSFERTEXT, 0);
	qcurl_easy_setopt(clc.downloadCURL, CURLOPT_REFERER, va("ioQ3://%s",
		NET_AdrToString(clc.serverAddress)));
	qcurl_easy_setopt(clc.downloadCURL, CURLOPT_USERAGENT, va("%s %s",
		Q3_VERSION, qcurl_version()));
	qcurl_easy_setopt(clc.downloadCURL, CURLOPT_WRITEFUNCTION,
		CL_cURL_CallbackWrite);
	qcurl_easy_setopt(clc.downloadCURL, CURLOPT_WRITEDATA, &clc.download);
	qcurl_easy_setopt(clc.downloadCURL, CURLOPT_NOPROGRESS, 0);
	qcurl_easy_setopt(clc.downloadCURL, CURLOPT_PROGRESSFUNCTION,
		CL_cURL_CallbackProgress);
	qcurl_easy_setopt(clc.downloadCURL, CURLOPT_PROGRESSDATA, NULL);
	qcurl_easy_setopt(clc.downloadCURL, CURLOPT_FAILONERROR, 1);
	qcurl_easy_setopt(clc.downloadCURL, CURLOPT_FOLLOWLOCATION, 1);
	qcurl_easy_setopt(clc.downloadCURL, CURLOPT_MAXREDIRS, 5);
	clc.downloadCURLM = qcurl_multi_init();	
	if(!clc.downloadCURLM) {
		qcurl_easy_cleanup(clc.downloadCURL);
		clc.downloadCURL = NULL;
		Com_Error(ERR_DROP, "CL_cURL_BeginDownload: qcurl_multi_init() "
			"failed\n");
		return;
	}
	qcurl_multi_add_handle(clc.downloadCURLM, clc.downloadCURL);

	if(!(clc.sv_allowDownload & DLF_NO_DISCONNECT) &&
		!clc.cURLDisconnected) {

		CL_AddReliableCommand("disconnect");
		CL_WritePacket();
		CL_WritePacket();
		CL_WritePacket();
		clc.cURLDisconnected = qtrue;
	}
}

void CL_cURL_PerformDownload(void)
{
	CURLMcode res;
	CURLMsg *msg;
	int c;
	int i = 0;

	res = qcurl_multi_perform(clc.downloadCURLM, &c);
	while(res == CURLM_CALL_MULTI_PERFORM && i < 100) {
		res = qcurl_multi_perform(clc.downloadCURLM, &c);
		i++;
	}
	if(res == CURLM_CALL_MULTI_PERFORM)
		return;
	msg = qcurl_multi_info_read(clc.downloadCURLM, &c);
	if(msg == NULL) {
		return;
	}
	FS_FCloseFile(clc.download);
	if(msg->msg == CURLMSG_DONE && msg->data.result == CURLE_OK) {
		FS_SV_Rename(clc.downloadTempName, clc.downloadName);
		clc.downloadRestart = qtrue;
	}
	else {
		long code;

		qcurl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE,
			&code);	
		Com_Error(ERR_DROP, "Download Error: %s Code: %ld URL: %s",
			qcurl_easy_strerror(msg->data.result),
			code, clc.downloadURL);
	}
	*clc.downloadTempName = *clc.downloadName = 0;
	Cvar_Set( "cl_downloadName", "" );
	CL_NextDownload();
}
#endif /* USE_CURL */
