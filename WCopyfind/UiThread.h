#pragma once

// CUiThread

#define DOC_TYPE_OLD 1
#define DOC_TYPE_NEW 2

class CUiThread : public CWinThread
{
	DECLARE_DYNCREATE(CUiThread)

protected:
	CUiThread();           // protected constructor used by dynamic creation
	virtual ~CUiThread();

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	afx_msg void OnStart(WPARAM, LPARAM);
	afx_msg void OnTerminate(WPARAM, LPARAM);

protected:
	DECLARE_MESSAGE_MAP()

private:
	int RunComparison();
	CCompareDocuments* m_pDoc;
	CListCtrl* m_pReport;
	CStatic* m_pStatus;
	CProgressCtrl* m_pProgress;
};


