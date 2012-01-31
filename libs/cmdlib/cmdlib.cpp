//
// start of shared cmdlib stuff
// 


#include "cmdlib.h"
#include "windows.h"

#define PATHSEPERATOR   '/'

// rad additions
// 11.29.99
PFN_ERR *g_pfnError = NULL;
PFN_PRINTF *g_pfnPrintf = NULL;
PFN_ERR_NUM *g_pfnErrorNum = NULL;
PFN_PRINTF_NUM *g_pfnPrintfNum = NULL;


void Error(const char *pFormat, ...)
{
  if (g_pfnError)
  {
    va_list arg_ptr;
    va_start(arg_ptr, pFormat);
    g_pfnError(pFormat, arg_ptr);
    va_end(arg_ptr);
  }
}

void Printf(const char *pFormat, ...)
{
  if (g_pfnPrintf)
  {
    va_list arg_ptr;
    va_start(arg_ptr, pFormat);
    g_pfnPrintf(pFormat, arg_ptr);
    va_end(arg_ptr);
  }
}

void ErrorNum(int nErr, const char *pFormat, ...)
{
  if (g_pfnErrorNum)
  {
    va_list arg_ptr;
    va_start(arg_ptr, pFormat);
    g_pfnErrorNum(nErr, pFormat, arg_ptr);
    va_end(arg_ptr);
  }
}

void PrintfNum(int nErr, const char *pFormat, ...)
{
  if (g_pfnPrintfNum)
  {
    va_list arg_ptr;
    va_start(arg_ptr, pFormat);
    g_pfnPrintfNum(nErr, pFormat, arg_ptr);
    va_end(arg_ptr);
  }
}



void SetErrorHandler(PFN_ERR pe)
{
  g_pfnError = pe;
}

void SetPrintfHandler(PFN_PRINTF pe)
{
  g_pfnPrintf = pe;
}

void SetErrorHandlerNum(PFN_ERR_NUM pe)
{
  g_pfnErrorNum = pe;
}

void SetPrintfHandler(PFN_PRINTF_NUM pe)
{
  g_pfnPrintfNum = pe;
}



// rad end

#define MEM_BLOCKSIZE 4096
void* qblockmalloc(size_t nSize)
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

void* qmalloc (size_t nSize)
{
	void *b;
	b = malloc(nSize + 1);
	memset (b, 0, nSize);
	return b;
}

/*
================
Q_filelength
================
*/
int Q_filelength (FILE *f)
{
	int		pos;
	int		end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}


// FIXME: need error handler
FILE *SafeOpenWrite (const char *filename)
{
	FILE	*f;

	f = fopen(filename, "wb");

	if (!f)
  {
		Error ("Error opening %s: %s",filename,strerror(errno));
  }

	return f;
}

FILE *SafeOpenRead (const char *filename)
{
	FILE	*f;

	f = fopen(filename, "rb");

	if (!f)
  {
		Error ("Error opening %s: %s",filename,strerror(errno));
  }

	return f;
}


void SafeRead (FILE *f, void *buffer, int count)
{
	if ( (int)fread (buffer, 1, count, f) != count)
		Error ("File read failure");
}


void SafeWrite (FILE *f, const void *buffer, int count)
{
	if ( (int)fwrite (buffer, 1, count, f) != count)
		Error ("File read failure");
}



/*
==============
LoadFile
==============
*/
int LoadFile (const char *filename, void **bufferptr)
{
	FILE	*f;
	int    length;
	void    *buffer;

  *bufferptr = NULL;
  
  if (filename == NULL || strlen(filename) == 0)
  {
    return -1;
  }

	f = fopen (filename, "rb");
	if (!f)
	{
		return -1;
	}
	length = Q_filelength (f);
	buffer = qblockmalloc (length+1);
	((char *)buffer)[length] = 0;
	SafeRead (f, buffer, length);
	fclose (f);

	*bufferptr = buffer;
	return length;
}


/*
==============
LoadFileNoCrash

returns -1 length if not present
==============
*/
int    LoadFileNoCrash (const char *filename, void **bufferptr)
{
	FILE	*f;
	int    length;
	void    *buffer;

	f = fopen (filename, "rb");
	if (!f)
		return -1;
	length = Q_filelength (f);
	buffer = qmalloc (length+1);
	((char *)buffer)[length] = 0;
	SafeRead (f, buffer, length);
	fclose (f);

	*bufferptr = buffer;
	return length;
}


/*
==============
SaveFile
==============
*/
void    SaveFile (const char *filename, void *buffer, int count)
{
	FILE	*f;

	f = SafeOpenWrite (filename);
	SafeWrite (f, buffer, count);
	fclose (f);
}



void DefaultExtension (char *path, char *extension)
{
	char    *src;
//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (*src != PATHSEPERATOR && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	strcat (path, extension);
}


void DefaultPath (char *path, char *basepath)
{
	char    temp[128];

	if (path[0] == PATHSEPERATOR)
		return;                   // absolute path location
	strcpy (temp,path);
	strcpy (path,basepath);
	strcat (path,temp);
}


void    StripFilename (char *path)
{
	int             length;

	length = strlen(path)-1;
	while (length > 0 && path[length] != PATHSEPERATOR)
		length--;
	path[length] = 0;
}

void    StripExtension (char *path)
{
	int             length;

	length = strlen(path)-1;
	while (length > 0 && path[length] != '.')
	{
		length--;
		if (path[length] == '/')
			return;		// no extension
	}
	if (length)
		path[length] = 0;
}


/*
====================
Extract file parts
====================
*/
void ExtractFilePath (const char *path, char *dest)
{
	const char *src;

	src = path + strlen(path) - 1;

//
// back up until a \ or the start
//
	while (src != path && *(src-1) != PATHSEPERATOR)
		src--;

	memcpy (dest, path, src-path);
	dest[src-path] = 0;
}

void ExtractFileName (const char *path, char *dest)
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

void ExtractFileBase (const char *path, char *dest)
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

void ExtractFileExtension (const char *path, char *dest)
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


void ConvertDOSToUnixName( char *dst, const char *src )
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


char* StrDup(char* pStr)
{ 
  if (pStr)
  {
    return strcpy(new char[strlen(pStr)+1], pStr); 
  }
  return NULL;
}

char* StrDup(const char* pStr)
{ 
  if (pStr)
  {
    return strcpy(new char[strlen(pStr)+1], pStr); 
  }
  return NULL;
}


/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

#ifdef _SGI_SOURCE
#define	__BIG_ENDIAN__
#endif

#ifdef __BIG_ENDIAN__

short   LittleShort (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short   BigShort (short l)
{
	return l;
}


int    LittleLong (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int    BigLong (int l)
{
	return l;
}


float	LittleFloat (float l)
{
	union {byte b[4]; float f;} in, out;
	
	in.f = l;
	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];
	
	return out.f;
}

float	BigFloat (float l)
{
	return l;
}


#else


short   BigShort (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short   LittleShort (short l)
{
	return l;
}


int    BigLong (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int    LittleLong (int l)
{
	return l;
}

float	BigFloat (float l)
{
	union {byte b[4]; float f;} in, out;
	
	in.f = l;
	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];
	
	return out.f;
}

float	LittleFloat (float l)
{
	return l;
}



#endif

