// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Louis A. Bloomfield
// MainDlg.cpp : the main WRepeatfind window

#include "stdafx.h"
#include <afxinet.h>
#include <shlwapi.h>
#include <locale.h>
#include <algorithm>
#include "WRepeatfind.h"
#include "MainDlg.h"
#include "PasteDlg.h"
#include "OptionsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ---------------------------------------------------------------------------------------------------------
// About box

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}

protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnLinkClick(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	ON_NOTIFY(NM_CLICK, IDC_LINK_WEB, OnLinkClick)
	ON_NOTIFY(NM_RETURN, IDC_LINK_WEB, OnLinkClick)
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetDlgItemText(IDC_STATIC_VERSION, WREPEATFIND_NAME);
	SetDlgItemText(IDC_EDIT_ABOUT,
		L"WRepeatfind finds the phrases that a document repeats, so writers can catch accidental repetition, "
		L"such as a passage that was moved while editing but whose original was never deleted. It checks a single "
		L"document or a book made of chapter files, and uses the comparison approach of its companion program, "
		L"WCopyfind.\r\n\r\n"
		L"This program is free software: you can redistribute it and/or modify it under the terms of the GNU General "
		L"Public License as published by the Free Software Foundation, either version 3 of the License, or (at your "
		L"option) any later version.\r\n\r\n"
		L"This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the "
		L"implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License "
		L"for more details. You should have received a copy of the GNU General Public License along with this program. "
		L"If not, see https://www.gnu.org/licenses/.\r\n\r\n"
		L"Source code: https://github.com/lab3e/WFindApps\r\n\r\n"
		L"If you have suggestions or find a problem, please let me know through the web site.\r\n\r\n"
		L"WRepeatfind reads .docx files with miniz, by Rich Geldreich and contributors (MIT License).");
	return TRUE;
}

void CAboutDlg::OnLinkClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	PNMLINK link = reinterpret_cast<PNMLINK>(pNMHDR);
	ShellExecuteW(m_hWnd, L"open", link->item.szUrl, nullptr, nullptr, SW_SHOWNORMAL);
	*pResult = 0;
}

// ---------------------------------------------------------------------------------------------------------
// The window

CMainDlg::CMainDlg(CWnd* pParent)
	: CDialogEx(IDD_WREPEATFIND_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_DOCS, m_ListDocs);
	DDX_Control(pDX, IDC_LIST_RESULTS, m_ListResults);
	DDX_Control(pDX, IDC_SPIN_PHRASE, m_SpinPhrase);
	DDX_Control(pDX, IDC_SPIN_TOLERANCE, m_SpinTolerance);
	DDX_Control(pDX, IDC_PROGRESS, m_Progress);
}

BEGIN_MESSAGE_MAP(CMainDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_DROPFILES()
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_PASTE, OnButtonPaste)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON_UP, OnButtonUp)
	ON_BN_CLICKED(IDC_BUTTON_DOWN, OnButtonDown)
	ON_BN_CLICKED(IDC_BUTTON_OPTIONS, OnButtonOptions)
	ON_BN_CLICKED(IDC_BUTTON_FIND, OnButtonFind)
	ON_BN_CLICKED(IDC_BUTTON_REPORT, OnButtonReport)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_DOCS, OnDocsItemChanged)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_DOCS, OnDocsDblClick)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_RESULTS, OnResultsDblClick)
	ON_MESSAGE(WU_PROGRESS, OnProgress)
	ON_MESSAGE(WU_DONE, OnDone)
END_MESSAGE_MAP()

