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
//  File name: mru.c
//
//  Description:
//
//      Routines for MRU support
//
//  Development Team:
//
//      Gilles Vollant (100144.2636@compuserve.com) 
//
//*************************************************************

#include "stdafx.h"
#include <windows.h>
#include <windowsx.h>
#include <string.h>

#include "mru.h"
// CreateMruMenu  : MRUMENU constructor
// wNbLruShowInit : nb of item showed in menu
// wNbLruMenuInit : nb of item stored in memory
// wMaxSizeLruItemInit : size max. of filename


//*************************************************************
//
//  CreateMruMenu()
//
//  Purpose:
//
//              Allocate and Initialize an MRU and return a pointer on it
//
//
//  Parameters:
//
//      WORD wNbLruShowInit -      Maximum number of item displayed on menu
//      WORD wNbLruMenuInit -      Maximum number of item stored in memory
//      WORD wMaxSizeLruItemInit - Maximum size of an item (ie size of pathname)
//      WORD wIdMruInit -          ID of the first item in the menu (default:IDMRU)
//
//
//  Return: (LPMRUMENU)
//
//      Pointer on a MRUMENU structure, used by other function
//
//
//  Comments:
//      wNbLruShowInit <= wNbLruMenuInit
//
//
//  History:    Date       Author       Comment
//              09/24/94   G. Vollant   Created
//
//*************************************************************

LPMRUMENU CreateMruMenu (WORD wNbLruShowInit,
            WORD wNbLruMenuInit,WORD wMaxSizeLruItemInit,WORD wIdMruInit)
{
LPMRUMENU lpMruMenu;
  lpMruMenu = (LPMRUMENU)GlobalAllocPtr(GHND,sizeof(MRUMENU));

  lpMruMenu->wNbItemFill = 0;                   
  lpMruMenu->wNbLruMenu = wNbLruMenuInit;             
  lpMruMenu->wNbLruShow = wNbLruShowInit;
  lpMruMenu->wIdMru = wIdMruInit;
  lpMruMenu->wMaxSizeLruItem = wMaxSizeLruItemInit;
  lpMruMenu->lpMRU = (LPSTR)GlobalAllocPtr(GHND,
                      lpMruMenu->wNbLruMenu*(UINT)lpMruMenu->wMaxSizeLruItem);
  if (lpMruMenu->lpMRU == NULL)
     {
       GlobalFreePtr(lpMruMenu);
       lpMruMenu =  NULL;
     }
  return lpMruMenu;
}

//*************************************************************
//
//  CreateMruMenuDefault()
//
//  Purpose:
//
//              Allocate and Initialize an MRU and return a pointer on it
//              Use default parameter
//
//
//  Parameters:
//
//
//  Return: (LPMRUMENU)
//
//      Pointer on a MRUMENU structure, used by other function
//
//
//  Comments:
//
//
//  History:    Date       Author       Comment
//              09/24/94   G. Vollant   Created
//
//*************************************************************

LPMRUMENU CreateMruMenuDefault()
{
  return CreateMruMenu (NBMRUMENUSHOW,NBMRUMENU,MAXSIZEMRUITEM,IDMRU);
}


//*************************************************************
//
//  DeleteMruMenu()
//
//  Purpose:
//              Destructor :
//              Clean and free a MRUMENU structure
//
//  Parameters:
//
//      LPMRUMENU lpMruMenu -      pointer on MRUMENU, allocated
//             by CreateMruMenu() or CreateMruMenuDefault()
//
//
//  Return: void
//
//
//  Comments:
//
//
//  History:    Date       Author       Comment
//              09/24/94   G. Vollant   Created
//
//*************************************************************
void DeleteMruMenu(LPMRUMENU lpMruMenu)
{
  GlobalFreePtr(lpMruMenu->lpMRU);
  GlobalFreePtr(lpMruMenu);
}

//*************************************************************
//
//  SetNbLruShow()
//
//  Purpose:
//              Change the maximum number of item displayed on menu
//
//  Parameters:
//      LPMRUMENU lpMruMenu -      pointer on MRUMENU
//      WORD wNbLruShowInit -      Maximum number of item displayed on menu
//
//
//  Return: void
//
//
//  Comments:
//
//
//  History:    Date       Author       Comment
//              09/24/94   G. Vollant   Created
//
//*************************************************************
void SetNbLruShow   (LPMRUMENU lpMruMenu,WORD wNbLruShowInit)
{
  lpMruMenu->wNbLruShow = min(wNbLruShowInit,lpMruMenu->wNbLruMenu);
}

