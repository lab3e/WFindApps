using System.Diagnostics;
using WCopyfindCS.Controls;
using WCopyfindCS.Core;

namespace WCopyfindCS.Forms;

public sealed partial class MainForm : Form
{
    private CancellationTokenSource? _cts;
    private ProgressForm?            _progressForm;
    private bool                     _running;

    public MainForm()
    {
        InitializeComponent();
        LoadSettings();
    }

    // ── Settings persistence (ApplicationSettings → Properties.Settings) ─────

    private void LoadSettings()
    {
        var s = Properties.Settings.Default;
        nudPhrase.Value      = s.PhraseLength;
        nudThreshold.Value   = s.WordThreshold;
        nudSkipLength.Value  = s.SkipLength;
        nudTolerance.Value   = s.MismatchTolerance;
        nudPercentage.Value  = s.MismatchPercentage;
        chkBriefReport.Checked           = s.BriefReport;
        chkIgnoreCase.Checked            = s.IgnoreCase;
        chkIgnoreNumbers.Checked         = s.IgnoreNumbers;
        chkIgnorePunctuation.Checked     = s.IgnorePunctuation;
        chkIgnoreOuterPunctuation.Checked = s.IgnoreOuterPunctuation;
        chkSkipLongWords.Checked         = s.SkipLongWords;
        chkSkipNonwords.Checked          = s.SkipNonwords;
        chkBasicCharacters.Checked       = s.BasicCharacters;
        txtReportFolder.Text             = s.ReportFolder;
        cmbLanguage.Text                 = s.Language;
        lstOld.SortOnLoad = s.SortOnLoadOld;
        lstNew.SortOnLoad = s.SortOnLoadNew;
    }

    private void SaveSettings()
    {
        var s = Properties.Settings.Default;
        s.PhraseLength          = (int)nudPhrase.Value;
        s.WordThreshold         = (int)nudThreshold.Value;
        s.SkipLength            = (int)nudSkipLength.Value;
        s.MismatchTolerance     = (int)nudTolerance.Value;
        s.MismatchPercentage    = (int)nudPercentage.Value;
        s.BriefReport           = chkBriefReport.Checked;
        s.IgnoreCase            = chkIgnoreCase.Checked;
        s.IgnoreNumbers         = chkIgnoreNumbers.Checked;
        s.IgnorePunctuation     = chkIgnorePunctuation.Checked;
        s.IgnoreOuterPunctuation = chkIgnoreOuterPunctuation.Checked;
        s.SkipLongWords         = chkSkipLongWords.Checked;
        s.SkipNonwords          = chkSkipNonwords.Checked;
        s.BasicCharacters       = chkBasicCharacters.Checked;
        s.ReportFolder          = txtReportFolder.Text;
        s.Language              = cmbLanguage.Text;
        s.SortOnLoadOld         = lstOld.SortOnLoad;
        s.SortOnLoadNew         = lstNew.SortOnLoad;
        s.Save();
    }

    protected override void OnFormClosing(FormClosingEventArgs e)
    {
        SaveSettings();
        base.OnFormClosing(e);
    }

    // ── Build settings from UI ────────────────────────────────────────────────

    private ComparisonSettings BuildSettings() => new()
    {
        PhraseLength          = (int)nudPhrase.Value,
        WordThreshold         = (int)nudThreshold.Value,
        SkipLength            = (int)nudSkipLength.Value,
        MismatchTolerance     = (int)nudTolerance.Value,
        MismatchPercentage    = (int)nudPercentage.Value,
        BriefReport           = chkBriefReport.Checked,
        IgnoreCase            = chkIgnoreCase.Checked,
        IgnoreNumbers         = chkIgnoreNumbers.Checked,
        IgnorePunctuation     = chkIgnorePunctuation.Checked,
        IgnoreOuterPunctuation = chkIgnoreOuterPunctuation.Checked,
        SkipLongWords         = chkSkipLongWords.Checked,
        SkipNonwords          = chkSkipNonwords.Checked,
        BasicCharacters       = chkBasicCharacters.Checked,
        ReportFolder          = txtReportFolder.Text,
        Language              = cmbLanguage.Text,
        SoftwareName          = "WCopyfind 5.0",
    };

    // ── Run button ────────────────────────────────────────────────────────────

