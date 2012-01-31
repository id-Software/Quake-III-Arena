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
#if !defined(AFX_PATCHDIALOG_H__DE62DFB4_E9EC_11D2_A509_0020AFEB881A__INCLUDED_)
#define AFX_PATCHDIALOG_H__DE62DFB4_E9EC_11D2_A509_0020AFEB881A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PatchDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPatchDialog dialog

class CPatchDialog : public CDialog
{
  patchMesh_t *m_Patch;
// Construction
public:
	void UpdateInfo();
	void SetPatchInfo();
	void GetPatchInfo();
	CPatchDialog(CWnd* pParent = NULL);   // standard constructor
  void UpdateSpinners(bool bUp, int nID);

// Dialog Data
	//{{AFX_DATA(CPatchDialog)
	enum { IDD = IDD_DIALOG_PATCH };
	CSpinButtonCtrl	m_wndVShift;
	CSpinButtonCtrl	m_wndVScale;
	CSpinButtonCtrl	m_wndRotate;
	CSpinButtonCtrl	m_wndHShift;
	CSpinButtonCtrl	m_wndHScale;
	CComboBox	m_wndType;
	CComboBox	m_wndRows;
	CComboBox	m_wndCols;
	CString	m_strName;
	float	m_fS;
	float	m_fT;
	float	m_fX;
	float	m_fY;
	float	m_fZ;
	float	m_fHScale;
	float	m_fHShift;
	float	m_fRotate;
	float	m_fVScale;
	float	m_fVShift;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPatchDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpdateRowColInfo();

	// Generated message map functions
	//{{AFX_MSG(CPatchDialog)
	afx_msg void OnBtnPatchdetails();
	afx_msg void OnBtnPatchfit();
	afx_msg void OnBtnPatchnatural();
	afx_msg void OnBtnPatchreset();
	afx_msg void OnSelchangeComboCol();
	afx_msg void OnSelchangeComboRow();
	afx_msg void OnSelchangeComboType();
	virtual void OnOK();
	afx_msg void OnDeltaposSpin(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnApply();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PATCHDIALOG_H__DE62DFB4_E9EC_11D2_A509_0020AFEB881A__INCLUDED_)