BOOL CMainDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if(pSysMenu != nullptr)
	{
		CString about;
		about.LoadString(IDS_ABOUTBOX);
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, about);
	}
	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);
	SetWindowText(WREPEATFIND_NAME);

	LOGFONT lf;
	GetFont()->GetLogFont(&lf);
	lf.lfWeight = FW_BOLD;
	lf.lfHeight = lf.lfHeight * 11 / 10;
	m_BoldFont.CreateFontIndirect(&lf);
	for(int id : {IDC_STATIC_STEP1, IDC_STATIC_STEP2, IDC_STATIC_STEP3}) GetDlgItem(id)->SetFont(&m_BoldFont);

	m_ListDocs.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	m_ListResults.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	CRect r;
	m_ListDocs.GetClientRect(&r);
	m_ListDocs.InsertColumn(0, L"Document", LVCFMT_LEFT, r.Width() * 45 / 100);
	m_ListDocs.InsertColumn(1, L"Folder", LVCFMT_LEFT, r.Width() * 55 / 100 - GetSystemMetrics(SM_CXVSCROLL));
	m_ListResults.GetClientRect(&r);
	int unit = r.Width() / 100;
	m_ListResults.InsertColumn(0, L"#", LVCFMT_RIGHT, unit * 6);
	m_ListResults.InsertColumn(1, L"Words", LVCFMT_RIGHT, unit * 8);
	m_ListResults.InsertColumn(2, L"Copies", LVCFMT_RIGHT, unit * 8);
	m_ListResults.InsertColumn(3, L"Where", LVCFMT_LEFT, unit * 28);
	m_ListResults.InsertColumn(4, L"Begins with", LVCFMT_LEFT, r.Width() - unit * 50 - GetSystemMetrics(SM_CXVSCROLL));

	m_SpinPhrase.SetRange32(2, 200);
	m_SpinTolerance.SetRange32(0, 10);

	m_Layout.Init(this);
	for(int id : {IDC_STATIC_HINT1}) m_Layout.Add(id, CDialogLayout::StretchX);
	m_Layout.Add(IDC_LIST_DOCS, CDialogLayout::StretchX);
	for(int id : {IDC_BUTTON_ADD, IDC_BUTTON_PASTE, IDC_BUTTON_REMOVE, IDC_BUTTON_UP, IDC_BUTTON_DOWN, IDC_BUTTON_OPTIONS})
		m_Layout.Add(id, CDialogLayout::TopRight);
	m_Layout.Add(IDC_STATIC_STATUS, CDialogLayout::StretchX);
	m_Layout.Add(IDC_PROGRESS, CDialogLayout::StretchX);
	m_Layout.Add(IDC_LIST_RESULTS, CDialogLayout::StretchXY);
	m_Layout.Add(IDC_STATIC_HINT3, CDialogLayout::StretchXBottom);
	m_Layout.Add(IDC_BUTTON_REPORT, CDialogLayout::BottomRight);

	DragAcceptFiles(TRUE);
	ChangeWindowMessageFilterEx(m_hWnd, WM_DROPFILES, MSGFLT_ALLOW, nullptr);	// allow drops from Explorer if run elevated
	ChangeWindowMessageFilterEx(m_hWnd, WM_COPYDATA, MSGFLT_ALLOW, nullptr);
	ChangeWindowMessageFilterEx(m_hWnd, 0x0049 /* WM_COPYGLOBALDATA */, MSGFLT_ALLOW, nullptr);

	WriteControls();
	RefreshDocuments({});
	SetDlgItemText(IDC_STATIC_STATUS, theApp.m_Documents.empty() ? L"Add a document to get started." : L"Ready.");
	UpdateButtons();
	GetDlgItem(IDC_BUTTON_FIND)->SetFocus();
	return FALSE;
}

void CMainDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg about;
		about.DoModal();
	}
	else CDialogEx::OnSysCommand(nID, lParam);
}

void CMainDlg::OnPaint()
{
	if(IsIconic())
	{
		CPaintDC dc(this);
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - GetSystemMetrics(SM_CXICON) + 1) / 2;
		int y = (rect.Height() - GetSystemMetrics(SM_CYICON) + 1) / 2;
		dc.DrawIcon(x, y, m_hIcon);
	}
	else CDialogEx::OnPaint();
}

HCURSOR CMainDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMainDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	if(nType == SIZE_MINIMIZED || !m_Layout.IsReady()) return;
	m_Layout.Resize();

	for(CListCtrl* list : {&m_ListDocs, &m_ListResults})		// let the last column take up the extra width
	{
		CHeaderCtrl* header = list->GetHeaderCtrl();
		int columns = header ? header->GetItemCount() : 0;
		if(columns < 2) continue;
		CRect client;
		list->GetClientRect(&client);
		int used = 0;
		for(int c = 0; c < columns - 1; c++) used += list->GetColumnWidth(c);
		list->SetColumnWidth(columns - 1, (std::max)(80, client.Width() - used));
	}
}

void CMainDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	if(m_Layout.IsReady())
	{
		lpMMI->ptMinTrackSize.x = m_Layout.MinTrackSize().cx;
		lpMMI->ptMinTrackSize.y = m_Layout.MinTrackSize().cy;
	}
	CDialogEx::OnGetMinMaxInfo(lpMMI);
}

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
	if((pMsg->message == WM_KEYDOWN) && (pMsg->hwnd == m_ListDocs.m_hWnd) && !m_Running)
	{
		if(pMsg->wParam == VK_DELETE)
		{
			OnButtonRemove();
			return TRUE;
		}
		if((pMsg->wParam == 'A') && (GetKeyState(VK_CONTROL) < 0))
		{
			for(int i = 0; i < m_ListDocs.GetItemCount(); i++) m_ListDocs.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
			return TRUE;
		}
	}
	if((pMsg->message == WM_KEYDOWN) && (pMsg->hwnd == m_ListResults.m_hWnd) && (pMsg->wParam == VK_RETURN))
	{
		LRESULT result;
		OnResultsDblClick(nullptr, &result);
		return TRUE;
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

void CMainDlg::OnOK()
{
	// Enter is handled by the default button (Find Repeats); don't close the window
}

void CMainDlg::OnCancel()
{
	if(m_Running)
	{
		if(AfxMessageBox(L"WRepeatfind is still looking for repeats. Stop and close?", MB_YESNO | MB_ICONQUESTION) != IDYES) return;
		StopWorker();
	}
	ReadControls();
	CDialogEx::OnCancel();
}

// ---------------------------------------------------------------------------------------------------------
// Document list

void CMainDlg::OnDropFiles(HDROP hDropInfo)
{
	std::vector<CString> paths;
	UINT count = DragQueryFileW(hDropInfo, 0xFFFFFFFF, nullptr, 0);
	for(UINT i = 0; i < count; i++)
	{
		UINT length = DragQueryFileW(hDropInfo, i, nullptr, 0);
		CString path;
		DragQueryFileW(hDropInfo, i, path.GetBuffer(length + 1), length + 1);
		path.ReleaseBuffer();
		paths.push_back(path);
	}
	DragFinish(hDropInfo);
	if(!m_Running) AddPaths(paths, true);
}

void CMainDlg::OnButtonAdd()
{
	static const wchar_t filter[] =
		L"Documents (*.docx;*.doc;*.txt;*.pdf;*.htm;*.html)|*.docx;*.doc;*.txt;*.pdf;*.htm;*.html|"
		L"Word Documents (*.docx;*.doc)|*.docx;*.doc|Text Files (*.txt)|*.txt|PDF Files (*.pdf)|*.pdf|"
		L"Web Pages (*.htm;*.html)|*.htm;*.html|All Files (*.*)|*.*||";
	CFileDialog dlg(TRUE, nullptr, nullptr, OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER, filter, this);
	std::vector<wchar_t> buffer(256 * 1024, 0);
	dlg.m_ofn.lpstrFile = buffer.data();
	dlg.m_ofn.nMaxFile = (DWORD)buffer.size();
	dlg.m_ofn.lpstrTitle = L"Add Documents";
	if(dlg.DoModal() != IDOK) return;

	std::vector<CString> paths;
	POSITION pos = dlg.GetStartPosition();
	while(pos != nullptr) paths.push_back(dlg.GetNextPathName(pos));
	AddPaths(paths, false);
}

void CMainDlg::OnButtonPaste()
{
	CPasteDlg dlg(this);
	if(dlg.DoModal() != IDOK) return;
	AddPaths({dlg.m_SavedPath}, false);
}

// Function: AddPaths
// Purpose: Adds documents to the list, skipping duplicates and unsupported files. Each batch is sorted in
//			natural order ("Chapter 2" before "Chapter 10") and a dropped folder contributes its documents.

void CMainDlg::AddPaths(std::vector<CString> paths, bool expandFolders)
{
	std::vector<CString> files;
	int unsupported = 0;
	for(const CString& path : paths)
	{
		if(expandFolders && PathIsDirectoryW(path))
		{
			CFileFind finder;
			BOOL more = finder.FindFile(path + L"\\*.*");
			while(more)
			{
				more = finder.FindNextFile();
				if(finder.IsDirectory() || finder.IsDots()) continue;
				if(CWRepeatfindApp::IsSupportedDocument(finder.GetFilePath())) files.push_back(finder.GetFilePath());
			}
		}
		else if(CWRepeatfindApp::IsSupportedDocument(path)) files.push_back(path);
		else unsupported++;
	}
	std::sort(files.begin(), files.end(), [](const CString& a, const CString& b) { return StrCmpLogicalW(a, b) < 0; });

	std::vector<CString>& docs = theApp.m_Documents;
	std::vector<bool> selected(docs.size(), false);
	for(const CString& file : files)
	{
		auto existing = std::find_if(docs.begin(), docs.end(), [&](const CString& d) { return d.CompareNoCase(file) == 0; });
		if(existing != docs.end())
		{
			selected[existing - docs.begin()] = true;
			continue;
		}
		docs.push_back(file);
		selected.push_back(true);
	}
	RefreshDocuments(selected);

	if(unsupported > 0)
		SetDlgItemText(IDC_STATIC_STATUS, L"Some files were skipped. WRepeatfind reads .docx, .doc, .txt, .pdf, and .htm/.html files.");
	else if(!docs.empty()) SetDlgItemText(IDC_STATIC_STATUS, L"Ready.");
}

void CMainDlg::RefreshDocuments(const std::vector<bool>& selected)
{
	CString pastedFolder = CWRepeatfindApp::PastedTextFolder();
	m_ListDocs.SetRedraw(FALSE);
	m_ListDocs.DeleteAllItems();
	const std::vector<CString>& docs = theApp.m_Documents;
	for(int i = 0; i < (int)docs.size(); i++)
	{
		CString folder = docs[i];
		PathRemoveFileSpecW(folder.GetBuffer());
		folder.ReleaseBuffer();
		CString name = PathFindFileNameW(docs[i]);
		if(folder.CompareNoCase(pastedFolder) == 0)
		{
			PathRemoveExtensionW(name.GetBuffer());
			name.ReleaseBuffer();
			folder = L"(pasted text)";
		}
		m_ListDocs.InsertItem(i, name);
		m_ListDocs.SetItemText(i, 1, folder);
		if((i < (int)selected.size()) && selected[i]) m_ListDocs.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
	}
	for(int i = 0; i < (int)selected.size(); i++)
		if(selected[i])
		{
			m_ListDocs.EnsureVisible(i, FALSE);
			break;
		}
	m_ListDocs.SetRedraw(TRUE);
	UpdateButtons();
}

std::vector<bool> CMainDlg::SelectedDocuments() const
{
	std::vector<bool> selected(m_ListDocs.GetItemCount(), false);
	for(int i = 0; i < (int)selected.size(); i++) selected[i] = (m_ListDocs.GetItemState(i, LVIS_SELECTED) & LVIS_SELECTED) != 0;
	return selected;
}

void CMainDlg::OnButtonRemove()
{
	std::vector<bool> selected = SelectedDocuments();
	std::vector<CString>& docs = theApp.m_Documents;
	int firstRemoved = -1;
	for(int i = (int)docs.size() - 1; i >= 0; i--)
		if(selected[i])
		{
			docs.erase(docs.begin() + i);
			firstRemoved = i;
		}
	if(firstRemoved < 0) return;
	std::vector<bool> keep(docs.size(), false);
	if(!docs.empty()) keep[(std::min)(firstRemoved, (int)docs.size() - 1)] = true;		// select a neighbor so Delete can repeat
	RefreshDocuments(keep);
	m_ListDocs.SetFocus();
}

void CMainDlg::MoveSelection(int direction)
{
	std::vector<bool> selected = SelectedDocuments();
	std::vector<CString>& docs = theApp.m_Documents;
	int n = (int)docs.size();
	if(direction < 0)
	{
		for(int i = 1; i < n; i++)
			if(selected[i] && !selected[i - 1])
			{
				std::swap(docs[i], docs[i - 1]);
				std::vector<bool>::swap(selected[i], selected[i - 1]);
			}
	}
	else
	{
		for(int i = n - 2; i >= 0; i--)
			if(selected[i] && !selected[i + 1])
			{
				std::swap(docs[i], docs[i + 1]);
				std::vector<bool>::swap(selected[i], selected[i + 1]);
			}
	}
	RefreshDocuments(selected);
}

void CMainDlg::OnButtonUp() { MoveSelection(-1); }
void CMainDlg::OnButtonDown() { MoveSelection(1); }

void CMainDlg::OnDocsItemChanged(NMHDR*, LRESULT* pResult)
{
	UpdateButtons();
	*pResult = 0;
}

void CMainDlg::OnDocsDblClick(NMHDR*, LRESULT* pResult)
{
	int item = m_ListDocs.GetNextItem(-1, LVNI_SELECTED);
	if(item >= 0 && item < (int)theApp.m_Documents.size())
		ShellExecuteW(m_hWnd, L"open", theApp.m_Documents[item], nullptr, nullptr, SW_SHOWNORMAL);
	*pResult = 0;
}

// ---------------------------------------------------------------------------------------------------------
// Settings

void CMainDlg::WriteControls()
{
	const RepeatSettings& s = theApp.m_Settings;
	SetDlgItemInt(IDC_EDIT_PHRASE, s.PhraseLength);
	SetDlgItemInt(IDC_EDIT_TOLERANCE, s.MismatchTolerance);
	CheckDlgButton(IDC_CHECK_IGNORE_CASE, s.IgnoreCase);
	CheckDlgButton(IDC_CHECK_IGNORE_PUNCTUATION, s.IgnorePunctuation);
	CheckDlgButton(IDC_CHECK_IGNORE_NUMBERS, s.IgnoreNumbers);
	CheckDlgButton(IDC_CHECK_ONEWORK, s.OneWork);
}

void CMainDlg::ReadControls()
{
	RepeatSettings& s = theApp.m_Settings;
	s.PhraseLength = (std::clamp)((int)GetDlgItemInt(IDC_EDIT_PHRASE), 2, 200);
	s.MismatchTolerance = (std::clamp)((int)GetDlgItemInt(IDC_EDIT_TOLERANCE), 0, 10);
	s.IgnoreCase = IsDlgButtonChecked(IDC_CHECK_IGNORE_CASE) != 0;
	s.IgnorePunctuation = IsDlgButtonChecked(IDC_CHECK_IGNORE_PUNCTUATION) != 0;
	s.IgnoreNumbers = IsDlgButtonChecked(IDC_CHECK_IGNORE_NUMBERS) != 0;
	s.OneWork = IsDlgButtonChecked(IDC_CHECK_ONEWORK) != 0;
	SetDlgItemInt(IDC_EDIT_PHRASE, s.PhraseLength);
	SetDlgItemInt(IDC_EDIT_TOLERANCE, s.MismatchTolerance);
}

void CMainDlg::OnButtonOptions()
{
	ReadControls();
	COptionsDlg dlg(this);
	dlg.m_Settings = theApp.m_Settings;
	dlg.m_Language = theApp.m_Language;
	dlg.m_ReportFolder = theApp.m_ReportFolder;
	dlg.m_AutoOpenReport = theApp.m_AutoOpenReport;
	if(dlg.DoModal() != IDOK) return;
	theApp.m_Settings = dlg.m_Settings;
	theApp.m_Language = dlg.m_Language;
	theApp.m_ReportFolder = dlg.m_ReportFolder;
	theApp.m_AutoOpenReport = dlg.m_AutoOpenReport;
	WriteControls();
	theApp.SaveSettings();
}

void CMainDlg::UpdateButtons()
{
	std::vector<bool> selected = SelectedDocuments();
	int n = (int)selected.size();
	bool any = std::find(selected.begin(), selected.end(), true) != selected.end();
	bool canUp = false, canDown = false;
	for(int i = 1; i < n; i++) if(selected[i] && !selected[i - 1]) canUp = true;
	for(int i = 0; i < n - 1; i++) if(selected[i] && !selected[i + 1]) canDown = true;

	GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(!m_Running && any);
	GetDlgItem(IDC_BUTTON_UP)->EnableWindow(!m_Running && canUp);
	GetDlgItem(IDC_BUTTON_DOWN)->EnableWindow(!m_Running && canDown);
	GetDlgItem(IDC_CHECK_ONEWORK)->EnableWindow(!m_Running && n > 1);
	GetDlgItem(IDC_BUTTON_FIND)->EnableWindow(m_Running || n > 0);
	GetDlgItem(IDC_BUTTON_REPORT)->EnableWindow(!m_Running && !m_ReportPath.IsEmpty() && PathFileExistsW(m_ReportPath));
}

// ---------------------------------------------------------------------------------------------------------
// Running

void CMainDlg::SetRunning(bool running)
{
	m_Running = running;
	for(int id : {IDC_LIST_DOCS, IDC_BUTTON_ADD, IDC_BUTTON_PASTE, IDC_EDIT_PHRASE, IDC_SPIN_PHRASE, IDC_EDIT_TOLERANCE, IDC_SPIN_TOLERANCE,
		IDC_CHECK_IGNORE_CASE, IDC_CHECK_IGNORE_PUNCTUATION, IDC_CHECK_IGNORE_NUMBERS, IDC_BUTTON_OPTIONS, IDC_LIST_RESULTS})
		GetDlgItem(id)->EnableWindow(!running);
	SetDlgItemText(IDC_BUTTON_FIND, running ? L"&Stop" : L"&Find Repeats");
	m_Progress.SetPos(0);
	m_Progress.ShowWindow(running ? SW_SHOW : SW_HIDE);
	UpdateButtons();
}

void CMainDlg::OnButtonFind()
{
	if(m_Running)
	{
		m_Abort = true;
		SetDlgItemText(IDC_STATIC_STATUS, L"Stopping...");
		return;
	}
	if(theApp.m_Documents.empty())
	{
		SetDlgItemText(IDC_STATIC_STATUS, L"Add a document to get started.");
		return;
	}

	ReadControls();
	theApp.SaveSettings();
	m_ListResults.DeleteAllItems();

	m_Finder = std::make_unique<CRepeatFinder>();
	m_Finder->Settings = theApp.m_Settings;
	if(theApp.m_Documents.size() < 2) m_Finder->Settings.OneWork = true;
	m_Abort = false;
	m_Finder->Abort = &m_Abort;

	std::wstring title;			// the report is named after the work, which is known before the documents are read
	CString first = PathFindFileNameW(theApp.m_Documents[0]);
	if(theApp.m_Documents.size() == 1) title = (LPCWSTR)first;
	else title = std::wstring((LPCWSTR)first) + L" and " + std::to_wstring(theApp.m_Documents.size() - 1)
		+ (theApp.m_Documents.size() == 2 ? L" other document" : L" other documents");
	m_ReportPath = theApp.ReportPath(title);

	SetRunning(true);
	SetDlgItemText(IDC_STATIC_STATUS, L"Starting...");
	m_Worker = std::thread(Work, m_hWnd, m_Finder.get(), theApp.m_Documents, theApp.m_Language, m_ReportPath);
}

// Function: Work
// Purpose: Runs on a worker thread: reads the documents, finds the repeats, and writes the report, posting
//			progress to the window. It touches nothing but the finder, which the window leaves alone until
//			WU_DONE arrives.

void CMainDlg::Work(HWND hwnd, CRepeatFinder* finder, std::vector<CString> documents, CString language, CString reportPath)
{
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);		// the .doc reader uses COM (IFilter)
	_wsetlocale(LC_ALL, language);
	auto post = [hwnd](int percent, const CString& status) { ::PostMessage(hwnd, WU_PROGRESS, percent, (LPARAM)new CString(status)); };

	int result = -1;
	CString message;
	for(size_t i = 0; i < documents.size(); i++)
	{
		if(*finder->Abort)
		{
			result = REPEAT_ERR_ABORT;
			break;
		}
		post((int)(20 * i / documents.size()), CString(L"Reading ") + PathFindFileNameW(documents[i]));
		result = finder->AddDocument((LPCWSTR)documents[i]);
		if(result > -1)
		{
			message.Format(L"Couldn't read %s. %s", PathFindFileNameW(documents[i]), CRepeatFinder::ErrorMessage(result).c_str());
			break;
		}
	}

	if(result == -1)
	{
		finder->Progress = [post](int percent, const std::wstring& status) { post(20 + percent * 3 / 4, status.c_str()); };
		result = finder->FindRepeats();
	}
	if(result == -1)
	{
		post(95, L"Writing the report");
		result = finder->WriteReport((LPCWSTR)reportPath, WREPEATFIND_NAME);
	}
	if(result > -1 && message.IsEmpty()) message = CRepeatFinder::ErrorMessage(result).c_str();
	::PostMessage(hwnd, WU_DONE, (WPARAM)result, (LPARAM)new CString(message));
	CoUninitialize();
}

