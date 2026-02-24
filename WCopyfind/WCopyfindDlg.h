
// WCopyfindDlg.h : header file
//

#pragma once

#include "FileDropListCtrl.h"
#include "afxwin.h"
#include "afxcmn.h"

// CWCopyfindDlg dialog
class CWCopyfindDlg : public CDialogEx
{
// Construction
public:
	CWCopyfindDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_WCOPYFIND_DIALOG };
	CButton	m_Check_Brief_Report;
	CSpinButtonCtrl	m_Spin_Percentage;
	CEdit	m_Edit_Percentage;
	CSpinButtonCtrl	m_Spin_Tolerance;
	CEdit	m_Edit_Tolerance;
	CButton	m_Check_Skip_Nonwords;
	CButton	m_Check_Skip_Long_Words;
	CButton	m_Check_Ignore_Outer_Punctuation;
	CButton m_Check_Basic_Characters;
	CEdit	m_Edit_Skip_Length;
	CButton	m_Check_Ignore_Punctuation;
	CButton	m_Check_Ignore_Numbers;
	CButton	m_Check_Ignore_Case;
	CEdit	m_Edit_Threshold;
	CSpinButtonCtrl	m_Spin_Threshold;
	CProgressCtrl	m_Progress;
	CStatic	m_Static_Status;
	CEdit	m_Edit_Folder;
	CEdit	m_Edit_Phrase;
	CSpinButtonCtrl	m_Spin_Phrase;
	CListCtrl	m_List_Report;
	CFileDropListCtrl	m_List_Old;
	CFileDropListCtrl	m_List_New;

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnButtonRun();
	afx_msg void OnCheck_Ignore_Case();
	virtual void OnOK();
	afx_msg void OnButtonVocabulary();
	afx_msg void OnKillfocusEditStringlen();
	afx_msg void OnKillfocusEditPercentage();
	afx_msg void OnKillfocusEditTolerance();
	afx_msg void OnNMDblclkListReport(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRclickListOld(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnSaveToFileOld();
	afx_msg void OnLoadFromFileOld();
	afx_msg void OnClearSelectionOld();
	afx_msg void OnClearAllOld();
	afx_msg void OnNMRclickListNew(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLoadFromFileNew();
	afx_msg void OnSaveToFileNew();
	afx_msg void OnClearSelectionNew();
	afx_msg void OnClearAll();
	afx_msg void OnBnClickedButtonFolder();
	afx_msg void OnSortOnLoadNew();
	afx_msg void OnSortOnLoadOld();
	afx_msg void OnBrowseForDocumentsNew();
	afx_msg void OnBrowseForDocumentsOld();
	afx_msg void OnNMDblclkListOld(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkListNew(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnKeydownListOld(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnKeydownListNew(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSaveToFileReport();
	afx_msg void OnClearSelectionReport();
	afx_msg void OnClearAllReport();
	afx_msg void OnNMRclickListReport(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnKeydownListReport(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnViewReportInBrowser();
	DECLARE_MESSAGE_MAP()
public:


private:
	void GetRegistryValue(const wchar_t *name,int *value,int valuedefault);
	void GetRegistrySValue(const wchar_t *name,CString *szvalue,const wchar_t *svaluedefault);
	void SetRegistryValue(const wchar_t *name,int value);
	void SetRegistrySValue(const wchar_t *name,const wchar_t *svalue);
	CImageList m_ImageListOld;
	CImageList m_ImageListNew;
	CPoint m_ptMsg;
	CString m_szMsg;
	int m_Menu;
	bool m_Sort_On_Load_New;
	bool m_Sort_On_Load_Old;
	void DoComparison( void *pArgument );
	CDialog *m_pAbortDlg;
	CUiThread *m_pUiThread;
	thread_args *m_pargs;
	LRESULT OnComparisonTerminated(WPARAM, LPARAM);
public:
	CComboBox m_Combo_Language;
};
