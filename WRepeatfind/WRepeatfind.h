// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Louis A. Bloomfield
// WRepeatfind.h : main header file for the WRepeatfind application

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include <vector>
#include "resource.h"
#include "RepeatFinder.h"

#define WREPEATFIND_NAME L"WRepeatfind 1.0.0"

class CWRepeatfindApp : public CWinApp
{
public:
	CWRepeatfindApp();
	virtual BOOL InitInstance();

	// settings, remembered between sessions
	RepeatSettings m_Settings;
	CString m_Language;
	CString m_ReportFolder;
	bool m_AutoOpenReport = true;
	std::vector<CString> m_Documents;

	void LoadSettings();
	void SaveSettings();
	static RepeatSettings DefaultSettings() { return RepeatSettings(); }
	static CString DefaultLanguage() { return L"English"; }
	static CString DefaultReportFolder();
	static CString PastedTextFolder();

	CString ReportPath(const std::wstring& workTitle) const;
	static void OpenReport(const CString& reportPath, const CString& anchor);
	static bool IsSupportedDocument(const CString& path);

private:
	int RunFromCommandLine(int argc, wchar_t** argv);

	DECLARE_MESSAGE_MAP()
};

extern CWRepeatfindApp theApp;
