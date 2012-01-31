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
// win_xy.c -- windows specific xy view code

#include "stdafx.h"
#include "qe3.h"

static HDC   s_hdcXY;
static HGLRC s_hglrcXY;

static unsigned s_stipple[32] =
{
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
};

/*
============
WXY_WndProc
============
*/
LONG WINAPI XYWndProc (
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
	int		fwKeys, xPos, yPos;
    RECT	rect;


    GetClientRect(hWnd, &rect);

    switch (uMsg)
    {
	case WM_CREATE:

    s_hdcXY = GetDC(hWnd);
		QEW_SetupPixelFormat(s_hdcXY, false);

		if ( ( s_hglrcXY = wglCreateContext( s_hdcXY ) ) == 0 )
			Error( "wglCreateContext in WXY_WndProc failed" );

        if (!wglMakeCurrent( s_hdcXY, s_hglrcXY ))
			Error ("wglMakeCurrent failed");

		if (!wglShareLists( g_qeglobals.d_hglrcBase, s_hglrcXY ) )
			Error( "wglShareLists in WXY_WndProc failed" );

		glPolygonStipple ((unsigned char *)s_stipple);
		glLineStipple (3, 0xaaaa);

		return 0;

	case WM_DESTROY:
		QEW_StopGL( hWnd, s_hglrcXY, s_hdcXY );
		return 0;

	case WM_PAINT:
        { 
		    PAINTSTRUCT	ps;

		    BeginPaint(hWnd, &ps);

            if (!wglMakeCurrent( s_hdcXY, s_hglrcXY ))
				Error ("wglMakeCurrent failed");

			QE_CheckOpenGLForErrors();
			XY_Draw ();
			QE_CheckOpenGLForErrors();

			SwapBuffers(s_hdcXY);

			EndPaint(hWnd, &ps);
        }
		return 0;

	case WM_KEYDOWN:
		return QE_KeyDown (wParam);
		
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		if ( GetTopWindow( g_qeglobals.d_hwndMain ) != hWnd)
			BringWindowToTop(hWnd);
		SetFocus( g_qeglobals.d_hwndXY );
		SetCapture( g_qeglobals.d_hwndXY );
		fwKeys = wParam;        // key flags 
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 
		yPos = (int)rect.bottom - 1 - yPos;
		XY_MouseDown (xPos, yPos, fwKeys);
		return 0;

	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
		fwKeys = wParam;        // key flags 
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 
		yPos = (int)rect.bottom - 1 - yPos;
		XY_MouseUp (xPos, yPos, fwKeys);
		if (! (fwKeys & (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON)))
			ReleaseCapture ();
		return 0;

	case WM_MOUSEMOVE:
    if (::GetFocus() != hWnd)
      ::SetFocus(hWnd);
		fwKeys = wParam;        // key flags 
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 
		yPos = (int)rect.bottom - 1 - yPos;
		XY_MouseMoved (xPos, yPos, fwKeys);
		return 0;

    case WM_SIZE:
		g_qeglobals.d_xy.width = rect.right;
		g_qeglobals.d_xy.height = rect.bottom - 16;
		InvalidateRect( g_qeglobals.d_hwndXY, NULL, false);
		return 0;

	case WM_NCCALCSIZE:// don't let windows copy pixels
		DefWindowProc (hWnd, uMsg, wParam, lParam);
		return WVR_REDRAW;

	case WM_KILLFOCUS:
	case WM_SETFOCUS:
		SendMessage( hWnd, WM_NCACTIVATE, uMsg == WM_SETFOCUS, 0 );
		return 0;

   	case WM_CLOSE:
        DestroyWindow (hWnd);
		return 0;
    }

	return DefWindowProc (hWnd, uMsg, wParam, lParam);
}


