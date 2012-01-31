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
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "io.h"
#include "pakstuff.h"
#include "unzip.h"
//#include "cmdlib.h"
#include "str.h"

int m_nPAKIndex;
FILE* pakfile[16];
struct PACKDirectory	pakdir;
PACKDirPtr				pakdirptr = &pakdir;
UInt16					dirsize;
boolean					pakopen = false;
int						f_type;
DIRECTORY				*paktextures = NULL;
boolean					HavePakColormap;
UInt32					PakColormapOffset;
UInt32					PakColormapSize;
DIRECTORY				*dirhead = NULL;
boolean g_bPK3 = false;
char g_strBasePath[1024];

struct PK3FileInfo
{
  unzFile m_zFile;
  char *m_pName;
  unz_s m_zInfo;
  long m_lSize;
  ~PK3FileInfo()
  {
    delete []m_pName;
  }
  bool operator ==(const PK3FileInfo& rhs) const { return strcmp(m_pName, rhs.m_pName) == 0; }
};

#define __PATHSEPERATOR   '/'

#define LOG_PAKFAIL

#ifdef LOG_PAKFAIL

class LogFile
{
public:
  FILE *m_pFile;
  LogFile(const char* pName)
  {
    m_pFile = fopen(pName, "w");
  }
  ~LogFile()
  {
    if (m_pFile)
    {
      fclose(m_pFile);
    }
  }
  void Log(const char *pFormat, ...)
  {
    va_list arg_ptr;
    va_start(arg_ptr, pFormat);
    fprintf(m_pFile, pFormat, arg_ptr);
    va_end(arg_ptr);
  }
};

LogFile g_LogFile("c:\\paklog.txt");
#endif

template <class T> class StrPtr : public Str
{
protected:
  T* m_pPtr;
  StrPtr()
  {
    m_pPtr = NULL;
  }

  StrPtr(const char *pStr, T *p) : Str(pStr)
  {
    m_pPtr = p;
  }

  T* Ptr()
  {
    return m_pPtr;
  }

  T& Ref()
  {
    return *m_pPtr;
  }
    

};
// PtrList
// a list of ptrs
// 
template <class T> class PtrList
{
protected:
  T *m_pPtr;
  PtrList *m_pNext;

public:

  PtrList()
  {
    m_pNext = NULL;
    m_pPtr = NULL;
  }

  PtrList(T *ip)
  {
    m_pNext = NULL;
    m_pPtr = ip;
  }

  ~PtrList()
  {
    delete m_pPtr;
  }

  PtrList* Next()
  {
    return m_pNext;
  }

  void Add(T *ip)
  {
    PtrList *pl = this;
    while (pl && pl->m_pNext)
    {
      pl = pl->Next();
    }
    pl->m_pNext = new PtrList(ip);
  }

  void Remove()
  {
    PtrList *p = m_pNext;
    if (p)
    {
      while (p->m_pNext != this && p->m_pNext != NULL)
      {
        p = p->m_pNext;
      }
      if (p->m_pNext == this)
      {
        p->m_pNext = m_pNext;
      }
    }
  }

  virtual PtrList* Find(T *ip)
  {
    PtrList *p = m_pNext;
    while (p)
    {
      if (*p->m_pPtr == *ip)
      {
        return p;
      }
      p = p->m_pNext;
    }
    return NULL;
  }

  // remove vp from the list
  void Remove(T *ip)
  {
    PtrList *p = Find(ip);
    if (p)
    {
      p->Remove();
    }
  }

  T* Ptr()
  {
    return m_pPtr;
  }

  T& Ref()
  {
    return *m_pPtr;
  }

  void RemoveAll()
  {
    PtrList *p = m_pNext;
    while (p)
    {
      PtrList *p2 = p;
      p = p->m_pNext;
      delete p2;
    }
  }
};


typedef PtrList<unzFile> ZFileList;
typedef PtrList<Str> StrList;
typedef PtrList<PK3FileInfo> PK3List;


