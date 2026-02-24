// UiThread.cpp : implementation file
//

#include "stdafx.h"
#include <afxinet.h>
#include <sys/stat.h>
#include <locale.h>
#include "WCopyfind.h"
#include "clib\InputDocument.h"
#include "clib\Words.h"
#include "clib\CompareDocuments.h"
#include "UiThread.h"
#include "WCopyfindDlg.h"


// CUiThread

IMPLEMENT_DYNCREATE(CUiThread, CWinThread)

CUiThread::CUiThread()
{
	m_pDoc=NULL;
}

CUiThread::~CUiThread()
{
	if (m_pDoc != NULL) {delete m_pDoc; m_pDoc=NULL;}
}

BOOL CUiThread::InitInstance()
{
	// TODO:  perform and per-thread initialization here
	return TRUE;
}

int CUiThread::ExitInstance()
{
	// TODO:  perform any per-thread cleanup here
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CUiThread, CWinThread)
	ON_THREAD_MESSAGE(WU_UITHREAD_START, OnStart)
	ON_THREAD_MESSAGE(WU_UITHREAD_TERMINATE, OnTerminate)
END_MESSAGE_MAP()


// CUiThread message handlers

void CUiThread::OnStart(WPARAM, LPARAM lp)
{
	thread_args* pargs = (thread_args*)lp;
	extern std::atomic<bool> g_abort;
	CWCopyfindDlg *pDialog = (CWCopyfindDlg*)pargs->pDialog;
	CString szMessage;

	CString szfolder;

	int irvalue;

	pDialog->m_Edit_Folder.GetWindowText(szfolder);
	struct _stat buf;
	int result = _tstat( szfolder, &buf );
	if( (result != 0) || ((buf.st_mode & _S_IFDIR) != _S_IFDIR))
	{
		pDialog->m_Progress.ShowWindow(SW_HIDE);
		szMessage.Format(L"Report Folder %s cannot be found. You must make sure a Report Folder exists before running WCopyfind.",szfolder);
		AfxMessageBox(szMessage);

		CWnd::FromHandle(pargs->hWnd)->PostMessage(WU_UITHREAD_TERMINATED,1,1);
		return;
	}
	
	int docs = pDialog->m_List_Old.GetItemCount() + pDialog->m_List_New.GetItemCount();
	if( ((m_pDoc = new CCompareDocuments(docs)) == NULL) || (m_pDoc->m_pDocs == NULL) )
	{
		pDialog->m_Progress.ShowWindow(SW_HIDE);
		AfxMessageBox(L"Could not allocate enough memory for the documents.");

		if (m_pDoc != NULL) {delete m_pDoc; m_pDoc=NULL;}
		CWnd::FromHandle(pargs->hWnd)->PostMessage(WU_UITHREAD_TERMINATED,1,1);
		return;
	}

	m_pDoc->m_szSoftwareName=L"WCopyfind.5.0.0";

	int fcount;
	int firstNewDoc = pDialog->m_List_Old.GetItemCount();

	for(fcount=0;fcount<pDialog->m_List_Old.GetItemCount();fcount++)
	{
		(m_pDoc->m_pDocs+fcount)->m_szDocumentName=pDialog->m_List_Old.GetItemText(fcount,0);
		(m_pDoc->m_pDocs+fcount)->m_DocumentType=DOC_TYPE_OLD;
	}
	for(fcount=0;fcount<pDialog->m_List_New.GetItemCount();fcount++)
	{
		(m_pDoc->m_pDocs+firstNewDoc+fcount)->m_szDocumentName=pDialog->m_List_New.GetItemText(fcount,0);
		(m_pDoc->m_pDocs+firstNewDoc+fcount)->m_DocumentType=DOC_TYPE_NEW;
	}

	CString szstring;
	pDialog->m_Edit_Phrase.GetWindowText(szstring);
	m_pDoc->m_PhraseLength = _wtoi(szstring);
	pDialog->m_Edit_Threshold.GetWindowText(szstring);
	m_pDoc->m_WordThreshold=_wtoi(szstring);
	pDialog->m_Edit_Skip_Length.GetWindowText(szstring);
	m_pDoc->m_SkipLength=_wtoi(szstring);
	pDialog->m_Edit_Tolerance.GetWindowText(szstring);
	m_pDoc->m_MismatchTolerance=_wtoi(szstring);
	pDialog->m_Edit_Percentage.GetWindowText(szstring);
	m_pDoc->m_MismatchPercentage=_wtoi(szstring);

	pDialog->m_Static_Status.ShowWindow(SW_SHOW);
	pDialog->m_Progress.ShowWindow(SW_SHOW);
	
	m_pDoc->m_bBriefReport = (pDialog->m_Check_Brief_Report.GetCheck() == TRUE);
	m_pDoc->m_bIgnoreCase = (pDialog->m_Check_Ignore_Case.GetCheck() == TRUE);
	m_pDoc->m_bIgnoreNumbers = (pDialog->m_Check_Ignore_Numbers.GetCheck() == TRUE);
	m_pDoc->m_bIgnoreOuterPunctuation = (pDialog->m_Check_Ignore_Outer_Punctuation.GetCheck() == TRUE);
	m_pDoc->m_bIgnorePunctuation = (pDialog->m_Check_Ignore_Punctuation.GetCheck() == TRUE);
	m_pDoc->m_bSkipLongWords = (pDialog->m_Check_Skip_Long_Words.GetCheck() == TRUE);
	m_pDoc->m_bSkipNonwords = (pDialog->m_Check_Skip_Nonwords.GetCheck() == TRUE);
	m_pDoc->m_bBasic_Characters = (pDialog->m_Check_Basic_Characters.GetCheck() == TRUE);

	m_pDoc->m_szReportFolder = szfolder;

	m_pReport = &pDialog->m_List_Report;
	m_pStatus = &pDialog->m_Static_Status;
	m_pProgress = &pDialog->m_Progress;

	pDialog->m_Combo_Language.GetWindowTextW(szstring);
	_wsetlocale(LC_ALL, szstring);

	irvalue = RunComparison();
	if(irvalue > -1)
	{
		switch(irvalue) {
		case ERR_CANNOT_OPEN_FILE :
			szMessage.Format(L"Error: Could not open file during comparison process");
			break;
		case ERR_CANNOT_ALLOCATE_WORKING_HASH_ARRAY :
			szMessage.Format(L"Error: Could not allocate working space for hash array during comparison process. Possibly out of memory.");
			break;
		case ERR_CANNOT_ALLOCATE_HASH_ARRAY :
			szMessage.Format(L"Error: Could not allocate hash array during comparison process. Possibly out of memory.");
			break;
		case ERR_CANNOT_ALLOCATE_SORTED_HASH_ARRAY :
			szMessage.Format(L"Error: Could not allocate sorted hash array during comparison process. Possibly out of memory.");
			break;
		case ERR_CANNOT_ALLOCATE_SORTED_NUMBER_ARRAY :
			szMessage.Format(L"Error: Could not allocate sorted number array during comparison process. Possibly out of memory.");
			break;
		case ERR_CANNOT_OPEN_LOG_FILE :
			szMessage.Format(L"Error: Could not open log file during comparison process");
			break;
		case ERR_CANNOT_OPEN_COMPARISON_REPORT_TXT_FILE :
			szMessage.Format(L"Error: Could not open comparison report text file during comparison process");
			break;
		case ERR_CANNOT_OPEN_COMPARISON_REPORT_HTML_FILE :
			szMessage.Format(L"Error: Could not open comparioson report html file during comparison process");
			break;
		case ERR_CANNOT_ALLOCATE_LEFT_MATCH_MARKERS :
			szMessage.Format(L"Error: Could not allocate left match marker array during comparison process. Possibly out of memory.");
			break;
		case ERR_CANNOT_ALLOCATE_RIGHT_MATCH_MARKERS :
			szMessage.Format(L"Error: Could not allocate right match marker array during comparison process. Possibly out of memory.");
			break;
		case ERR_CANNOT_ALLOCATE_LEFTA_MATCH_MARKERS :
			szMessage.Format(L"Error: Could not allocate leftA match marker array during comparison process. Possibly out of memory.");
			break;
		case ERR_CANNOT_ALLOCATE_RIGHTA_MATCH_MARKERS :
			szMessage.Format(L"Error: Could not allocate rightA match marker array during comparison process. Possibly out of memory.");
			break;
		case ERR_CANNOT_ALLOCATE_LEFTT_MATCH_MARKERS :
			szMessage.Format(L"Error: Could not allocate leftT match marker array during comparison process. Possibly out of memory.");
			break;
		case ERR_CANNOT_ALLOCATE_RIGHTT_MATCH_MARKERS :
			szMessage.Format(L"Error: Could not allocate rightT match marker array during comparison process. Possibly out of memory.");
			break;
		case ERR_CANNOT_OPEN_LEFT_HTML_FILE :
			szMessage.Format(L"Error: Could not open left html file during comparison process");
			break;
		case ERR_CANNOT_OPEN_LEFT_DOCUMENT_FILE :
			szMessage.Format(L"Error: Could not open left document file during comparison process");
			break;
		case ERR_CANNOT_OPEN_RIGHT_HTML_FILE :
			szMessage.Format(L"Error: Could not open right html file during comparison process");
			break;
		case ERR_CANNOT_OPEN_RIGHT_DOCUMENT_FILE :
			szMessage.Format(L"Error: Could not open right document file during comparison process");
			break;
		case ERR_CANNOT_OPEN_SIDE_BY_SIDE_HTML_FILE :
			szMessage.Format(L"Error: Could not open side-by-side html file during comparison process");
			break;
		case ERR_CANNOT_ACCESS_URL :
			szMessage.Format(L"Error: URL could not be accessed");
			break;
		case ERR_NO_FILE_OPEN :
			szMessage.Format(L"Software Bug: Trying to read from a file that is not open");
			break;
		case ERR_CANNOT_FIND_FILE :
			szMessage.Format(L"Error: File could not be found");
			break;
		case ERR_CANNOT_FIND_FILE_EXTENSION :
			szMessage.Format(L"Error: File is missing an extension and its type cannot be determined");
			break;
		case ERR_BAD_DOCX_FILE :
			szMessage.Format(L"Error: This docx file cannot be read properly");
			break;
		case ERR_BAD_PDF_FILE :
			szMessage.Format(L"Error: This pdf file cannot be read properly");
			break;
		case ERR_CANNOT_FIND_URL_LINK :
			szMessage.Format(L"Error: URL link cannot be found");
			break;
		case ERR_CANNOT_OPEN_INPUT_FILE :
			szMessage.Format(L"Error: File cannot be opened, perhaps because it is already opened by other software");
			break;
		default :
			szMessage.Format(L"Error %d Occurred During Comparison Process",irvalue);
		}
		AfxMessageBox(szMessage);

		if (m_pDoc != NULL) {delete m_pDoc; m_pDoc=NULL;}
		CWnd::FromHandle(pargs->hWnd)->PostMessage(WU_UITHREAD_TERMINATED,1,1);
		return;
	}

	pDialog->m_Progress.ShowWindow(SW_HIDE);

	if (m_pDoc != NULL) {delete m_pDoc; m_pDoc=NULL;}

	pDialog->m_Edit_Folder.GetWindowText(szfolder);
	CString szbrowser;
	szbrowser.Format(L"%s\\matches.html",szfolder);

	HINSTANCE hinst1 = ShellExecute(NULL, // no parent hwnd
        NULL, // open
        szbrowser, // file name
        NULL, // no parameters
        NULL, // no directory name
        SW_SHOWNORMAL); // yes, show it

	CWnd::FromHandle(pargs->hWnd)->PostMessage(WU_UITHREAD_TERMINATED,1,1);
}
void CUiThread::OnTerminate(WPARAM, LPARAM lp)
{
	extern std::atomic<bool> g_abort;
	if(g_abort) PostQuitMessage(1);
	else PostQuitMessage(0);
}