/*
==============
WXY_Create
==============
*/
void WXY_Create (HINSTANCE hInstance)
{
  ASSERT(0);
#if 0
    WNDCLASS   wc;

    /* Register the camera class */
	memset (&wc, 0, sizeof(wc));

    wc.style         = CS_NOCLOSE;
    wc.lpfnWndProc   = (WNDPROC)WXY_WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = 0;
    wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
    wc.hbrBackground = NULL; //(HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = XY_WINDOW_CLASS;

    if (!RegisterClass (&wc) )
        Error ("RegisterClass: failed");

	g_qeglobals.d_hwndXY = CreateWindow (XY_WINDOW_CLASS ,
		"XY View",
		QE3_STYLE ,
		ZWIN_WIDTH,
		(int)(screen_height*CWIN_SIZE)-20,
		screen_width-ZWIN_WIDTH,
		(int)(screen_height*(1.0-CWIN_SIZE)-38),	// size

		g_qeglobals.d_hwndMain,	// parent
		0,		// no menu
		hInstance,
		NULL);

	if (!g_qeglobals.d_hwndXY )
		Error ("Couldn't create XY View");

	LoadWindowState(g_qeglobals.d_hwndXY, "xywindow");
    ShowWindow(g_qeglobals.d_hwndXY, SW_SHOWDEFAULT);
#endif
}

static void WXY_InitPixelFormat( PIXELFORMATDESCRIPTOR *pPFD )
{
	memset( pPFD, 0, sizeof( *pPFD ) );

	pPFD->nSize    = sizeof( PIXELFORMATDESCRIPTOR );
	pPFD->nVersion = 1;
	pPFD->dwFlags  = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
	pPFD->iPixelType = PFD_TYPE_RGBA;
	pPFD->cColorBits = 24;
	pPFD->cDepthBits = 32;
	pPFD->iLayerType = PFD_MAIN_PLANE;

}

void WXY_Print( void )
{
	DOCINFO di;

	PRINTDLG pd;

	/*
	** initialize the PRINTDLG struct and execute it
	*/
	memset( &pd, 0, sizeof( pd ) );
	pd.lStructSize = sizeof( pd );
	pd.hwndOwner = g_qeglobals.d_hwndXY;
	pd.Flags = PD_RETURNDC;
	pd.hInstance = 0;
	if ( !PrintDlg( &pd ) || !pd.hDC )
	{
		MessageBox( g_qeglobals.d_hwndMain, "Could not PrintDlg()", "QE4 Print Error", MB_OK | MB_ICONERROR );
		return;
	}

	/*
	** StartDoc
	*/
	memset( &di, 0, sizeof( di ) );
	di.cbSize = sizeof( di );
	di.lpszDocName = "QE4";
	if ( StartDoc( pd.hDC, &di ) <= 0 )
	{
		MessageBox( g_qeglobals.d_hwndMain, "Could not StartDoc()", "QE4 Print Error", MB_OK | MB_ICONERROR );
		return;
	}

	/*
	** StartPage
	*/
	if ( StartPage( pd.hDC ) <= 0 )
	{
		MessageBox( g_qeglobals.d_hwndMain, "Could not StartPage()", "QE4 Print Error", MB_OK | MB_ICONERROR );
		return;
	}

	/*
	** read pixels from the XY window
	*/
	{
		int bmwidth = 320, bmheight = 320;
		int pwidth, pheight;

		RECT r;

		GetWindowRect( g_qeglobals.d_hwndXY, &r );

		bmwidth  = r.right - r.left;
		bmheight = r.bottom - r.top;

		pwidth  = GetDeviceCaps( pd.hDC, PHYSICALWIDTH ) - GetDeviceCaps( pd.hDC, PHYSICALOFFSETX );
		pheight = GetDeviceCaps( pd.hDC, PHYSICALHEIGHT ) - GetDeviceCaps( pd.hDC, PHYSICALOFFSETY );

		StretchBlt( pd.hDC,
			0, 0,
			pwidth, pheight,
			s_hdcXY,
			0, 0,
			bmwidth, bmheight,
			SRCCOPY );
	}

	/*
	** EndPage and EndDoc
	*/
	if ( EndPage( pd.hDC ) <= 0 )
	{
		MessageBox( g_qeglobals.d_hwndMain, "QE4 Print Error", "Could not EndPage()", MB_OK | MB_ICONERROR );
		return;
	}

	if ( EndDoc( pd.hDC ) <= 0 )
	{
		MessageBox( g_qeglobals.d_hwndMain, "QE4 Print Error", "Could not EndDoc()", MB_OK | MB_ICONERROR );
		return;
	}
}