//*************************************************************
//
//  SetMenuItem()
//
//  Purpose:
//              Set the filename of an item 
//
//  Parameters:
//      LPMRUMENU lpMruMenu -      pointer on MRUMENU
//      WORD wItem -               Number of Item to set, zero based
//      LPSTR lpItem -             String, contain the filename of the item
//
//
//  Return: (BOOL)
//      TRUE  - Function run successfully
//      FALSE - Function don't run successfully
//
//
//  Comments:
//      used when load .INI or reg database
//
//  History:    Date       Author       Comment
//              09/24/94   G. Vollant   Created
//
//*************************************************************
BOOL SetMenuItem    (LPMRUMENU lpMruMenu,WORD wItem,LPSTR lpItem)
{                                      
  if (wItem >= NBMRUMENU) 
    return FALSE;
  _fstrncpy((lpMruMenu->lpMRU) + 
            ((lpMruMenu->wMaxSizeLruItem) * (UINT)wItem),
            lpItem,lpMruMenu->wMaxSizeLruItem-1);
  lpMruMenu->wNbItemFill = max(lpMruMenu->wNbItemFill,wItem+1);
  return TRUE;
}

//*************************************************************
//
//  GetMenuItem()
//
//  Purpose:
//              Get the filename of an item 
//
//  Parameters:
//      LPMRUMENU lpMruMenu -      pointer on MRUMENU
//      WORD wItem -               Number of Item to set, zero based
//      BOOL fIDMBased -           TRUE :  wItem is based on ID menu item
//                                 FALSE : wItem is zero-based
//      LPSTR lpItem -             String where the filename of the item will be
//                                   stored by GetMenuItem()
//      UINT  uiSize -             Size of the lpItem buffer
//
//
//  Return: (BOOL)
//      TRUE  - Function run successfully
//      FALSE - Function don't run successfully
//
//
//  Comments:
//      Used for saving in .INI or reg database, or when user select
//        an MRU in File menu
//
//  History:    Date       Author       Comment
//              09/24/94   G. Vollant   Created
//
//*************************************************************
BOOL GetMenuItem    (LPMRUMENU lpMruMenu,WORD wItem,
                     BOOL fIDMBased,LPSTR lpItem,UINT uiSize)
{
  if (fIDMBased) 
    wItem -= (lpMruMenu->wIdMru + 1);
  if (wItem >= lpMruMenu->wNbItemFill) 
    return FALSE;
  _fstrncpy(lpItem,(lpMruMenu->lpMRU) + 
                ((lpMruMenu->wMaxSizeLruItem) * (UINT)(wItem)),uiSize);
  *(lpItem+uiSize-1) = '\0';
  return TRUE;
}

//*************************************************************
//
//  AddNewItem()
//
//  Purpose:
//              Add an item at the begin of the list
//
//  Parameters:
//      LPMRUMENU lpMruMenu -      pointer on MRUMENU
//      LPSTR lpItem -             String contain the filename to add
//
//  Return: (BOOL)
//      TRUE  - Function run successfully
//      FALSE - Function don't run successfully
//
//
//  Comments:
//      Used when used open a file (using File Open common
//        dialog, Drag and drop or MRU)
//
//  History:    Date       Author       Comment
//              09/24/94   G. Vollant   Created
//
//*************************************************************
void AddNewItem     (LPMRUMENU lpMruMenu,LPSTR lpItem)
{
WORD i,j;
  for (i=0;i<lpMruMenu->wNbItemFill;i++)
    if (lstrcmpi(lpItem,(lpMruMenu->lpMRU) + 
        ((lpMruMenu->wMaxSizeLruItem) * (UINT)i)) == 0)
      {         
        // Shift the other items
        for (j=i;j>0;j--)
         lstrcpy((lpMruMenu->lpMRU) + (lpMruMenu->wMaxSizeLruItem * (UINT)j),
                 (lpMruMenu->lpMRU) + (lpMruMenu->wMaxSizeLruItem * (UINT)(j-1)));
        _fstrncpy(lpMruMenu->lpMRU,lpItem,lpMruMenu->wMaxSizeLruItem-1);  
        return ;
      }
  lpMruMenu->wNbItemFill = min(lpMruMenu->wNbItemFill+1,lpMruMenu->wNbLruMenu);
  for (i=lpMruMenu->wNbItemFill-1;i>0;i--)
     lstrcpy(lpMruMenu->lpMRU + (lpMruMenu->wMaxSizeLruItem * (UINT)i),
             lpMruMenu->lpMRU + (lpMruMenu->wMaxSizeLruItem * (UINT)(i-1)));
  _fstrncpy(lpMruMenu->lpMRU,lpItem,lpMruMenu->wMaxSizeLruItem-1);  
}

