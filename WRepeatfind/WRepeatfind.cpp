// WRepeatfind.cpp : defines the class behaviors for the application

#include "stdafx.h"
#include <afxinet.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <locale.h>
#include <atomic>
#include "WRepeatfind.h"
#include "MainDlg.h"

#pragma comment(lib, "shlwapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CWRepeatfindApp, CWinApp)
END_MESSAGE_MAP()

CWRepeatfindApp::CWRepeatfindApp()
{
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
}

CWRepeatfindApp theApp;

BOOL CWRepeatfindApp::InitInstance()
{
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();
	AfxEnableControlContainer();

	SetRegistryKey(L"WRepeatfind");
	LoadSettings();

	int argc = 0;
	wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if(argv != nullptr)
	{
		bool commandLine = false;
		for(int i = 1; i < argc; i++)
			if((_wcsicmp(argv[i], L"/report") == 0) || (_wcsicmp(argv[i], L"/?") == 0)) commandLine = true;
		if(commandLine)
		{
			RunFromCommandLine(argc, argv);
			LocalFree(argv);
			return FALSE;
		}
		for(int i = 1; i < argc; i++)									// documents dropped onto the program's icon
		{
			if(i == 1) m_Documents.clear();
			if(IsSupportedDocument(argv[i]) && PathFileExistsW(argv[i])) m_Documents.push_back(argv[i]);
		}
		LocalFree(argv);
	}

	CMainDlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();
	SaveSettings();
	return FALSE;
}

// Function: RunFromCommandLine
// Purpose: Lets WRepeatfind run without its window, for batch use and testing:
//   WRepeatfind.exe /report <report.html> [/summary <summary.txt>] [/separate] [/phrase N] [/tolerance N] <documents...>
// The saved settings are used for everything not given on the command line.

int CWRepeatfindApp::RunFromCommandLine(int argc, wchar_t** argv)
{
	CRepeatFinder finder;
	finder.Settings = m_Settings;
	std::wstring report, summary;
	std::vector<std::wstring> documents;

	for(int i = 1; i < argc; i++)
	{
		if((_wcsicmp(argv[i], L"/report") == 0) && (i + 1 < argc)) report = argv[++i];
		else if((_wcsicmp(argv[i], L"/summary") == 0) && (i + 1 < argc)) summary = argv[++i];
		else if((_wcsicmp(argv[i], L"/phrase") == 0) && (i + 1 < argc)) finder.Settings.PhraseLength = _wtoi(argv[++i]);
		else if((_wcsicmp(argv[i], L"/tolerance") == 0) && (i + 1 < argc)) finder.Settings.MismatchTolerance = _wtoi(argv[++i]);
		else if(_wcsicmp(argv[i], L"/separate") == 0) finder.Settings.OneWork = false;
		else if(argv[i][0] != L'/') documents.push_back(argv[i]);
	}
	if(report.empty() || documents.empty())
	{
		AfxMessageBox(L"Usage: WRepeatfind.exe /report <report.html> [/summary <summary.txt>] [/separate] [/phrase N] [/tolerance N] <documents...>");
		return -1;
	}

	_wsetlocale(LC_ALL, m_Language);
	std::wstring log;
	int result = -1;
	for(const std::wstring& doc : documents)
	{
		result = finder.AddDocument(doc);
		if(result > -1)
		{
			log = doc + L": " + CRepeatFinder::ErrorMessage(result) + L"\r\n";
			break;
		}
	}
	if(result == -1) result = finder.FindRepeats();
	if(result == -1) result = finder.WriteReport(report, WREPEATFIND_NAME);
	if(result > -1 && log.empty()) log = CRepeatFinder::ErrorMessage(result) + L"\r\n";

	if(!summary.empty())
	{
		log += L"Words\t" + std::to_wstring(finder.WordsTotal()) + L"\r\nRepeated\t" + std::to_wstring(finder.RepeatedWords()) + L"\r\n";
		for(const RepeatGroup& g : finder.Groups())
		{
			log += std::to_wstring(g.Number) + L"\t" + std::to_wstring(g.Words) + L"\t" + std::to_wstring(g.Copies.size());
			for(const RepeatCopy& c : g.Copies)
				log += L"\t" + finder.CopyLocation(c, true) + L" [" + std::to_wstring(c.FirstWord) + L"-" + std::to_wstring(c.LastWord) + L"] " + finder.CopyText(c, 60);
			log += L"\r\n";
		}
		FILE* file = nullptr;
		if(_wfopen_s(&file, summary.c_str(), L"w, ccs=UTF-8") == 0 && file != nullptr)
		{
			fputws(log.c_str(), file);
			fclose(file);
		}
	}
	return result;
}

