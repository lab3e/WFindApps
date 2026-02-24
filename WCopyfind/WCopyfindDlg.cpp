
// WCopyfindDlg.cpp : implementation file
//

#include "stdafx.h"
#include <afxinet.h>
#include "afxdialogex.h"
#include "afxwin.h"
#include <stdlib.h>
#include <atlbase.h>
#include <atlstr.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "clib\InputDocument.h"
#include "clib\Words.h"
#include "clib\CompareDocuments.h"
#include "UiThread.h"
#include "WCopyfind.h"
#include "WCopyfindDlg.h"
#include "AbortDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CEdit m_Edit_About;
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_ABOUT, m_Edit_About);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CWCopyfindDlg dialog

CWCopyfindDlg::CWCopyfindDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CWCopyfindDlg::IDD, pParent)
	, m_ptMsg(0)
	, m_szMsg(_T(""))
	, m_Menu(0)
	, m_Sort_On_Load_New(false)
	, m_Sort_On_Load_Old(false)
	, m_pAbortDlg(NULL)
	, m_pUiThread(NULL)
	, m_pargs(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_ptMsg=CPoint(0,0);
}

void CWCopyfindDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_BRIEF_REPORT, m_Check_Brief_Report);
	DDX_Control(pDX, IDC_SPIN_PERCENTAGE, m_Spin_Percentage);
	DDX_Control(pDX, IDC_EDIT_PERCENTAGE, m_Edit_Percentage);
	DDX_Control(pDX, IDC_SPIN_TOLERANCE, m_Spin_Tolerance);
	DDX_Control(pDX, IDC_EDIT_TOLERANCE, m_Edit_Tolerance);
	DDX_Control(pDX, IDC_CHECK_SKIP_NONWORDS, m_Check_Skip_Nonwords);
	DDX_Control(pDX, IDC_CHECK_SKIP_LONG_WORDS, m_Check_Skip_Long_Words);
	DDX_Control(pDX, IDC_CHECK_IGNORE_OUTER_PUNCTUATION, m_Check_Ignore_Outer_Punctuation);
	DDX_Control(pDX, IDC_EDIT_SKIP_LENGTH, m_Edit_Skip_Length);
	DDX_Control(pDX, IDC_CHECK_IGNORE_PUNCTUATION, m_Check_Ignore_Punctuation);
	DDX_Control(pDX, IDC_CHECK_IGNORE_NUMBERS, m_Check_Ignore_Numbers);
	DDX_Control(pDX, IDC_CHECK_IGNORE_CASE, m_Check_Ignore_Case);
	DDX_Control(pDX, IDC_EDIT_THRESHOLD, m_Edit_Threshold);
	DDX_Control(pDX, IDC_SPIN_THRESHOLD, m_Spin_Threshold);
	DDX_Control(pDX, IDC_PROGRESS, m_Progress);
	DDX_Control(pDX, IDC_STATIC_STATUS, m_Static_Status);
	DDX_Control(pDX, IDC_EDIT_FOLDER, m_Edit_Folder);
	DDX_Control(pDX, IDC_EDIT_PHRASE, m_Edit_Phrase);
	DDX_Control(pDX, IDC_SPIN_PHRASE, m_Spin_Phrase);
	DDX_Control(pDX, IDC_LIST_REPORT, m_List_Report);
	DDX_Control(pDX, IDC_LIST_OLD, m_List_Old);
	DDX_Control(pDX, IDC_LIST_NEW, m_List_New);
	DDX_Control(pDX, IDC_COMBO_LANGUAGE, m_Combo_Language);
	DDX_Control(pDX, IDC_CHECK_BASIC_CHARACTERS, m_Check_Basic_Characters);
}

BEGIN_MESSAGE_MAP(CWCopyfindDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_RUN, OnButtonRun)
	ON_BN_CLICKED(IDC_BUTTON_VOCABULARY, OnButtonVocabulary)
	ON_EN_KILLFOCUS(IDC_EDIT_PERCENTAGE, OnKillfocusEditPercentage)
	ON_EN_KILLFOCUS(IDC_EDIT_TOLERANCE, OnKillfocusEditTolerance)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_REPORT, OnNMDblclkListReport)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_OLD, OnNMRclickListOld)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID__SAVETOFILEOLD, OnSaveToFileOld)
	ON_COMMAND(ID__LOADFROMFILEOLD, OnLoadFromFileOld)
	ON_COMMAND(ID__CLEARSELECTIONOLD, OnClearSelectionOld)
	ON_COMMAND(ID__CLEARALLOLD, OnClearAllOld)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_NEW, OnNMRclickListNew)
	ON_COMMAND(ID__LOADFROMFILENEW, OnLoadFromFileNew)
	ON_COMMAND(ID__SAVETOFILENEW, OnSaveToFileNew)
	ON_COMMAND(ID__CLEARSELECTIONNEW, OnClearSelectionNew)
	ON_COMMAND(ID__CLEARALLNEW, OnClearAll)
	ON_BN_CLICKED(IDC_BUTTON_FOLDER, OnBnClickedButtonFolder)
	ON_COMMAND(ID__SORTONLOADNEW, OnSortOnLoadNew)
	ON_COMMAND(ID__SORTONLOADOLD, OnSortOnLoadOld)
	ON_COMMAND(ID__BROWSEFORDOCUMENTSNEW, OnBrowseForDocumentsNew)
	ON_COMMAND(ID__BROWSEFORDOCUMENTSOLD, OnBrowseForDocumentsOld)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_OLD, OnNMDblclkListOld)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_NEW, OnNMDblclkListNew)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST_OLD, OnLvnKeydownListOld)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST_NEW, OnLvnKeydownListNew)
	ON_COMMAND(ID__SAVETOFILE, OnSaveToFileReport)
	ON_COMMAND(ID__CLEARSELECTION, OnClearSelectionReport)
	ON_COMMAND(ID__CLEARALL, OnClearAllReport)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_REPORT, OnNMRclickListReport)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST_REPORT, OnLvnKeydownListReport)
	ON_MESSAGE(WU_UITHREAD_TERMINATED, OnComparisonTerminated)
	ON_COMMAND(ID__VIEWREPORTINBROWSER, OnViewReportInBrowser)
