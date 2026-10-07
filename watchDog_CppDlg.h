
// watchDog_CppDlg.h : header file
//

#pragma once
#include "ProcessMoniter.h"
#include <deque>
#include <mutex>
#include <string>


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
	LRESULT OnTaskbarCreated(WPARAM wParam, LPARAM lParam);
	void handle_rbuttonup();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	LRESULT OnAppLog(WPARAM wParam, LPARAM lParam);
	CListBox m_listBOx;
	int m_count = 0;
	bool m_bMonitoring = false;
	// 图标当前是否应该存在于托盘（窗口被收进托盘时为 true）。
	// Explorer 重启后据此判断要不要重新 NIM_ADD
	bool m_bTrayIcon = false;

	// 监控线程只投递消息，真正刷新列表在 UI 线程完成
	std::mutex m_logMutex;
	std::deque<std::string> m_logQueue;
};
