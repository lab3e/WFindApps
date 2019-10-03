
// WCopyfind.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
const int WU_UITHREAD_START = WM_APP + 1;
const int WU_UITHREAD_TERMINATE = WM_APP + 2;
const int WU_UITHREAD_TERMINATED = WM_APP + 3;

// CWCopyfindApp:
// See WCopyfind.cpp for the implementation of this class
//

typedef struct
{
	HWND hWnd;
	CDialog* pDialog;
	int returnVal;
} thread_args;


class CWCopyfindApp : public CWinApp
{
public:
	CWCopyfindApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CWCopyfindApp theApp;