END_MESSAGE_MAP()


// CWCopyfindDlg message handlers

BOOL CWCopyfindDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	CFileDropListCtrl::DROPLISTMODE OldDropMode;
	
	OldDropMode.iMask = CFileDropListCtrl::DL_ACCEPT_FILES;

	m_List_Old.SetDropMode(OldDropMode);

	RECT ListOldRect;
	m_List_Old.GetClientRect(&ListOldRect);
	int nVScrollBarWidth = ::GetSystemMetrics(SM_CXVSCROLL);
	int nListOldWidth = (ListOldRect.right - ListOldRect.left - nVScrollBarWidth);

	m_List_Old.InsertColumn(0, _T("Old Documents"), LVCFMT_LEFT, nListOldWidth,0);

	m_List_Old.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
	m_ImageListOld.Create(IDR_FILE_IMAGES, 16, 16, RGB(255,0,255));
	m_List_Old.SetImageList(&m_ImageListOld, LVSIL_SMALL);

	CFileDropListCtrl::DROPLISTMODE NewDropMode;
	
	NewDropMode.iMask = CFileDropListCtrl::DL_ACCEPT_FILES;

	m_List_New.SetDropMode(NewDropMode);

	RECT ListNewRect;
	m_List_New.GetClientRect(&ListNewRect);
	int nListNewWidth = (ListNewRect.right - ListNewRect.left - nVScrollBarWidth);

	m_List_New.InsertColumn(0, _T("New Documents"), LVCFMT_LEFT, nListNewWidth,0);

	m_List_New.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
	m_ImageListNew.Create(IDR_FILE_IMAGES, 16, 16, RGB(255,0,255));
	m_List_New.SetImageList(&m_ImageListNew, LVSIL_SMALL);
	
	RECT ListReportRect;
	m_List_Report.GetClientRect(&ListReportRect);
	int nListReportWidth = (ListReportRect.right - ListReportRect.left - nVScrollBarWidth);

	int nCol0ReportWidth = nListReportWidth * 1 / 4;
	int nCol1ReportWidth = nListReportWidth * 1 / 4;
	int nCol2ReportWidth = nListReportWidth * 1 / 4;
	int nCol3ReportWidth = nListReportWidth * 1 / 4;

	m_List_Report.InsertColumn(0, _T("Perfect Match"), LVCFMT_LEFT, nCol0ReportWidth,0);
	m_List_Report.InsertColumn(1, _T("Overall Match"), LVCFMT_LEFT, nCol1ReportWidth,1);
	m_List_Report.InsertColumn(2, _T("File L"), LVCFMT_LEFT, nCol2ReportWidth,2);
	m_List_Report.InsertColumn(3, _T("File R"), LVCFMT_LEFT, nCol3ReportWidth,3);
	m_List_Report.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	m_Spin_Phrase.SetRange(1,999);
	m_Spin_Threshold.SetRange(1,9999);
//	m_Spin_Stringlen.SetRange(1,WORDMAXIMUMLENGTH);
	m_Spin_Tolerance.SetRange(0,9);
	m_Spin_Percentage.SetRange(0,100);

	wchar_t sstring[256];
	CString szstring;

	m_Combo_Language.AddString(L"Chinese");
	m_Combo_Language.AddString(L"Chinese-Simplified");
	m_Combo_Language.AddString(L"Chinese-Traditional");
	m_Combo_Language.AddString(L"Czech");
	m_Combo_Language.AddString(L"Danish");
	m_Combo_Language.AddString(L"Dutch");
	m_Combo_Language.AddString(L"Dutch-Belgian");
	m_Combo_Language.AddString(L"English");
	m_Combo_Language.AddString(L"English-American");
	m_Combo_Language.AddString(L"English-Aus");
	m_Combo_Language.AddString(L"English-Can");
	m_Combo_Language.AddString(L"English-Nz");
	m_Combo_Language.AddString(L"English-Uk");
	m_Combo_Language.AddString(L"Finnish");
	m_Combo_Language.AddString(L"French");
	m_Combo_Language.AddString(L"French-Belgian");
	m_Combo_Language.AddString(L"French-Canadian");
	m_Combo_Language.AddString(L"French-Swiss");
	m_Combo_Language.AddString(L"German");
	m_Combo_Language.AddString(L"German-Austrian");
	m_Combo_Language.AddString(L"German-Swiss");
	m_Combo_Language.AddString(L"Greek");
	m_Combo_Language.AddString(L"Hungarian");
	m_Combo_Language.AddString(L"Icelandic");
	m_Combo_Language.AddString(L"Italian");
	m_Combo_Language.AddString(L"Italian-Swiss");
	m_Combo_Language.AddString(L"Japanese");
	m_Combo_Language.AddString(L"Korean");
	m_Combo_Language.AddString(L"Norwegian");
	m_Combo_Language.AddString(L"Norwegian-Bokmal");
	m_Combo_Language.AddString(L"Norwegian-Nynorsk");
	m_Combo_Language.AddString(L"Polish");
	m_Combo_Language.AddString(L"Portuguese");
	m_Combo_Language.AddString(L"Portuguese-Brazilian");
	m_Combo_Language.AddString(L"Russian");
	m_Combo_Language.AddString(L"Slovak");
	m_Combo_Language.AddString(L"Spanish");
	m_Combo_Language.AddString(L"Spanish-Mexican");
	m_Combo_Language.AddString(L"Spanish-Modern");
	m_Combo_Language.AddString(L"Swedish");
	m_Combo_Language.AddString(L"Turkish");