StrList g_PK3TexturePaths;
PK3List g_PK3Files;
ZFileList g_zFiles;
#define WORK_LEN 1024
#define TEXTURE_PATH "textures"
#define PATH_SEPERATORS "/\\:\0"


char* __StrDup(char* pStr)
{ 
  if (pStr)
  {
    return strcpy(new char[strlen(pStr)+1], pStr); 
  }
  return NULL;
}

char* __StrDup(const char* pStr)
{ 
  if (pStr)
  {
    return strcpy(new char[strlen(pStr)+1], pStr); 
  }
  return NULL;
}

#define MEM_BLOCKSIZE 4096
void* __qblockmalloc(size_t nSize)
{
	void *b;
  // round up to threshold
  int nAllocSize = nSize % MEM_BLOCKSIZE;
  if ( nAllocSize > 0)
  {
    nSize += MEM_BLOCKSIZE - nAllocSize;
  }
	b = malloc(nSize + 1);
	memset (b, 0, nSize);
	return b;
}

void* __qmalloc (size_t nSize)
{
	void *b;
	b = malloc(nSize + 1);
	memset (b, 0, nSize);
	return b;
}


/*
====================
Extract file parts
====================
*/
void __ExtractFilePath (const char *path, char *dest)
{
	const char *src;

	src = path + strlen(path) - 1;

//
// back up until a \ or the start
//
	while (src != path && *(src-1) != __PATHSEPERATOR)
		src--;

	memcpy (dest, path, src-path);
	dest[src-path] = 0;
}

void __ExtractFileName (const char *path, char *dest)
{
	const char *src;

	src = path + strlen(path) - 1;

//
// back up until a \ or the start
//
	while (src != path && *(src-1) != '/' 
		 && *(src-1) != '\\' )
		src--;

	while (*src)
	{
		*dest++ = *src++;
	}
	*dest = 0;
}

void __ExtractFileBase (const char *path, char *dest)
{
	const char *src;

	src = path + strlen(path) - 1;

//
// back up until a \ or the start
//
	while (src != path && *(src-1) != '/' 
		 && *(src-1) != '\\' )
		src--;

	while (*src && *src != '.')
	{
		*dest++ = *src++;
	}
	*dest = 0;
}

void __ExtractFileExtension (const char *path, char *dest)
{
	const char *src;

	src = path + strlen(path) - 1;

//
// back up until a . or the start
//
	while (src != path && *(src-1) != '.')
		src--;
	if (src == path)
	{
		*dest = 0;	// no extension
		return;
	}

	strcpy (dest,src);
}


void __ConvertDOSToUnixName( char *dst, const char *src )
{
	while ( *src )
	{
		if ( *src == '\\' )
			*dst = '/';
		else
			*dst = *src;
		dst++; src++;
	}
	*dst = 0;
}





void AddSlash(Str& str)
{
  int nLen = str.GetLength();
  if (nLen > 0)
  {
    if (str[nLen-1] != '\\' && str[nLen-1] != '/')
      str += '\\';
  }
}

void FindReplace(Str& strContents, const char* pTag, const char* pValue)
{
  if (strcmp(pTag, pValue) == 0)
    return;
  for (int nPos = strContents.Find(pTag); nPos >= 0; nPos = strContents.Find(pTag))
  {
    int nRightLen = strContents.GetLength() - strlen(pTag) - nPos;
    Str strLeft(strContents.Left(nPos));
    Str strRight(strContents.Right(nRightLen));
    strLeft += pValue;
    strLeft += strRight;
    strContents = strLeft;
  }
}





void ProgError(char *errstr, ...)
{
  va_list args;

  va_start(args, errstr);
  printf("\nProgram Error: *** ");
  vprintf(errstr, args);
  printf(" ***\n");
  va_end(args);
  exit(5);
}

