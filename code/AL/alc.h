#ifndef ALC_CONTEXT_H_
#define ALC_CONTEXT_H_

#include "altypes.h"
#include "alctypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ALC_VERSION_0_1         1

#ifdef _WIN32
 #ifdef _OPENAL32LIB
  #define ALCAPI __declspec(dllexport)
 #else
  #define ALCAPI __declspec(dllimport)
 #endif

 typedef struct ALCdevice_struct ALCdevice;
 typedef struct ALCcontext_struct ALCcontext;

 #define ALCAPIENTRY __cdecl
#else
 #ifdef TARGET_OS_MAC
  #if TARGET_OS_MAC
   #pragma export on
  #endif
 #endif

 #define ALCAPI
 #define ALCAPIENTRY
#endif

#ifndef AL_NO_PROTOTYPES

ALCAPI ALCcontext * ALCAPIENTRY alcCreateContext( ALCdevice *dev,
						ALint* attrlist );

/**
 * There is no current context, as we can mix
 *  several active contexts. But al* calls
 *  only affect the current context.
 */
ALCAPI ALCenum ALCAPIENTRY alcMakeContextCurrent( ALCcontext *alcHandle );

/**
 * Perform processing on a synced context, non-op on a asynchronous
 * context.
 */
ALCAPI ALCcontext * ALCAPIENTRY alcProcessContext( ALCcontext *alcHandle );

/**
 * Suspend processing on an asynchronous context, non-op on a
 * synced context.
 */
ALCAPI void ALCAPIENTRY alcSuspendContext( ALCcontext *alcHandle );

ALCAPI ALCenum ALCAPIENTRY alcDestroyContext( ALCcontext *alcHandle );

ALCAPI ALCenum ALCAPIENTRY alcGetError( ALCdevice *dev );

ALCAPI ALCcontext * ALCAPIENTRY alcGetCurrentContext( ALvoid );

ALCAPI ALCdevice *alcOpenDevice( const ALubyte *tokstr );
ALCAPI void alcCloseDevice( ALCdevice *dev );

ALCAPI ALboolean ALCAPIENTRY alcIsExtensionPresent(ALCdevice *device, ALubyte *extName);
ALCAPI ALvoid  * ALCAPIENTRY alcGetProcAddress(ALCdevice *device, ALubyte *funcName);
ALCAPI ALenum    ALCAPIENTRY alcGetEnumValue(ALCdevice *device, ALubyte *enumName);

ALCAPI ALCdevice* ALCAPIENTRY alcGetContextsDevice(ALCcontext *context);


/**
 * Query functions
 */
const ALubyte * alcGetString( ALCdevice *deviceHandle, ALenum token );
void alcGetIntegerv( ALCdevice *deviceHandle, ALenum  token , ALsizei  size , ALint *dest );

#else
      ALCcontext *   (*alcCreateContext)( ALCdevice *dev, ALint* attrlist );
      ALCenum	     (*alcMakeContextCurrent)( ALCcontext *alcHandle );
      ALCcontext *   (*alcProcessContext)( ALCcontext *alcHandle );
      void           (*alcSuspendContext)( ALCcontext *alcHandle );
      ALCenum	     (*alcDestroyContext)( ALCcontext *alcHandle );
      ALCenum	     (*alcGetError)( ALCdevice *dev );
      ALCcontext *   (*alcGetCurrentContext)( ALvoid );
      ALCdevice *    (*alcOpenDevice)( const ALubyte *tokstr );
      void           (*alcCloseDevice)( ALCdevice *dev );
      ALboolean      (*alcIsExtensionPresent)( ALCdevice *device, ALubyte *extName );
      ALvoid  *      (*alcGetProcAddress)(ALCdevice *device, ALubyte *funcName );
      ALenum         (*alcGetEnumValue)(ALCdevice *device, ALubyte *enumName);
      ALCdevice*     (*alcGetContextsDevice)(ALCcontext *context);
      const ALubyte* (*alcGetString)( ALCdevice *deviceHandle, ALenum token );
      void           (*alcGetIntegerv)( ALCdevice *deviceHandle, ALenum  token , ALsizei  size , ALint *dest );

#endif /* AL_NO_PROTOTYPES */

#ifdef TARGET_OS_MAC
#if TARGET_OS_MAC
#pragma export off
#endif /* TARGET_OS_MAC */
#endif /* TARGET_OS_MAC */

#ifdef __cplusplus
}
#endif

#endif /* ALC_CONTEXT_H_ */
