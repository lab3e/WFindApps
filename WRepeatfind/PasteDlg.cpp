// PasteDlg.cpp : lets the writer paste text to check without saving a file first
//
// The pasted text is saved as a UTF-8 text file in %LOCALAPPDATA%\WRepeatfind\Pasted Text, named after the
// title the writer gives it, so the ordinary document machinery can read it and the report can be rebuilt.
// Pasting again under the same name replaces the earlier text, which suits checking a draft repeatedly.

#include "stdafx.h"
#include <shlobj.h>
#include <string>
#include "WRepeatfind.h"
#include "PasteDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CPasteDlg::CPasteDlg(CWnd* pParent)
	: CDialogEx(IDD_PASTE, pParent)
{
}

BEGIN_MESSAGE_MAP(CPasteDlg, CDialogEx)
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_BN_CLICKED(IDC_BUTTON_CLIPBOARD, OnButtonClipboard)
	ON_EN_CHANGE(IDC_EDIT_TEXT, OnChangeText)
END_MESSAGE_MAP()

BOOL CPasteDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	static_cast<CEdit*>(GetDlgItem(IDC_EDIT_TEXT))->SetLimitText(0);		// no 30,000-character limit
	SetDlgItemText(IDC_EDIT_TITLE, L"Pasted text");

	m_Layout.Init(this);
	m_Layout.Add(IDC_EDIT_TITLE, CDialogLayout::StretchX);
	m_Layout.Add(IDC_BUTTON_CLIPBOARD, CDialogLayout::TopRight);
	m_Layout.Add(IDC_EDIT_TEXT, CDialogLayout::StretchXY);
	m_Layout.Add(IDC_STATIC_WORDCOUNT, CDialogLayout::BottomLeft);
	m_Layout.Add(IDOK, CDialogLayout::BottomRight);
	m_Layout.Add(IDCANCEL, CDialogLayout::BottomRight);

	GetDlgItem(IDC_BUTTON_CLIPBOARD)->EnableWindow(IsClipboardFormatAvailable(CF_UNICODETEXT));
	GetDlgItem(IDC_EDIT_TEXT)->SetFocus();								// ready for Ctrl+V
	return FALSE;
}

void CPasteDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	if(nType != SIZE_MINIMIZED) m_Layout.Resize();
}

void CPasteDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	if(m_Layout.IsReady())
	{
		lpMMI->ptMinTrackSize.x = m_Layout.MinTrackSize().cx;
		lpMMI->ptMinTrackSize.y = m_Layout.MinTrackSize().cy;
	}
	CDialogEx::OnGetMinMaxInfo(lpMMI);
}

BOOL CPasteDlg::PreTranslateMessage(MSG* pMsg)
{
	// Ctrl+A selects all of the text, which a multiline edit control doesn't do by itself
	if((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == 'A') && (GetKeyState(VK_CONTROL) < 0)
		&& (pMsg->hwnd == GetDlgItem(IDC_EDIT_TEXT)->m_hWnd))
	{
		static_cast<CEdit*>(GetDlgItem(IDC_EDIT_TEXT))->SetSel(0, -1);
		return TRUE;
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

void CPasteDlg::OnButtonClipboard()
{
	if(!OpenClipboard()) return;
	HANDLE data = GetClipboardData(CF_UNICODETEXT);
	if(data != nullptr)
	{
		const wchar_t* text = static_cast<const wchar_t*>(GlobalLock(data));
		if(text != nullptr)
		{
			SetDlgItemText(IDC_EDIT_TEXT, text);
			GlobalUnlock(data);
			OnChangeText();			// a multiline edit doesn't send EN_CHANGE for text set this way
		}
	}
	CloseClipboard();
}

void CPasteDlg::OnChangeText()
{
	CString text;
	GetDlgItemText(IDC_EDIT_TEXT, text);
	int words = 0;
	bool inWord = false;
	for(int i = 0; i < text.GetLength(); i++)
	{
		bool space = iswspace(text[i]) != 0;
		if(!space && !inWord) words++;
		inWord = !space;
	}
	CString count;
	if(words > 0) count.Format(L"%d words", words);
	SetDlgItemText(IDC_STATIC_WORDCOUNT, count);
}

void CPasteDlg::OnOK()
{
	CString text, title;
	GetDlgItemText(IDC_EDIT_TEXT, text);
	GetDlgItemText(IDC_EDIT_TITLE, title);
	if(text.Trim().IsEmpty())
	{
		AfxMessageBox(L"Paste or type some text to check.", MB_ICONINFORMATION);
		GetDlgItem(IDC_EDIT_TEXT)->SetFocus();
		return;
	}
	title.Trim();
	if(title.IsEmpty()) title = L"Pasted text";
	for(const wchar_t* bad = L"\\/:*?\"<>|"; *bad; bad++) title.Replace(*bad, L'_');

	CString folder = CWRepeatfindApp::PastedTextFolder();
	SHCreateDirectoryExW(nullptr, folder, nullptr);
	CString path = folder + L"\\" + title + L".txt";

	int bytes = WideCharToMultiByte(CP_UTF8, 0, text, text.GetLength(), nullptr, 0, nullptr, nullptr);
	std::string utf8(bytes, '\0');
	WideCharToMultiByte(CP_UTF8, 0, text, text.GetLength(), utf8.data(), bytes, nullptr, nullptr);

	FILE* file = nullptr;
	if((_wfopen_s(&file, path, L"wb") != 0) || (file == nullptr))
	{
		AfxMessageBox(L"The pasted text could not be saved to " + path, MB_ICONWARNING);
		return;
	}
	fwrite("\xEF\xBB\xBF", 1, 3, file);							// UTF-8 byte-order mark, so the reader decodes it as UTF-8
	fwrite(utf8.data(), 1, utf8.size(), file);
	fclose(file);

	m_SavedPath = path;
	CDialogEx::OnOK();
}