boolean ReadBytes(FILE *file, void *addr, UInt32 size)
{
  while (size > 0x8000)
    {
      if (fread(addr, 1, 0x8000, file) != 0x8000)
	return false;
      addr = (char *)addr + 0x8000;
      size -= 0x8000;
    }
  if (fread(addr, 1, size, file) != size)
    return false;
  return true;
}
int ReadMagic(FILE *file)
{
  UInt8 buf[4];

  if (ReadBytes(file, buf, 4) == FALSE)
    return FTYPE_ERROR;
  if (!strncmp(reinterpret_cast<const char*>(&buf[0]), "IWAD", 4))
    return FTYPE_IWAD;
  if (!strncmp(reinterpret_cast<const char*>(&buf[0]), "PWAD", 4))
    return FTYPE_PWAD;
  if (!strncmp(reinterpret_cast<const char*>(&buf[0]), "PACK", 4))
    return FTYPE_PACK;
  if (!strncmp(reinterpret_cast<const char*>(&buf[0]), "WAD2", 4))
    return FTYPE_WAD2;
  if (buf[0] == 0x17 && buf[1] == 0 && buf[2] == 0 && buf[3] == 0)
    return FTYPE_BSP;
  if (!strncmp(reinterpret_cast<const char*>(&buf[0]), "IDPO", 4))
    return FTYPE_MODEL;
  if (!strncmp(reinterpret_cast<const char*>(&buf[0]), "IDSP", 4))
    return FTYPE_SPRITE;
  if (!strncmp(reinterpret_cast<const char*>(&buf[0]), "RIFF", 4))
    return FTYPE_WAV;
  if (!strncmp(reinterpret_cast<const char*>(&buf[0]), ".snd", 4))
    return FTYPE_AU;
  if (buf[0] == 'P')
    {
      if (buf[1] == '1')
	return FTYPE_PBM_ASC;
      if (buf[1] == '2')
	return FTYPE_PGM_ASC;
      if (buf[1] == '3')
	return FTYPE_PPM_ASC;
      if (buf[1] == '4')
	return FTYPE_PBM_RAW;
      if (buf[1] == '5')
	return FTYPE_PGM_RAW;
      if (buf[1] == '6')
	return FTYPE_PPM_RAW;
    }
  if (buf[0] == 'B' && buf[1] == 'M')
    return FTYPE_BMP;
  if (!strncmp(reinterpret_cast<const char*>(&buf[0]), "GIF8", 4))
    return FTYPE_GIF;
  if (buf[0] == 0x0a && buf[1] == 0x05 && buf[2] == 0x01 && buf[3] == 0x08)
    return FTYPE_PCX;
  return FTYPE_UNKNOWN;
}
FILE *OpenFileReadMagic(const char *filename, int *ftype_r)
{
  FILE *f;

  *ftype_r = FTYPE_ERROR;
  if ((f = fopen(filename, "rb")) == NULL)
    return NULL;
  *ftype_r = ReadMagic(f);
  if (*ftype_r == FTYPE_ERROR)
    {
      fclose(f);
      return NULL;
    }
  return f;
}
boolean WriteBytes(FILE *file, void *addr, UInt32 size)
{
  while (size > 0x8000)
    {
      if (fwrite(addr, 1, 0x8000, file) != 0x8000)
	return FALSE;
      addr = (char *)addr + 0x8000;
      size -= 0x8000;
    }
  if (fwrite(addr, 1, size, file) != size)
    return FALSE;
  return TRUE;
}
char *ConvertFilePath(char *filename)
{
  char *cp;
  
  if (filename == NULL)
    ProgError("BUG: cannot convert a NULL pathname");
  for (cp = filename; *cp; cp++)
    if (*cp == '/' || *cp == '\\')
      {
#ifdef QEU_DOS
	*cp = '\\';
#else
	*cp = '/';
#endif
      }
  return filename;
}

/*
 * Read the PACK directory into memory.  The optional offset to the
 * start of the PACK file is given in "offset".  The number of files in
 * the directory is returned in *dirsize_r.
 */
