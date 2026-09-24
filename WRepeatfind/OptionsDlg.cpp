// OptionsDlg.cpp : the less commonly needed settings

#include "stdafx.h"
#include <algorithm>
#include "WRepeatfind.h"
#include "OptionsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static const wchar_t* LANGUAGES[] = {
	L"Chinese", L"Chinese-Simplified", L"Chinese-Traditional", L"Czech", L"Danish", L"Dutch", L"Dutch-Belgian",
	L"English", L"English-American", L"English-Aus", L"English-Can", L"English-Nz", L"English-Uk", L"Finnish",
	L"French", L"French-Belgian", L"French-Canadian", L"French-Swiss", L"German", L"German-Austrian", L"German-Swiss",
	L"Greek", L"Hungarian", L"Icelandic", L"Italian", L"Italian-Swiss", L"Japanese", L"Korean", L"Norwegian",
	L"Norwegian-Bokmal", L"Norwegian-Nynorsk", L"Polish", L"Portuguese", L"Portuguese-Brazilian", L"Russian",
	L"Slovak", L"Spanish", L"Spanish-Mexican", L"Spanish-Modern", L"Swedish", L"Turkish" };

COptionsDlg::COptionsDlg(CWnd* pParent)
	: CDialogEx(IDD_OPTIONS, pParent)
{
}

BEGIN_MESSAGE_MAP(COptionsDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_FOLDER, OnButtonFolder)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULTS, OnButtonDefaults)
	ON_BN_CLICKED(IDC_CHECK_SKIP_LONG, OnCheckSkipLong)
END_MESSAGE_MAP()

BOOL COptionsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_SPIN_PERCENTAGE))->SetRange32(1, 100);
	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_SPIN_SKIP_LENGTH))->SetRange32(1, 255);
	CComboBox* combo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_LANGUAGE));
	for(const wchar_t* language : LANGUAGES) combo->AddString(language);
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
	if(combo->SelectString(-1, m_Language) == CB_ERR) combo->SelectString(-1, CWRepeatfindApp::DefaultLanguage());
	SetDlgItemText(IDC_EDIT_FOLDER, m_ReportFolder);
	CheckDlgButton(IDC_CHECK_AUTO_OPEN, m_AutoOpenReport);
	GetDlgItem(IDC_CHECK_OUTER_PUNCTUATION)->EnableWindow(!m_Settings.IgnorePunctuation);
	OnCheckSkipLong();
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
	m_Settings = CWRepeatfindApp::DefaultSettings();
	m_Language = CWRepeatfindApp::DefaultLanguage();
	m_ReportFolder = CWRepeatfindApp::DefaultReportFolder();
	m_AutoOpenReport = true;
	WriteControls();											// the main window's settings are reset too, when OK is clicked
}

void COptionsDlg::OnOK()
{
	m_Settings.MismatchPercentage = (std::clamp)((int)GetDlgItemInt(IDC_EDIT_PERCENTAGE), 1, 100);
	m_Settings.IgnoreOuterPunctuation = IsDlgButtonChecked(IDC_CHECK_OUTER_PUNCTUATION) != 0;
	m_Settings.SkipNonwords = IsDlgButtonChecked(IDC_CHECK_SKIP_NONWORDS) != 0;
	m_Settings.SkipLongWords = IsDlgButtonChecked(IDC_CHECK_SKIP_LONG) != 0;
	m_Settings.SkipLength = (std::clamp)((int)GetDlgItemInt(IDC_EDIT_SKIP_LENGTH), 1, 255);
	m_Settings.BasicCharacters = IsDlgButtonChecked(IDC_CHECK_BASIC_CHARACTERS) != 0;
	GetDlgItemText(IDC_COMBO_LANGUAGE, m_Language);
	GetDlgItemText(IDC_EDIT_FOLDER, m_ReportFolder);
	m_ReportFolder.Trim();
	m_AutoOpenReport = IsDlgButtonChecked(IDC_CHECK_AUTO_OPEN) != 0;
	if(m_ReportFolder.IsEmpty()) m_ReportFolder = CWRepeatfindApp::DefaultReportFolder();
	CDialogEx::OnOK();
}
