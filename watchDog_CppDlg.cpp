
// watchDog_CppDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "watchDog_Cpp.h"
#include "watchDog_CppDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_MY_SHOWTASK (WM_USER + 120)

// CwatchDogCppDlg dialog



CwatchDogCppDlg::CwatchDogCppDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_WATCHDOG_CPP_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CwatchDogCppDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CwatchDogCppDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_MY_SHOWTASK, OnShowTask)
	ON_BN_CLICKED(IDOK, &CwatchDogCppDlg::OnBnClickedOk)
	ON_WM_SYSCOMMAND()
	ON_WM_DESTROY()
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CwatchDogCppDlg message handlers
#include "myLogger.h"
BOOL CwatchDogCppDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	SetTimer(1, 1000, nullptr);
	pm.startMoniter();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CwatchDogCppDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CwatchDogCppDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CwatchDogCppDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	CString text;
	GetDlgItemText(IDOK,text);
	if (text == "start")
	{
		SetDlgItemText(IDOK, "stop");
		pm.stopMoniter();
	}
	else {
		SetDlgItemText(IDOK, "start");
		pm.startMoniter();
	}
}


void CwatchDogCppDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// TODO: Add your message handler code here and/or call default
	if (nID == SC_MINIMIZE || nID == SC_CLOSE)
	{
		ToTray();
	}
	else
		CDialogEx::OnSysCommand(nID, lParam);
}

void CwatchDogCppDlg::ToTray()
{
	NOTIFYICONDATA nid;
	nid.cbSize = DWORD(sizeof(NOTIFYICONDATA));
	nid.hWnd = this->m_hWnd;
	nid.uID = IDR_MAINFRAME;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_MY_SHOWTASK;
	nid.hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));
	CString strTip;
	strTip.Format("watchDog");
	strcpy(nid.szTip,  strTip);

	if (IsWindowVisible())
		Shell_NotifyIcon(NIM_ADD, &nid);
	else
		Shell_NotifyIcon(NIM_MODIFY, &nid);
	ShowWindow(SW_HIDE);
}

void CwatchDogCppDlg::DeleteTray()
{
	NOTIFYICONDATA nid;
	nid.cbSize = DWORD(sizeof(NOTIFYICONDATA));
	nid.hWnd = this->m_hWnd;
	nid.uID = IDR_MAINFRAME;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_MY_SHOWTASK;
	nid.hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));
	Shell_NotifyIcon(NIM_DELETE, &nid);
}

LRESULT CwatchDogCppDlg::OnShowTask(WPARAM wParam, LPARAM lParam)
{
	if (wParam != IDR_MAINFRAME)
		return 1;
	switch (lParam)
	{
	case WM_RBUTTONUP:
	{
		handle_rbuttonup();
		break;
	}
	case WM_LBUTTONDOWN:
	{
		this->ShowWindow(SW_SHOW);
		DeleteTray();
		break;
	}
	default:
		break;
	}
	return 0;
}

enum TrayCmd
{
	TRAY_OPEN_LOG_FOLDER = 100,
	TRAY_OPEN_CONFIG_FOLDER,
	TRAY_OPEN_SETUP_FOLDER,
	TRAY_DESTROY,
	TRAY_HIDE_BACKEND,
	TRAY_DISPLAY_BACKEND,
	TRAY_SHUTDOWN_BACKEND,
	TRAY_START_BACKEND,
	TRAY_FORMAT_BACKEND,
	TRAY_CLEAN_LOGS_AND_START_BACKEND,
	TRAY_CLEAN_LOGS,
	TRAY_OPEN_WEBCONFIG,
	TRAY_OPEN_DOCKER_FOLDER,
	TRAY_OPEN_RESTSERVICE_FOLDER
};
void CwatchDogCppDlg::handle_rbuttonup()
{
	CMenu menu;
	if (!menu.CreatePopupMenu())
		return;
	
	menu.AppendMenu(MF_STRING, TRAY_DESTROY, _T("Exit"));

	SetForegroundWindow();
	const std::unique_ptr<POINT> lpoint = std::make_unique<POINT>();
	GetCursorPos(lpoint.get());
	const TrayCmd iSelectedMenuId = static_cast<TrayCmd>(menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, lpoint->x, lpoint->y, this));
	switch (iSelectedMenuId)
	{
	case TRAY_DESTROY:
	{
		PostQuitMessage(0);
		break;
	}
	default:break;
	}
	menu.Detach();
	menu.DestroyMenu();
}

void CwatchDogCppDlg::OnDestroy()
{
	pm.stopMoniter();
	DeleteTray();

	CDialogEx::OnDestroy();

}


void CwatchDogCppDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	ToTray();
	KillTimer(1);
	CDialogEx::OnTimer(nIDEvent);
}