//	m_Combo_Language.SelectString(-1,L"English");
	
// reload settings from registry
	
	int value;

	GetRegistryValue(L"Phrase_Length",&value,6);
	_itow_s(value,sstring,10);
	m_Edit_Phrase.SetWindowTextW(sstring);

	GetRegistryValue(L"Report_Threshold",&value,100);
	_itow_s(value,sstring,10);
	m_Edit_Threshold.SetWindowTextW(sstring);

//	GetRegistryValue(L"String_Length",&value,100);
//	_itow_s(value,sstring,10);
//	m_Edit_Stringlen.SetWindowTextW(sstring);

	GetRegistryValue(L"Tolerance",&value,0);
	_itow_s(value,sstring,10);
	m_Edit_Tolerance.SetWindowTextW(sstring);

	GetRegistryValue(L"Percentage",&value,100);
	_itow_s(value,sstring,10);
	m_Edit_Percentage.SetWindowTextW(sstring);

	GetRegistryValue(L"Skip_Length",&value,20);
	_itow_s(value,sstring,10);
	m_Edit_Skip_Length.SetWindowTextW(sstring);
	
	GetRegistryValue(L"Ignore_Punctuation",&value,FALSE);
	m_Check_Ignore_Punctuation.SetCheck(value);

	GetRegistryValue(L"Ignore_Outer_Punctuation",&value,FALSE);
	m_Check_Ignore_Outer_Punctuation.SetCheck(value);

	GetRegistryValue(L"Ignore_Numbers",&value,FALSE);
	m_Check_Ignore_Numbers.SetCheck(value);

	GetRegistryValue(L"Ignore_Case",&value,FALSE);
	m_Check_Ignore_Case.SetCheck(value);

	GetRegistryValue(L"Skip_Long_Words",&value,FALSE);
	m_Check_Skip_Long_Words.SetCheck(value);

	GetRegistryValue(L"Skip_Nonwords",&value,FALSE);
	m_Check_Skip_Nonwords.SetCheck(value);

	GetRegistryValue(L"Basic_Characters",&value,FALSE);
	m_Check_Basic_Characters.SetCheck(value);

	GetRegistrySValue(L"Report_Folder",&szstring,L"C:\\WCopyfind\\Report");
	m_Edit_Folder.SetWindowTextW(szstring);
	
	GetRegistryValue(L"Brief_Report",&value,FALSE);
	m_Check_Brief_Report.SetCheck(value);

	GetRegistrySValue(L"Language",&szstring,L"English");
	m_Combo_Language.SelectString(-1,szstring);

// done reloading settings from registry

	m_Progress.SetPos(0);

	m_Menu=0;

	m_Sort_On_Load_Old=true;
	m_Sort_On_Load_New=true;

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CWCopyfindDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CWCopyfindDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CWCopyfindDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


	unsigned int  threadFunc( void* pArgument );

void CWCopyfindDlg::OnButtonRun() 
{
	if(m_pUiThread) return;

	extern std::atomic<bool> g_abort;
	g_abort = false;

	m_pAbortDlg = new CAbortDlg;
	m_pAbortDlg->Create(IDD_ABORTDLG, this);
	m_pAbortDlg->ShowWindow(SW_SHOW);

	m_pargs = new thread_args;
	m_pargs->hWnd = GetSafeHwnd();
	m_pargs->pDialog = this;
	m_pargs->returnVal = 0;
	
	m_pUiThread = (CUiThread*)AfxBeginThread(RUNTIME_CLASS(CUiThread));

	m_pUiThread->PostThreadMessage(WU_UITHREAD_START,0,(LPARAM)m_pargs);
}
LRESULT CWCopyfindDlg::OnComparisonTerminated(WPARAM, LPARAM)
{
	if(m_pAbortDlg){ 
		m_pAbortDlg->DestroyWindow();
		delete m_pAbortDlg; m_pAbortDlg = NULL;
	}
	if(m_pargs){
		delete m_pargs; m_pargs = NULL;
	}
	m_pUiThread->PostThreadMessage(WU_UITHREAD_TERMINATE,0,0);
	WaitForSingleObject(m_pUiThread->m_hThread,INFINITE);
	m_pUiThread = NULL;
	return 0;
}