//*************************************************************
//
//  DelMenuItem()
//
//  Purpose:
//              Delete an item
//
//  Parameters:
//      LPMRUMENU lpMruMenu -      pointer on MRUMENU
//      WORD wItem -               Number of Item to set, zero based
//      BOOL fIDMBased -           TRUE :  wItem is based on ID menu item
//                                 FALSE : wItem is zero-based
//
//  Return: (BOOL)
//      TRUE  - Function run successfully
//      FALSE - Function don't run successfully
//
//
//  Comments:
//      Used when used open a file, using MRU, and when an error
//         occured (by example, when file was deleted)
//
//  History:    Date       Author       Comment
//              09/24/94   G. Vollant   Created
//
//*************************************************************
BOOL DelMenuItem(LPMRUMENU lpMruMenu,WORD wItem,BOOL fIDMBased)
{ 
WORD i;
  if (fIDMBased) 
    wItem -= (lpMruMenu->wIdMru + 1);
  if (lpMruMenu->wNbItemFill <= wItem) 
    return FALSE;
  lpMruMenu->wNbItemFill--;
  for (i=wItem;i<lpMruMenu->wNbItemFill;i++)
     lstrcpy(lpMruMenu->lpMRU + (lpMruMenu->wMaxSizeLruItem * (UINT)i),
             lpMruMenu->lpMRU + (lpMruMenu->wMaxSizeLruItem * (UINT)(i+1)));
  return TRUE;
}
                           
//*************************************************************
//
//  PlaceMenuMRUItem()
//
//  Purpose:
//              Add MRU at the end of a menu
//
//  Parameters:
//      LPMRUMENU lpMruMenu -      pointer on MRUMENU
//      HMENU hMenu -              Handle of menu where MRU must be added
//      UINT uiItem -              Item of menu entry where MRU must be added
//
//  Return: void
//
//
//  Comments:
//      Used MRU is modified, for refresh the File menu
//
//  History:    Date       Author       Comment
//              09/24/94   G. Vollant   Created
//
//*************************************************************
void PlaceMenuMRUItem(LPMRUMENU lpMruMenu,HMENU hMenu,UINT uiItem)
{
int  i;   
WORD wNbShow;
  if (hMenu == NULL) 
    return;
  // remove old MRU in menu
  for (i=0;i<=(int)(lpMruMenu->wNbLruMenu);i++)
    RemoveMenu(hMenu,i+lpMruMenu->wIdMru,MF_BYCOMMAND);

  if (lpMruMenu->wNbItemFill == 0) 
    return;

  // If they are item, insert a separator before the files
  InsertMenu(hMenu,uiItem,MF_SEPARATOR,lpMruMenu->wIdMru,NULL);

  wNbShow = min(lpMruMenu->wNbItemFill,lpMruMenu->wNbLruShow);
  for (i=(int)wNbShow-1;i>=0;i--)
  {
  LPSTR lpTxt;
    if (lpTxt = (LPSTR)GlobalAllocPtr(GHND,lpMruMenu->wMaxSizeLruItem + 20))
      {
        wsprintf(lpTxt,"&%lu %s",
                 (DWORD)(i+1),lpMruMenu->lpMRU + (lpMruMenu->wMaxSizeLruItem*(UINT)i));
        InsertMenu(hMenu,(((WORD)i)!=(wNbShow-1)) ? (lpMruMenu->wIdMru+i+2) : lpMruMenu->wIdMru,
                   MF_STRING,lpMruMenu->wIdMru+i+1,lpTxt);   
        GlobalFreePtr(lpTxt);
      }
  }

}

///////////////////////////////////////////



