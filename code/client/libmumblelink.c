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

typedef struct
{
	uint32_t uiVersion;
	uint32_t uiTick;
	float   fPosition[3];
	float   fFront[3];
	float   fTop[3];
	wchar_t name[256];
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
	}
	close(shmfd);
#endif
	mbstowcs(lm->name, name, sizeof(lm->name));

	return 0;
}

void mumble_update_coordinates(float fPosition[3], float fFront[3], float fTop[3])
{
	if (!lm)
		return;

	memcpy(lm->fPosition, fPosition, sizeof(fPosition));
	memcpy(lm->fFront, fFront, sizeof(fFront));
	memcpy(lm->fTop, fTop, sizeof(fTop));
	lm->uiVersion = 1;
	lm->uiTick = GetTickCount();
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
