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
// DlgCamera.cpp : implementation file
//

#include "stdafx.h"
#include "DlgCamera.h"
#include "DlgEvent.h"
#include "NameDlg.h"
#include "splines/splines.h"
#include "CameraTargetDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDlgCamera g_dlgCamera;

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
char	*va( char *format, ... ) {
	va_list		argptr;
	static char		string[2][32000];	// in case va is called by nested functions
	static int		index = 0;
	char	*buf;

	buf = string[index & 1];
	index++;

	va_start (argptr, format);
	vsprintf (buf, format,argptr);
	va_end (argptr);

	return buf;
}


void showCameraInspector() {
	if (g_dlgCamera.GetSafeHwnd() == NULL) {
		g_dlgCamera.Create(IDD_DLG_CAMERA);
		CRect rct;
		LONG lSize = sizeof(rct);
		if (LoadRegistryInfo("Radiant::CameraInspector", &rct, &lSize)) {
			g_dlgCamera.SetWindowPos(NULL, rct.left, rct.top, 0,0, SWP_NOSIZE | SWP_SHOWWINDOW);
		}
		Sys_UpdateWindows(W_ALL);
	} 
	g_dlgCamera.ShowWindow(SW_SHOW);
	g_dlgCamera.setupFromCamera();
}
/////////////////////////////////////////////////////////////////////////////
// CDlgCamera dialog


CDlgCamera::CDlgCamera(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgCamera::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgCamera)
	m_strName = _T("");
	m_fSeconds = 0.0f;
	m_trackCamera = TRUE;
	m_numSegments = 0;
	m_currentSegment = 0;
	m_strType = _T("");
	m_editPoints = 0;
	//}}AFX_DATA_INIT
}


void CDlgCamera::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgCamera)
	DDX_Control(pDX, IDC_SCROLLBAR_SEGMENT, m_wndSegments);
	DDX_Control(pDX, IDC_LIST_EVENTS, m_wndEvents);
	DDX_Control(pDX, IDC_COMBO_SPLINES, m_wndSplines);
	DDX_Text(pDX, IDC_EDIT_CAM_NAME, m_strName);
	DDX_Text(pDX, IDC_EDIT_LENGTH, m_fSeconds);
	DDX_Check(pDX, IDC_CHECK_TRACKCAMERA, m_trackCamera);
	DDX_Text(pDX, IDC_EDIT_TOTALSEGMENTS, m_numSegments);
	DDX_Text(pDX, IDC_EDIT_SEGMENT, m_currentSegment);
	DDX_Text(pDX, IDC_EDIT_TYPE, m_strType);
	DDX_Radio(pDX, IDC_RADIO_EDITPOINTS, m_editPoints);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgCamera, CDialog)
	//{{AFX_MSG_MAP(CDlgCamera)
	ON_BN_CLICKED(IDC_BTN_ADDEVENT, OnBtnAddevent)
	ON_BN_CLICKED(IDC_BTN_ADDTARGET, OnBtnAddtarget)
	ON_BN_CLICKED(IDC_BTN_DELEVENT, OnBtnDelevent)
	ON_CBN_DBLCLK(IDC_COMBO_SPLINES, OnDblclkComboSplines)
	ON_CBN_SELCHANGE(IDC_COMBO_SPLINES, OnSelchangeComboSplines)
	ON_LBN_SELCHANGE(IDC_LIST_EVENTS, OnSelchangeListEvents)
	ON_LBN_DBLCLK(IDC_LIST_EVENTS, OnDblclkListEvents)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDAPPLY, OnApply)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(ID_FILE_NEW, OnFileNew)
	ON_BN_CLICKED(ID_FILE_OPEN, OnFileOpen)
	ON_BN_CLICKED(ID_FILE_SAVE, OnFileSave)
	ON_BN_CLICKED(ID_TESTCAMERA, OnTestcamera)
	ON_BN_CLICKED(IDC_BTN_DELETEPOINTS, OnBtnDeletepoints)
	ON_BN_CLICKED(IDC_BTN_SELECTALL, OnBtnSelectall)
	ON_BN_CLICKED(IDC_RADIO_EDITPOINTS, OnRadioEditpoints)
	ON_BN_CLICKED(IDC_RADIO_EDITPOINTS2, OnRadioAddPoints)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgCamera message handlers