LRESULT CMainDlg::OnProgress(WPARAM wParam, LPARAM lParam)
{
	CString* status = reinterpret_cast<CString*>(lParam);
	if(m_Running && !m_Abort)
	{
		m_Progress.SetPos((int)wParam);
		SetDlgItemText(IDC_STATIC_STATUS, *status);
	}
	delete status;
	return 0;
}

LRESULT CMainDlg::OnDone(WPARAM wParam, LPARAM lParam)
{
	CString* message = reinterpret_cast<CString*>(lParam);
	int result = (int)wParam;
	if(m_Worker.joinable()) m_Worker.join();
	SetRunning(false);

	if(result > -1)
	{
		SetDlgItemText(IDC_STATIC_STATUS, *message);
		if(result != REPEAT_ERR_ABORT) AfxMessageBox(*message, MB_ICONWARNING);
	}
	else ShowResults();
	delete message;
	UpdateButtons();
	return 0;
}

void CMainDlg::ShowResults()
{
	const CRepeatFinder& finder = *m_Finder;
	const std::vector<RepeatGroup>& groups = finder.Groups();

	m_ListResults.SetRedraw(FALSE);
	m_ListResults.DeleteAllItems();
	for(int i = 0; i < (int)groups.size(); i++)
	{
		const RepeatGroup& g = groups[i];
		std::wstring where;
		for(size_t c = 0; c < g.Copies.size(); c++)
		{
			if(c) where += L"; ";
			where += finder.CopyLocation(g.Copies[c], true);
		}
		m_ListResults.InsertItem(i, std::to_wstring(g.Number).c_str());
		m_ListResults.SetItemText(i, 1, std::to_wstring(g.Words).c_str());
		m_ListResults.SetItemText(i, 2, std::to_wstring(g.Copies.size()).c_str());
		m_ListResults.SetItemText(i, 3, where.c_str());
		m_ListResults.SetItemText(i, 4, finder.CopyText(g.Copies[0], 16).c_str());
		m_ListResults.SetItemData(i, g.Number);
	}
	m_ListResults.SetRedraw(TRUE);

	CString status;
	if(groups.empty())
		status.Format(L"No repeated phrases of %d or more words were found in %d words.", finder.Settings.PhraseLength, finder.WordsTotal());
	else
	{
		status.Format(L"Found %d repeated passage%s (%d of %d words are repeat copies).",
			(int)groups.size(), groups.size() == 1 ? L"" : L"s", finder.RepeatedWords(), finder.WordsTotal());
		if(theApp.m_AutoOpenReport) CWRepeatfindApp::OpenReport(m_ReportPath, L"");
		m_ListResults.SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	}
	SetDlgItemText(IDC_STATIC_STATUS, status);
}

void CMainDlg::OnResultsDblClick(NMHDR*, LRESULT* pResult)
{
	int item = m_ListResults.GetNextItem(-1, LVNI_SELECTED);
	if(item >= 0 && !m_ReportPath.IsEmpty())
	{
		CString anchor;
		anchor.Format(L"g%dc0", (int)m_ListResults.GetItemData(item));
		CWRepeatfindApp::OpenReport(m_ReportPath, anchor);
	}
	*pResult = 0;
}

void CMainDlg::OnButtonReport()
{
	if(!m_ReportPath.IsEmpty()) CWRepeatfindApp::OpenReport(m_ReportPath, L"");
}

void CMainDlg::StopWorker()
{
	m_Abort = true;
	if(m_Worker.joinable()) m_Worker.join();
	MSG msg;												// discard messages the worker posted, freeing their strings
	while(PeekMessage(&msg, m_hWnd, WU_PROGRESS, WU_DONE, PM_REMOVE)) delete reinterpret_cast<CString*>(msg.lParam);
	m_Running = false;
}
