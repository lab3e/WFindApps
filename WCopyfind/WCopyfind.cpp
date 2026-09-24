// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Louis A. Bloomfield
// WCopyfind.cpp : defines the class behaviors for the application

#include "stdafx.h"
#include <afxinet.h>
#include <shlobj.h>
#include <shlwapi.h>
#include "WCopyfind.h"
#include "WCopyfindDlg.h"

#pragma comment(lib, "shlwapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CWCopyfindApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

CWCopyfindApp::CWCopyfindApp()
{
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
}

CWCopyfindApp theApp;

BOOL CWCopyfindApp::InitInstance()
{
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES | ICC_LINK_CLASS;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();
	AfxEnableControlContainer();
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);		// for resolving dropped shortcuts and for the .doc reader

	SetRegistryKey(L"WCopyfind");
	LoadSettings();

	CWCopyfindDlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();
	SaveSettings();

	CoUninitialize();
	return FALSE;
}

// Settings live under HKEY_CURRENT_USER\Software\WCopyfind\WCopyfind. Versions up to 5.0.0 kept theirs as values
// directly under HKEY_CURRENT_USER\Software\WCopyfind; they are carried over the first time 6.0.0 runs.

void CWCopyfindApp::LoadSettings()
{
	const wchar_t* section = L"Settings";
	if(GetProfileInt(section, L"PhraseLength", -1) == -1) MigrateVersion5Settings();

	CopyfindSettings& s = m_Settings;
	s.PhraseLength = GetProfileInt(section, L"PhraseLength", s.PhraseLength);
	s.WordThreshold = GetProfileInt(section, L"WordThreshold", s.WordThreshold);
	s.MismatchTolerance = GetProfileInt(section, L"MismatchTolerance", s.MismatchTolerance);
	s.MismatchPercentage = GetProfileInt(section, L"MismatchPercentage", s.MismatchPercentage);
	s.SkipLength = GetProfileInt(section, L"SkipLength", s.SkipLength);
	s.IgnoreCase = GetProfileInt(section, L"IgnoreCase", s.IgnoreCase) != 0;
	s.IgnorePunctuation = GetProfileInt(section, L"IgnorePunctuation", s.IgnorePunctuation) != 0;
	s.IgnoreOuterPunctuation = GetProfileInt(section, L"IgnoreOuterPunctuation", s.IgnoreOuterPunctuation) != 0;
	s.IgnoreNumbers = GetProfileInt(section, L"IgnoreNumbers", s.IgnoreNumbers) != 0;
	s.SkipLongWords = GetProfileInt(section, L"SkipLongWords", s.SkipLongWords) != 0;
	s.SkipNonwords = GetProfileInt(section, L"SkipNonwords", s.SkipNonwords) != 0;
	s.BasicCharacters = GetProfileInt(section, L"BasicCharacters", s.BasicCharacters) != 0;
	s.BriefReport = GetProfileInt(section, L"BriefReport", s.BriefReport) != 0;
	if(m_Language.IsEmpty()) m_Language = DefaultLanguage();
	m_Language = GetProfileString(section, L"Language", m_Language);
	if(m_ReportFolder.IsEmpty()) m_ReportFolder = DefaultReportFolder();
	m_ReportFolder = GetProfileString(section, L"ReportFolder", m_ReportFolder);
	m_AutoOpenReport = GetProfileInt(section, L"AutoOpenReport", 1) != 0;

	const wchar_t* listSections[2] = {L"NewDocuments", L"OldDocuments"};
	for(int list = 0; list < 2; list++)
	{
		m_KeepSorted[list] = GetProfileInt(listSections[list], L"KeepSorted", 1) != 0;
		m_Documents[list].clear();
		int count = GetProfileInt(listSections[list], L"Count", 0);
		for(int i = 0; i < count; i++)
		{
			CString name;
			name.Format(L"Document%d", i);
			CString path = GetProfileString(listSections[list], name, L"");
			if(!path.IsEmpty()) m_Documents[list].push_back(path);
		}
	}
}

void CWCopyfindApp::SaveSettings()
{
	const wchar_t* section = L"Settings";
	const CopyfindSettings& s = m_Settings;
	WriteProfileInt(section, L"PhraseLength", s.PhraseLength);
	WriteProfileInt(section, L"WordThreshold", s.WordThreshold);
	WriteProfileInt(section, L"MismatchTolerance", s.MismatchTolerance);
	WriteProfileInt(section, L"MismatchPercentage", s.MismatchPercentage);
	WriteProfileInt(section, L"SkipLength", s.SkipLength);
	WriteProfileInt(section, L"IgnoreCase", s.IgnoreCase);
	WriteProfileInt(section, L"IgnorePunctuation", s.IgnorePunctuation);
	WriteProfileInt(section, L"IgnoreOuterPunctuation", s.IgnoreOuterPunctuation);
	WriteProfileInt(section, L"IgnoreNumbers", s.IgnoreNumbers);
	WriteProfileInt(section, L"SkipLongWords", s.SkipLongWords);
	WriteProfileInt(section, L"SkipNonwords", s.SkipNonwords);
	WriteProfileInt(section, L"BasicCharacters", s.BasicCharacters);
	WriteProfileInt(section, L"BriefReport", s.BriefReport);
	WriteProfileString(section, L"Language", m_Language);
	WriteProfileString(section, L"ReportFolder", m_ReportFolder);
	WriteProfileInt(section, L"AutoOpenReport", m_AutoOpenReport);

	const wchar_t* listSections[2] = {L"NewDocuments", L"OldDocuments"};
	for(int list = 0; list < 2; list++)
	{
		WriteProfileString(listSections[list], nullptr, nullptr);			// clear the old list
		WriteProfileInt(listSections[list], L"KeepSorted", m_KeepSorted[list]);
		WriteProfileInt(listSections[list], L"Count", (int)m_Documents[list].size());
		for(size_t i = 0; i < m_Documents[list].size(); i++)
		{
			CString name;
			name.Format(L"Document%d", (int)i);
			WriteProfileString(listSections[list], name, m_Documents[list][i]);
		}
	}
}