void CWCopyfindDlg::OnButtonVocabulary() 
{
	FILE *fvocab = NULL;
	CString szExtension = L"txt";
	static wchar_t BASED_CODE szFilter[] = L"Text Files (*.txt)|*.txt|All Files (*.*)|*.*||";
	
	while(fvocab == NULL)
	{
		CFileDialog ifiledlg(FALSE,szExtension,NULL,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST,szFilter);
		if(ifiledlg.DoModal() != IDOK) return;
		_wfopen_s(&fvocab,ifiledlg.GetPathName(),L"w");
	}
	
	CString szstring;
	unsigned int skip_length;

	m_Edit_Skip_Length.GetWindowText(szstring);
	skip_length=_wtoi(szstring);

	int docs=m_List_Old.GetItemCount()+m_List_New.GetItemCount();
	
	wchar_t docname[256];
	wchar_t word[WORDBUFFERLENGTH];
	int fcount;
	std::unordered_map<std::wstring, int> vocab;
	CInputDocument indoc;
	int delimitertype;

	m_Static_Status.ShowWindow(SW_SHOW);
	m_Progress.ShowWindow(SW_SHOW);
	m_Static_Status.SetWindowTextW(L"Building Vocabulary");
	
	bool bignore_case = (m_Check_Ignore_Case.GetCheck() == TRUE);
	bool bignore_numbers = (m_Check_Ignore_Numbers.GetCheck() == TRUE);
	bool bignore_punctuation = (m_Check_Ignore_Punctuation.GetCheck() == TRUE);
	bool bignore_outer_punctuation = (m_Check_Ignore_Outer_Punctuation.GetCheck() == TRUE);
	bool bskip_long_words = (m_Check_Skip_Long_Words.GetCheck() == TRUE);
	bool bskip_nonwords = (m_Check_Skip_Nonwords.GetCheck() == TRUE);
	bool bBasic_Characters = (m_Check_Basic_Characters.GetCheck() == TRUE);

	int irvalue;

	for(fcount=0;fcount<m_List_Old.GetItemCount();fcount++)
	{
		m_Progress.SetPos(fcount*100/docs);
		wcscpy_s(docname,256,m_List_Old.GetItemText(fcount,0));
		irvalue = indoc.OpenDocument(docname);
		if(irvalue > -1)
		{
			indoc.CloseDocument();							// close document
			continue;
		}

		delimitertype=DEL_TYPE_WHITE;
		while(delimitertype != DEL_TYPE_EOF)
		{
			indoc.GetWord(word,delimitertype);

			if(bignore_punctuation) WordRemovePunctuation(word);	// if ignore punctuation is active, remove punctuation
			if(bignore_outer_punctuation) wordxouterpunct(word);	// if ignore outer punctuation is active, remove outer punctuation
			if(bignore_numbers) WordRemoveNumbers(word);			// if ignore numbers is active, remove numbers
			if(bignore_case) WordToLowerCase(word);			// if ignore case is active, remove case
			if( bskip_long_words && (wcslen(word)>skip_length) ) continue;	// if skip too-long words is active, skip them
			if( bskip_nonwords && (!WordCheck(word)) ) continue;	// if skip nonwords is active, skip them
			
			vocab[word]++;
		}
		indoc.CloseDocument();
	}
	
	for(fcount=0;fcount<m_List_New.GetItemCount();fcount++)
	{
		m_Progress.SetPos((m_List_Old.GetItemCount()+fcount)*100/docs);
		wcscpy_s(docname,256,m_List_New.GetItemText(fcount,0));
		irvalue = indoc.OpenDocument(docname);
		if(irvalue > -1)
		{
			indoc.CloseDocument();							// close document
			continue;
		}
		delimitertype=DEL_TYPE_WHITE;
		while(delimitertype != DEL_TYPE_EOF)
		{
			indoc.GetWord(word,delimitertype);

			if(bignore_punctuation) WordRemovePunctuation(word);	// if ignore punctuation is active, remove punctuation
			if(bignore_outer_punctuation) wordxouterpunct(word);	// if ignore outer punctuation is active, remove outer punctuation
			if(bignore_numbers) WordRemoveNumbers(word);			// if ignore numbers is active, remove numbers
			if(bignore_case) WordToLowerCase(word);			// if ignore case is active, remove case
			if( bskip_long_words && (wcslen(word)>skip_length) ) continue;	// if skip too-long words is active, skip them
			if( bskip_nonwords && (!WordCheck(word)) ) continue;	// if skip nonwords is active, skip them
			
			vocab[word]++;
		}
		indoc.CloseDocument();
	}

	m_Static_Status.SetWindowTextW(L"Saving Vocabulary to File");
	for (const auto& entry : vocab)
		fwprintf(fvocab, L"%d\t%s\n", entry.second, entry.first.c_str());
	fclose(fvocab);

	m_Static_Status.ShowWindow(SW_HIDE);
	m_Progress.ShowWindow(SW_HIDE);
}

void CWCopyfindDlg::OnOK() 
{
// save settings to registry
	
	int value;
	CString szstring;
	wchar_t string[256];

	m_Edit_Phrase.GetWindowText(szstring);
	value=_wtoi(szstring);
	SetRegistryValue(L"Phrase_Length",value);

	m_Edit_Threshold.GetWindowText(szstring);
	value=_wtoi(szstring);
	SetRegistryValue(L"Report_Threshold",value);

	m_Edit_Tolerance.GetWindowText(szstring);
	value=_wtoi(szstring);
	SetRegistryValue(L"Tolerance",value);

	m_Edit_Percentage.GetWindowText(szstring);
	value=_wtoi(szstring);
	SetRegistryValue(L"Percentage",value);

	m_Edit_Skip_Length.GetWindowText(szstring);
	value=_wtoi(szstring);
	SetRegistryValue(L"Skip_Length",value);

	value=m_Check_Ignore_Punctuation.GetCheck();
	SetRegistryValue(L"Ignore_Punctuation",value);

	value=m_Check_Ignore_Outer_Punctuation.GetCheck();
	SetRegistryValue(L"Ignore_Outer_Punctuation",value);

	value=m_Check_Ignore_Numbers.GetCheck();
	SetRegistryValue(L"Ignore_Numbers",value);

	value=m_Check_Ignore_Case.GetCheck();
	SetRegistryValue(L"Ignore_Case",value);

	value=m_Check_Skip_Long_Words.GetCheck();
	SetRegistryValue(L"Skip_Long_Words",value);

	value=m_Check_Skip_Nonwords.GetCheck();
	SetRegistryValue(L"Skip_Nonwords",value);

	value=m_Check_Basic_Characters.GetCheck();
	SetRegistryValue(L"Basic_Characters",value);

	m_Edit_Folder.GetWindowText(string,256);
	SetRegistrySValue(L"Report_Folder",string);

	value=m_Check_Brief_Report.GetCheck();
	SetRegistryValue(L"Brief_Report",value);

	m_Combo_Language.GetWindowTextW(string,256);
	SetRegistrySValue(L"Language",string);

// done saving settings to registry
	
	CDialog::OnOK();
}

