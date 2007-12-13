#ifndef ALC_CONTEXT_H_
#define ALC_CONTEXT_H_

#include "altypes.h"
#include "alctypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ALC_VERSION_0_1         1

#ifdef _WIN32
 typedef struct ALCdevice_struct ALCdevice;
 typedef struct ALCcontext_struct ALCcontext;
 #ifndef _XBOX
  #ifdef _OPENAL32LIB
   #define ALCAPI __declspec(dllexport)
  #else
   #define ALCAPI __declspec(dllimport)
  #endif
  #define ALCAPIENTRY __cdecl
 #endif
#endif

#ifdef TARGET_OS_MAC
 #if TARGET_OS_MAC
  #pragma export on
 #endif
#endif

#ifndef ALCAPI
 #define ALCAPI
#endif

#ifndef ALCAPIENTRY
 #define ALCAPIENTRY
#endif


#ifndef ALC_NO_PROTOTYPES

/*
 * Context Management
 */
ALCAPI ALCcontext *    ALCAPIENTRY alcCreateContext( ALCdevice *device, const ALCint* attrlist );

ALCAPI ALCboolean      ALCAPIENTRY alcMakeContextCurrent( ALCcontext *context );

ALCAPI void            ALCAPIENTRY alcProcessContext( ALCcontext *context );

ALCAPI void            ALCAPIENTRY alcSuspendContext( ALCcontext *context );

ALCAPI void            ALCAPIENTRY alcDestroyContext( ALCcontext *context );

ALCAPI ALCcontext *    ALCAPIENTRY alcGetCurrentContext( ALCvoid );

ALCAPI ALCdevice*      ALCAPIENTRY alcGetContextsDevice( ALCcontext *context );


/*
 * Device Management
 */
ALCAPI ALCdevice *     ALCAPIENTRY alcOpenDevice( const ALchar *devicename );

ALCAPI ALCboolean      ALCAPIENTRY alcCloseDevice( ALCdevice *device );


/*
 * Error support.
 * Obtain the most recent Context error
 */
ALCAPI ALCenum         ALCAPIENTRY alcGetError( ALCdevice *device );


/* 
 * Extension support.
 * Query for the presence of an extension, and obtain any appropriate
 * function pointers and enum values.
 */
ALCAPI ALCboolean      ALCAPIENTRY alcIsExtensionPresent( ALCdevice *device, const ALCchar *extname );

ALCAPI void  *         ALCAPIENTRY alcGetProcAddress( ALCdevice *device, const ALCchar *funcname );

ALCAPI ALCenum         ALCAPIENTRY alcGetEnumValue( ALCdevice *device, const ALCchar *enumname );


/*
 * Query functions
 */
ALCAPI const ALCchar * ALCAPIENTRY alcGetString( ALCdevice *device, ALCenum param );

ALCAPI void            ALCAPIENTRY alcGetIntegerv( ALCdevice *device, ALCenum param, ALCsizei size, ALCint *data );


/*
 * Capture functions
 */
ALCAPI ALCdevice*      ALCAPIENTRY alcCaptureOpenDevice( const ALCchar *devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize );

ALCAPI ALCboolean      ALCAPIENTRY alcCaptureCloseDevice( ALCdevice *device );

ALCAPI void            ALCAPIENTRY alcCaptureStart( ALCdevice *device );

ALCAPI void            ALCAPIENTRY alcCaptureStop( ALCdevice *device );

ALCAPI void            ALCAPIENTRY alcCaptureSamples( ALCdevice *device, ALCvoid *buffer, ALCsizei samples );

