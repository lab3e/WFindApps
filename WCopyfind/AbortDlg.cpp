// AbortDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WCopyfind.h"
#include "AbortDlg.h"
#include ".\abortdlg.h"


// CAbortDlg dialog

IMPLEMENT_DYNAMIC(CAbortDlg, CDialog)
CAbortDlg::CAbortDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAbortDlg::IDD, pParent)
	, m_abort_clicked(false)
{
}

CAbortDlg::~CAbortDlg()
{
}

void CAbortDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CAbortDlg, CDialog)
	ON_BN_CLICKED(IDC_ABORT, OnBnClickedAbort)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CAbortDlg message handlers

void CAbortDlg::OnBnClickedAbort()
{
	extern std::atomic<bool> g_abort;
	g_abort=true;
	m_abort_clicked=true;
	PostMessage(WM_CLOSE);
}

void CAbortDlg::OnClose()
{
	if(!m_abort_clicked) return;
	CDialog::OnClose();
}
