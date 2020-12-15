
// watchDog_Cpp.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
#include "framework.h"
#include "watchDog_Cpp.h"
#include "watchDog_CppDlg.h"
#include "ProcessMoniter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CwatchDogCppApp

BEGIN_MESSAGE_MAP(CwatchDogCppApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CwatchDogCppApp construction

CwatchDogCppApp::CwatchDogCppApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CwatchDogCppApp object

CwatchDogCppApp theApp;
#include "myLogger.h"

// CwatchDogCppApp initialization

void windowsRun(char* exePath)
{
	//写入注册表,开机自启动
	HKEY hKey;
	//找到系统的启动项
	const char* lpRun = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
	//打开启动项Key
	long lRet = RegOpenKeyExA(HKEY_LOCAL_MACHINE, lpRun, 0, KEY_WRITE, &hKey);
	
	if (lRet == ERROR_SUCCESS)
	{
		//得到程序自身的全路径
		DWORD dwRet = strlen(exePath);
		char*p1 = strrchr(exePath, '\\');
		const char* saKey = p1 + 1;// ("anlyzeJson");
		//添加一个子Key,并设置值 // 下面的"getip"是应用程序名字（不加后缀.exe）
		lRet = RegSetValueExA(hKey, saKey, 0, REG_SZ, (BYTE*)exePath, dwRet);

		//关闭注册表
		RegCloseKey(hKey);
	}

	if (lRet != ERROR_SUCCESS)
	{
		//printf("系统参数错误,不能随系统启动");
		
		LOGE << "注册系统启动失败 ，请使用管理员权限运行";
		return;
	}

}
BOOL CwatchDogCppApp::InitInstance()
{

	HANDLE mutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE,"CwatchDogCppApp::InitInstance_202012150954");
	if (mutex == nullptr)
		mutex = CreateMutexA(nullptr, FALSE, "CwatchDogCppApp::InitInstance_202012150954");
	else
	{
		CloseHandle(mutex);
		return EXIT_FAILURE;
	}

	initLogger();
	LOGD << "############ CwatchDogCppDlg start ############";
	char path[255];
	GetModuleFileNameA(nullptr, path, 255);
	windowsRun(path);

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	CwatchDogCppDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
		TRACE(traceAppMsg, 0, "Warning: if you are using MFC controls on the dialog, you cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
	}

	// Delete the shell manager created above.
	if (pShellManager != nullptr)
	{
		delete pShellManager;
	}

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