void CWCopyfindDlg::GetRegistryValue(const wchar_t *name, int *value, int valuedefault)
{
	HKEY HKEY_Software,HKEY_WCopyfind;
	unsigned long RegResult;
	unsigned long lvalue;
	unsigned long ltype;
	unsigned long llength;

	ltype=REG_DWORD;
	llength=4;
	lvalue=valuedefault;

	if(RegCreateKeyEx(HKEY_CURRENT_USER,L"Software",0,nullptr,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&HKEY_Software,&RegResult) == ERROR_SUCCESS)
	{
		if(RegCreateKeyEx(HKEY_Software,L"WCopyfind",0,nullptr,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&HKEY_WCopyfind,&RegResult) == ERROR_SUCCESS)
		{
            if(RegQueryValueEx(HKEY_WCopyfind,name,0,&ltype,(LPBYTE)&lvalue,&llength) != ERROR_SUCCESS)
			{
				lvalue=valuedefault;
			}
			RegCloseKey(HKEY_WCopyfind);
		}
		RegCloseKey(HKEY_Software);
	}

	*value=lvalue;
}

void CWCopyfindDlg::SetRegistryValue(const wchar_t *name, int value)
{
	HKEY HKEY_Software,HKEY_WCopyfind;
	unsigned long RegResult;
	unsigned long lvalue;
	unsigned long ltype;
	unsigned long llength;

	ltype=REG_DWORD;
	llength=4;
	lvalue=value;

	if(RegCreateKeyEx(HKEY_CURRENT_USER,L"Software",0,nullptr,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&HKEY_Software,&RegResult) == ERROR_SUCCESS)
	{
        if(RegCreateKeyEx(HKEY_Software,L"WCopyfind",0,nullptr,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&HKEY_WCopyfind,&RegResult) == ERROR_SUCCESS)
		{
            RegSetValueEx(HKEY_WCopyfind,name,0,ltype,(LPBYTE)&lvalue,llength);
			RegCloseKey(HKEY_WCopyfind);
		}
		RegCloseKey(HKEY_Software);
	}
}

void CWCopyfindDlg::GetRegistrySValue(const wchar_t *name, CString *szvalue, const wchar_t *svaluedefault)
{
	HKEY HKEY_Software,HKEY_WCopyfind;
	unsigned long RegResult;

	RegCreateKeyEx(HKEY_CURRENT_USER,L"Software",0,nullptr,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&HKEY_Software,&RegResult);
	RegCreateKeyEx(HKEY_Software,L"WCopyfind",0,nullptr,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&HKEY_WCopyfind,&RegResult);

	unsigned long ltype;
	unsigned long llength;

	ltype=REG_SZ;
	llength=256;

	char string8[256];

	if(RegQueryValueEx(HKEY_WCopyfind,name,0,&ltype,(LPBYTE)string8,&llength) == ERROR_SUCCESS)
	{
		*szvalue=string8;
	}
	else
	{
		*szvalue=*svaluedefault;
	}
	RegCloseKey(HKEY_WCopyfind);
	RegCloseKey(HKEY_Software);
}

void CWCopyfindDlg::SetRegistrySValue(const wchar_t *name, const wchar_t *svalue)
{
	HKEY HKEY_Software,HKEY_WCopyfind;
	unsigned long RegResult;

	RegCreateKeyEx(HKEY_CURRENT_USER,L"Software",0,nullptr,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&HKEY_Software,&RegResult);
	RegCreateKeyEx(HKEY_Software,L"WCopyfind",0,nullptr,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&HKEY_WCopyfind,&RegResult);

	unsigned long ltype;

	ltype=REG_SZ;

	char string8[256];
	size_t string8Length;

	wcstombs_s(&string8Length,string8,256,svalue,_TRUNCATE);

	RegSetValueEx(HKEY_WCopyfind,name,0,ltype,(const BYTE*)string8,(DWORD)string8Length);
	RegCloseKey(HKEY_WCopyfind);
	RegCloseKey(HKEY_Software);
}

void CWCopyfindDlg::OnKillfocusEditPercentage() 
{
	int percentage;
	CString szstring;
	wchar_t sstring[10];
	m_Edit_Percentage.GetWindowText(szstring);
	percentage=_wtoi(szstring);
	if(percentage>100) percentage=100;
	if(percentage<0) percentage=0;
	_itow_s(percentage,sstring,10);
	m_Edit_Percentage.SetWindowTextW(sstring);
}

void CWCopyfindDlg::OnKillfocusEditTolerance() 
{
	int tolerance;
	CString szstring;
	wchar_t sstring[10];
	m_Edit_Tolerance.GetWindowText(szstring);
	tolerance=_wtoi(szstring);
	if(tolerance>9) tolerance=9;
	if(tolerance<0) tolerance=0;
	_itow_s(tolerance,sstring,10);
	m_Edit_Tolerance.SetWindowTextW(sstring);
}

void CWCopyfindDlg::OnNMDblclkListReport(NMHDR *pNMHDR, LRESULT *pResult)
{
	int nitem;
	CString szreportfileL;
	CString szreportfileR;
	wchar_t szdocL[256];
	wchar_t szdocR[256];
	CString szfolder;
	CString szcommand;

	nitem=m_List_Report.GetSelectionMark();
	m_Edit_Folder.GetWindowText(szfolder);
	m_List_Report.GetItemText(nitem,2,szdocL,256);
	m_List_Report.GetItemText(nitem,3,szdocR,256);

	szreportfileL = szfolder + L"\\" + szdocL + L"." + szdocR + L".html";
	szreportfileR = szfolder + L"\\" + szdocR + L"." + szdocL + L".html";

    HINSTANCE hinst1 = ShellExecute(NULL, // no parent hwnd
            NULL, // open
            szreportfileL, // file name
            NULL, // no parameters
            NULL, // no directory name
            SW_SHOWNORMAL); // yes, show it

    HINSTANCE hinst2 = ShellExecute(NULL, // no parent hwnd
            NULL, // open
            szreportfileR, // file name
            NULL, // no parameters
            NULL, // no directory name
            SW_SHOWNORMAL); // yes, show it

	*pResult = 0;
}

void CWCopyfindDlg::OnNMRclickListOld(NMHDR *pNMHDR, LRESULT *pResult)
{
//	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	m_Menu=1;
	*pResult = 0;
}