//*************************************************************
//
//  SaveMruInIni()
//
//  Purpose:
//              Save MRU in a private .INI
//
//  Parameters:
//      LPMRUMENU lpMruMenu -      pointer on MRUMENU
//      LPSTR lpszSection  -       Points to a null-terminated string containing
//                                      the name of the section 
//      LPSTR lpszFile -           Points to a null-terminated string that names 
//                                      the initialization file. 
//
//  Return: (BOOL)
//      TRUE  - Function run successfully
//      FALSE - Function don't run successfully
//
//
//  Comments:
//      See WritePrivateProfileString API for more info on lpszSection and lpszFile
//
//  History:    Date       Author       Comment
//              09/24/94   G. Vollant   Created
//
//*************************************************************
BOOL SaveMruInIni(LPMRUMENU lpMruMenu,LPSTR lpszSection,LPSTR lpszFile)
{
LPSTR lpTxt;
WORD i;

  lpTxt = (LPSTR)GlobalAllocPtr(GHND,lpMruMenu->wMaxSizeLruItem + 20);
  if (lpTxt == NULL) 
    return FALSE;

  for (i=0;i<lpMruMenu->wNbLruMenu;i++)
    {
    char szEntry[16];
      wsprintf(szEntry,"File%lu",(DWORD)i+1);
      if (!GetMenuItem(lpMruMenu,i,FALSE,lpTxt,lpMruMenu->wMaxSizeLruItem + 10))
        *lpTxt = '\0';
      WritePrivateProfileString(lpszSection,szEntry,lpTxt,lpszFile);
    }
  GlobalFreePtr(lpTxt);
  WritePrivateProfileString(NULL,NULL,NULL,lpszFile); // flush cache 
  return TRUE;
}


//*************************************************************
//
//  LoadMruInIni()
//
//  Purpose:
//              Load MRU from a private .INI
//
//  Parameters:
//      LPMRUMENU lpMruMenu -      pointer on MRUMENU
//      LPSTR lpszSection  -       Points to a null-terminated string containing
//                                      the name of the section 
//      LPSTR lpszFile -           Points to a null-terminated string that names 
//                                      the initialization file. 
//
//  Return: (BOOL)
//      TRUE  - Function run successfully
//      FALSE - Function don't run successfully
//
//
//  Comments:
//      See GetPrivateProfileString API for more info on lpszSection and lpszFile
//
//  History:    Date       Author       Comment
//              09/24/94   G. Vollant   Created
//
//*************************************************************
BOOL LoadMruInIni(LPMRUMENU lpMruMenu,LPSTR lpszSection,LPSTR lpszFile)
{
LPSTR lpTxt;
WORD i;
  lpTxt = (LPSTR)GlobalAllocPtr(GHND,lpMruMenu->wMaxSizeLruItem + 20);
  if (lpTxt == NULL) 
    return FALSE;

  for (i=0;i<lpMruMenu->wNbLruMenu;i++)
    {
    char szEntry[16];
      
      wsprintf(szEntry,"File%lu",(DWORD)i+1);
      GetPrivateProfileString(lpszSection,szEntry,"",lpTxt,
                              lpMruMenu->wMaxSizeLruItem + 10,lpszFile);
      if (*lpTxt == '\0')
        break;
      SetMenuItem(lpMruMenu,i,lpTxt);
    }
  GlobalFreePtr(lpTxt);
  return TRUE;
}

#ifdef WIN32

BOOL IsWin395OrHigher(void)
{
  WORD wVer;

  wVer = LOWORD(GetVersion());
  wVer = (((WORD)LOBYTE(wVer)) << 8) | (WORD)HIBYTE(wVer);

  return (wVer >= 0x035F);              // 5F = 95 dec
}


