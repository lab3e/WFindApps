// MainDlg.h : the main WRepeatfind window

#pragma once
#include <memory>
#include <thread>
#include <atomic>
#include <vector>
#include "DialogLayout.h"
#include "RepeatFinder.h"

const UINT WU_PROGRESS = WM_APP + 10;	// wParam = percent, lParam = CString* status (receiver deletes)
const UINT WU_DONE = WM_APP + 11;		// wParam = result code, lParam = CString* message (receiver deletes)

class CMainDlg : public CDialogEx
{
public:
	CMainDlg(CWnd* pParent = nullptr);
	enum { IDD = IDD_WREPEATFIND_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonPaste();
	afx_msg void OnButtonRemove();
	afx_msg void OnButtonUp();
	afx_msg void OnButtonDown();
	afx_msg void OnButtonOptions();
	afx_msg void OnButtonFind();
	afx_msg void OnButtonReport();
	afx_msg void OnDocsItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDocsDblClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnResultsDblClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnProgress(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDone(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:
	void AddPaths(std::vector<CString> paths, bool expandFolders);
	void RefreshDocuments(const std::vector<bool>& selected);
	std::vector<bool> SelectedDocuments() const;
	void MoveSelection(int direction);
	void ReadControls();
	void WriteControls();
	void UpdateButtons();
	void SetRunning(bool running);
	void ShowResults();
	void StopWorker();
	static void Work(HWND hwnd, CRepeatFinder* finder, std::vector<CString> documents, CString language, CString reportPath);

	HICON m_hIcon;
	CFont m_BoldFont;
	CDialogLayout m_Layout;
	CListCtrl m_ListDocs;
	CListCtrl m_ListResults;
	CSpinButtonCtrl m_SpinPhrase;
	CSpinButtonCtrl m_SpinTolerance;
	CProgressCtrl m_Progress;

	std::unique_ptr<CRepeatFinder> m_Finder;
	std::thread m_Worker;
	std::atomic<bool> m_Abort{false};
	bool m_Running = false;
	CString m_ReportPath;
};
