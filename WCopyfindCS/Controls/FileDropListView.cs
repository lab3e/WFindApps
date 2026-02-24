namespace WCopyfindCS.Controls;

/// <summary>
/// A ListView that accepts files dragged from Windows Explorer.
/// Also supports context-menu operations (load, save, clear, browse, delete key).
/// </summary>
public sealed class FileDropListView : ListView
{
    public bool SortOnLoad { get; set; } = true;

    public FileDropListView()
    {
        View          = View.Details;
        FullRowSelect = true;
        AllowDrop     = true;

        DragEnter += OnDragEnter;
        DragDrop  += OnDragDrop;
        KeyDown   += OnKeyDown;
    }

    // ── Drag-drop ────────────────────────────────────────────────────────────

    private static void OnDragEnter(object? sender, DragEventArgs e)
    {
        e.Effect = e.Data?.GetDataPresent(DataFormats.FileDrop) == true
            ? DragDropEffects.Copy
            : DragDropEffects.None;
    }

    private void OnDragDrop(object? sender, DragEventArgs e)
    {
        if (e.Data?.GetData(DataFormats.FileDrop) is not string[] files) return;
        foreach (string path in files)
            InsertPath(path);
    }

    // ── Public helpers ───────────────────────────────────────────────────────

    public void InsertPath(string path)
    {
        // Reject duplicates
        foreach (ListViewItem existing in Items)
            if (string.Equals(existing.Text, path, StringComparison.OrdinalIgnoreCase)) return;

        if (SortOnLoad)
        {
            // Binary-search insert in sorted order
            int lo = 0, hi = Items.Count;
            while (lo < hi)
            {
                int mid = (lo + hi) / 2;
                if (string.CompareOrdinal(Items[mid].Text, path) < 0) lo = mid + 1;
                else hi = mid;
            }
            Items.Insert(lo, path);
        }
        else
        {
            Items.Add(path);
        }
    }

    public List<string> GetAllPaths()
    {
        var list = new List<string>(Items.Count);
        foreach (ListViewItem item in Items) list.Add(item.Text);
        return list;
    }

    // ── Keyboard: Delete key removes selection ────────────────────────────────

    private void OnKeyDown(object? sender, KeyEventArgs e)
    {
        if (e.KeyCode == Keys.Delete)
        {
            // Remove selected items (iterate backwards to keep indices valid)
            var selected = new List<ListViewItem>();
            foreach (ListViewItem item in SelectedItems) selected.Add(item);
            foreach (var item in selected) Items.Remove(item);
        }
    }
}