int CUiThread::RunComparison()
{
	long long DocL,DocR;								// document number of left document and right document
	CString szMessage;									// status messages
	int i;												// local index counter
	int irvalue;
	extern std::atomic<bool> g_abort;								// abort signal when true

	irvalue = m_pDoc->SetupReports(); if(irvalue > -1) return irvalue;		// setup reporting files	

	m_pDoc->SetupLoading();								// setup document loading step
	m_pStatus->SetWindowTextW(L"Loading and Hash-Coding Documents");
	
	for(i=0;i<m_pDoc->m_Documents;i++)			// loop for all document entries
	{
		if(g_abort)
		{
			m_pStatus->SetWindowTextW(L"Comparison Aborted");
			return ERR_ABORT;
		}
		m_pProgress->SetPos(i*100/m_pDoc->m_Documents);
		szMessage.Format(L"Loading: %s",wcsrchr((m_pDoc->m_pDocs+i)->m_szDocumentName,0x5C)+1);
		m_pStatus->SetWindowTextW(szMessage);

		irvalue = m_pDoc->LoadDocument((m_pDoc->m_pDocs+i)); if(irvalue > -1) return irvalue;			// load this document
		
	}

	m_pDoc->FinishLoading();									// Finish document loading step

	irvalue = m_pDoc->SetupComparisons(); if(irvalue > -1) return irvalue;
	
	m_pStatus->SetWindowTextW(L"Comparing Documents");
	m_pProgress->SetPos(0);

	m_pDoc->SetupProgressReports(DOC_TYPE_OLD,DOC_TYPE_NEW,DOC_TYPE_NEW);

	for(DocL=0;DocL<m_pDoc->m_Documents;DocL++)			// for all possible left documents
	{
		m_pDoc->m_pDocL = m_pDoc->m_pDocs + DocL;	// obtain a quick pointer to the left document

		for(DocR=0;DocR<DocL;DocR++)					// for all possible right documents
		{
			m_pDoc->m_pDocR = m_pDoc->m_pDocs + DocR;	// obtain a quick pointer to the right document

			if(g_abort){
				m_pStatus->SetWindowTextW(L"Comparison Aborted");
				return ERR_ABORT;
			}

			if( (m_pDoc->m_pDocL->m_DocumentType == DOC_TYPE_OLD) && (m_pDoc->m_pDocR->m_DocumentType == DOC_TYPE_OLD) ) continue;	// don't compare an old document with an old document

			irvalue = m_pDoc->ComparePair(m_pDoc->m_pDocL,m_pDoc->m_pDocR); if(irvalue > -1) return irvalue;			// compare the two documents
			
			if( (m_pDoc->m_Compares%m_pDoc->m_CompareStep)	== 0 )				// if count is divisible by 1000,
			{
				szMessage.Format(L"Comparing Documents, %d Completed",m_pDoc->m_Compares);
				m_pStatus->SetWindowTextW(szMessage);
				m_pProgress->SetPos(int((100.0*double(m_pDoc->m_Compares))/double(m_pDoc->m_TotalCompares)));
			}
			
			if(m_pDoc->m_MatchingWordsPerfect>=m_pDoc->m_WordThreshold)		// if there are enough matches to report,
			{
				m_pDoc->m_MatchingDocumentPairs++;				// increment count of matched pairs of documents
				m_pDoc->ReportMatchedPair();

				int nItem;
				nItem=m_pReport->GetItemCount();

				CString szPerfectMatch;
				CString szOverallMatch;
				szPerfectMatch.Format(L"%d (%d%% L, %d%% R)", m_pDoc->m_MatchingWordsPerfect,100*m_pDoc->m_MatchingWordsPerfect/m_pDoc->m_pDocL->m_WordsTotal,
					100*m_pDoc->m_MatchingWordsPerfect/m_pDoc->m_pDocR->m_WordsTotal);
				szOverallMatch.Format(L"%d (%d%%) L; %d (%d%%) R", m_pDoc->m_MatchingWordsTotalL,100*m_pDoc->m_MatchingWordsTotalL/m_pDoc->m_pDocL->m_WordsTotal,
					m_pDoc->m_MatchingWordsTotalR,100*m_pDoc->m_MatchingWordsTotalR/m_pDoc->m_pDocR->m_WordsTotal);

				m_pReport->InsertItem(nItem,szPerfectMatch);
				m_pReport->SetItemText(nItem,1,szOverallMatch);
				m_pReport->SetItemText(nItem,2,m_pDoc->m_szDocL);
				m_pReport->SetItemText(nItem,3,m_pDoc->m_szDocR);
				m_pReport->EnsureVisible(nItem,FALSE);
				m_pReport->Update(nItem);
			}
		}
	}

	m_pDoc->FinishComparisons();
	m_pDoc->FinishReports();

	szMessage.Format(L"Done. Total CPU Time: %.3f seconds",m_pDoc->m_Time);
	m_pStatus->SetWindowTextW(szMessage);

	return -1;
}
