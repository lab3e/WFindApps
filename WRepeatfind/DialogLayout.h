// DialogLayout.h : keeps dialog controls anchored to the window edges as a resizable dialog changes size

#pragma once
#include <vector>

class CDialogLayout
{
public:
	// fractions (0-100) of the width/height change applied to a control's position and size
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
		CWnd* control = m_pDialog->GetDlgItem(id);
		if(control == nullptr) return;
		CRect rect;
		control->GetWindowRect(&rect);
		m_pDialog->ScreenToClient(&rect);
		m_Items.push_back({id, anchor, rect});
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
			switch(item.Anchor)
			{
			case TopLeft: break;
			case TopRight: r.OffsetRect(dx, 0); break;
			case BottomLeft: r.OffsetRect(0, dy); break;
			case BottomRight: r.OffsetRect(dx, dy); break;
			case StretchX: r.right += dx; break;
			case StretchXBottom: r.right += dx; r.OffsetRect(0, dy); break;
			case StretchXY: r.right += dx; r.bottom += dy; break;
			}
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
	struct Item { int Id; Anchor Anchor; CRect Rect; };
	CWnd* m_pDialog = nullptr;
	CSize m_Original;
	CSize m_MinTrack;
	std::vector<Item> m_Items;
};
