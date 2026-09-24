// OptionsDlg.h : the less commonly needed settings

#pragma once
#include "RepeatFinder.h"

class COptionsDlg : public CDialogEx
{
public:
	COptionsDlg(CWnd* pParent = nullptr);
	enum { IDD = IDD_OPTIONS };

	RepeatSettings m_Settings;
	CString m_Language;
	CString m_ReportFolder;
	bool m_AutoOpenReport = true;

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonFolder();
	afx_msg void OnButtonDefaults();
	afx_msg void OnCheckSkipLong();
	DECLARE_MESSAGE_MAP()

private:
	void WriteControls();
};
