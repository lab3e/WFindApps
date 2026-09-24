// PasteDlg.h : lets the writer paste text (from a web editor, email, etc.) to check without saving a file first

#pragma once
#include "DialogLayout.h"

class CPasteDlg : public CDialogEx
{
public:
	CPasteDlg(CWnd* pParent = nullptr);
	enum { IDD = IDD_PASTE };

	CString m_SavedPath;		// the text file the pasted text was saved to

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnButtonClipboard();
	afx_msg void OnChangeText();
	DECLARE_MESSAGE_MAP()

private:
	CDialogLayout m_Layout;
};
