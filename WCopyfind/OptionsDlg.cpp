// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Louis A. Bloomfield
// OptionsDlg.cpp : the less commonly needed settings, and the vocabulary tool

#include "stdafx.h"
#include <afxinet.h>
#include <locale.h>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>
#include "InputDocument.h"
#include "Words.h"
#include "WCopyfind.h"
#include "OptionsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

COptionsDlg::COptionsDlg(CWnd* pParent)
	: CDialogEx(IDD_OPTIONS, pParent)
{
}

BEGIN_MESSAGE_MAP(COptionsDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_FOLDER, OnButtonFolder)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULTS, OnButtonDefaults)
	ON_BN_CLICKED(IDC_BUTTON_VOCABULARY, OnButtonVocabulary)
	ON_BN_CLICKED(IDC_CHECK_SKIP_LONG, OnCheckSkipLong)
END_MESSAGE_MAP()

BOOL COptionsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_SPIN_PERCENTAGE))->SetRange32(0, 100);
	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_SPIN_SKIP_LENGTH))->SetRange32(1, WORDMAXIMUMLENGTH);
	CComboBox* combo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_LANGUAGE));
	for(const CString& language : CWCopyfindApp::Languages()) combo->AddString(language);
	WriteControls();
	return TRUE;
}

void COptionsDlg::WriteControls()
{
	SetDlgItemInt(IDC_EDIT_PERCENTAGE, m_Settings.MismatchPercentage);
	CheckDlgButton(IDC_CHECK_OUTER_PUNCTUATION, m_Settings.IgnoreOuterPunctuation);
	CheckDlgButton(IDC_CHECK_SKIP_NONWORDS, m_Settings.SkipNonwords);
	CheckDlgButton(IDC_CHECK_SKIP_LONG, m_Settings.SkipLongWords);
	SetDlgItemInt(IDC_EDIT_SKIP_LENGTH, m_Settings.SkipLength);
	CheckDlgButton(IDC_CHECK_BASIC_CHARACTERS, m_Settings.BasicCharacters);
	CComboBox* combo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_LANGUAGE));
	if(combo->SelectString(-1, m_Language) == CB_ERR) combo->SelectString(-1, CWCopyfindApp::DefaultLanguage());
	SetDlgItemText(IDC_EDIT_FOLDER, m_ReportFolder);
	CheckDlgButton(IDC_CHECK_BRIEF_REPORT, m_Settings.BriefReport);
	CheckDlgButton(IDC_CHECK_AUTO_OPEN, m_AutoOpenReport);
	GetDlgItem(IDC_CHECK_OUTER_PUNCTUATION)->EnableWindow(!m_Settings.IgnorePunctuation);
	OnCheckSkipLong();
}

void COptionsDlg::ReadControls()
{
	m_Settings.MismatchPercentage = std::clamp((int)GetDlgItemInt(IDC_EDIT_PERCENTAGE), 0, 100);
	m_Settings.IgnoreOuterPunctuation = IsDlgButtonChecked(IDC_CHECK_OUTER_PUNCTUATION) != 0;
	m_Settings.SkipNonwords = IsDlgButtonChecked(IDC_CHECK_SKIP_NONWORDS) != 0;
	m_Settings.SkipLongWords = IsDlgButtonChecked(IDC_CHECK_SKIP_LONG) != 0;
	m_Settings.SkipLength = std::clamp((int)GetDlgItemInt(IDC_EDIT_SKIP_LENGTH), 1, WORDMAXIMUMLENGTH);
	m_Settings.BasicCharacters = IsDlgButtonChecked(IDC_CHECK_BASIC_CHARACTERS) != 0;
	m_Settings.BriefReport = IsDlgButtonChecked(IDC_CHECK_BRIEF_REPORT) != 0;
	m_AutoOpenReport = IsDlgButtonChecked(IDC_CHECK_AUTO_OPEN) != 0;
	GetDlgItemText(IDC_COMBO_LANGUAGE, m_Language);
	GetDlgItemText(IDC_EDIT_FOLDER, m_ReportFolder);
	m_ReportFolder.Trim();
	m_ReportFolder.TrimRight(L'\\');
	if(m_ReportFolder.IsEmpty()) m_ReportFolder = CWCopyfindApp::DefaultReportFolder();
}

void COptionsDlg::OnCheckSkipLong()
{
	bool on = IsDlgButtonChecked(IDC_CHECK_SKIP_LONG) != 0;
	GetDlgItem(IDC_EDIT_SKIP_LENGTH)->EnableWindow(on);
	GetDlgItem(IDC_SPIN_SKIP_LENGTH)->EnableWindow(on);
}

