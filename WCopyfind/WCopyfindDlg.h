// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Louis A. Bloomfield
// WCopyfindDlg.h : the main WCopyfind window

#pragma once
#include <memory>
#include <thread>
#include <atomic>
#include <vector>
#include <set>
#include <string>
#include "DialogLayout.h"
#include "clib\CompareDocuments.h"

const UINT WU_PROGRESS = WM_APP + 10;	// wParam = percent, lParam = CString* status (receiver deletes)
const UINT WU_PAIR = WM_APP + 11;		// lParam = CCompareDocuments::PairRecord* (receiver deletes)
const UINT WU_DONE = WM_APP + 12;		// wParam = result code, lParam = CString* message (receiver deletes)

class CWCopyfindDlg : public CDialogEx
{
public:
	CWCopyfindDlg(CWnd* pParent = nullptr);
	enum { IDD = IDD_WCOPYFIND_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnAddNew();
	afx_msg void OnAddOld();
	afx_msg void OnRemoveNew();
	afx_msg void OnRemoveOld();
	afx_msg void OnAddDropDown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListCommand(UINT id);
	afx_msg void OnReportCommand(UINT id);
	afx_msg void OnButtonOptions();
	afx_msg void OnButtonCompare();
	afx_msg void OnButtonReport();
	afx_msg void OnDocsItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDocsDblClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnReportDblClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnReportColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnProgress(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPair(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDone(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:
	enum { NEW_LIST = 0, OLD_LIST = 1 };

	CListCtrl& List(int list) { return list == NEW_LIST ? m_ListNew : m_ListOld; }
	void AddDocuments(int list);
	void AddFolder(int list);
	void AddPaths(int list, const std::vector<CString>& paths, bool fromFolder);
	void RefreshList(int list, const std::vector<bool>& selected);
	std::vector<bool> Selected(int list);
	void RemoveSelected(int list);
	void ShowListMenu(int list, CPoint screenPoint);
	void LoadList(int list);
	void SaveList(int list);
	void SortList(int list);
	void MoveSelected(int list);
	void ReadControls();
	void WriteControls();
	void UpdateControls();
	void SetRunning(bool running);
	void AddResultRow(int index);
	void FitColumns();
	CString DisplayName(const CString& path) const;
	void RefillResults();
	void OpenPair(int item);
	void SaveResults();
	void StopWorker();
	static void Work(HWND hwnd, CCompareDocuments* doc, std::atomic<bool>* abort, CString language);
	static CString ErrorMessage(int code);

	HICON m_hIcon;
	CFont m_BoldFont;
	CDialogLayout m_Layout;
	CListCtrl m_ListNew;
	CListCtrl m_ListOld;
	CListCtrl m_ListReport;
	CSpinButtonCtrl m_SpinPhrase;
	CSpinButtonCtrl m_SpinThreshold;
	CSpinButtonCtrl m_SpinTolerance;
	CProgressCtrl m_Progress;
	int m_MenuList = NEW_LIST;				// which document list the open list menu acts on

	std::unique_ptr<CCompareDocuments> m_Compare;
	std::thread m_Worker;
	std::atomic<bool> m_Abort{false};
	bool m_Running = false;
	CString m_IndexPath;					// the last report's matches.html
	CString m_ReportFolder;					// the folder the last report was written to
	std::vector<CCompareDocuments::PairRecord> m_Results;	// every matching pair from the last comparison
	std::vector<bool> m_Removed;							// pairs the user removed from the list
	std::set<std::wstring> m_DuplicateNames;				// lowercase file names used by more than one document
	int m_SortColumn = -1;
	bool m_SortDescending = false;
};
