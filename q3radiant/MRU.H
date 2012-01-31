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
//*************************************************************
//  File name: mru.h
//
//  Description:
//
//      Header for MRU support
//
//  Development Team:
//
//      Gilles Vollant (100144.2636@compuserve.com) 
//
//*************************************************************

#ifndef __MRU_H__
#define __MRU_H__

#define NBMRUMENUSHOW   6       // Default number of MRU showed in the menu File
#define NBMRUMENU       9       // Default number of MRU stored
#define IDMRU           8000    // Default First ID of MRU
#ifdef  OFS_MAXPATHNAME
#define MAXSIZEMRUITEM  OFS_MAXPATHNAME
#else
#define MAXSIZEMRUITEM  128     // Default max size of an entry
#endif

typedef struct
{
WORD wNbItemFill;
WORD wNbLruShow;
WORD wNbLruMenu;
WORD wMaxSizeLruItem;
WORD wIdMru;
LPSTR lpMRU;
} MRUMENU;

typedef MRUMENU FAR * LPMRUMENU;

#ifdef __cplusplus
LPMRUMENU       CreateMruMenu  (WORD wNbLruShowInit=NBMRUMENUSHOW,
                                WORD wNbLruMenuInit=NBMRUMENU,
                                WORD wMaxSizeLruItemInit=MAXSIZEMRUITEM,
                                WORD wIdMruInit=IDMRU);
#else
LPMRUMENU       CreateMruMenu  (WORD wNbLruShowInit,
                                WORD wNbLruMenuInit,
                                WORD wMaxSizeLruItemInit,
                                WORD wIdMruInit);
#endif

LPMRUMENU       CreateMruMenuDefault();
void            DeleteMruMenu  (LPMRUMENU lpMruMenu);  

void            SetNbLruShow   (LPMRUMENU lpMruMenu,WORD wNbLruShowInit);
BOOL            SetMenuItem    (LPMRUMENU lpMruMenu,WORD wItem,
                                LPSTR lpItem);
BOOL            GetMenuItem    (LPMRUMENU lpMruMenu,WORD wItem,
                                BOOL fIDMBased,LPSTR lpItem,UINT uiSize);
BOOL            DelMenuItem    (LPMRUMENU lpMruMenu,WORD wItem,BOOL fIDMBased);
void            AddNewItem     (LPMRUMENU lpMruMenu,LPSTR lpItem);
void            PlaceMenuMRUItem(LPMRUMENU lpMruMenu,HMENU hMenu,UINT uiItem);

BOOL            SaveMruInIni   (LPMRUMENU lpMruMenu,LPSTR lpszSection,LPSTR lpszFile);
BOOL            LoadMruInIni   (LPMRUMENU lpMruMenu,LPSTR lpszSection,LPSTR lpszFile);
#ifdef WIN32
BOOL            SaveMruInReg   (LPMRUMENU lpMruMenu,LPSTR lpszKey);
BOOL            LoadMruInReg   (LPMRUMENU lpMruMenu,LPSTR lpszKey);

typedef enum 
{
WIN32S,
WINNT,
WIN95ORGREATHER
} WIN32KIND;
WIN32KIND GetWin32Kind();
#endif


//////////////////////////////////////////////////////////////
#endif
