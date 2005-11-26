
/* Not all environments have a working getenv()/putenv() */

#if defined(macintosh) || defined(_WIN32_WCE)
#define NEED_SDL_GETENV
#endif

#ifdef NEED_SDL_GETENV

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Put a variable of the form "name=value" into the environment */
extern DECLSPEC int SDLCALL SDL_putenv(const char *variable);
#define putenv(X)   SDL_putenv(X)

/* Retrieve a variable named "name" from the environment */
extern DECLSPEC char * SDLCALL SDL_getenv(const char *name);
#define getenv(X)     SDL_getenv(X)

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* NEED_GETENV */