void CWCopyfindDlg::OnNMRclickListNew(NMHDR *pNMHDR, LRESULT *pResult)
{
//	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	m_Menu=2;
	*pResult = 0;
}

void CWCopyfindDlg::OnNMRclickListReport(NMHDR *pNMHDR, LRESULT *pResult)
{
//	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	m_Menu=3;
	*pResult = 0;
}

void CWCopyfindDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{

	if(m_Menu == 1)
	{
		CMenu oldMenu;
		m_ptMsg = point;
		ScreenToClient( &m_ptMsg);
		oldMenu.LoadMenu( IDR_POPUP_OLD );
		if(m_Sort_On_Load_Old)
		{
			oldMenu.CheckMenuItem(ID__SORTONLOADOLD,MF_BYCOMMAND|MF_CHECKED);
		}
		else
		{
			oldMenu.CheckMenuItem(ID__SORTONLOADOLD,MF_BYCOMMAND|MF_UNCHECKED);
		}
		CMenu* pPopup = oldMenu.GetSubMenu(0);
		pPopup->TrackPopupMenu( TPM_LEFTALIGN|TPM_RIGHTBUTTON,point.x,point.y,this );
	}
	else if(m_Menu == 2)
	{
		CMenu newMenu;
		m_ptMsg = point;
		ScreenToClient( &m_ptMsg);
		newMenu.LoadMenu( IDR_POPUP_NEW );
		if(m_Sort_On_Load_New)
		{
			newMenu.CheckMenuItem(ID__SORTONLOADNEW,MF_BYCOMMAND|MF_CHECKED);
		}
		else
		{
			newMenu.CheckMenuItem(ID__SORTONLOADNEW,MF_BYCOMMAND|MF_UNCHECKED);
		}
		CMenu* pPopup = newMenu.GetSubMenu(0);
		pPopup->TrackPopupMenu( TPM_LEFTALIGN|TPM_RIGHTBUTTON,point.x,point.y,this );
	}
	else if(m_Menu == 3)
	{
		CMenu reportMenu;
		m_ptMsg = point;
		ScreenToClient( &m_ptMsg);
		reportMenu.LoadMenu( IDR_POPUP_REPORT );
		CMenu* pPopup = reportMenu.GetSubMenu(0);
		pPopup->TrackPopupMenu( TPM_LEFTALIGN|TPM_RIGHTBUTTON,point.x,point.y,this );
	}
	else if(m_Menu == 4)
	{
		CMenu languageMenu;
		m_ptMsg = point;
		ScreenToClient( &m_ptMsg);
		languageMenu.LoadMenu( IDR_POPUP_LANGUAGE );
		CMenu* pPopup = languageMenu.GetSubMenu(0);
		pPopup->TrackPopupMenu( TPM_LEFTALIGN|TPM_RIGHTBUTTON,point.x,point.y,this );
	}
	m_Menu = 0;
}

void CWCopyfindDlg::OnLoadFromFileOld()
{
	FILE *fin;
	CString szExtension = L"txt";
	static wchar_t BASED_CODE szFilter[] = L"Text Files (*.txt)|*.txt|All Files (*.*)|*.*||";

	while(true)
	{
		CFileDialog ifiledlg(TRUE,szExtension,NULL,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_FILEMUSTEXIST,szFilter);
		if(ifiledlg.DoModal() != IDOK)
		{
			return;
		}
		_wfopen_s(&fin,ifiledlg.GetPathName(),L"r");
		if(fin != NULL) break;
	}

	wchar_t sstring[256];
	
	while(fgetws(sstring,255,fin) != NULL)
	{
		sstring[wcslen(sstring)-1]=0;		// bump out the end-of-line character
		LVFINDINFO lvInfo;
		lvInfo.flags = LVFI_STRING;
		lvInfo.psz = sstring;

		if(m_List_Old.FindItem(&lvInfo, -1) != -1)
			continue;
		
		m_List_Old.InsertItem(m_List_Old.GetItemCount(),sstring);
	}
		
	fclose(fin);

	return;
}

void CWCopyfindDlg::OnSaveToFileOld()
{
	FILE *fout;
	CString szExtension = L"txt";
	static wchar_t BASED_CODE szFilter[] = L"Text Files (*.txt)|*.txt|All Files (*.*)|*.*||";

	while(true)
	{
		CFileDialog ifiledlg(FALSE,szExtension,NULL,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST,szFilter);
		if(ifiledlg.DoModal() != IDOK)
		{
			return;
		}
		_wfopen_s(&fout,ifiledlg.GetPathName(),L"w");
		if(fout != NULL) break;
	}

	int fcount=m_List_Old.GetItemCount();

	for(int count=0;count<fcount;count++)
	{
		fwprintf(fout,L"%s\n",(LPCWSTR)m_List_Old.GetItemText(count,0));
	}

	fclose(fout);

	return;
}

void CWCopyfindDlg::OnClearSelectionOld()
{
	int nItem;
	POSITION pos;

	while(TRUE)
	{
		pos = m_List_Old.GetFirstSelectedItemPosition();
		if(pos == NULL) break;
		nItem = m_List_Old.GetNextSelectedItem(pos);
		m_List_Old.DeleteItem(nItem);
	}
	
	return;
}

void CWCopyfindDlg::OnClearAllOld()
{
	m_List_Old.DeleteAllItems();	
}