void CDlgCamera::OnBtnAddevent() 
{
	CDlgEvent dlg;
	if (dlg.DoModal() == IDOK) {
		long n = m_wndSegments.GetScrollPos() / 4 * 1000;
		g_splineList->addEvent(static_cast<idCameraEvent::eventType>(dlg.m_event+1), dlg.m_strParm, n);
		setupFromCamera();
	}
}

void CDlgCamera::OnBtnAddtarget() 
{
	CCameraTargetDlg dlg;
	if (dlg.DoModal() == IDOK) {
		g_splineList->addTarget(dlg.m_strName, static_cast<idCameraPosition::positionType>(dlg.m_nType));
		setupFromCamera();
		m_wndSplines.SetCurSel(g_splineList->numTargets());
		OnSelchangeComboSplines();
		OnRadioAddPoints();
	}
}

void CDlgCamera::OnBtnDelevent() 
{
	// TODO: Add your control notification handler code here
	
}

void CDlgCamera::OnBtnDeltarget() 
{
	// TODO: Add your control notification handler code here
	
}

void CDlgCamera::OnDblclkComboSplines() 
{
	// TODO: Add your control notification handler code here
	
}

void CDlgCamera::OnSelchangeComboSplines() 
{
	UpdateData(TRUE);
	g_qeglobals.d_select_mode = (m_editPoints == 0) ? sel_editpoint : sel_addpoint;
	g_qeglobals.d_numpoints = 0;
	g_qeglobals.d_num_move_points = 0;
	int i = m_wndSplines.GetCurSel();
	if (i > 0) {
		g_splineList->setActiveTarget(i-1);
		g_qeglobals.selectObject = g_splineList->getActiveTarget(i-1);
		g_splineList->startEdit(false);
	} else {
		g_splineList->startEdit(true);
		g_qeglobals.selectObject = g_splineList->getPositionObj();
	}

	// * 4.0 to set increments in quarter seconds
	m_wndSegments.SetScrollRange(0, g_splineList->getTotalTime() * 4.0);

	Sys_UpdateWindows(W_ALL);
}

void CDlgCamera::OnSelchangeListEvents() 
{
	int sel = m_wndEvents.GetCurSel();
	//g_splineList->setActiveSegment(sel >= 0 ? sel : 0);
}

void CDlgCamera::OnDblclkListEvents() 
{
	// TODO: Add your control notification handler code here
	
}

void CDlgCamera::setupFromCamera()
{
	if (m_wndSplines.GetSafeHwnd()) {
		int i;
		idStr str;
		m_strName = g_splineList->getName();
		m_strType = g_splineList->getPositionObj()->typeStr();
		m_wndSplines.ResetContent();
		m_wndSplines.AddString("Path");
		for (i = 0; i < g_splineList->numTargets(); i++) {
			m_wndSplines.AddString(g_splineList->getActiveTarget(i)->getName());
		}
		m_wndSplines.SetCurSel(0);
		m_fSeconds = g_splineList->getBaseTime();
		m_wndSegments.SetScrollRange(0, g_splineList->getTotalTime() * 4.0);

		m_wndEvents.ResetContent();
		for (i = 0; i < g_splineList->numEvents(); i++) {
			str = va("%s\t%s", g_splineList->getEvent(i)->typeStr(), g_splineList->getEvent(i)->getParam());
			m_wndEvents.AddString(str);
		}
		//m_currentSegment = g_splineList->getActiveSegment();
		//m_numSegments = g_splineList->numSegments();
	}
	g_splineList->startEdit(true);
	UpdateData(FALSE);
}

