#ifndef __alu_h_
#define __alu_h_

#ifdef _WIN32
#define ALAPI       __declspec(dllexport)
#define ALAPIENTRY  __cdecl
#else  /* _WIN32 */

#ifdef TARGET_OS_MAC
#if TARGET_OS_MAC
#pragma export on
#endif /* TARGET_OS_MAC */
#endif /* TARGET_OS_MAC */

#define ALAPI
#define ALAPIENTRY
#define AL_CALLBACK
#endif /* _WIN32 */

#if defined(__MACH__) && defined(__APPLE__)
#include <OpenAL/al.h>
#include <OpenAL/alutypes.h>
#else
#include <AL/al.h>
#include <AL/alutypes.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef AL_NO_PROTOTYPES



#else





#endif /* AL_NO_PROTOTYPES */

#ifdef TARGET_OS_MAC
#if TARGET_OS_MAC
#pragma export off
#endif /* TARGET_OS_MAC */
#endif /* TARGET_OS_MAC */

#ifdef __cplusplus
}
#endif

#endif /* __alu_h_ */