PACKDirPtr ReadPACKDirectory(FILE *packfile, UInt32 offset, UInt16 *dirsize_r)
{
  PACKDirPtr dir;
  UInt32     pos, size;
  UInt16     max, i;

  *dirsize_r = 0;
  if (packfile == NULL)
    return NULL;
  if ((fseek(packfile, offset, SEEK_SET) < 0)
      || (ReadMagic(packfile) != FTYPE_PACK)
      || (ReadInt32(packfile, &pos) == FALSE)
      || (ReadInt32(packfile, &size) == FALSE)
      || (size == 0L)
      || (size / sizeof(struct PACKDirectory) > 65535L)
      || (fseek(packfile, offset + pos, SEEK_SET) < 0))
    return NULL;
  dir = (PACKDirPtr)__qmalloc(size);
  max = (UInt16)(size / sizeof(struct PACKDirectory));
  for (i = 0; i < max; i++)
    {
      if (ReadBytes(packfile, &dir[i], sizeof(struct PACKDirectory)) == FALSE)
	{
	  free(dir);
	  return NULL;
	}
      ConvertFilePath(dir[i].name);
      dir[i].offset = SwapInt32(dir[i].offset);
      dir[i].size = SwapInt32(dir[i].size);
    }
  *dirsize_r = max;
  return dir;
}

/*
 * Print the contents of the PACK directory in "outf".
 */
void DumpPACKDirectory(FILE *outf, PACKDirPtr dir, UInt16 dirsize)
{
	UInt16 i;
	UInt32 sum;
	char   buf[57];

	if (outf == NULL || dir == NULL || dirsize == 0)
		return;
	fprintf(outf, "num    offset     size    file name\n");
	fprintf(outf, "       (hex)      (dec)\n");
	sum = 0L;
	for (i = 0; i < dirsize; i++)
	{
		if(!strnicmp(dir[i].name, "textures", 8))
		{
   	   strncpy(buf, dir[i].name, 56);
	      buf[56] = '\0';
      	fprintf(outf, "%3u  0x%08lx  %6ld   %s\n",
		      i, dir[i].offset, dir[i].size, buf);
	      sum += dir[i].size;
		}
    }
	fprintf(outf, "\nTotal size for %3u entries:  %7lu bytes.\n", dirsize, sum);
	fprintf(outf, "Size of the PACK directory:  %7lu bytes.\n",
		(UInt32)dirsize * (UInt32)sizeof(struct PACKDirectory));
	fprintf(outf, "Total (header + data + dir): %7lu bytes.\n",
		12L + sum + (UInt32)dirsize * (UInt32)sizeof(struct PACKDirectory));
}

void ClearFileList(FILELIST **list)
{
	FILELIST	*temp;

	while(*list)
	{
		temp = *list;
		*list = (*list)->next;
		free(temp);
	}
}

void ClearDirList(DIRLIST **list)
{
	DIRLIST	*temp;

	while(*list)
	{
		temp = *list;
		*list = (*list)->next;
		free(temp);
	}
}

DIRECTORY *FindPakDir(DIRECTORY *dir, char *name)
{
	DIRECTORY	*currentPtr;

	for(currentPtr = dir; currentPtr; currentPtr = currentPtr->next)
	{
		if(!stricmp(name, currentPtr->name))
		{
			return currentPtr;
		}
	}
	return NULL;
}