BOOL CDlgCamera::OnInitDialog() 
{
	CDialog::OnInitDialog();
	setupFromCamera();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgCamera::OnOK() 
{
	g_dlgCamera.ShowWindow(SW_HIDE);
	clearSelection();
	g_splineList->stopEdit();
	Sys_UpdateWindows(W_ALL);
}

void CDlgCamera::OnDestroy() 
{
	if (GetSafeHwnd()) {
		CRect rct;
		GetWindowRect(rct);
		SaveRegistryInfo("Radiant::CameraInspector", &rct, sizeof(rct));
	}
	CDialog::OnDestroy();
	Sys_UpdateWindows(W_ALL);
}


void CDlgCamera::OnApply() 
{
	UpdateData(TRUE);
	g_splineList->setBaseTime(m_fSeconds);
	g_splineList->setName(m_strName);
	g_splineList->buildCamera();
	m_wndSegments.SetScrollRange(0, g_splineList->getTotalTime() * 4.0);
}

void CDlgCamera::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
	int max = g_splineList->getTotalTime() * 4;
	if (max == 0) {
		max = 1;
	}
	int n = pScrollBar->GetScrollPos();
	switch (nSBCode) {
		case SB_LINEUP : {
			n--;
		}
		break;
		case SB_LINEDOWN : {
			n++;
		}
		break;
		case SB_PAGEUP : {
			n -= (float)max * 0.10;
		}
		break;
		case SB_PAGEDOWN : {
			n += (float)max * 0.10;
		}
		break;
		case SB_THUMBPOSITION : {
			n = nPos;
		}
		break;
		case SB_THUMBTRACK : {
			n = nPos;
		}
	}
//	if (n < 0) {
//		n = 0;
//	} else if (n >= g_splineList->numSegments()) {
//		if (g_splineList->numSegments() == 0) {
//			g_splineList->buildCamera();
//		}
//		n = g_splineList->numSegments() - 1;
//	}
	pScrollBar->SetScrollPos(n);
	if (m_trackCamera) {
		float p = (float)n / max;
		p *= g_splineList->getTotalTime() * 1000;
		g_splineList->startCamera(0);
		g_splineList->buildCamera();
		vec3_t dir;
		float fov;
		g_splineList->getCameraInfo(p, g_pParentWnd->GetCamera()->Camera().origin, dir, &fov);
		g_pParentWnd->GetCamera()->Camera().angles[1] = atan2 (dir[1], dir[0])*180/3.14159;
		g_pParentWnd->GetCamera()->Camera().angles[0] = asin (dir[2])*180/3.14159;

	}
	UpdateData(FALSE);
	Sys_UpdateWindows(W_XY | W_CAMERA);
}

void CDlgCamera::OnFileNew() 
{
	g_splineList->clear();
	g_qeglobals.selectObject = g_splineList->getPositionObj();
	setupFromCamera();
	Sys_UpdateWindows(W_ALL);
}

void CDlgCamera::OnFileOpen() 
{
	CNameDlg dlg("Open Camera File");
	if (dlg.DoModal() == IDOK) {
		g_splineList->clear();
		g_qeglobals.selectObject = g_splineList->getPositionObj();
		g_splineList->load(va("%s.camera", dlg.m_strName));
		setupFromCamera();
		Sys_UpdateWindows(W_ALL);
	}
}

void CDlgCamera::OnFileSave() 
{
	CNameDlg dlg("Save Camera File");
	if (dlg.DoModal() == IDOK) {
		g_splineList->save(va("%s.camera", dlg.m_strName));
	}
}

void CDlgCamera::OnTestcamera() 
{
	// TODO: Add your control notification handler code here
	
}

void CDlgCamera::OnBtnDeletepoints() 
{
	// TODO: Add your control notification handler code here
	
}

void CDlgCamera::OnBtnSelectall() 
{
	// TODO: Add your control notification handler code here
	
}

void CDlgCamera::OnRadioEditpoints() 
{
	UpdateData(TRUE);
	g_qeglobals.d_select_mode = sel_editpoint;
}

void CDlgCamera::OnRadioAddPoints() 
{
	UpdateData(TRUE);
	g_qeglobals.d_select_mode = sel_addpoint;
}