void COptionsDlg::OnButtonFolder()
{
	CString current;
	GetDlgItemText(IDC_EDIT_FOLDER, current);
	CFolderPickerDialog dlg(current, 0, this);
	if(dlg.DoModal() == IDOK) SetDlgItemText(IDC_EDIT_FOLDER, dlg.GetPathName());
}

void COptionsDlg::OnButtonDefaults()
{
	m_Settings = CopyfindSettings();
	m_Language = CWCopyfindApp::DefaultLanguage();
	m_ReportFolder = CWCopyfindApp::DefaultReportFolder();
	m_AutoOpenReport = true;
	WriteControls();											// the main window's settings are reset too, when OK is clicked
}

void COptionsDlg::OnOK()
{
	ReadControls();
	CDialogEx::OnOK();
}

// Function: OnButtonVocabulary
// Purpose: Writes every word in the new and old documents, after the current filters, with how often it appears,
//			most frequent first. The file is UTF-8, so words in any language survive.

void COptionsDlg::OnButtonVocabulary()
{
	ReadControls();
	std::vector<CString> docs = theApp.m_Documents[1];
	docs.insert(docs.end(), theApp.m_Documents[0].begin(), theApp.m_Documents[0].end());
	if(docs.empty())
	{
		AfxMessageBox(L"Add some documents first; the vocabulary list is made from the documents in both lists.", MB_ICONINFORMATION);
		return;
	}

	CFileDialog dlg(FALSE, L"txt", L"Vocabulary.txt", OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, L"Text Files (*.txt)|*.txt|All Files (*.*)|*.*||", this);
	dlg.m_ofn.lpstrTitle = L"Save the Vocabulary List";
	if(dlg.DoModal() != IDOK) return;

	CWaitCursor wait;
	_wsetlocale(LC_ALL, m_Language);
	const CopyfindSettings& s = m_Settings;
	std::unordered_map<std::wstring, int> vocabulary;
	std::vector<CString> unreadable;
	wchar_t word[WORDBUFFERLENGTH];

	for(const CString& path : docs)
	{
		CInputDocument indoc;
		indoc.m_bBasic_Characters = s.BasicCharacters;
		if(indoc.OpenDocument(path) > -1)
		{
			indoc.CloseDocument();
			unreadable.push_back(path);
			continue;
		}
		int delimiter = DEL_TYPE_WHITE;
		while(delimiter != DEL_TYPE_EOF)
		{
			if(indoc.GetWord(word, delimiter) > -1) break;
			if(s.IgnorePunctuation) WordRemovePunctuation(word);
			if(s.IgnoreOuterPunctuation) wordxouterpunct(word);
			if(s.IgnoreNumbers) WordRemoveNumbers(word);
			if(s.IgnoreCase) WordToLowerCase(word);
			if(word[0] == 0) continue;
			if(s.SkipLongWords && ((int)wcslen(word) > s.SkipLength)) continue;
			if(s.SkipNonwords && !WordCheck(word)) continue;
			vocabulary[word]++;
		}
		indoc.CloseDocument();
	}

	std::vector<std::pair<std::wstring, int>> words(vocabulary.begin(), vocabulary.end());
	std::sort(words.begin(), words.end(), [](const auto& a, const auto& b) { return a.second != b.second ? a.second > b.second : a.first < b.first; });
	std::wstring text;
	for(const auto& entry : words) text += std::to_wstring(entry.second) + L"\t" + entry.first + L"\r\n";

	FILE* file = nullptr;
	bool saved = false;
	if(_wfopen_s(&file, dlg.GetPathName(), L"wb") == 0 && file != nullptr)
	{
		int bytes = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), nullptr, 0, nullptr, nullptr);
		std::string utf8(bytes, '\0');
		WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), utf8.data(), bytes, nullptr, nullptr);
		fwrite("\xEF\xBB\xBF", 1, 3, file);
		saved = fwrite(utf8.data(), 1, utf8.size(), file) == utf8.size();
		fclose(file);
	}

	CString message;
	if(!saved) message = L"The vocabulary list could not be saved.";
	else
	{
		message.Format(L"Saved %d different words from %d documents.", (int)words.size(), (int)(docs.size() - unreadable.size()));
		if(!unreadable.empty())
		{
			message += L"\n\nThese documents couldn't be read:";
			for(size_t i = 0; i < unreadable.size() && i < 10; i++) message += L"\n" + unreadable[i];
			if(unreadable.size() > 10) message += L"\n...";
		}
	}
	AfxMessageBox(message, saved ? MB_ICONINFORMATION : MB_ICONWARNING);
}
