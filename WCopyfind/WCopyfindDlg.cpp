// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Louis A. Bloomfield
// WCopyfindDlg.cpp : the main WCopyfind window

#include "stdafx.h"
#include <afxinet.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <locale.h>
#include <algorithm>
#include <string>
#include "InputDocument.h"
#include "WCopyfind.h"
#include "WCopyfindDlg.h"
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
	SetDlgItemText(IDC_STATIC_VERSION, WCOPYFIND_NAME);
	SetDlgItemText(IDC_EDIT_ABOUT,
		L"WCopyfind finds the phrases that documents share. It was written to detect plagiarism in student papers "
		L"and has since been used for many other kinds of comparisons.\r\n\r\n"
		L"This program is free software: you can redistribute it and/or modify it under the terms of the GNU General "
		L"Public License as published by the Free Software Foundation, either version 3 of the License, or (at your "
		L"option) any later version.\r\n\r\n"
		L"This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the "
		L"implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License "
		L"for more details. You should have received a copy of the GNU General Public License along with this program. "
		L"If not, see https://www.gnu.org/licenses/.\r\n\r\n"
		L"Source code: https://github.com/lab3e/WFindApps\r\n\r\n"
		L"If you significantly improve this program, please let me know through the web site.\r\n\r\n"
		L"WCopyfind reads .docx files with miniz, by Rich Geldreich and contributors (MIT License).");
	return TRUE;
}

void CAboutDlg::OnLinkClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	PNMLINK link = reinterpret_cast<PNMLINK>(pNMHDR);
	ShellExecuteW(m_hWnd, L"open", link->item.szUrl, nullptr, nullptr, SW_SHOWNORMAL);
	*pResult = 0;
}

// ---------------------------------------------------------------------------------------------------------
// Helpers

static CString FileName(const CString& path) { return PathFindFileNameW(path); }

static CString FolderName(const CString& path)
{
	CString folder = path;
	PathRemoveFileSpecW(folder.GetBuffer());
	folder.ReleaseBuffer();
	return folder;
}

static bool NaturalLess(const CString& a, const CString& b)		// by file name ("paper2" before "paper10"), then folder
{
	int c = StrCmpLogicalW(FileName(a), FileName(b));
	if(c != 0) return c < 0;
	return StrCmpLogicalW(a, b) < 0;
}

static CString ResolveShortcut(const CString& path)
{
	if(_wcsicmp(PathFindExtensionW(path), L".lnk") != 0) return path;
	CString target = path;
	IShellLinkW* link = nullptr;
	if(SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&link)))
	{
		IPersistFile* file = nullptr;
		if(SUCCEEDED(link->QueryInterface(IID_IPersistFile, (void**)&file)))
		{
			wchar_t resolved[MAX_PATH] = {0};
			if(SUCCEEDED(file->Load(path, STGM_READ)) && SUCCEEDED(link->GetPath(resolved, MAX_PATH, nullptr, 0)) && resolved[0])
				target = resolved;
			file->Release();
		}
		link->Release();
	}
	return target;
}

static void CollectFolder(const CString& folder, std::vector<CString>& files)	// documents in a folder and its subfolders
{
	CFileFind finder;
	BOOL more = finder.FindFile(folder + L"\\*.*");
	while(more)
	{
		more = finder.FindNextFile();
		if(finder.IsDots()) continue;
		if(finder.IsDirectory()) CollectFolder(finder.GetFilePath(), files);
		else if(CWCopyfindApp::IsDocumentType(finder.GetFilePath())) files.push_back(finder.GetFilePath());
	}
}

static std::vector<CString> ReadListFile(const CString& path)		// a saved document list, in any common encoding
{
	std::vector<CString> lines;
	FILE* file = nullptr;
	if(_wfopen_s(&file, path, L"rb") != 0 || file == nullptr) return lines;
	std::string bytes;
	char buffer[65536];
	size_t n;
	while((n = fread(buffer, 1, sizeof(buffer), file)) > 0) bytes.append(buffer, n);
	fclose(file);

	CString text;
	if(bytes.size() >= 2 && (unsigned char)bytes[0] == 0xFF && (unsigned char)bytes[1] == 0xFE)
		text = CString(reinterpret_cast<const wchar_t*>(bytes.data() + 2), (int)(bytes.size() - 2) / 2);
	else
	{
		size_t start = (bytes.size() >= 3 && bytes.compare(0, 3, "\xEF\xBB\xBF") == 0) ? 3 : 0;
		const char* data = bytes.data() + start;
		int length = (int)(bytes.size() - start);
		UINT codePage = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, data, length, nullptr, 0) > 0 || length == 0 ? CP_UTF8 : CP_ACP;
		int wide = MultiByteToWideChar(codePage, 0, data, length, nullptr, 0);
		MultiByteToWideChar(codePage, 0, data, length, text.GetBuffer(wide), wide);
		text.ReleaseBuffer(wide);
	}

	int pos = 0;
	CString line = text.Tokenize(L"\r\n", pos);
	while(pos != -1)
	{
		line.Trim();
		line.Trim(L'"');
		if(!line.IsEmpty()) lines.push_back(line);
		line = text.Tokenize(L"\r\n", pos);
	}
	return lines;
}

static bool WriteTextFile(const CString& path, const CString& text)		// UTF-8 with a byte-order mark
{
	FILE* file = nullptr;
	if(_wfopen_s(&file, path, L"wb") != 0 || file == nullptr) return false;
	int bytes = WideCharToMultiByte(CP_UTF8, 0, text, text.GetLength(), nullptr, 0, nullptr, nullptr);
	std::string utf8(bytes, '\0');
	WideCharToMultiByte(CP_UTF8, 0, text, text.GetLength(), utf8.data(), bytes, nullptr, nullptr);
	fwrite("\xEF\xBB\xBF", 1, 3, file);
	bool ok = fwrite(utf8.data(), 1, utf8.size(), file) == utf8.size();
	fclose(file);
	return ok;
}

static CString Plural(int count, const wchar_t* one, const wchar_t* many)
{
	CString s;
	s.Format(L"%d %s", count, count == 1 ? one : many);
	return s;
}

// ---------------------------------------------------------------------------------------------------------
// The window

CWCopyfindDlg::CWCopyfindDlg(CWnd* pParent)
	: CDialogEx(IDD_WCOPYFIND_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWCopyfindDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_NEW, m_ListNew);
	DDX_Control(pDX, IDC_LIST_OLD, m_ListOld);
	DDX_Control(pDX, IDC_LIST_REPORT, m_ListReport);
	DDX_Control(pDX, IDC_SPIN_PHRASE, m_SpinPhrase);
	DDX_Control(pDX, IDC_SPIN_THRESHOLD, m_SpinThreshold);
	DDX_Control(pDX, IDC_SPIN_TOLERANCE, m_SpinTolerance);
	DDX_Control(pDX, IDC_PROGRESS, m_Progress);
}