// LoadPK3FileList
// ---------------
//
// This gets passed a file mask which we want to remove as 
// we are only interested in the directory name and any given
// extension. Only handles explicit filenames or *.something
//
boolean LoadPK3FileList(FILELIST **filelist, const char *pattern)
{
  char cSearch[WORK_LEN];
	__ConvertDOSToUnixName( cSearch, pattern );
  char cPath[WORK_LEN];
  char cExt[WORK_LEN];
  char cFile[WORK_LEN];
  char cWork[WORK_LEN];
  __ExtractFilePath(pattern, cPath);
  __ExtractFileName(pattern, cFile);
  __ExtractFileExtension(pattern, cExt);
  const char *pCompare = (strnicmp(cFile, "*.", 2) == 0) ? cExt : cFile;
  strcpy(cWork, cPath);
  sprintf(cPath, "textures/%s", cWork);

  PK3List *p = g_PK3Files.Next();
  while (p != NULL)
  {
    // qualify the path
    PK3FileInfo *pKey = p->Ptr();
    if (strstr(pKey->m_pName, cPath) && strstr(pKey->m_pName, pCompare))
    {
      __ExtractFileName(pKey->m_pName, cWork); 
      AddToFileListAlphabetized(filelist, cWork, 0, 0, false);
    }
    p = p->Next();
  }
  return (*filelist) != NULL;
}

boolean GetPackFileList(FILELIST **filelist, char *pattern)
{
	char					*str1, *str2;
	int						i;
	DIRECTORY				*dummy = paktextures;
	FILELIST				*temp;

	if (!pakopen)
		return false;

  if (g_bPK3)
  {
    return LoadPK3FileList(filelist, pattern);
  }

	str1 = pattern;

	for(i = 0; pattern[i] != '\0'; i++)
	{
		if(pattern[i] == '\\')
			pattern[i] = '/';
	}

	while(strchr(str1, '/'))
	{
		str2 = strchr(str1, '/');
		*str2++ = '\0';
		dummy = FindPakDir(dummy, str1);
		if(!dummy)
			return false;
		str1 = str2;
	}
	for(temp = dummy->files; temp; temp=temp->next)
	{
	  AddToFileListAlphabetized(filelist, temp->filename, temp->offset, 0, false);
	}
	return true;
}

boolean GetPackTextureDirs(DIRLIST **dirlist)
{
	UInt16					i;
	char					buf[57];

	if (!pakopen)
		return 1;

  if (g_bPK3)
  {
    StrList *pl = g_PK3TexturePaths.Next();
    while (pl != NULL)
    {
      AddToDirListAlphabetized(dirlist, pl->Ref(), 0);
      pl = pl->Next();
    }
    return true;
  }

	for (i = 0; i < dirsize; i++)
	{
		if(!strnicmp(pakdirptr[i].name, "textures", 8))
		{
			strncpy(buf, &(pakdirptr[i].name[9]), 46);
			if(strchr(buf, '\\'))
	      	*strchr(buf, '\\') = '\0';
			else if(strchr(buf, '/'))
	      	*strchr(buf, '/') = '\0';
			else
	      	buf[56] = '\0';

			if(strchr(buf, '.'))
				continue;

			AddToDirListAlphabetized(dirlist, buf, 0);
		}
	}
	return true;
}

boolean AddToDirListAlphabetized(DIRLIST **list, char *dirname, int from)
{
	DIRLIST	*currentPtr, *previousPtr, *newPtr;

	strlwr(dirname);
	for(currentPtr = *list; currentPtr; currentPtr = currentPtr->next)
	{
		if(!stricmp(dirname, currentPtr->dirname))
		{
			return false;
		}
	}
	previousPtr = NULL;
	currentPtr = *list;

	if((newPtr = (DIRLIST *)__qmalloc(sizeof(DIRLIST))) == NULL)
		return false;

	strcpy(newPtr->dirname, dirname);
	newPtr->from = from;

	while(currentPtr != NULL && stricmp(dirname, currentPtr->dirname) > 0)
	{
		previousPtr = currentPtr;
		currentPtr = currentPtr->next;
	} //End while
	if(previousPtr == NULL)
	{
		newPtr->next = *list;
		*list = newPtr;
	} //End if
	else
	{
		previousPtr->next = newPtr;
		newPtr->next = currentPtr;
	} //End else
	return true;
}