    private void btnRun_Click(object? sender, EventArgs e)
    {
        if (_running) return;

        string folder = txtReportFolder.Text;
        if (!Directory.Exists(folder))
        {
            MessageBox.Show($"Report folder \"{folder}\" does not exist. Please create it first.",
                "WCopyfind", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return;
        }

        int oldCount = lstOld.Items.Count;
        int newCount = lstNew.Items.Count;
        if (oldCount + newCount == 0)
        {
            MessageBox.Show("Please add documents to the Old or New lists first.",
                "WCopyfind", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return;
        }

        var oldPaths = lstOld.GetAllPaths();
        var newPaths = lstNew.GetAllPaths();
        var settings = BuildSettings();

        _running = true;
        btnRun.Enabled = false;
        progressBar.Visible = true;
        lblStatus.Visible = true;
        lstReport.Items.Clear();

        _cts = new CancellationTokenSource();
        _progressForm = new ProgressForm(_cts);

        // Position progress form near parent
        _progressForm.Location = new Point(Left + Width / 2 - 100, Top + Height / 2 - 30);
        _progressForm.Show(this);

        var progress = new Progress<ComparisonProgress>(p =>
        {
            lblStatus.Text = p.Status;
            progressBar.Value = Math.Max(0, Math.Min(100, p.Percent));
        });

        var token = _cts.Token;
        _ = Task.Run(() => RunComparison(oldPaths, newPaths, settings, progress, token), token)
            .ContinueWith(OnComparisonCompleted, TaskScheduler.FromCurrentSynchronizationContext());
    }

    // ── Comparison worker ─────────────────────────────────────────────────────

    private static List<MatchResult> RunComparison(
        List<string> oldPaths, List<string> newPaths,
        ComparisonSettings settings,
        IProgress<ComparisonProgress> progress,
        CancellationToken ct)
    {
        var results = new List<MatchResult>();
        var comparer = new DocumentComparer(settings);

        comparer.SetupReports();

        // ── Load documents ────────────────────────────────────────────────────
        int total = oldPaths.Count + newPaths.Count;
        var oldDocs = new List<Document>(oldPaths.Count);
        var newDocs = new List<Document>(newPaths.Count);

        for (int i = 0; i < oldPaths.Count; i++)
        {
            ct.ThrowIfCancellationRequested();
            progress.Report(new ComparisonProgress($"Loading: {Path.GetFileName(oldPaths[i])}", i * 100 / total));
            oldDocs.Add(comparer.LoadDocument(oldPaths[i], DocType.Old));
        }
        for (int i = 0; i < newPaths.Count; i++)
        {
            ct.ThrowIfCancellationRequested();
            progress.Report(new ComparisonProgress($"Loading: {Path.GetFileName(newPaths[i])}", (oldPaths.Count + i) * 100 / total));
            newDocs.Add(comparer.LoadDocument(newPaths[i], DocType.New));
        }

        // Compute max words for working array allocation
        int maxWords = 0;
        foreach (var d in oldDocs) maxWords = Math.Max(maxWords, d.WordsTotal);
        foreach (var d in newDocs) maxWords = Math.Max(maxWords, d.WordsTotal);
        if (maxWords == 0) maxWords = 1;

        comparer.SetupComparisons(maxWords);

        // ── Compare ───────────────────────────────────────────────────────────
        progress.Report(new ComparisonProgress("Comparing Documents", 0));

        // Old vs New + New vs New (same as original: DocL goes through all, DocR < DocL, skip old-old)
        var allDocs = oldDocs.Concat(newDocs).ToList();
        long totalPairs = (long)oldDocs.Count * newDocs.Count +
                          ((long)newDocs.Count * (newDocs.Count - 1)) / 2;
        long done = 0;
        int pairIndex = 0;

        for (int l = 0; l < allDocs.Count; l++)
        {
            for (int r = 0; r < l; r++)
            {
                ct.ThrowIfCancellationRequested();

                var docL = allDocs[l];
                var docR = allDocs[r];

                // Skip old-old pairs
                if (docL.Type == DocType.Old && docR.Type == DocType.Old) continue;

                int pct = totalPairs > 0 ? (int)(done * 100 / totalPairs) : 0;
                progress.Report(new ComparisonProgress($"Comparing Documents, {done} completed", pct));

                bool matched = comparer.ComparePair(docL, docR, ct);
                done++;

                if (matched)
                {
                    pairIndex++;
                    var result = comparer.ReportMatchedPair(docL, docR, pairIndex);
                    results.Add(result);
                }
            }
        }

        comparer.FinishReports(pairIndex);
        return results;
    }

    // ── Completion callback (runs on UI thread) ───────────────────────────────

    private void OnComparisonCompleted(Task<List<MatchResult>> task)
    {
        _running = false;
        btnRun.Enabled = true;
        progressBar.Visible = false;

        _progressForm?.Close();
        _progressForm?.Dispose();
        _progressForm = null;
        _cts?.Dispose();
        _cts = null;

        if (task.IsCanceled)
        {
            lblStatus.Text = "Comparison aborted.";
            return;
        }
        if (task.IsFaulted)
        {
            string msg = task.Exception?.InnerException?.Message ?? "Unknown error";
            lblStatus.Text = $"Error: {msg}";
            MessageBox.Show($"An error occurred during the comparison:\n{msg}",
                "WCopyfind", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return;
        }

        var results = task.Result;
        foreach (var r in results)
        {
            var item = new ListViewItem(r.PerfectMatch);
            item.SubItems.Add(r.OverallMatch);
            item.SubItems.Add(r.FileL);
            item.SubItems.Add(r.FileR);
            lstReport.Items.Add(item);
        }
        lstReport.EnsureVisible(lstReport.Items.Count - 1);

        lblStatus.Text = $"Done. {results.Count} matching pair(s) found.";

        // Auto-open matches.html
        string matchesHtml = Path.Combine(txtReportFolder.Text, "matches.html");
        if (File.Exists(matchesHtml))
            Process.Start(new ProcessStartInfo(matchesHtml) { UseShellExecute = true });
    }

    // ── Report list double-click ──────────────────────────────────────────────

    private void lstReport_DoubleClick(object? sender, EventArgs e)
    {
        if (lstReport.SelectedItems.Count == 0) return;
        var item = lstReport.SelectedItems[0];
        string fileL = item.SubItems[2].Text;
        string fileR = item.SubItems[3].Text;
        string folder = txtReportFolder.Text;

        string pathL = Path.Combine(folder, $"{fileL}.{fileR}.html");
        string pathR = Path.Combine(folder, $"{fileR}.{fileL}.html");

        if (File.Exists(pathL))
            Process.Start(new ProcessStartInfo(pathL) { UseShellExecute = true });
        if (File.Exists(pathR))
            Process.Start(new ProcessStartInfo(pathR) { UseShellExecute = true });
    }

    // ── Browse for report folder ──────────────────────────────────────────────

    private void btnBrowseFolder_Click(object? sender, EventArgs e)
    {
        using var dlg = new FolderBrowserDialog { Description = "Select Reporting Folder" };
        if (dlg.ShowDialog(this) == DialogResult.OK)
            txtReportFolder.Text = dlg.SelectedPath;
    }

    // ── View report in browser ────────────────────────────────────────────────

    private void btnViewReport_Click(object? sender, EventArgs e)
    {
        string path = Path.Combine(txtReportFolder.Text, "matches.html");
        if (File.Exists(path))
            Process.Start(new ProcessStartInfo(path) { UseShellExecute = true });
        else
            MessageBox.Show("matches.html not found. Run a comparison first.",
                "WCopyfind", MessageBoxButtons.OK, MessageBoxIcon.Information);
    }

    // ── Vocabulary builder ────────────────────────────────────────────────────

    private void btnVocabulary_Click(object? sender, EventArgs e)
    {
        using var dlg = new SaveFileDialog
        {
            Title  = "Save Vocabulary",
            Filter = "Text Files (*.txt)|*.txt|All Files (*.*)|*.*",
        };
        if (dlg.ShowDialog(this) != DialogResult.OK) return;

        var settings = BuildSettings();
        var vocab    = new Dictionary<string, int>(StringComparer.Ordinal);

        progressBar.Visible = true;
        lblStatus.Visible   = true;
        lblStatus.Text      = "Building Vocabulary…";
        Application.DoEvents();

        var allPaths = lstOld.GetAllPaths().Concat(lstNew.GetAllPaths()).ToList();
        int total = allPaths.Count;

        for (int i = 0; i < total; i++)
        {
            progressBar.Value = i * 100 / Math.Max(1, total);
            Application.DoEvents();

            foreach (var tok in DocumentReader.ReadWords(allPaths[i], settings.BasicCharacters))
            {
                if (tok.DelimType == DelimType.Eof) break;
                string? filtered = WordProcessor.ApplyFilters(tok.Word, settings);
                if (filtered is null || filtered.Length == 0) continue;
                vocab.TryGetValue(filtered, out int cnt);
                vocab[filtered] = cnt + 1;
            }
        }

        lblStatus.Text = "Saving vocabulary…";
        Application.DoEvents();

        using var sw = new System.IO.StreamWriter(dlg.FileName, false, new System.Text.UTF8Encoding(false));
        foreach (var kv in vocab.OrderByDescending(kv => kv.Value))
            sw.WriteLine($"{kv.Value}\t{kv.Key}");

        progressBar.Visible = false;
        lblStatus.Text      = "Vocabulary saved.";
    }

    // ── File-list context menus ───────────────────────────────────────────────

    private void ShowListContextMenu(FileDropListView lst, MouseEventArgs e)
    {
        bool isOld = lst == lstOld;
        string label = isOld ? "Old" : "New";

        var menu = new ContextMenuStrip();
        menu.Items.Add($"Browse for {label} Documents…",   null, (_, _) => BrowseDocuments(lst));
        menu.Items.Add($"Load from File…",                 null, (_, _) => LoadListFromFile(lst));
        menu.Items.Add($"Save to File…",                   null, (_, _) => SaveListToFile(lst));
        menu.Items.Add(new ToolStripSeparator());
        var sortItem = new ToolStripMenuItem("Sort on Load") { Checked = lst.SortOnLoad };
        sortItem.Click += (_, _) => { lst.SortOnLoad = !lst.SortOnLoad; sortItem.Checked = lst.SortOnLoad; };
        menu.Items.Add(sortItem);
        menu.Items.Add(new ToolStripSeparator());
        menu.Items.Add("Clear Selected",  null, (_, _) => { foreach (ListViewItem i in lst.SelectedItems) lst.Items.Remove(i); });
        menu.Items.Add("Clear All",       null, (_, _) => lst.Items.Clear());
        menu.Show(lst, e.Location);
    }

    private void ShowReportContextMenu(MouseEventArgs e)
    {
        var menu = new ContextMenuStrip();
        menu.Items.Add("View Report in Browser", null, (_, _) => btnViewReport_Click(this, EventArgs.Empty));
        menu.Items.Add("Save to File…", null, (_, _) => SaveReportToFile());
        menu.Items.Add(new ToolStripSeparator());
        menu.Items.Add("Clear Selected", null, (_, _) => { foreach (ListViewItem i in lstReport.SelectedItems) lstReport.Items.Remove(i); });
        menu.Items.Add("Clear All",      null, (_, _) => lstReport.Items.Clear());
        menu.Show(lstReport, e.Location);
    }

    private void BrowseDocuments(FileDropListView lst)
    {
        using var dlg = new OpenFileDialog
        {
            Multiselect = true,
            Filter      = "All Files (*.*)|*.*",
            Title       = lst == lstOld ? "Select Old Documents" : "Select New Documents",
        };
        if (dlg.ShowDialog(this) != DialogResult.OK) return;
        foreach (string path in dlg.FileNames) lst.InsertPath(path);
    }

    private void LoadListFromFile(FileDropListView lst)
    {
        using var dlg = new OpenFileDialog { Filter = "Text Files (*.txt)|*.txt|All Files (*.*)|*.*" };
        if (dlg.ShowDialog(this) != DialogResult.OK) return;
        foreach (string line in File.ReadLines(dlg.FileName))
        {
            string path = line.TrimEnd('\r', '\n');
            if (!string.IsNullOrWhiteSpace(path)) lst.InsertPath(path);
        }
    }

    private void SaveListToFile(FileDropListView lst)
    {
        using var dlg = new SaveFileDialog { Filter = "Text Files (*.txt)|*.txt|All Files (*.*)|*.*" };
        if (dlg.ShowDialog(this) != DialogResult.OK) return;
        File.WriteAllLines(dlg.FileName, lst.GetAllPaths());
    }

    private void SaveReportToFile()
    {
        using var dlg = new SaveFileDialog { Filter = "Text Files (*.txt)|*.txt|All Files (*.*)|*.*" };
        if (dlg.ShowDialog(this) != DialogResult.OK) return;
        using var sw = new System.IO.StreamWriter(dlg.FileName);
        foreach (ListViewItem item in lstReport.Items)
            sw.WriteLine($"{item.Text}\t{item.SubItems[1].Text}\t{item.SubItems[2].Text}\t{item.SubItems[3].Text}");
    }
}