BEGIN_MESSAGE_MAP(CWCopyfindDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_DROPFILES()
	ON_WM_CONTEXTMENU()
	ON_BN_CLICKED(IDC_BUTTON_ADD_NEW, OnAddNew)
	ON_BN_CLICKED(IDC_BUTTON_ADD_OLD, OnAddOld)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_NEW, OnRemoveNew)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_OLD, OnRemoveOld)
	ON_NOTIFY(BCN_DROPDOWN, IDC_BUTTON_ADD_NEW, OnAddDropDown)
	ON_NOTIFY(BCN_DROPDOWN, IDC_BUTTON_ADD_OLD, OnAddDropDown)
	ON_COMMAND_RANGE(ID_LIST_LOAD, ID_LIST_ADD, OnListCommand)
	ON_COMMAND_RANGE(ID_REPORT_OPEN, ID_REPORT_CLEAR, OnReportCommand)
	ON_BN_CLICKED(IDC_BUTTON_OPTIONS, OnButtonOptions)
	ON_BN_CLICKED(IDC_BUTTON_COMPARE, OnButtonCompare)
	ON_BN_CLICKED(IDC_BUTTON_REPORT, OnButtonReport)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_NEW, OnDocsItemChanged)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_OLD, OnDocsItemChanged)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_NEW, OnDocsDblClick)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_OLD, OnDocsDblClick)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_REPORT, OnReportDblClick)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_REPORT, OnReportColumnClick)
	ON_MESSAGE(WU_PROGRESS, OnProgress)
	ON_MESSAGE(WU_PAIR, OnPair)
	ON_MESSAGE(WU_DONE, OnDone)
END_MESSAGE_MAP()

BOOL CWCopyfindDlg::OnInitDialog()
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
	SetWindowText(WCOPYFIND_NAME);

	LOGFONT lf;
	GetFont()->GetLogFont(&lf);
	lf.lfWeight = FW_BOLD;
	lf.lfHeight = lf.lfHeight * 11 / 10;
	m_BoldFont.CreateFontIndirect(&lf);
	for(int id : {IDC_STATIC_STEP1, IDC_STATIC_STEP2, IDC_STATIC_STEP3}) GetDlgItem(id)->SetFont(&m_BoldFont);

	int scroll = GetSystemMetrics(SM_CXVSCROLL);
	for(int list : {NEW_LIST, OLD_LIST})
	{
		CListCtrl& ctrl = List(list);
		ctrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
		CRect r;
		ctrl.GetClientRect(&r);
		ctrl.InsertColumn(0, L"Document", LVCFMT_LEFT, r.Width() * 40 / 100);
		ctrl.InsertColumn(1, L"Folder", LVCFMT_LEFT, r.Width() * 60 / 100 - scroll);
	}
	m_ListReport.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	CRect r;
	m_ListReport.GetClientRect(&r);
	int unit = r.Width() / 100;
	m_ListReport.InsertColumn(0, L"", LVCFMT_LEFT, 0);		// a list's first column can't be right-aligned, so start with a
	m_ListReport.InsertColumn(1, L"Matching words", LVCFMT_RIGHT, unit * 17);	// placeholder and delete it below
	m_ListReport.InsertColumn(2, L"% of A", LVCFMT_RIGHT, unit * 9);
	m_ListReport.InsertColumn(3, L"% of B", LVCFMT_RIGHT, unit * 9);
	m_ListReport.InsertColumn(4, L"Document A", LVCFMT_LEFT, unit * 32);
	m_ListReport.InsertColumn(5, L"Document B", LVCFMT_LEFT, r.Width() - unit * 67 - scroll);
	m_ListReport.DeleteColumn(0);

	m_SpinPhrase.SetRange32(1, 999);
	m_SpinThreshold.SetRange32(1, 99999);
	m_SpinTolerance.SetRange32(0, 9);

	// When the window grows, the new-documents list takes 25% of the extra height, the old-documents list 15%,
	// and the list of matching pairs the remaining 60%.
	m_Layout.Init(this);
	m_Layout.Add(IDC_STATIC_NEW, 0, 0, 100, 0);
	m_Layout.Add(IDC_STATIC_NEW_COUNT, 100, 0, 0, 0);
	m_Layout.Add(IDC_LIST_NEW, 0, 0, 100, 25);
	m_Layout.Add(IDC_BUTTON_ADD_NEW, 100, 0, 0, 0);
	m_Layout.Add(IDC_BUTTON_REMOVE_NEW, 100, 0, 0, 0);
	m_Layout.Add(IDC_STATIC_OLD, 0, 25, 100, 0);
	m_Layout.Add(IDC_STATIC_OLD_COUNT, 100, 25, 0, 0);
	m_Layout.Add(IDC_LIST_OLD, 0, 25, 100, 15);
	m_Layout.Add(IDC_BUTTON_ADD_OLD, 100, 25, 0, 0);
	m_Layout.Add(IDC_BUTTON_REMOVE_OLD, 100, 25, 0, 0);
	m_Layout.Add(IDC_STATIC_HINT1, 0, 40, 100, 0);
	for(int id : {IDC_STATIC_STEP2, IDC_STATIC_PHRASE1, IDC_EDIT_PHRASE, IDC_SPIN_PHRASE, IDC_STATIC_PHRASE2, IDC_STATIC_THRESHOLD1,
		IDC_EDIT_THRESHOLD, IDC_SPIN_THRESHOLD, IDC_STATIC_THRESHOLD2, IDC_STATIC_TOLERANCE1, IDC_EDIT_TOLERANCE, IDC_SPIN_TOLERANCE,
		IDC_STATIC_TOLERANCE2, IDC_CHECK_IGNORE_CASE, IDC_CHECK_IGNORE_PUNCTUATION, IDC_CHECK_IGNORE_NUMBERS, IDC_STATIC_STEP3, IDC_BUTTON_COMPARE})
		m_Layout.Add(id, 0, 40, 0, 0);
	m_Layout.Add(IDC_BUTTON_OPTIONS, 100, 40, 0, 0);
	m_Layout.Add(IDC_STATIC_STATUS, 0, 40, 100, 0);
	m_Layout.Add(IDC_PROGRESS, 0, 40, 100, 0);
	m_Layout.Add(IDC_LIST_REPORT, 0, 40, 100, 60);
	m_Layout.Add(IDC_STATIC_HINT3, 0, 100, 100, 0);
	m_Layout.Add(IDC_BUTTON_REPORT, 100, 100, 0, 0);

	DragAcceptFiles(TRUE);
	ChangeWindowMessageFilterEx(m_hWnd, WM_DROPFILES, MSGFLT_ALLOW, nullptr);	// allow drops from Explorer if run elevated
	ChangeWindowMessageFilterEx(m_hWnd, WM_COPYDATA, MSGFLT_ALLOW, nullptr);
	ChangeWindowMessageFilterEx(m_hWnd, 0x0049 /* WM_COPYGLOBALDATA */, MSGFLT_ALLOW, nullptr);

	WriteControls();
	RefreshList(NEW_LIST, {});
	RefreshList(OLD_LIST, {});
	CString index = theApp.m_ReportFolder + L"\\matches.html";
	if(PathFileExistsW(index))
	{
		m_IndexPath = index;
		m_ReportFolder = theApp.m_ReportFolder;
	}
	SetDlgItemText(IDC_STATIC_STATUS, theApp.m_Documents[NEW_LIST].empty() ? L"Add the documents to compare to get started." : L"Ready.");
	UpdateControls();
	GetDlgItem(IDC_BUTTON_COMPARE)->SetFocus();
	return FALSE;
}

void CWCopyfindDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg about;
		about.DoModal();
	}
	else CDialogEx::OnSysCommand(nID, lParam);
}

void CWCopyfindDlg::OnPaint()
{
	if(IsIconic())
	{
		CPaintDC dc(this);
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
		CRect rect;
		GetClientRect(&rect);
		dc.DrawIcon((rect.Width() - GetSystemMetrics(SM_CXICON) + 1) / 2, (rect.Height() - GetSystemMetrics(SM_CYICON) + 1) / 2, m_hIcon);
	}
	else CDialogEx::OnPaint();
}

HCURSOR CWCopyfindDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CWCopyfindDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	if(nType == SIZE_MINIMIZED || !m_Layout.IsReady()) return;
	m_Layout.Resize();
	FitColumns();
}

void CWCopyfindDlg::FitColumns()
{
	for(CListCtrl* list : {&m_ListNew, &m_ListOld, &m_ListReport})	// let the last column take up the remaining width
	{
		if(list->GetSafeHwnd() == nullptr) continue;
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

void CWCopyfindDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	if(m_Layout.IsReady())
	{
		lpMMI->ptMinTrackSize.x = m_Layout.MinTrackSize().cx;
		lpMMI->ptMinTrackSize.y = m_Layout.MinTrackSize().cy;
	}
	CDialogEx::OnGetMinMaxInfo(lpMMI);
}

BOOL CWCopyfindDlg::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN && !m_Running)
	{
		for(int list : {NEW_LIST, OLD_LIST})
		{
			if(pMsg->hwnd != List(list).m_hWnd) continue;
			if(pMsg->wParam == VK_DELETE)
			{
				RemoveSelected(list);
				return TRUE;
			}
			if(pMsg->wParam == 'A' && GetKeyState(VK_CONTROL) < 0)
			{
				for(int i = 0; i < List(list).GetItemCount(); i++) List(list).SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
				return TRUE;
			}
		}
		if(pMsg->hwnd == m_ListReport.m_hWnd)
		{
			if(pMsg->wParam == VK_RETURN)
			{
				OpenPair(m_ListReport.GetNextItem(-1, LVNI_SELECTED));
				return TRUE;
			}
			if(pMsg->wParam == VK_DELETE)
			{
				OnReportCommand(ID_REPORT_REMOVE);
				return TRUE;
			}
		}
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

void CWCopyfindDlg::OnOK()
{
	// Enter is handled by the default button (Compare Documents); don't close the window
}

void CWCopyfindDlg::OnCancel()
{
	if(m_Running)
	{
		if(AfxMessageBox(L"WCopyfind is still comparing documents. Stop and close?", MB_YESNO | MB_ICONQUESTION) != IDYES) return;
		StopWorker();
	}
	ReadControls();
	CDialogEx::OnCancel();
}

// ---------------------------------------------------------------------------------------------------------
// Document lists

void CWCopyfindDlg::OnAddNew() { AddDocuments(NEW_LIST); }
void CWCopyfindDlg::OnAddOld() { AddDocuments(OLD_LIST); }
void CWCopyfindDlg::OnRemoveNew() { RemoveSelected(NEW_LIST); }
void CWCopyfindDlg::OnRemoveOld() { RemoveSelected(OLD_LIST); }

void CWCopyfindDlg::OnAddDropDown(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMBCDROPDOWN* drop = reinterpret_cast<NMBCDROPDOWN*>(pNMHDR);
	CPoint point(drop->rcButton.left, drop->rcButton.bottom);
	::ClientToScreen(drop->hdr.hwndFrom, &point);
	ShowListMenu(drop->hdr.idFrom == IDC_BUTTON_ADD_NEW ? NEW_LIST : OLD_LIST, point);
	*pResult = 0;
}

void CWCopyfindDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if(m_Running) return;
	for(int list : {NEW_LIST, OLD_LIST})
	{
		if(pWnd->GetSafeHwnd() != List(list).m_hWnd) continue;
		if(point.x == -1 && point.y == -1)			// from the keyboard
		{
			CRect r;
			List(list).GetWindowRect(&r);
			point = r.TopLeft();
		}
		ShowListMenu(list, point);
		return;
	}
	if(pWnd->GetSafeHwnd() == m_ListReport.m_hWnd)
	{
		if(point.x == -1 && point.y == -1)
		{
			CRect r;
			m_ListReport.GetWindowRect(&r);
			point = r.TopLeft();
		}
		CMenu menu;
		menu.LoadMenu(IDR_MENU_REPORT);
		CMenu* popup = menu.GetSubMenu(0);
		bool selected = m_ListReport.GetSelectedCount() > 0;
		bool any = m_ListReport.GetItemCount() > 0;
		popup->EnableMenuItem(ID_REPORT_OPEN, selected ? MF_ENABLED : MF_GRAYED);
		popup->EnableMenuItem(ID_REPORT_REMOVE, selected ? MF_ENABLED : MF_GRAYED);
		popup->EnableMenuItem(ID_REPORT_SAVE, any ? MF_ENABLED : MF_GRAYED);
		popup->EnableMenuItem(ID_REPORT_CLEAR, any ? MF_ENABLED : MF_GRAYED);
		popup->EnableMenuItem(ID_REPORT_INDEX, !m_IndexPath.IsEmpty() && PathFileExistsW(m_IndexPath) ? MF_ENABLED : MF_GRAYED);
		popup->SetDefaultItem(ID_REPORT_OPEN);
		popup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
	}
}

