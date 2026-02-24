#pragma once


// CAbortDlg dialog

class CAbortDlg : public CDialog
{
	DECLARE_DYNAMIC(CAbortDlg)

public:
	CAbortDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAbortDlg();

// Dialog Data
	enum { IDD = IDD_ABORTDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedAbort();
	bool m_abort_clicked;
	afx_msg void OnClose();
};
