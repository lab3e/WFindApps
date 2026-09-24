// WCopyfind.h : main header file for the WCopyfind application

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include <vector>
#include "resource.h"

#define WCOPYFIND_VERSION L"6.0.0"
#ifdef _WIN64
#define WCOPYFIND_NAME L"WCopyfind " WCOPYFIND_VERSION
#else
#define WCOPYFIND_NAME L"WCopyfind " WCOPYFIND_VERSION L" (32-bit)"
#endif

struct CopyfindSettings
{
	int PhraseLength = 6;				// shortest phrase to match, in words
	int WordThreshold = 100;			// fewest matching words for a pair to be reported
	int MismatchTolerance = 0;			// most imperfections in a row to bridge
	int MismatchPercentage = 100;		// minimum percentage of perfectly matching words in an imperfect phrase
	int SkipLength = 20;
	bool IgnoreCase = false;
	bool IgnorePunctuation = false;
	bool IgnoreOuterPunctuation = false;
	bool IgnoreNumbers = false;
	bool SkipLongWords = false;
	bool SkipNonwords = false;
	bool BasicCharacters = false;
	bool BriefReport = false;
};

class CWCopyfindApp : public CWinApp
{
public:
	CWCopyfindApp();
	virtual BOOL InitInstance();

	// settings, remembered between sessions
	CopyfindSettings m_Settings;
	CString m_Language;
	CString m_ReportFolder;
	bool m_AutoOpenReport = true;
	bool m_KeepSorted[2] = {true, true};			// indexed by list: 0 = new documents, 1 = old documents
	std::vector<CString> m_Documents[2];

	void LoadSettings();
	void SaveSettings();
	static CString DefaultLanguage() { return L"English"; }
	static CString DefaultReportFolder();
	static const std::vector<CString>& Languages();

	static void OpenInBrowser(const CString& path);
	static bool IsDocumentType(const CString& path);

private:
	void MigrateVersion5Settings();

	DECLARE_MESSAGE_MAP()
};

extern CWCopyfindApp theApp;