void CWCopyfindDlg::ShowListMenu(int list, CPoint screenPoint)
{
	if(m_Running) return;
	m_MenuList = list;
	CMenu menu;
	menu.LoadMenu(IDR_MENU_LIST);
	CMenu* popup = menu.GetSubMenu(0);
	bool any = !theApp.m_Documents[list].empty();
	bool selected = List(list).GetSelectedCount() > 0;
	popup->ModifyMenu(ID_LIST_MOVE, MF_BYCOMMAND | MF_STRING, ID_LIST_MOVE,
		list == NEW_LIST ? L"&Move Selected to Old Documents" : L"&Move Selected to New Documents");
	popup->CheckMenuItem(ID_LIST_KEEP_SORTED, theApp.m_KeepSorted[list] ? MF_CHECKED : MF_UNCHECKED);
	for(UINT id : {ID_LIST_SAVE, ID_LIST_SORT, ID_LIST_CLEAR}) popup->EnableMenuItem(id, any ? MF_ENABLED : MF_GRAYED);
	for(UINT id : {ID_LIST_MOVE, ID_LIST_REMOVE}) popup->EnableMenuItem(id, selected ? MF_ENABLED : MF_GRAYED);
	popup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, screenPoint.x, screenPoint.y, this);
}

void CWCopyfindDlg::OnListCommand(UINT id)
{
	int list = m_MenuList;
	switch(id)
	{
	case ID_LIST_ADD: AddDocuments(list); break;
	case ID_LIST_ADD_FOLDER: AddFolder(list); break;
	case ID_LIST_LOAD: LoadList(list); break;
	case ID_LIST_SAVE: SaveList(list); break;
	case ID_LIST_SORT: SortList(list); break;
	case ID_LIST_KEEP_SORTED:
		theApp.m_KeepSorted[list] = !theApp.m_KeepSorted[list];
		if(theApp.m_KeepSorted[list]) SortList(list);
		break;
	case ID_LIST_MOVE: MoveSelected(list); break;
	case ID_LIST_REMOVE: RemoveSelected(list); break;
	case ID_LIST_CLEAR:
		if(AfxMessageBox(list == NEW_LIST ? L"Remove all the new documents from the list?" : L"Remove all the old documents from the list?",
			MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
		{
			theApp.m_Documents[list].clear();
			RefreshList(list, {});
		}
		break;
	}
}

void CWCopyfindDlg::AddDocuments(int list)
{
	if(m_Running) return;
	static const wchar_t filter[] =
		L"Documents (*.docx;*.doc;*.txt;*.pdf;*.htm;*.html;*.url)|*.docx;*.doc;*.txt;*.pdf;*.htm;*.html;*.url|All Files (*.*)|*.*||";
	CFileDialog dlg(TRUE, nullptr, nullptr, OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER, filter, this);
	std::vector<wchar_t> buffer(4 * 1024 * 1024, 0);		// room for thousands of file names
	dlg.m_ofn.lpstrFile = buffer.data();
	dlg.m_ofn.nMaxFile = (DWORD)buffer.size();
	dlg.m_ofn.lpstrTitle = list == NEW_LIST ? L"Add New Documents" : L"Add Old Documents";
	if(dlg.DoModal() != IDOK) return;

	std::vector<CString> paths;
	POSITION pos = dlg.GetStartPosition();
	while(pos != nullptr) paths.push_back(dlg.GetNextPathName(pos));
	AddPaths(list, paths, false);
}

void CWCopyfindDlg::AddFolder(int list)
{
	CFolderPickerDialog dlg(nullptr, 0, this);
	dlg.m_ofn.lpstrTitle = list == NEW_LIST ? L"Add a Folder of New Documents" : L"Add a Folder of Old Documents";
	if(dlg.DoModal() == IDOK) AddPaths(list, {dlg.GetPathName()}, true);
}

// Function: AddPaths
// Purpose: Adds files to a list, expanding folders (with their subfolders) and shortcuts and skipping duplicates.
//			A folder contributes only document files; a file chosen or dropped by itself may be of any type.

void CWCopyfindDlg::AddPaths(int list, const std::vector<CString>& paths, bool fromFolder)
{
	std::vector<CString> files;
	for(const CString& path : paths)
	{
		if(PathIsDirectoryW(path)) CollectFolder(path, files);
		else
		{
			CString file = ResolveShortcut(path);
			if(PathIsDirectoryW(file)) CollectFolder(file, files);
			else if(!fromFolder || CWCopyfindApp::IsDocumentType(file)) files.push_back(file);
		}
	}
	std::sort(files.begin(), files.end(), NaturalLess);

	std::vector<CString>& docs = theApp.m_Documents[list];
	std::vector<bool> selected(docs.size(), false);
	int added = 0;
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
		added++;
	}
	if(theApp.m_KeepSorted[list])
	{
		std::vector<std::pair<CString, bool>> items;
		for(size_t i = 0; i < docs.size(); i++) items.push_back({docs[i], selected[i]});
		std::stable_sort(items.begin(), items.end(), [](const auto& a, const auto& b) { return NaturalLess(a.first, b.first); });
		for(size_t i = 0; i < items.size(); i++)
		{
			docs[i] = items[i].first;
			selected[i] = items[i].second;
		}
	}
	RefreshList(list, selected);

	if(paths.size() > 0 && files.empty()) SetDlgItemText(IDC_STATIC_STATUS, L"No documents were found there.");
	else if(added > 0)
	{
		CString status;
		status.Format(L"Added %s.", (LPCWSTR)Plural(added, list == NEW_LIST ? L"new document" : L"old document",
			list == NEW_LIST ? L"new documents" : L"old documents"));
		SetDlgItemText(IDC_STATIC_STATUS, status);
	}
}

void CWCopyfindDlg::RefreshList(int list, const std::vector<bool>& selected)
{
	CListCtrl& ctrl = List(list);
	const std::vector<CString>& docs = theApp.m_Documents[list];
	ctrl.SetRedraw(FALSE);
	ctrl.DeleteAllItems();
	int first = -1;
	for(int i = 0; i < (int)docs.size(); i++)
	{
		ctrl.InsertItem(i, FileName(docs[i]));
		ctrl.SetItemText(i, 1, FolderName(docs[i]));
		if(i < (int)selected.size() && selected[i])
		{
			ctrl.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
			if(first < 0) first = i;
		}
	}
	if(first >= 0) ctrl.EnsureVisible(first, FALSE);
	ctrl.SetRedraw(TRUE);

	SetDlgItemText(list == NEW_LIST ? IDC_STATIC_NEW_COUNT : IDC_STATIC_OLD_COUNT,
		docs.empty() ? CString() : Plural((int)docs.size(), L"document", L"documents"));
	FitColumns();
	UpdateControls();
}

std::vector<bool> CWCopyfindDlg::Selected(int list)
{
	std::vector<bool> selected(List(list).GetItemCount(), false);
	for(int i = 0; i < (int)selected.size(); i++) selected[i] = (List(list).GetItemState(i, LVIS_SELECTED) & LVIS_SELECTED) != 0;
	return selected;
}

void CWCopyfindDlg::RemoveSelected(int list)
{
	if(m_Running) return;
	std::vector<bool> selected = Selected(list);
	std::vector<CString>& docs = theApp.m_Documents[list];
	int firstRemoved = -1;
	for(int i = (int)docs.size() - 1; i >= 0; i--)
		if(i < (int)selected.size() && selected[i])
		{
			docs.erase(docs.begin() + i);
			firstRemoved = i;
		}
	if(firstRemoved < 0) return;
	std::vector<bool> keep(docs.size(), false);
	if(!docs.empty()) keep[(std::min)(firstRemoved, (int)docs.size() - 1)] = true;	// select a neighbor so Delete can repeat
	RefreshList(list, keep);
	List(list).SetFocus();
}

void CWCopyfindDlg::MoveSelected(int list)
{
	std::vector<bool> selected = Selected(list);
	std::vector<CString> moving;
	for(size_t i = 0; i < selected.size(); i++) if(selected[i]) moving.push_back(theApp.m_Documents[list][i]);
	RemoveSelected(list);
	AddPaths(1 - list, moving, false);
}

void CWCopyfindDlg::SortList(int list)
{
	std::vector<CString>& docs = theApp.m_Documents[list];
	std::stable_sort(docs.begin(), docs.end(), NaturalLess);
	RefreshList(list, {});
}

void CWCopyfindDlg::LoadList(int list)
{
	CFileDialog dlg(TRUE, L"txt", nullptr, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, L"Text Files (*.txt)|*.txt|All Files (*.*)|*.*||", this);
	dlg.m_ofn.lpstrTitle = L"Load a List of Documents";
	if(dlg.DoModal() != IDOK) return;
	std::vector<CString> lines = ReadListFile(dlg.GetPathName());
	std::vector<CString> found;
	int missing = 0;
	for(const CString& line : lines)
	{
		if(PathFileExistsW(line)) found.push_back(line);
		else missing++;
	}
	AddPaths(list, found, false);
	if(missing > 0)
	{
		CString message;
		message.Format(L"%s in the list could not be found and %s left out.", (LPCWSTR)Plural(missing, L"document", L"documents"),
			missing == 1 ? L"was" : L"were");
		AfxMessageBox(message, MB_ICONINFORMATION);
	}
}

void CWCopyfindDlg::SaveList(int list)
{
	CFileDialog dlg(FALSE, L"txt", list == NEW_LIST ? L"New Documents.txt" : L"Old Documents.txt", OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		L"Text Files (*.txt)|*.txt|All Files (*.*)|*.*||", this);
	dlg.m_ofn.lpstrTitle = L"Save the List of Documents";
	if(dlg.DoModal() != IDOK) return;
	CString text;
	for(const CString& doc : theApp.m_Documents[list]) text += doc + L"\r\n";
	if(!WriteTextFile(dlg.GetPathName(), text)) AfxMessageBox(L"The list could not be saved.", MB_ICONWARNING);
}

void CWCopyfindDlg::OnDropFiles(HDROP hDropInfo)
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
	POINT point;
	DragQueryPoint(hDropInfo, &point);
	DragFinish(hDropInfo);
	if(m_Running) return;

	int list = NEW_LIST;								// drops land in the old-documents list only when aimed at it
	CRect oldArea;
	m_ListOld.GetWindowRect(&oldArea);
	ScreenToClient(&oldArea);
	CRect oldLabel;
	GetDlgItem(IDC_STATIC_OLD)->GetWindowRect(&oldLabel);
	ScreenToClient(&oldLabel);
	oldArea.UnionRect(oldArea, oldLabel);
	oldArea.right += 200;								// include the buttons beside it
	if(oldArea.PtInRect(point)) list = OLD_LIST;
	AddPaths(list, paths, false);
}

