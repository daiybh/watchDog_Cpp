
// watchDog_CppDlg.h : header file
//

#pragma once
#include "ProcessMoniter.h"


// CwatchDogCppDlg dialog
class CwatchDogCppDlg : public CDialogEx
{
// Construction
public:
	CwatchDogCppDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WATCHDOG_CPP_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	ProcessMoniter pm;
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	void ToTray();
	void DeleteTray();
	LRESULT OnShowTask(WPARAM wParam, LPARAM lParam);
	void handle_rbuttonup();
	afx_msg void OnDestroy();
};