boolean AddToFileListAlphabetized(FILELIST **list, char *filename, UInt32 offset, UInt32 size, boolean dirs)
{
	FILELIST	*currentPtr, *previousPtr, *newPtr;

	for(currentPtr = *list; currentPtr; currentPtr = currentPtr->next)
	{
		if(!stricmp(filename, currentPtr->filename))
		{
			return false;
		}
	}
	previousPtr = NULL;
	currentPtr = *list;

	if((newPtr = (FILELIST *)__qmalloc(sizeof(FILELIST))) == NULL)
		return false;

	strcpy(newPtr->filename, filename);
	newPtr->offset = offset;
	newPtr->size = size;

	while(currentPtr != NULL && stricmp(filename, currentPtr->filename) > 0)
	{
		previousPtr = currentPtr;
		currentPtr = currentPtr->next;
	} //End while
	if(previousPtr == NULL)
	{
		newPtr->next = *list;
		*list = newPtr;
	} //End if
	else
	{
		previousPtr->next = newPtr;
		newPtr->next = currentPtr;
	} //End else
	return true;
}

boolean PakLoadFile(const char *filename, void **bufferptr)
{
	FILELIST	*p = NULL;
	DIRECTORY	*dummy;
	void		*buffer;
	char		*str1, *str2;

	if(!pakopen)
		return false;

  Str str(filename);
  __ConvertDOSToUnixName(str, str);

	dummy = paktextures;
	str1 = str;

	while(strchr(str1, '/'))
	{
		str2 = strchr(str1, '/');
		*str2++ = '\0';
		dummy = FindPakDir(dummy, str1);
		if(!dummy)
			return false;
		str1 = str2;
	}

  // FIXME: add error handling routines
	for(p = dummy->files; p; p = p->next)
	{
		if(!stricmp(str1, p->filename))
		{
			if (fseek(pakfile[m_nPAKIndex], p->offset, SEEK_SET) < 0)
			{
				//Sys_Printf("Unexpected EOF in pakfile\n");
				return false;
			}
			if((buffer = __qmalloc(p->size+5)) == NULL)
				//Error("Could not allocate memory");
	
			if(fread(buffer, 1, p->size, pakfile[m_nPAKIndex]) != p->size)
			{
				//Sys_Printf("Error reading %s from pak\n", str1);
				free(buffer);
				return false;
			}
			*bufferptr = buffer;
			return true;
		}
	}
	return false;
}

int PakLoadAnyFile(const char *filename, void **bufferptr)
{
  char cWork[WORK_LEN];
  if (g_bPK3)
  {
    PK3FileInfo *pInfo;
    Str strKey;
    // need to lookup the file without the base/texture path on it
    Str strBase(g_strBasePath);
    AddSlash(strBase);
    __ConvertDOSToUnixName(cWork, strBase);
    Str strFile(filename);
    __ConvertDOSToUnixName(strFile, strFile);
    strFile.MakeLower();
    strlwr(cWork);
    FindReplace(strFile, cWork, "");

    PK3FileInfo infoFind;
    infoFind.m_pName = __StrDup(strFile.GetBuffer());
    PK3List *pList = g_PK3Files.Find(&infoFind);
    if (pList)
    {
      pInfo = pList->Ptr();
      memcpy(pInfo->m_zFile, &pInfo->m_zInfo, sizeof(unz_s));
      if (unzOpenCurrentFile(pInfo->m_zFile) == UNZ_OK)
      {
        void *buffer = __qblockmalloc(pInfo->m_lSize+1);
        int n = unzReadCurrentFile(pInfo->m_zFile , buffer, pInfo->m_lSize);
        *bufferptr = buffer;
        unzCloseCurrentFile(pInfo->m_zFile);
        return n;
      }
    }
#ifdef LOG_PAKFAIL
    sprintf(cWork, "PAK failed on %s\n", filename);
    g_LogFile.Log(cWork);
#endif
    return -1;
  }

	for (int i = 0; i < dirsize; i++)
	{
		if(!stricmp(filename, pakdirptr[i].name))
		{
			if (fseek(pakfile[m_nPAKIndex], pakdirptr[i].offset, SEEK_SET) >= 0)
      {
	      void *buffer = __qmalloc (pakdirptr[i].size+1);
	      ((char *)buffer)[pakdirptr[i].size] = 0;
			  if (fread(buffer, 1, pakdirptr[i].size, pakfile[m_nPAKIndex]) == pakdirptr[i].size)
        {
          *bufferptr = buffer;
          return pakdirptr[i].size;
        }
      }
		}
	}
#ifdef LOG_PAKFAIL
    sprintf(cWork, "PAK failed on %s\n", filename);
    g_LogFile.Log(cWork);
#endif
  return -1;
}