void CWCopyfindDlg::OnLoadFromFileNew()
{
	FILE *fin;
	CString szExtension = L"txt";
	static wchar_t BASED_CODE szFilter[] = L"Text Files (*.txt)|*.txt|All Files (*.*)|*.*||";

	while(true)
	{
		CFileDialog ifiledlg(TRUE,szExtension,NULL,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_FILEMUSTEXIST,szFilter);
		if(ifiledlg.DoModal() != IDOK)
		{
			return;
		}
		_wfopen_s(&fin,ifiledlg.GetPathName(),L"r");
		if(fin != NULL) break;
	}

	wchar_t sstring[256];
	
	while(fgetws(sstring,255,fin) != NULL)
	{
		sstring[wcslen(sstring)-1]=0;		// bump out the end-of-line character
		LVFINDINFO lvInfo;
		lvInfo.flags = LVFI_STRING;
		lvInfo.psz = sstring;

		if(m_List_New.FindItem(&lvInfo, -1) != -1)
			continue;
		
		m_List_New.InsertItem(m_List_New.GetItemCount(),sstring);
	}
		
	fclose(fin);

	return;
}

void CWCopyfindDlg::OnSaveToFileNew()
{
	FILE *fout;
	CString szExtension = L"txt";
	static wchar_t BASED_CODE szFilter[] = L"Text Files (*.txt)|*.txt|All Files (*.*)|*.*||";

	while(true)
	{
		CFileDialog ifiledlg(FALSE,szExtension,NULL,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST,szFilter);
		if(ifiledlg.DoModal() != IDOK)
		{
			return;
		}
		_wfopen_s(&fout,ifiledlg.GetPathName(),L"w");
		if(fout != NULL) break;
	}

	int fcount=m_List_New.GetItemCount();

	for(int count=0;count<fcount;count++)
	{
		fwprintf(fout,L"%s\n",(LPCWSTR)m_List_New.GetItemText(count,0));
	}

	fclose(fout);

	return;
}

void CWCopyfindDlg::OnClearSelectionNew()
{
	int nItem;
	POSITION pos;

	while(TRUE)
	{
		pos = m_List_New.GetFirstSelectedItemPosition();
		if(pos == NULL) break;
		nItem = m_List_New.GetNextSelectedItem(pos);
		m_List_New.DeleteItem(nItem);
	}
	
	return;
}

void CWCopyfindDlg::OnClearAll()
{
	m_List_New.DeleteAllItems();
}

void CWCopyfindDlg::OnBnClickedButtonFolder()
{
	wchar_t sfoldername[1000];
	CString sztitle;
	sztitle=L"Select Reporting Folder";
		
	LPBROWSEINFO lpbi;
	BROWSEINFO pbi;

	pbi.hwndOwner = NULL;
	pbi.pidlRoot = NULL;
	pbi.pszDisplayName = sfoldername;
	pbi.lpszTitle = sztitle;
	pbi.ulFlags = BIF_USENEWUI;
	pbi.lpfn = NULL;
	pbi.lParam = NULL;
	pbi.iImage = 0;

	LPITEMIDLIST lpitem;

	lpbi= &pbi;
	lpitem = SHBrowseForFolder(lpbi); 

	if(lpitem==NULL)
	{
		return;
	}

    wchar_t szPath[1000];
	LPWSTR pszPath;
	pszPath=szPath;
	SHGetPathFromIDList(lpitem,pszPath);
	ILFree(lpitem);
	m_Edit_Folder.SetWindowTextW(szPath);
}

void CWCopyfindDlg::OnSortOnLoadNew()
{
	m_Sort_On_Load_New = !m_Sort_On_Load_New;
	long istyles;
	istyles = GetWindowLong(m_List_New,GWL_STYLE);
	
	if( m_Sort_On_Load_New )
	{
		istyles = istyles | LVS_SORTASCENDING;
	}
	else
	{
		istyles = istyles & ~LVS_SORTASCENDING;
	}
	SetWindowLong(m_List_New,GWL_STYLE,istyles);
}

void CWCopyfindDlg::OnSortOnLoadOld()
{
	m_Sort_On_Load_Old = !m_Sort_On_Load_Old;
	long istyles;
	istyles = GetWindowLong(m_List_Old,GWL_STYLE);
	
	if( m_Sort_On_Load_Old )
	{
		istyles = istyles | LVS_SORTASCENDING;
	}
	else
	{
		istyles = istyles & ~LVS_SORTASCENDING;
	}
	SetWindowLong(m_List_Old,GWL_STYLE,istyles);
}

void CWCopyfindDlg::OnBrowseForDocumentsNew()
{
	CString szExtension = L"txt";
	static wchar_t BASED_CODE szFilter[] = L"All Files (*.*)|*.*||";
	
	CFileDialog* ifiledlg;
	ifiledlg = new CFileDialog(TRUE,szExtension,NULL,OFN_HIDEREADONLY|OFN_FILEMUSTEXIST|OFN_NODEREFERENCELINKS|OFN_ALLOWMULTISELECT,szFilter);
	std::vector<wchar_t> pstrBuf(10000000);
	LPWSTR pstr = pstrBuf.data();
	pstr[0] = L'\0';
	ifiledlg->m_pOFN->lpstrFile=pstr;
	ifiledlg->m_pOFN->nMaxFile=10000000;
	ifiledlg->m_pOFN->lpstrTitle=L"Select New Documents to Include in the Comparison";
	if(ifiledlg->DoModal() != IDOK)
	{
		if(ifiledlg != NULL) {delete ifiledlg; ifiledlg=NULL;}
		return;
	}

	CString szpath;
	POSITION pos;
	pos=ifiledlg->GetStartPosition();

	while(pos != NULL)
	{
		szpath=ifiledlg->GetNextPathName(pos);
		LVFINDINFO lvInfo;
		lvInfo.flags = LVFI_STRING;
		lvInfo.psz = szpath;
		if(m_List_New.FindItem(&lvInfo, -1) != -1)
			continue;

		m_List_New.InsertItem(m_List_New.GetItemCount(),szpath);
	}
	if(ifiledlg != NULL) {delete ifiledlg; ifiledlg=NULL;}
	return;
}

