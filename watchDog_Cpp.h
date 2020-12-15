
// watchDog_Cpp.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CwatchDogCppApp:
// See watchDog_Cpp.cpp for the implementation of this class
//

class CwatchDogCppApp : public CWinApp
{
public:
	CwatchDogCppApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CwatchDogCppApp theApp;