void CWCopyfindDlg::OnDocsItemChanged(NMHDR*, LRESULT* pResult)
{
	UpdateControls();
	*pResult = 0;
}

void CWCopyfindDlg::OnDocsDblClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	int list = pNMHDR->idFrom == IDC_LIST_NEW ? NEW_LIST : OLD_LIST;
	int item = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR)->iItem;
	if(item >= 0 && item < (int)theApp.m_Documents[list].size())
		ShellExecuteW(m_hWnd, L"open", theApp.m_Documents[list][item], nullptr, nullptr, SW_SHOWNORMAL);
	else AddDocuments(list);							// double-clicking empty space adds documents, as before
	*pResult = 0;
}

// ---------------------------------------------------------------------------------------------------------
// Settings

void CWCopyfindDlg::WriteControls()
{
	const CopyfindSettings& s = theApp.m_Settings;
	SetDlgItemInt(IDC_EDIT_PHRASE, s.PhraseLength);
	SetDlgItemInt(IDC_EDIT_THRESHOLD, s.WordThreshold);
	SetDlgItemInt(IDC_EDIT_TOLERANCE, s.MismatchTolerance);
	CheckDlgButton(IDC_CHECK_IGNORE_CASE, s.IgnoreCase);
	CheckDlgButton(IDC_CHECK_IGNORE_PUNCTUATION, s.IgnorePunctuation);
	CheckDlgButton(IDC_CHECK_IGNORE_NUMBERS, s.IgnoreNumbers);
}

void CWCopyfindDlg::ReadControls()
{
	CopyfindSettings& s = theApp.m_Settings;
	s.PhraseLength = std::clamp((int)GetDlgItemInt(IDC_EDIT_PHRASE), 1, 999);
	s.WordThreshold = std::clamp((int)GetDlgItemInt(IDC_EDIT_THRESHOLD), 1, 99999);
	s.MismatchTolerance = std::clamp((int)GetDlgItemInt(IDC_EDIT_TOLERANCE), 0, 9);
	s.IgnoreCase = IsDlgButtonChecked(IDC_CHECK_IGNORE_CASE) != 0;
	s.IgnorePunctuation = IsDlgButtonChecked(IDC_CHECK_IGNORE_PUNCTUATION) != 0;
	s.IgnoreNumbers = IsDlgButtonChecked(IDC_CHECK_IGNORE_NUMBERS) != 0;
	WriteControls();
}

void CWCopyfindDlg::OnButtonOptions()
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

