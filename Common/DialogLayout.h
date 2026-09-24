// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Louis A. Bloomfield
// DialogLayout.h : keeps dialog controls anchored as a resizable dialog changes size
//
// Each control is given the percentage (0-100) of the dialog's growth in width and height that it moves
// by and that it grows by. For example, a list that should take 30% of any extra height is added with
// sizeY = 30, and every control below it with moveY = 30.

#pragma once
#include <vector>

class CDialogLayout
{
public:
	enum Anchor
	{
		TopLeft,			// doesn't move
		TopRight,			// moves right
		BottomLeft,			// moves down
		BottomRight,		// moves right and down
		StretchX,			// widens
		StretchXBottom,		// widens and moves down
		StretchXY,			// widens and heightens
	};

	void Init(CWnd* dialog)
	{
		m_pDialog = dialog;
		CRect client;
		dialog->GetClientRect(&client);
		m_Original = client.Size();
		CRect window;
		dialog->GetWindowRect(&window);
		m_MinTrack = window.Size();
	}

	void Add(int id, Anchor anchor)
	{
		switch(anchor)
		{
		case TopLeft: Add(id, 0, 0, 0, 0); break;
		case TopRight: Add(id, 100, 0, 0, 0); break;
		case BottomLeft: Add(id, 0, 100, 0, 0); break;
		case BottomRight: Add(id, 100, 100, 0, 0); break;
		case StretchX: Add(id, 0, 0, 100, 0); break;
		case StretchXBottom: Add(id, 0, 100, 100, 0); break;
		case StretchXY: Add(id, 0, 0, 100, 100); break;
		}
	}

	void Add(int id, int moveX, int moveY, int sizeX, int sizeY)
	{
		CWnd* control = m_pDialog->GetDlgItem(id);
		if(control == nullptr) return;
		CRect rect;
		control->GetWindowRect(&rect);
		m_pDialog->ScreenToClient(&rect);
		m_Items.push_back({id, moveX, moveY, sizeX, sizeY, rect});
	}

	void Resize()
	{
		if(m_pDialog == nullptr || m_Items.empty()) return;
		CRect client;
		m_pDialog->GetClientRect(&client);
		int dx = client.Width() - m_Original.cx;
		int dy = client.Height() - m_Original.cy;
		if(dx < 0) dx = 0;
		if(dy < 0) dy = 0;

		HDWP hdwp = BeginDeferWindowPos((int)m_Items.size());
		for(const Item& item : m_Items)
		{
			CRect r = item.Rect;
			r.OffsetRect(dx * item.MoveX / 100, dy * item.MoveY / 100);
			r.right += dx * item.SizeX / 100;
			r.bottom += dy * item.SizeY / 100;
			CWnd* control = m_pDialog->GetDlgItem(item.Id);
			if(control != nullptr && hdwp != nullptr)
				hdwp = DeferWindowPos(hdwp, control->GetSafeHwnd(), nullptr, r.left, r.top, r.Width(), r.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
		}
		if(hdwp != nullptr) EndDeferWindowPos(hdwp);
		m_pDialog->Invalidate();
	}

	CSize MinTrackSize() const { return m_MinTrack; }
	bool IsReady() const { return m_pDialog != nullptr; }

private:
	struct Item { int Id; int MoveX, MoveY, SizeX, SizeY; CRect Rect; };
	CWnd* m_pDialog = nullptr;
	CSize m_Original;
	CSize m_MinTrack;
	std::vector<Item> m_Items;
};
