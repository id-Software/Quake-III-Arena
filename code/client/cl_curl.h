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


#ifndef __QCURL_H__
#define __QCURL_H__

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#ifdef USE_LOCAL_HEADERS
  #include "../libcurl/curl/curl.h"
#else
  #include <curl/curl.h>
#endif

#ifdef USE_CURL_DLOPEN
#ifdef WIN32
  #define DEFAULT_CURL_LIB "libcurl-4.dll"
  #define ALTERNATE_CURL_LIB "libcurl-3.dll"
#elif defined(MACOS_X)
  #define DEFAULT_CURL_LIB "libcurl.dylib"
#else
  #define DEFAULT_CURL_LIB "libcurl.so.4"
  #define ALTERNATE_CURL_LIB "libcurl.so.3"
#endif

extern cvar_t *cl_cURLLib;

extern char* (*qcurl_version)(void);

extern CURL* (*qcurl_easy_init)(void);
extern CURLcode (*qcurl_easy_setopt)(CURL *curl, CURLoption option, ...);
extern CURLcode (*qcurl_easy_perform)(CURL *curl);
extern void (*qcurl_easy_cleanup)(CURL *curl);
extern CURLcode (*qcurl_easy_getinfo)(CURL *curl, CURLINFO info, ...);
extern void (*qcurl_easy_reset)(CURL *curl);
extern const char *(*qcurl_easy_strerror)(CURLcode);

extern CURLM* (*qcurl_multi_init)(void);
extern CURLMcode (*qcurl_multi_add_handle)(CURLM *multi_handle,
						CURL *curl_handle);
extern CURLMcode (*qcurl_multi_remove_handle)(CURLM *multi_handle,
						CURL *curl_handle);
extern CURLMcode (*qcurl_multi_fdset)(CURLM *multi_handle,
						fd_set *read_fd_set,
						fd_set *write_fd_set,
						fd_set *exc_fd_set,
						int *max_fd);
extern CURLMcode (*qcurl_multi_perform)(CURLM *multi_handle,
						int *running_handles);
extern CURLMcode (*qcurl_multi_cleanup)(CURLM *multi_handle);
extern CURLMsg *(*qcurl_multi_info_read)(CURLM *multi_handle,
						int *msgs_in_queue);
extern const char *(*qcurl_multi_strerror)(CURLMcode);
#else
#define qcurl_version curl_version

#define qcurl_easy_init curl_easy_init
#define qcurl_easy_setopt curl_easy_setopt
#define qcurl_easy_perform curl_easy_perform
#define qcurl_easy_cleanup curl_easy_cleanup
#define qcurl_easy_getinfo curl_easy_getinfo
#define qcurl_easy_duphandle curl_easy_duphandle
#define qcurl_easy_reset curl_easy_reset
#define qcurl_easy_strerror curl_easy_strerror

#define qcurl_multi_init curl_multi_init
#define qcurl_multi_add_handle curl_multi_add_handle
#define qcurl_multi_remove_handle curl_multi_remove_handle
#define qcurl_multi_fdset curl_multi_fdset
#define qcurl_multi_perform curl_multi_perform
#define qcurl_multi_cleanup curl_multi_cleanup
#define qcurl_multi_info_read curl_multi_info_read
#define qcurl_multi_strerror curl_multi_strerror
#endif

qboolean CL_cURL_Init( void );
void CL_cURL_Shutdown( void );
void CL_cURL_BeginDownload( const char *localName, const char *remoteURL );
void CL_cURL_PerformDownload( void );
void CL_cURL_Cleanup( void );
#endif	// __QCURL_H__