void CWCopyfindDlg::UpdateControls()
{
	int newDocs = (int)theApp.m_Documents[NEW_LIST].size();
	int oldDocs = (int)theApp.m_Documents[OLD_LIST].size();
	bool canCompare = newDocs >= 2 || (newDocs >= 1 && oldDocs >= 1);
	GetDlgItem(IDC_BUTTON_REMOVE_NEW)->EnableWindow(!m_Running && m_ListNew.GetSelectedCount() > 0);
	GetDlgItem(IDC_BUTTON_REMOVE_OLD)->EnableWindow(!m_Running && m_ListOld.GetSelectedCount() > 0);
	GetDlgItem(IDC_BUTTON_COMPARE)->EnableWindow(m_Running || canCompare);
	GetDlgItem(IDC_BUTTON_REPORT)->EnableWindow(!m_Running && !m_IndexPath.IsEmpty() && PathFileExistsW(m_IndexPath));
}

// ---------------------------------------------------------------------------------------------------------
// Comparing

void CWCopyfindDlg::SetRunning(bool running)
{
	m_Running = running;
	for(int id : {IDC_LIST_NEW, IDC_LIST_OLD, IDC_BUTTON_ADD_NEW, IDC_BUTTON_ADD_OLD, IDC_EDIT_PHRASE, IDC_SPIN_PHRASE, IDC_EDIT_THRESHOLD,
		IDC_SPIN_THRESHOLD, IDC_EDIT_TOLERANCE, IDC_SPIN_TOLERANCE, IDC_CHECK_IGNORE_CASE, IDC_CHECK_IGNORE_PUNCTUATION,
		IDC_CHECK_IGNORE_NUMBERS, IDC_BUTTON_OPTIONS})
		GetDlgItem(id)->EnableWindow(!running);
	SetDlgItemText(IDC_BUTTON_COMPARE, running ? L"&Stop" : L"Co&mpare Documents");
	m_Progress.SetPos(0);
	m_Progress.ShowWindow(running ? SW_SHOW : SW_HIDE);
	UpdateControls();
}

void CWCopyfindDlg::OnButtonCompare()
{
	if(m_Running)
	{
		m_Abort = true;
		SetDlgItemText(IDC_STATIC_STATUS, L"Stopping...");
		return;
	}
	ReadControls();
	theApp.SaveSettings();

	const std::vector<CString>& newDocs = theApp.m_Documents[NEW_LIST];
	const std::vector<CString>& oldDocs = theApp.m_Documents[OLD_LIST];
	if(!(newDocs.size() >= 2 || (newDocs.size() >= 1 && oldDocs.size() >= 1)))
	{
		SetDlgItemText(IDC_STATIC_STATUS, L"Add at least two new documents, or one new and one old document.");
		return;
	}

	m_Results.clear();
	m_Removed.clear();
	m_ListReport.DeleteAllItems();

	m_DuplicateNames.clear();									// file names shared by documents in different folders
	std::set<std::wstring> seen;
	for(const std::vector<CString>* docs : {&newDocs, &oldDocs})
		for(const CString& doc : *docs)
		{
			CString lower = FileName(doc);
			lower.MakeLower();
			if(!seen.insert(std::wstring(lower)).second) m_DuplicateNames.insert(std::wstring(lower));
		}

	const CopyfindSettings& s = theApp.m_Settings;
	m_Compare = std::make_unique<CCompareDocuments>((int)(newDocs.size() + oldDocs.size()));
	CCompareDocuments& c = *m_Compare;
	int i = 0;
	for(const CString& doc : oldDocs)
	{
		c.m_pDocs[i].m_szDocumentName = doc;
		c.m_pDocs[i++].m_DocumentType = DOC_TYPE_OLD;
	}
	for(const CString& doc : newDocs)
	{
		c.m_pDocs[i].m_szDocumentName = doc;
		c.m_pDocs[i++].m_DocumentType = DOC_TYPE_NEW;
	}
	c.m_PhraseLength = s.PhraseLength;
	c.m_WordThreshold = s.WordThreshold;
	c.m_MismatchTolerance = s.MismatchTolerance;
	c.m_MismatchPercentage = s.MismatchPercentage;
	c.m_SkipLength = s.SkipLength;
	c.m_bIgnoreCase = s.IgnoreCase;
	c.m_bIgnorePunctuation = s.IgnorePunctuation;
	c.m_bIgnoreOuterPunctuation = s.IgnoreOuterPunctuation;
	c.m_bIgnoreNumbers = s.IgnoreNumbers;
	c.m_bSkipLongWords = s.SkipLongWords;
	c.m_bSkipNonwords = s.SkipNonwords;
	c.m_bBasic_Characters = s.BasicCharacters;
	c.m_bBriefReport = s.BriefReport;
	c.m_szSoftwareName = WCOPYFIND_NAME;
	c.m_szReportFolder = theApp.m_ReportFolder;
	m_ReportFolder = theApp.m_ReportFolder;
	m_IndexPath = m_ReportFolder + L"\\matches.html";

	m_Abort = false;
	SetRunning(true);
	SetDlgItemText(IDC_STATIC_STATUS, L"Starting...");
	m_Worker = std::thread(Work, m_hWnd, m_Compare.get(), &m_Abort, theApp.m_Language);
}

// Function: Work
// Purpose: Runs on a worker thread: reads the documents, compares every pair that should be compared, and writes
//			the reports, posting progress and each matching pair to the window. It touches nothing but the
//			CCompareDocuments object, which the window leaves alone until WU_DONE arrives. A document that can't
//			be read is left out (and listed in the report) rather than stopping the whole comparison.