DIRECTORY *AddPakDir(DIRECTORY **dir, char *name)
{
	DIRECTORY	*currentPtr, *previousPtr, *newPtr;

	for(currentPtr = *dir; currentPtr; currentPtr = currentPtr->next)
	{
		if(!stricmp(name, currentPtr->name))
		{
			return currentPtr;
		}
	}
	previousPtr = NULL;
	currentPtr = *dir;

	if((newPtr = (DIRECTORY *)__qmalloc(sizeof(DIRECTORY))) == NULL)
		return NULL;

	strcpy(newPtr->name, name);
	newPtr->files = NULL;

	while(currentPtr != NULL && stricmp(name, currentPtr->name) > 0)
	{
		previousPtr = currentPtr;
		currentPtr = currentPtr->next;
	}
	if(previousPtr == NULL)
	{
		newPtr->next = *dir;
		*dir = newPtr;
	}
	else
	{
		previousPtr->next = newPtr;
		newPtr->next = currentPtr;
	}
	return newPtr;
}


// OpenPK3
// -------
// Opens a PK3 ( or zip ) file and creates a list of filenames
// and zip info structures
// 
boolean OpenPK3(const char *filename)
{
  char cFilename[WORK_LEN];
  char cName[WORK_LEN];
  char cWork[WORK_LEN];
  unz_file_info zInfo;
  unzFile *zFile = new unzFile(unzOpen(filename));
  g_zFiles.Add(zFile);
  if (zFile != NULL)
  {
    int nStatus = unzGoToFirstFile(*zFile);
    while (nStatus == UNZ_OK)
    {
      cFilename[0] = '\0';
      unzGetCurrentFileInfo(*zFile, &zInfo, cFilename, WORK_LEN, NULL, 0, NULL, 0);
      strlwr(cFilename);
    	__ConvertDOSToUnixName( cWork, cFilename);
      if (strstr(cWork, ".") != NULL)
      {
        PK3FileInfo *pInfo = new PK3FileInfo();
        pInfo->m_pName = __StrDup(cWork);
        memcpy(&pInfo->m_zInfo, (unz_s*)*zFile, sizeof(unz_s));
        pInfo->m_lSize = zInfo.uncompressed_size;
        pInfo->m_zFile = *zFile;
        g_PK3Files.Add(pInfo);
      }
      char *p = strstr(cFilename, TEXTURE_PATH);
      if (p != NULL)
      {
        // FIXME: path differences per os ?
        // catch solo directory entry
        if (strlen(p) > strlen(TEXTURE_PATH) + 1)
        {
          // skip textures + path seperator
          p += strlen(TEXTURE_PATH) + 1;
          int nEnd = strcspn(p, PATH_SEPERATORS);
          strncpy(cName, p, nEnd);
          cName[nEnd] = '\0';

          boolean bFound = false;
          StrList *pl = g_PK3TexturePaths.Next();
          while (pl != NULL)
          {
            if (strcmpi(pl->Ref(), cName) == 0)
            {
              // already have this, continue
              bFound = true;
              break;
            }
            pl = pl->Next();
          }
          if (!bFound)
          {
            g_PK3TexturePaths.Add(new Str(cName));
          }
        }
      }
      nStatus = unzGoToNextFile(*zFile);
    }
  }
  return (zFile != NULL);
}

void closePK3(unzFile zf)
{
  unzClose(zf);
}