void CWCopyfindDlg::OnBrowseForDocumentsOld()
{
	CString szExtension = L"txt";
	static wchar_t BASED_CODE szFilter[] = L"All Files (*.*)|*.*||";
	
	CFileDialog* ifiledlg=NULL;
	ifiledlg = new CFileDialog(TRUE,szExtension,NULL,OFN_HIDEREADONLY|OFN_FILEMUSTEXIST|OFN_NODEREFERENCELINKS|OFN_ALLOWMULTISELECT,szFilter);
	std::vector<wchar_t> pstrBuf(10000000);
	LPWSTR pstr = pstrBuf.data();
	pstr[0] = L'\0';
	ifiledlg->m_pOFN->lpstrFile=pstr;
	ifiledlg->m_pOFN->nMaxFile=10000000;
	ifiledlg->m_pOFN->lpstrTitle=L"Select Old Documents to Include in the Comparison";
	if(ifiledlg->DoModal() != IDOK)
	{
		if(ifiledlg != NULL) {delete ifiledlg; ifiledlg=NULL;}
		return;
	}

	CString szpath;
	POSITION pos;
	pos=ifiledlg->GetStartPosition();

	while(pos != NULL)
	{
		szpath=ifiledlg->GetNextPathName(pos);
		LVFINDINFO lvInfo;
		lvInfo.flags = LVFI_STRING;
		lvInfo.psz = szpath;
		if(m_List_Old.FindItem(&lvInfo, -1) != -1)
			continue;
		
		m_List_Old.InsertItem(m_List_Old.GetItemCount(),szpath);
	}
	if(ifiledlg != NULL) {delete ifiledlg; ifiledlg=NULL;}
	return;
}

void CWCopyfindDlg::OnNMDblclkListOld(NMHDR *pNMHDR, LRESULT *pResult)
{
	OnBrowseForDocumentsOld();
	*pResult = 0;
}

void CWCopyfindDlg::OnNMDblclkListNew(NMHDR *pNMHDR, LRESULT *pResult)
{
	OnBrowseForDocumentsNew();
	*pResult = 0;
}

void CWCopyfindDlg::OnLvnKeydownListOld(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	int key=pLVKeyDow->wVKey;
	if(key == 0x2E)
	{
		OnClearSelectionOld();
	}
	*pResult = 0;
}

void CWCopyfindDlg::OnLvnKeydownListNew(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	int key=pLVKeyDow->wVKey;
	if(key == 0x2E)
	{
		OnClearSelectionNew();
	}
	*pResult = 0;
}

void CWCopyfindDlg::OnSaveToFileReport()
{
	FILE *fout;
	CString szExtension = L"txt";
	static wchar_t BASED_CODE szFilter[] = L"Text Files (*.txt)|*.txt|All Files (*.*)|*.*||";

	while(true)
	{
		CFileDialog ifiledlg(FALSE,szExtension,NULL,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST,szFilter);
		if(ifiledlg.DoModal() != IDOK)
		{
			return;
		}
		_wfopen_s(&fout,ifiledlg.GetPathName(),L"w");
		if(fout != NULL) break;
	}

	int fcount=m_List_Report.GetItemCount();

	for(int count=0;count<fcount;count++)
	{
		fwprintf(fout,L"%s\t%s\t%s\t%s\n",
			(LPCWSTR)m_List_Report.GetItemText(count,0),
			(LPCWSTR)m_List_Report.GetItemText(count,1),
			(LPCWSTR)m_List_Report.GetItemText(count,2),
			(LPCWSTR)m_List_Report.GetItemText(count,3));
	}

	fclose(fout);

	return;
}

void CWCopyfindDlg::OnClearSelectionReport()
{
	int nItem;
	POSITION pos;

	while(TRUE)
	{
		pos = m_List_Report.GetFirstSelectedItemPosition();
		if(pos == NULL) break;
		nItem = m_List_Report.GetNextSelectedItem(pos);
		m_List_Report.DeleteItem(nItem);
	}
	
	return;
}

void CWCopyfindDlg::OnClearAllReport()
{
	m_List_Report.DeleteAllItems();	
}

void CWCopyfindDlg::OnLvnKeydownListReport(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	int key=pLVKeyDow->wVKey;
	if(key == 0x2E)
	{
		OnClearSelectionReport();
	}
	*pResult = 0;
}

void CWCopyfindDlg::OnViewReportInBrowser()
{
	CString szfolder;
	m_Edit_Folder.GetWindowText(szfolder);
	CString szbrowser;
	szbrowser.Format(L"%s%s",szfolder,L"/matches.html");

	HINSTANCE hinst1 = ShellExecute(NULL, // no parent hwnd
        NULL, // open
        szbrowser, // file name
        NULL, // no parameters
        NULL, // no directory name
        SW_SHOWNORMAL); // yes, show it
}


BOOL CAboutDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_Edit_About.SetWindowText(L"This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.\r\n\r\nThis program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.\r\n\r\nYou should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.\r\n\r\nLouis A. Bloomfield, Department of Physics, University of Virginia\r\n382 McCormick Road, P.O. Box 400714, Charlottesville, VA 22904-4714\r\nemail: bloomfield@virginia.edu\r\nweb site: www.plagiarism.phys.virginia.edu \r\n            (see this web site for latest version of copyfind)\r\n\r\nIf you significantly improve this program, please let me know about it and I will consider distributing your version from my web site.\r\n\r\n\tThanks to:\r\n\r\n\tHandling of dropped files obtained from:\r\n\t\tCFileDropListCtrl, 2000 Stuart Carter\r\n\tHandling of dropped files modified from:\r\n\t\tCDropEdit, 1997 Chris Losinger\r\n\t\thttp://www.codeguru.com/editctrl/filedragedit.shtml\r\n\tShortcut expansion code modified from:\r\n\t\tCShortcut, 1996 Rob Warner\r\n\tThreading code structure thanks to:\r\n\t\tLenop, 2002 Clemons Schmidt\r\n\t\thttp://www.codeguru.com/misc/lenop.html");
	m_Edit_About.SetReadOnly(TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}