void CWCopyfindDlg::Work(HWND hwnd, CCompareDocuments* c, std::atomic<bool>* abort, CString language)
{
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);		// the .doc reader uses COM (IFilter)
	_wsetlocale(LC_ALL, language);
	auto post = [hwnd](int percent, const CString& status) { ::PostMessage(hwnd, WU_PROGRESS, percent, (LPARAM)new CString(status)); };

	bool stopped = false;
	int result = c->SetupReports();
	if(result == -1)
	{
		c->SetupLoading();
		for(int i = 0; i < c->m_Documents; i++)
		{
			if(*abort)
			{
				stopped = true;
				break;
			}
			CCompareDocuments::Document* doc = c->m_pDocs + i;
			post(20 * i / c->m_Documents, L"Reading " + CCompareDocuments::FileNameOf(doc->m_szDocumentName));
			int loaded = c->LoadDocument(doc);
			if(loaded > -1)
			{
				c->m_Unreadable.push_back({doc->m_szDocumentName, ErrorMessage(loaded)});
				doc->m_DocumentType = DOC_TYPE_UNDEFINED;		// leave it out of the comparisons
			}
		}
		c->FinishLoading();

		if(!stopped) result = c->SetupComparisons();
		if(result == -1 && !stopped)
		{
			c->SetupProgressReports(DOC_TYPE_OLD, DOC_TYPE_NEW, DOC_TYPE_NEW);
			for(int l = 0; l < c->m_Documents && result == -1 && !stopped; l++)
			{
				c->m_pDocL = c->m_pDocs + l;
				if(c->m_pDocL->m_DocumentType == DOC_TYPE_UNDEFINED) continue;
				for(int r = 0; r < l; r++)
				{
					c->m_pDocR = c->m_pDocs + r;
					if(c->m_pDocR->m_DocumentType == DOC_TYPE_UNDEFINED) continue;
					if(c->m_pDocL->m_DocumentType == DOC_TYPE_OLD && c->m_pDocR->m_DocumentType == DOC_TYPE_OLD) continue;
					if(*abort)
					{
						stopped = true;
						break;
					}

					c->ComparePair(c->m_pDocL, c->m_pDocR);
					if(c->m_Compares % c->m_CompareStep == 0 && c->m_TotalCompares > 0)
					{
						CString status;
						status.Format(L"Compared %lld of %lld pairs", c->m_Compares, c->m_TotalCompares);
						post(20 + (int)(80.0 * c->m_Compares / c->m_TotalCompares), status);
					}
					if(c->m_MatchingWordsPerfect >= c->m_WordThreshold)
					{
						result = c->ReportMatchedPair();
						if(result > -1) break;
						::PostMessage(hwnd, WU_PAIR, 0, (LPARAM)new CCompareDocuments::PairRecord(c->m_PairRecords.back()));
					}
				}
			}
			c->FinishComparisons();
		}
		post(100, L"Writing the report");
		int finished = c->FinishReports(stopped);
		if(result == -1) result = finished;
	}
	if(result == -1 && stopped) result = ERR_ABORT;
	CString message = result > -1 ? ErrorMessage(result) : CString();
	::PostMessage(hwnd, WU_DONE, (WPARAM)result, (LPARAM)new CString(message));
	CoUninitialize();
}

LRESULT CWCopyfindDlg::OnProgress(WPARAM wParam, LPARAM lParam)
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

LRESULT CWCopyfindDlg::OnPair(WPARAM, LPARAM lParam)
{
	CCompareDocuments::PairRecord* record = reinterpret_cast<CCompareDocuments::PairRecord*>(lParam);
	m_Results.push_back(*record);
	m_Removed.push_back(false);
	delete record;
	AddResultRow((int)m_Results.size() - 1);
	m_ListReport.EnsureVisible(m_ListReport.GetItemCount() - 1, FALSE);
	if(m_Results.size() < 50) FitColumns();					// once the scroll bar has appeared, the widths are settled
	return 0;
}

LRESULT CWCopyfindDlg::OnDone(WPARAM wParam, LPARAM lParam)
{
	CString* message = reinterpret_cast<CString*>(lParam);
	int result = (int)wParam;
	if(m_Worker.joinable()) m_Worker.join();
	SetRunning(false);

	const CCompareDocuments& c = *m_Compare;
	CString status;
	if(result > 0)
	{
		status = *message;
		AfxMessageBox(*message, MB_ICONWARNING);
	}
	else
	{
		status.Format(L"%s %s among %d documents (%lld pairs compared).",
			result == ERR_ABORT ? L"Stopped. Found" : L"Done. Found", (LPCWSTR)Plural((int)c.m_PairRecords.size(), L"matching pair", L"matching pairs"),
			c.m_Documents - (int)c.m_Unreadable.size(), c.m_Compares);
		if(!c.m_Unreadable.empty())
			status += L" " + Plural((int)c.m_Unreadable.size(), L"document", L"documents") + L" couldn't be read (see the report).";
		if(theApp.m_AutoOpenReport && PathFileExistsW(m_IndexPath)) CWCopyfindApp::OpenInBrowser(m_IndexPath);
	}
	SetDlgItemText(IDC_STATIC_STATUS, status);
	delete message;
	if(m_SortColumn >= 0) RefillResults();
	FitColumns();
	UpdateControls();
	return 0;
}

void CWCopyfindDlg::StopWorker()
{
	m_Abort = true;
	if(m_Worker.joinable()) m_Worker.join();
	MSG msg;												// discard what the worker posted, freeing it
	while(PeekMessage(&msg, m_hWnd, WU_PROGRESS, WU_DONE, PM_REMOVE))
	{
		if(msg.message == WU_PAIR) delete reinterpret_cast<CCompareDocuments::PairRecord*>(msg.lParam);
		else delete reinterpret_cast<CString*>(msg.lParam);
	}
	m_Running = false;
}

CString CWCopyfindDlg::ErrorMessage(int code)
{
	switch(code)
	{
	case ERR_ABORT: return L"Stopped.";
	case ERR_CANNOT_ALLOCATE_WORKING_HASH_ARRAY:
	case ERR_CANNOT_ALLOCATE_HASH_ARRAY:
	case ERR_CANNOT_ALLOCATE_SORTED_HASH_ARRAY:
	case ERR_CANNOT_ALLOCATE_SORTED_NUMBER_ARRAY: return L"There isn't enough memory to read the document.";
	case ERR_CANNOT_CREATE_REPORT_FOLDER: return L"The report folder could not be created. Choose another folder in More Options.";
	case ERR_CANNOT_OPEN_LOG_FILE:
	case ERR_CANNOT_OPEN_COMPARISON_REPORT_TXT_FILE:
	case ERR_CANNOT_OPEN_COMPARISON_REPORT_HTML_FILE:
	case ERR_CANNOT_OPEN_SIDE_BY_SIDE_HTML_FILE:
		return L"The report could not be written. Check that you can save files in the report folder (see More Options), "
			L"and that no report file is open in another program.";
	case ERR_CANNOT_OPEN_LEFT_DOCUMENT_FILE:
	case ERR_CANNOT_OPEN_RIGHT_DOCUMENT_FILE: return L"A document could not be reopened to write its part of the report. Was it moved or changed?";
	case ERR_CANNOT_ACCESS_URL: return L"The web address could not be reached.";
	case ERR_NO_FILE_OPEN: return L"Software problem: reading from a file that is not open.";
	case ERR_CANNOT_FIND_FILE: return L"The file could not be found.";
	case ERR_CANNOT_FIND_FILE_EXTENSION: return L"The file has no extension, so its type can't be determined.";
	case ERR_BAD_DOCX_FILE: return L"This .docx file can't be read.";
	case ERR_BAD_PDF_FILE: return L"This .pdf file can't be read. (PDF reading needs pdftotext.exe in the same folder as WCopyfind.exe.)";
	case ERR_CANNOT_FIND_URL_LINK: return L"The web link could not be found.";
	case ERR_CANNOT_OPEN_INPUT_FILE: return L"The file can't be opened. It may be damaged, or another program may have it open.";
	default:
		CString s;
		s.Format(L"Error %d occurred.", code);
		return s;
	}
}

// ---------------------------------------------------------------------------------------------------------
// Matching pairs

