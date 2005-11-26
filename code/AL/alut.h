#ifndef _ALUT_H_
#define _ALUT_H_

#include "altypes.h"
#include "aluttypes.h"

#ifdef _WIN32
#define ALAPI         __declspec(dllexport)
#define ALAPIENTRY    __cdecl
#define AL_CALLBACK
#else  /* _WIN32 */

#ifdef TARGET_OS_MAC
#if TARGET_OS_MAC
#pragma export on
#endif /* TARGET_OS_MAC */
#endif /* TARGET_OS_MAC */

#ifndef ALAPI
#define ALAPI
#endif

#ifndef ALAPIENTRY
#define ALAPIENTRY
#endif

#ifndef AL_CALLBACK
#define AL_CALLBACK
#endif 

#endif /* _WIN32 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef AL_NO_PROTOTYPES

ALAPI void ALAPIENTRY alutInit(ALint *argc, ALbyte **argv);
ALAPI void ALAPIENTRY alutExit(ALvoid);

ALAPI ALboolean ALAPIENTRY alutLoadWAV( const char *fname,
                        ALvoid **wave,
			ALsizei *format,
			ALsizei *size,
			ALsizei *bits,
			ALsizei *freq );

ALAPI void ALAPIENTRY alutLoadWAVFile(ALbyte *file,
				      ALenum *format,
				      ALvoid **data,
				      ALsizei *size,
				      ALsizei *freq,
				      ALboolean *loop);
ALAPI void ALAPIENTRY alutLoadWAVMemory(ALbyte *memory,
					ALenum *format,
					ALvoid **data,
					ALsizei *size,
					ALsizei *freq,
					ALboolean *loop);
ALAPI void ALAPIENTRY alutUnloadWAV(ALenum format,
				    ALvoid *data,
				    ALsizei size,
				    ALsizei freq);

#else
      void 	(*alutInit)(int *argc, char *argv[]);
      void 	(*alutExit)(ALvoid);

      ALboolean 	(*alutLoadWAV)( const char *fname,
                        ALvoid **wave,
			ALsizei *format,
			ALsizei *size,
			ALsizei *bits,
			ALsizei *freq );

      void (*alutLoadWAVFile(ALbyte *file,ALenum *format,ALvoid **data,ALsizei *size,ALsizei *freq,ALboolean *loop);
      void (*alutLoadWAVMemory)(ALbyte *memory,ALenum *format,ALvoid **data,ALsizei *size,ALsizei *freq,ALboolean *loop);
      void (*alutUnloadWAV)(ALenum format,ALvoid *data,ALsizei size,ALsizei freq);


#endif /* AL_NO_PROTOTYPES */

#ifdef TARGET_OS_MAC
#if TARGET_OS_MAC
#pragma export off
#endif /* TARGET_OS_MAC */
#endif /* TARGET_OS_MAC */

#ifdef __cplusplus
}
#endif

#endif