//*************************************************************
//
//  SaveMruInReg()
//
//  Purpose:
//              Save MRU in the registry
//
//  Parameters:
//      LPMRUMENU lpMruMenu -      pointer on MRUMENU
//      LPSTR lpszKey  -           Points to a null-terminated string 
//                                      specifying  the name of a key that 
//                                      this function opens or creates.
//
//  Return: (BOOL)
//      TRUE  - Function run successfully
//      FALSE - Function don't run successfully
//
//
//  Comments:
//      Win32 function designed for Windows NT and Windows 95
//      See RegCreateKeyEx API for more info on lpszKey
//
//  History:    Date       Author       Comment
//              09/24/94   G. Vollant   Created
//
//*************************************************************
BOOL SaveMruInReg(LPMRUMENU lpMruMenu,LPSTR lpszKey)
{
LPSTR lpTxt;
WORD i;
HKEY hCurKey;
DWORD dwDisp;

  lpTxt = (LPSTR)GlobalAllocPtr(GHND,lpMruMenu->wMaxSizeLruItem + 20);
  if (lpTxt == NULL) 
    return FALSE;

  RegCreateKeyEx(HKEY_CURRENT_USER,lpszKey,0,NULL,
                  REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hCurKey,&dwDisp);

  for (i=0;i<lpMruMenu->wNbLruMenu;i++)
    {
    char szEntry[16];
      wsprintf(szEntry,"File%lu",(DWORD)i+1);
      if (!GetMenuItem(lpMruMenu,i,FALSE,lpTxt,lpMruMenu->wMaxSizeLruItem + 10))
        *lpTxt = '\0';
      RegSetValueEx(hCurKey,szEntry,0,REG_SZ,(unsigned char*)lpTxt,lstrlen(lpTxt));
    }
  RegCloseKey(hCurKey);
  GlobalFreePtr(lpTxt);
  return TRUE;
}

//*************************************************************
//
//  LoadMruInReg()
//
//  Purpose:
//              Load MRU from the registry
//
//  Parameters:
//      LPMRUMENU lpMruMenu -      pointer on MRUMENU
//      LPSTR lpszKey  -           Points to a null-terminated string 
//                                      specifying  the name of a key that 
//                                      this function opens or creates.
//
//  Return: (BOOL)
//      TRUE  - Function run successfully
//      FALSE - Function don't run successfully
//
//
//  Comments:
//      Win32 function designed for Windows NT and Windows 95
//      See RegOpenKeyEx API for more info on lpszKey
//
//  History:    Date       Author       Comment
//              09/24/94   G. Vollant   Created
//
//*************************************************************
BOOL LoadMruInReg(LPMRUMENU lpMruMenu,LPSTR lpszKey)
{
LPSTR lpTxt;
WORD i;
HKEY hCurKey;
DWORD dwType;
  lpTxt = (LPSTR)GlobalAllocPtr(GHND,lpMruMenu->wMaxSizeLruItem + 20);
  if (lpTxt == NULL) 
    return FALSE;

  RegOpenKeyEx(HKEY_CURRENT_USER,lpszKey,0,KEY_READ,&hCurKey);


  for (i=0;i<lpMruMenu->wNbLruMenu;i++)
    {
    char szEntry[16];
    DWORD dwSizeBuf;  
      wsprintf(szEntry,"File%lu",(DWORD)i+1);
      *lpTxt = '\0';
      dwSizeBuf = lpMruMenu->wMaxSizeLruItem + 10;
      RegQueryValueEx(hCurKey,szEntry,NULL,&dwType,(LPBYTE)lpTxt,&dwSizeBuf);
      *(lpTxt+dwSizeBuf)='\0';
      if (*lpTxt == '\0')
        break;
      SetMenuItem(lpMruMenu,i,lpTxt);
    }
  RegCloseKey(hCurKey);
  GlobalFreePtr(lpTxt);
  return TRUE;
}


//*************************************************************
//
//  GetWin32Kind()
//
//  Purpose:
//              Get the Win32 platform
//
//  Parameters:
//
//  Return: (WIN32KIND)
//      WINNT -           Run under Windows NT
//      WIN32S -          Run under Windows 3.1x + Win32s
//      WIN95ORGREATHER - Run under Windows 95
//
//
//  Comments:
//      Win32 function designed for Windows NT and Windows 95
//      See RegOpenKeyEx API for more info on lpszKey
//
//  History:    Date       Author       Comment
//              09/24/94   G. Vollant   Created
//
//*************************************************************
WIN32KIND GetWin32Kind()
{
BOOL IsWin395OrHigher(void);

  WORD wVer;

  if ((GetVersion() & 0x80000000) == 0)
    return WINNT;
  wVer = LOWORD(GetVersion());
  wVer = (((WORD)LOBYTE(wVer)) << 8) | (WORD)HIBYTE(wVer);

  if (wVer >= 0x035F)
    return WIN95ORGREATHER;
  else
    return WIN32S;
}
#endif