void CWCopyfindDlg::AddResultRow(int index)
{
	const CCompareDocuments::PairRecord& r = m_Results[index];
	CString text;
	int item = m_ListReport.GetItemCount();
	text.Format(L"%d", r.Perfect);
	m_ListReport.InsertItem(item, text);
	for(int side = 0; side < 2; side++)
	{
		int words = side ? r.WordsR : r.WordsL;
		int percent = words ? (int)(100LL * r.Perfect / words) : 0;
		if(percent == 0 && r.Perfect > 0) text = L"<1%";
		else text.Format(L"%d%%", percent);
		m_ListReport.SetItemText(item, 1 + side, text);
	}
	m_ListReport.SetItemText(item, 3, DisplayName(r.PathL));
	m_ListReport.SetItemText(item, 4, DisplayName(r.PathR));
	m_ListReport.SetItemData(item, index);
}

// Function: DisplayName
// Purpose: A document's file name, with its folder's name in front when another document has the same file name.

CString CWCopyfindDlg::DisplayName(const CString& path) const
{
	CString name = FileName(path);
	CString lower = name;
	lower.MakeLower();
	if(m_DuplicateNames.count(std::wstring(lower)) == 0) return name;
	return FileName(FolderName(path)) + L"\\" + name;
}

void CWCopyfindDlg::RefillResults()
{
	std::vector<int> order(m_Results.size());
	for(size_t i = 0; i < order.size(); i++) order[i] = (int)i;
	if(m_SortColumn >= 0)
	{
		auto key = [&](int i, int column) -> double
		{
			const CCompareDocuments::PairRecord& r = m_Results[i];
			if(column == 0) return r.Perfect;
			if(column == 1) return r.WordsL ? double(r.Perfect) / r.WordsL : 0;
			return r.WordsR ? double(r.Perfect) / r.WordsR : 0;
		};
		std::stable_sort(order.begin(), order.end(), [&](int a, int b)
		{
			int c;
			if(m_SortColumn <= 2)
			{
				double x = key(a, m_SortColumn), y = key(b, m_SortColumn);
				c = x < y ? -1 : (x > y ? 1 : 0);
			}
			else
			{
				const CString& x = m_SortColumn == 3 ? m_Results[a].PathL : m_Results[a].PathR;
				const CString& y = m_SortColumn == 3 ? m_Results[b].PathL : m_Results[b].PathR;
				c = StrCmpLogicalW(FileName(x), FileName(y));
			}
			return m_SortDescending ? c > 0 : c < 0;
		});
	}
	m_ListReport.SetRedraw(FALSE);
	m_ListReport.DeleteAllItems();
	for(int i : order) if(!m_Removed[i]) AddResultRow(i);
	m_ListReport.SetRedraw(TRUE);

	CHeaderCtrl* header = m_ListReport.GetHeaderCtrl();
	for(int col = 0; col < header->GetItemCount(); col++)
	{
		HDITEM item = {HDI_FORMAT};
		header->GetItem(col, &item);
		item.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
		if(col == m_SortColumn) item.fmt |= m_SortDescending ? HDF_SORTDOWN : HDF_SORTUP;
		header->SetItem(col, &item);
	}
}

void CWCopyfindDlg::OnReportColumnClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	int column = reinterpret_cast<NMLISTVIEW*>(pNMHDR)->iSubItem;
	if(column == m_SortColumn) m_SortDescending = !m_SortDescending;
	else
	{
		m_SortColumn = column;
		m_SortDescending = column <= 2;			// biggest matches first; names alphabetically
	}
	RefillResults();
	*pResult = 0;
}

void CWCopyfindDlg::OpenPair(int item)
{
	if(item < 0 || item >= m_ListReport.GetItemCount()) return;
	const CCompareDocuments::PairRecord& r = m_Results[m_ListReport.GetItemData(item)];
	CString page = m_ReportFolder + L"\\" + r.File;
	page.Replace(L'/', L'\\');
	if(PathFileExistsW(page)) CWCopyfindApp::OpenInBrowser(page);
	else SetDlgItemText(IDC_STATIC_STATUS, L"That pair's page is no longer in the report folder. Compare the documents again to rebuild it.");
}

void CWCopyfindDlg::OnReportDblClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	OpenPair(reinterpret_cast<NMITEMACTIVATE*>(pNMHDR)->iItem);
	*pResult = 0;
}

void CWCopyfindDlg::OnButtonReport()
{
	if(!m_IndexPath.IsEmpty() && PathFileExistsW(m_IndexPath)) CWCopyfindApp::OpenInBrowser(m_IndexPath);
}

void CWCopyfindDlg::SaveResults()
{
	CFileDialog dlg(FALSE, L"txt", L"Matching Pairs.txt", OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, L"Text Files (*.txt)|*.txt|All Files (*.*)|*.*||", this);
	dlg.m_ofn.lpstrTitle = L"Save the List of Matching Pairs";
	if(dlg.DoModal() != IDOK) return;
	CString text = L"Matching words\t% of A\t% of B\tWords in matching phrases (A)\tWords in matching phrases (B)\tDocument A\tDocument B\r\n";
	for(int item = 0; item < m_ListReport.GetItemCount(); item++)
	{
		const CCompareDocuments::PairRecord& r = m_Results[m_ListReport.GetItemData(item)];
		CString line;
		line.Format(L"%d\t%d\t%d\t%d\t%d\t%s\t%s\r\n", r.Perfect, r.WordsL ? (int)(100LL * r.Perfect / r.WordsL) : 0,
			r.WordsR ? (int)(100LL * r.Perfect / r.WordsR) : 0, r.TotalL, r.TotalR, (LPCWSTR)r.PathL, (LPCWSTR)r.PathR);
		text += line;
	}
	if(!WriteTextFile(dlg.GetPathName(), text)) AfxMessageBox(L"The list could not be saved.", MB_ICONWARNING);
}

void CWCopyfindDlg::OnReportCommand(UINT id)
{
	switch(id)
	{
	case ID_REPORT_OPEN: OpenPair(m_ListReport.GetNextItem(-1, LVNI_SELECTED)); break;
	case ID_REPORT_INDEX: OnButtonReport(); break;
	case ID_REPORT_SAVE: SaveResults(); break;
	case ID_REPORT_REMOVE:
		for(int item = m_ListReport.GetItemCount() - 1; item >= 0; item--)
			if(m_ListReport.GetItemState(item, LVIS_SELECTED) & LVIS_SELECTED)
			{
				m_Removed[m_ListReport.GetItemData(item)] = true;
				m_ListReport.DeleteItem(item);
			}
		break;
	case ID_REPORT_CLEAR:
		m_Results.clear();
		m_Removed.clear();
		m_ListReport.DeleteAllItems();
		break;
	}
}