void CWRepeatfindApp::LoadSettings()
{
	const wchar_t* section = L"Settings";
	RepeatSettings d = DefaultSettings();
	m_Settings.PhraseLength = GetProfileInt(section, L"PhraseLength", d.PhraseLength);
	m_Settings.MismatchTolerance = GetProfileInt(section, L"MismatchTolerance", d.MismatchTolerance);
	m_Settings.MismatchPercentage = GetProfileInt(section, L"MismatchPercentage", d.MismatchPercentage);
	m_Settings.IgnoreCase = GetProfileInt(section, L"IgnoreCase", d.IgnoreCase) != 0;
	m_Settings.IgnorePunctuation = GetProfileInt(section, L"IgnorePunctuation", d.IgnorePunctuation) != 0;
	m_Settings.IgnoreOuterPunctuation = GetProfileInt(section, L"IgnoreOuterPunctuation", d.IgnoreOuterPunctuation) != 0;
	m_Settings.IgnoreNumbers = GetProfileInt(section, L"IgnoreNumbers", d.IgnoreNumbers) != 0;
	m_Settings.SkipLongWords = GetProfileInt(section, L"SkipLongWords", d.SkipLongWords) != 0;
	m_Settings.SkipLength = GetProfileInt(section, L"SkipLength", d.SkipLength);
	m_Settings.SkipNonwords = GetProfileInt(section, L"SkipNonwords", d.SkipNonwords) != 0;
	m_Settings.BasicCharacters = GetProfileInt(section, L"BasicCharacters", d.BasicCharacters) != 0;
	m_Settings.OneWork = GetProfileInt(section, L"OneWork", d.OneWork) != 0;
	m_Language = GetProfileString(section, L"Language", DefaultLanguage());
	m_ReportFolder = GetProfileString(section, L"ReportFolder", DefaultReportFolder());
	m_AutoOpenReport = GetProfileInt(section, L"AutoOpenReport", 1) != 0;

	m_Documents.clear();
	int count = GetProfileInt(L"Documents", L"Count", 0);
	for(int i = 0; i < count; i++)
	{
		CString name;
		name.Format(L"Document%d", i);
		CString path = GetProfileString(L"Documents", name, L"");
		if(!path.IsEmpty() && PathFileExistsW(path)) m_Documents.push_back(path);
	}
}

void CWRepeatfindApp::SaveSettings()
{
	const wchar_t* section = L"Settings";
	WriteProfileInt(section, L"PhraseLength", m_Settings.PhraseLength);
	WriteProfileInt(section, L"MismatchTolerance", m_Settings.MismatchTolerance);
	WriteProfileInt(section, L"MismatchPercentage", m_Settings.MismatchPercentage);
	WriteProfileInt(section, L"IgnoreCase", m_Settings.IgnoreCase);
	WriteProfileInt(section, L"IgnorePunctuation", m_Settings.IgnorePunctuation);
	WriteProfileInt(section, L"IgnoreOuterPunctuation", m_Settings.IgnoreOuterPunctuation);
	WriteProfileInt(section, L"IgnoreNumbers", m_Settings.IgnoreNumbers);
	WriteProfileInt(section, L"SkipLongWords", m_Settings.SkipLongWords);
	WriteProfileInt(section, L"SkipLength", m_Settings.SkipLength);
	WriteProfileInt(section, L"SkipNonwords", m_Settings.SkipNonwords);
	WriteProfileInt(section, L"BasicCharacters", m_Settings.BasicCharacters);
	WriteProfileInt(section, L"OneWork", m_Settings.OneWork);
	WriteProfileString(section, L"Language", m_Language);
	WriteProfileString(section, L"ReportFolder", m_ReportFolder);
	WriteProfileInt(section, L"AutoOpenReport", m_AutoOpenReport);

	WriteProfileString(L"Documents", nullptr, nullptr);					// clear the old list
	WriteProfileInt(L"Documents", L"Count", (int)m_Documents.size());
	for(size_t i = 0; i < m_Documents.size(); i++)
	{
		CString name;
		name.Format(L"Document%d", (int)i);
		WriteProfileString(L"Documents", name, m_Documents[i]);
	}
}

static CString KnownFolder(REFKNOWNFOLDERID id)
{
	CString result;
	PWSTR path = nullptr;
	if(SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &path))) result = path;
	CoTaskMemFree(path);
	return result;
}

CString CWRepeatfindApp::DefaultReportFolder()
{
	return KnownFolder(FOLDERID_Documents) + L"\\WRepeatfind Reports";
}

CString CWRepeatfindApp::PastedTextFolder()
{
	return KnownFolder(FOLDERID_LocalAppData) + L"\\WRepeatfind\\Pasted Text";
}

CString CWRepeatfindApp::ReportPath(const std::wstring& workTitle) const
{
	CString folder = m_ReportFolder.IsEmpty() ? DefaultReportFolder() : m_ReportFolder;
	SHCreateDirectoryExW(nullptr, folder, nullptr);

	CString name = workTitle.c_str();
	for(const wchar_t* bad = L"\\/:*?\"<>|"; *bad; bad++) name.Replace(*bad, L'_');
	return folder + L"\\Repeats in " + name + L".html";
}

// Function: OpenReport
// Purpose: Opens the report in the default browser, scrolled to a particular repeat if an anchor is given.
// Details: Windows drops the #anchor when a file is opened through its association, so when there is an
//			anchor the browser program is looked up and handed a file:// URL directly.

void CWRepeatfindApp::OpenReport(const CString& reportPath, const CString& anchor)
{
	if(!anchor.IsEmpty())
	{
		wchar_t browser[MAX_PATH];
		DWORD size = MAX_PATH;
		if(SUCCEEDED(AssocQueryStringW(ASSOCF_NONE, ASSOCSTR_EXECUTABLE, L".html", L"open", browser, &size)))
		{
			wchar_t url[4096];
			DWORD length = 4096;
			if(SUCCEEDED(UrlCreateFromPathW(reportPath, url, &length, 0)))
			{
				CString argument;
				argument.Format(L"\"%s#%s\"", url, (LPCWSTR)anchor);
				if((INT_PTR)ShellExecuteW(nullptr, L"open", browser, argument, nullptr, SW_SHOWNORMAL) > 32) return;
			}
		}
	}
	ShellExecuteW(nullptr, L"open", reportPath, nullptr, nullptr, SW_SHOWNORMAL);
}

bool CWRepeatfindApp::IsSupportedDocument(const CString& path)
{
	const wchar_t* ext = PathFindExtensionW(path);
	for(const wchar_t* ok : {L".txt", L".docx", L".doc", L".pdf", L".htm", L".html"})
		if(_wcsicmp(ext, ok) == 0) return true;
	return false;
}
