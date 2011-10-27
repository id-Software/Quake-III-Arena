/* libmumblelink.c -- mumble link interface

  Copyright (C) 2008 Ludwig Nussel <ludwig.nussel@suse.de>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

*/

#ifdef WIN32
#include <windows.h>
#define uint32_t UINT32
#else
#include <unistd.h>
#ifdef __sun
#define _POSIX_C_SOURCE 199309L
#endif
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#endif

#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libmumblelink.h"

#ifndef MIN
#define MIN(a, b) ((a)<(b)?(a):(b))
#endif

typedef struct
{
	uint32_t uiVersion;
	uint32_t uiTick;
	float   fAvatarPosition[3];
	float   fAvatarFront[3];
	float   fAvatarTop[3];
	wchar_t name[256];
	/* new in mumble 1.2 */
	float   fCameraPosition[3];
	float   fCameraFront[3];
	float   fCameraTop[3];
	wchar_t identity[256];
	uint32_t context_len;
	unsigned char context[256];
	wchar_t description[2048];
} LinkedMem;

static LinkedMem *lm = NULL;

#ifdef WIN32
static HANDLE hMapObject = NULL;
#else
static int32_t GetTickCount(void)
{
	struct timeval tv;
	gettimeofday(&tv,NULL);

	return tv.tv_usec / 1000 + tv.tv_sec * 1000;
}
#endif

int mumble_link(const char* name)
{
#ifdef WIN32
	if(lm)
		return 0;

	hMapObject = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"MumbleLink");
	if (hMapObject == NULL)
		return -1;

	lm = (LinkedMem *) MapViewOfFile(hMapObject, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(LinkedMem));
	if (lm == NULL) {
		CloseHandle(hMapObject);
		hMapObject = NULL;
		return -1;
	}
#else
	char file[256];
	int shmfd;
	if(lm)
		return 0;

	snprintf(file, sizeof (file), "/MumbleLink.%d", getuid());
	shmfd = shm_open(file, O_RDWR, S_IRUSR | S_IWUSR);
	if(shmfd < 0) {
		return -1;
	}

	lm = (LinkedMem *) (mmap(NULL, sizeof(LinkedMem), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd,0));
	if (lm == (void *) (-1)) {
		lm = NULL;
		close(shmfd);
		return -1;
	}
	close(shmfd);
#endif
	memset(lm, 0, sizeof(LinkedMem));
	mbstowcs(lm->name, name, sizeof(lm->name) / sizeof(wchar_t));

	return 0;
}

void mumble_update_coordinates(float fPosition[3], float fFront[3], float fTop[3])
{
	mumble_update_coordinates2(fPosition, fFront, fTop, fPosition, fFront, fTop);
}

void mumble_update_coordinates2(float fAvatarPosition[3], float fAvatarFront[3], float fAvatarTop[3],
		float fCameraPosition[3], float fCameraFront[3], float fCameraTop[3])
{
	if (!lm)
		return;

	memcpy(lm->fAvatarPosition, fAvatarPosition, sizeof(lm->fAvatarPosition));
	memcpy(lm->fAvatarFront, fAvatarFront, sizeof(lm->fAvatarFront));
	memcpy(lm->fAvatarTop, fAvatarTop, sizeof(lm->fAvatarTop));
	memcpy(lm->fCameraPosition, fCameraPosition, sizeof(lm->fCameraPosition));
	memcpy(lm->fCameraFront, fCameraFront, sizeof(lm->fCameraFront));
	memcpy(lm->fCameraTop, fCameraTop, sizeof(lm->fCameraTop));
	lm->uiVersion = 2;
	lm->uiTick = GetTickCount();
}

void mumble_set_identity(const char* identity)
{
	size_t len;
	if (!lm)
		return;
	len = MIN(sizeof(lm->identity), strlen(identity)+1);
	mbstowcs(lm->identity, identity, len);
}

void mumble_set_context(const unsigned char* context, size_t len)
{
	if (!lm)
		return;
	len = MIN(sizeof(lm->context), len);
	lm->context_len = len;
	memcpy(lm->context, context, len);
}

void mumble_set_description(const char* description)
{
	size_t len;
	if (!lm)
		return;
	len = MIN(sizeof(lm->description), strlen(description)+1);
	mbstowcs(lm->description, description, len);
}

void mumble_unlink()
{
	if(!lm)
		return;
#ifdef WIN32
	UnmapViewOfFile(lm);
	CloseHandle(hMapObject);
	hMapObject = NULL;
#else
	munmap(lm, sizeof(LinkedMem));
#endif
	lm = NULL;
}

int mumble_islinked(void)
{
	return lm != NULL;
}