// Function: MigrateVersion5Settings
// Purpose: Reads the settings WCopyfind 5.0.0 and earlier left in the registry. Those versions stored their two
//			text settings as ANSI bytes inside a Unicode registry value, and a bug made their first-run defaults
//			a single character ("C" for the report folder), so text settings are used only if they make sense.

void CWCopyfindApp::MigrateVersion5Settings()
{
	HKEY key;
	if(RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\WCopyfind", 0, KEY_READ, &key) != ERROR_SUCCESS) return;

	auto readInt = [&](const wchar_t* name, int& value)
	{
		DWORD data = 0, size = sizeof(data), type = 0;
		if(RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<BYTE*>(&data), &size) == ERROR_SUCCESS && type == REG_DWORD) value = (int)data;
	};
	auto readBool = [&](const wchar_t* name, bool& value)
	{
		int v = value;
		readInt(name, v);
		value = v != 0;
	};
	auto readText = [&](const wchar_t* name) -> CString
	{
		char bytes[1024] = {0};
		DWORD size = sizeof(bytes) - 2, type = 0;
		if(RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<BYTE*>(bytes), &size) != ERROR_SUCCESS) return CString();
		return CString(CStringA(bytes, (int)strnlen(bytes, size)));
	};

	CopyfindSettings& s = m_Settings;
	readInt(L"Phrase_Length", s.PhraseLength);
	readInt(L"Report_Threshold", s.WordThreshold);
	readInt(L"Tolerance", s.MismatchTolerance);
	readInt(L"Percentage", s.MismatchPercentage);
	readInt(L"Skip_Length", s.SkipLength);
	readBool(L"Ignore_Punctuation", s.IgnorePunctuation);
	readBool(L"Ignore_Outer_Punctuation", s.IgnoreOuterPunctuation);
	readBool(L"Ignore_Numbers", s.IgnoreNumbers);
	readBool(L"Ignore_Case", s.IgnoreCase);
	readBool(L"Skip_Long_Words", s.SkipLongWords);
	readBool(L"Skip_Nonwords", s.SkipNonwords);
	readBool(L"Basic_Characters", s.BasicCharacters);
	readBool(L"Brief_Report", s.BriefReport);

	CString folder = readText(L"Report_Folder");
	if(folder.GetLength() > 3 && PathIsDirectoryW(folder)) m_ReportFolder = folder;
	CString language = readText(L"Language");
	for(const CString& known : Languages()) if(language.CompareNoCase(known) == 0) m_Language = known;

	RegCloseKey(key);
}

CString CWCopyfindApp::DefaultReportFolder()
{
	CString result;
	PWSTR path = nullptr;
	if(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &path))) result = path;
	CoTaskMemFree(path);
	return result + L"\\WCopyfind Reports";
}

const std::vector<CString>& CWCopyfindApp::Languages()
{
	static const std::vector<CString> languages = {
		L"Chinese", L"Chinese-Simplified", L"Chinese-Traditional", L"Czech", L"Danish", L"Dutch", L"Dutch-Belgian",
		L"English", L"English-American", L"English-Aus", L"English-Can", L"English-Nz", L"English-Uk", L"Finnish",
		L"French", L"French-Belgian", L"French-Canadian", L"French-Swiss", L"German", L"German-Austrian", L"German-Swiss",
		L"Greek", L"Hungarian", L"Icelandic", L"Italian", L"Italian-Swiss", L"Japanese", L"Korean", L"Norwegian",
		L"Norwegian-Bokmal", L"Norwegian-Nynorsk", L"Polish", L"Portuguese", L"Portuguese-Brazilian", L"Russian",
		L"Slovak", L"Spanish", L"Spanish-Mexican", L"Spanish-Modern", L"Swedish", L"Turkish" };
	return languages;
}

void CWCopyfindApp::OpenInBrowser(const CString& path)
{
	ShellExecuteW(nullptr, L"open", path, nullptr, nullptr, SW_SHOWNORMAL);
}

// Function: IsDocumentType
// Purpose: The file types WCopyfind reads well; used when a dropped folder is expanded. Files chosen one at a time
//			may be of any type, as they always could be.

bool CWCopyfindApp::IsDocumentType(const CString& path)
{
	const wchar_t* ext = PathFindExtensionW(path);
	for(const wchar_t* ok : {L".txt", L".docx", L".doc", L".pdf", L".htm", L".html", L".url"})
		if(_wcsicmp(ext, ok) == 0) return true;
	return false;
}