void OpenPakFile(const char *filename)
{
	int			i;
	char		*str1, *str2;
	DIRECTORY	*dummy;

	if(!pakopen)
		paktextures = NULL;

	HavePakColormap = false;

  Str strTest(filename);
  strTest.MakeLower();
  if (strTest.Find("pk3") >= 0 || strTest.Find("zip") >= 0)
  {
    pakopen = g_bPK3 = OpenPK3(filename);
    return;
  }


	if((pakfile[m_nPAKIndex] = OpenFileReadMagic(filename, &f_type)) == NULL)
	{
    //FIXME: error routine
		//Sys_Printf("ERROR: Could not open %s", filename);
		return;
	}
	if(f_type != FTYPE_PACK)
	{
		//Sys_Printf("ERROR: %s is not a valid pack file", filename);
		if(f_type != FTYPE_ERROR)
			fclose(pakfile[m_nPAKIndex]);
		return;
	}
	pakdirptr = ReadPACKDirectory(pakfile[m_nPAKIndex], 0, &dirsize);
	if (pakdirptr == NULL)
	{
		//Sys_Printf("ERROR: Could not read pack directory", filename);
		fclose(pakfile[m_nPAKIndex]);
		return;
	}
	if (dirsize == 0)
	{
		fclose(pakfile[m_nPAKIndex]);
		return;
	}
	for (i = 0; i < dirsize; i++)
	{
		if(!strnicmp("textures/", pakdirptr[i].name, 9))
		{
			dummy = paktextures;
			str1 = pakdirptr[i].name+9;
			while(strchr(str1, '/'))
			{
				str2 = strchr(str1, '/');
				*str2++ = '\0';
					dummy = AddPakDir(dummy==paktextures?&paktextures:&dummy, str1);
				str1 = str2;
			}

			AddToFileListAlphabetized(&(dummy->files), str1, pakdirptr[i].offset, pakdirptr[i].size, true);
		}
		else if(!strnicmp("pics/colormap.pcx", pakdirptr[i].name, 17))
		{
			HavePakColormap = true;
			PakColormapOffset = pakdirptr[i].offset;
			PakColormapSize = pakdirptr[i].size;
		}
	}
	pakopen = true;

}

void ClearPaKDir(DIRECTORY **dir)
{
	DIRECTORY	*d1 = *dir, *d2;

	while(d1)
	{
		ClearFileList(&(d1->files));
		d2 = d1;
		d1 = d1->next;
		free(d2);
	}
}

void CleanUpPakDirs()
{
  ClearPaKDir(&paktextures);
  paktextures = NULL;
  dirhead = NULL;
  g_PK3TexturePaths.RemoveAll();
  g_PK3Files.RemoveAll();
}

void ClosePakFile(void)
{
	if(pakopen)
  {
    if (g_bPK3)
    {
      ZFileList *p = g_zFiles.Next();
      while (p != NULL)
      {
        unzFile uz = p->Ref();
        closePK3(uz);
        p = p->Next();
      }
    }
    else
    {
      fclose(pakfile[m_nPAKIndex]);
    }
  }
	pakopen = false;
  CleanUpPakDirs();
}


void WINAPI InitPakFile(const char * pBasePath, const char *pName)
{
  m_nPAKIndex = 0;
  pakopen = false;
	paktextures = NULL;
  strcpy(g_strBasePath, pBasePath);
  if (pName == NULL)
  {
    char cWork[WORK_LEN];
	  Str strPath(pBasePath);
    AddSlash(strPath);
  	strPath += "*.pk3";
  	bool bGo = true;
	  struct _finddata_t fileinfo;
  	int handle = _findfirst (strPath, &fileinfo);
	  if (handle != -1)
  	{
	  	do
		  {
        sprintf(cWork, "%s\\%s", pBasePath, fileinfo.name);
        OpenPakFile(cWork);
		  } while (_findnext( handle, &fileinfo ) != -1);
	    _findclose (handle);
    }
	}
  else
  {
	  OpenPakFile(pName);
  }
}