#else /* ALC_NO_PROTOTYPES */
/*
ALCAPI ALCcontext *    (ALCAPIENTRY *alcCreateContext)( ALCdevice *device, const ALCint* attrlist );
ALCAPI ALCboolean      (ALCAPIENTRY *alcMakeContextCurrent)( ALCcontext *context );
ALCAPI void            (ALCAPIENTRY *alcProcessContext)( ALCcontext *context );
ALCAPI void            (ALCAPIENTRY *alcSuspendContext)( ALCcontext *context );
ALCAPI void	           (ALCAPIENTRY *alcDestroyContext)( ALCcontext *context );
ALCAPI ALCcontext *    (ALCAPIENTRY *alcGetCurrentContext)( ALCvoid );
ALCAPI ALCdevice *     (ALCAPIENTRY *alcGetContextsDevice)( ALCcontext *context );
ALCAPI ALCdevice *     (ALCAPIENTRY *alcOpenDevice)( const ALCchar *devicename );
ALCAPI ALCboolean      (ALCAPIENTRY *alcCloseDevice)( ALCdevice *device );
ALCAPI ALCenum	       (ALCAPIENTRY *alcGetError)( ALCdevice *device );
ALCAPI ALCboolean      (ALCAPIENTRY *alcIsExtensionPresent)( ALCdevice *device, const ALCchar *extname );
ALCAPI void *          (ALCAPIENTRY *alcGetProcAddress)( ALCdevice *device, const ALCchar *funcname );
ALCAPI ALCenum         (ALCAPIENTRY *alcGetEnumValue)( ALCdevice *device, const ALCchar *enumname );
ALCAPI const ALCchar*  (ALCAPIENTRY *alcGetString)( ALCdevice *device, ALCenum param );
ALCAPI void            (ALCAPIENTRY *alcGetIntegerv)( ALCdevice *device, ALCenum param, ALCsizei size, ALCint *dest );
ALCAPI ALCdevice *     (ALCAPIENTRY *alcCaptureOpenDevice)( const ALCchar *devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize );
ALCAPI ALCboolean      (ALCAPIENTRY *alcCaptureCloseDevice)( ALCdevice *device );
ALCAPI void            (ALCAPIENTRY *alcCaptureStart)( ALCdevice *device );
ALCAPI void            (ALCAPIENTRY *alcCaptureStop)( ALCdevice *device );
ALCAPI void            (ALCAPIENTRY *alcCaptureSamples)( ALCdevice *device, ALCvoid *buffer, ALCsizei samples );
*/
/* Type definitions */
typedef ALCcontext *   (ALCAPIENTRY *LPALCCREATECONTEXT) (ALCdevice *device, const ALCint *attrlist);
typedef ALCboolean     (ALCAPIENTRY *LPALCMAKECONTEXTCURRENT)( ALCcontext *context );
typedef void           (ALCAPIENTRY *LPALCPROCESSCONTEXT)( ALCcontext *context );
typedef void           (ALCAPIENTRY *LPALCSUSPENDCONTEXT)( ALCcontext *context );
typedef void	       (ALCAPIENTRY *LPALCDESTROYCONTEXT)( ALCcontext *context );
typedef ALCcontext *   (ALCAPIENTRY *LPALCGETCURRENTCONTEXT)( ALCvoid );
typedef ALCdevice *    (ALCAPIENTRY *LPALCGETCONTEXTSDEVICE)( ALCcontext *context );
typedef ALCdevice *    (ALCAPIENTRY *LPALCOPENDEVICE)( const ALCchar *devicename );
typedef ALCboolean     (ALCAPIENTRY *LPALCCLOSEDEVICE)( ALCdevice *device );
typedef ALCenum	       (ALCAPIENTRY *LPALCGETERROR)( ALCdevice *device );
typedef ALCboolean     (ALCAPIENTRY *LPALCISEXTENSIONPRESENT)( ALCdevice *device, const ALCchar *extname );
typedef void *         (ALCAPIENTRY *LPALCGETPROCADDRESS)(ALCdevice *device, const ALCchar *funcname );
typedef ALCenum        (ALCAPIENTRY *LPALCGETENUMVALUE)(ALCdevice *device, const ALCchar *enumname );
typedef const ALCchar* (ALCAPIENTRY *LPALCGETSTRING)( ALCdevice *device, ALCenum param );
typedef void           (ALCAPIENTRY *LPALCGETINTEGERV)( ALCdevice *device, ALCenum param, ALCsizei size, ALCint *dest );
typedef ALCdevice *    (ALCAPIENTRY *LPALCCAPTUREOPENDEVICE)( const ALCchar *devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize );
typedef ALCboolean     (ALCAPIENTRY *LPALCCAPTURECLOSEDEVICE)( ALCdevice *device );
typedef void           (ALCAPIENTRY *LPALCCAPTURESTART)( ALCdevice *device );
typedef void           (ALCAPIENTRY *LPALCCAPTURESTOP)( ALCdevice *device );
typedef void           (ALCAPIENTRY *LPALCCAPTURESAMPLES)( ALCdevice *device, ALCvoid *buffer, ALCsizei samples );

#endif /* ALC_NO_PROTOTYPES */

#ifdef TARGET_OS_MAC
#if TARGET_OS_MAC
#pragma export off
#endif /* TARGET_OS_MAC */
#endif /* TARGET_OS_MAC */

#ifdef __cplusplus
}
#endif

#endif /* ALC_CONTEXT_H_ */
