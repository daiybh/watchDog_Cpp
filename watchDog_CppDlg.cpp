
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
#define WM_APP_LOG     (WM_USER + 121)

// Explorer（任务栏）崩溃或重启后会向所有顶层窗口广播 TaskbarCreated，
// 此前 NIM_ADD 的托盘图标会被全部丢弃，必须在收到时重新添加。
// 注意：必须用 ON_REGISTERED_MESSAGE（消息 ID 是运行时才确定的），
// 而且该变量要定义在 BEGIN_MESSAGE_MAP 之前。
static const UINT g_uTaskbarCreatedMsg = RegisterWindowMessage(_T("TaskbarCreated"));

// CwatchDogCppDlg dialog



CwatchDogCppDlg::CwatchDogCppDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_WATCHDOG_CPP_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CwatchDogCppDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_listBOx);
}

BEGIN_MESSAGE_MAP(CwatchDogCppDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_MY_SHOWTASK, OnShowTask)
	ON_MESSAGE(WM_APP_LOG, OnAppLog)
	ON_REGISTERED_MESSAGE(g_uTaskbarCreatedMsg, OnTaskbarCreated)
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
	// 注意：日志回调会被监控线程调用，这里只能投递消息，不能直接操作 UI 控件
	pm.setLogCallback([this](std::string log) {
		{
			std::lock_guard<std::mutex> lock(m_logMutex);
			if (m_logQueue.size() >= 200)
				m_logQueue.pop_front();
			m_logQueue.push_back(std::move(log));
		}
		PostMessage(WM_APP_LOG);
		});
	SetDlgItemText(IDOK, _T("start"));

	SetTimer(1, 1000, nullptr);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// 由监控线程 PostMessage 触发，在 UI 线程里刷新列表框
LRESULT CwatchDogCppDlg::OnAppLog(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (!::IsWindow(m_listBOx.GetSafeHwnd()))
		return 0;

	std::deque<std::string> logs;
	{
		std::lock_guard<std::mutex> lock(m_logMutex);
		logs.swap(m_logQueue);
	}
	if (logs.empty())
		return 0;

	const CTime t = CTime::GetCurrentTime();
	for (const std::string &log : logs)
	{
		CString sLog(log.c_str());
		CString a;
		a.Format(_T("%d %s>> %s"), m_count++, (LPCTSTR)t.Format(_T("%D %H:%M:%S")), (LPCTSTR)sLog);
		m_listBOx.InsertString(0, a);   // 最新的在最上面
	}
	// 只保留最近 10 条（去掉 LBS_SORT 后，末尾的就是最旧的）
	while (m_listBOx.GetCount() > 10)
		m_listBOx.DeleteString(m_listBOx.GetCount() - 1);

	return 0;
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
	if (m_bMonitoring)
	{
		m_bMonitoring = false;
		SetDlgItemText(IDOK, _T("start"));
		pm.stopMoniter();
	}
	else {
		m_bMonitoring = true;
		SetDlgItemText(IDOK, _T("stop"));
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

// 填充托盘图标数据（ToTray / 任务栏重建后重加 共用同一份逻辑）
static void FillTrayData(NOTIFYICONDATA &nid, HWND hWnd)
{
	nid = {};
	nid.cbSize = DWORD(sizeof(NOTIFYICONDATA));
	nid.hWnd = hWnd;
	nid.uID = IDR_MAINFRAME;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_MY_SHOWTASK;
	nid.hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));
	_tcsncpy_s(nid.szTip, _countof(nid.szTip), _T("watchDog"), _TRUNCATE);
}

void CwatchDogCppDlg::ToTray()
{
	NOTIFYICONDATA nid;
	FillTrayData(nid, m_hWnd);

	// 用状态位而不是 IsWindowVisible() 判断：任务栏重建后要依据同一个状态位决定是否重加
	if (m_bTrayIcon)
		Shell_NotifyIcon(NIM_MODIFY, &nid);
	else
		m_bTrayIcon = Shell_NotifyIcon(NIM_ADD, &nid) ? true : false;

	ShowWindow(SW_HIDE);
}

void CwatchDogCppDlg::DeleteTray()
{
	if (!m_bTrayIcon)
		return;
	NOTIFYICONDATA nid;
	FillTrayData(nid, m_hWnd);
	Shell_NotifyIcon(NIM_DELETE, &nid);
	m_bTrayIcon = false;
}

// 任务栏（Explorer）重建：之前添加的图标已经丢了，按状态位重新添加
LRESULT CwatchDogCppDlg::OnTaskbarCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (!m_bTrayIcon)
		return 0;   // 窗口当前是显示状态，本来就没有图标

	NOTIFYICONDATA nid;
	FillTrayData(nid, m_hWnd);
	if (!Shell_NotifyIcon(NIM_ADD, &nid))
	{
		// 极少数情况下图标其实还在（NIM_ADD 会失败），改用 MODIFY 刷新
		if (!Shell_NotifyIcon(NIM_MODIFY, &nid))
			LOGW << "TaskbarCreated: refresh tray icon failed, err=" << GetLastError();
		// 重加失败时保持 m_bTrayIcon = true：下次 ToTray/再收到广播时会继续补
	}
	return 0;
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
	POINT lpoint = {};
	GetCursorPos(&lpoint);
	const int iSelectedMenuId = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, lpoint.x, lpoint.y, this);
	PostMessage(WM_NULL);   // 让弹出的菜单立刻消失
	switch (iSelectedMenuId)
	{
	case TRAY_DESTROY:
	{
		// 用 EndDialog 关闭模态对话框：会正常走 WM_DESTROY（移除托盘图标、停监控线程），
		// 原来的 PostQuitMessage(0) 可能让对话框窗口挂住不销毁
		EndDialog(IDCANCEL);
		break;
	}
	default:break;
	}
	// 这里不要调用 Detach()：Detach 之后 CMenu 析构不会再销毁菜单句柄，句柄会泄漏
}

void CwatchDogCppDlg::OnDestroy()
{
	KillTimer(1);
	pm.stopMoniter();
	DeleteTray();

	CDialogEx::OnDestroy();

}


void CwatchDogCppDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	if (nIDEvent == 1)
	{
		KillTimer(1);
		// 启动 1 秒后自动开始监控并收起窗口。
		// 原来这里调用 OnBnClickedOk()（切换按钮状态），如果用户在第 1 秒内已经手动点过，
		// 就会被反向切换成“停止”，监控反而没启动
		if (!m_bMonitoring)
		{
			m_bMonitoring = true;
			SetDlgItemText(IDOK, _T("stop"));
			pm.startMoniter();
		}
		ToTray();
	}
	CDialogEx::OnTimer(nIDEvent);
}
