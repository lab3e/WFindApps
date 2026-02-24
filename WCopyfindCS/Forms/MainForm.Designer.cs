#nullable enable
using WCopyfindCS.Controls;

namespace WCopyfindCS.Forms;

partial class MainForm
{
    private System.ComponentModel.IContainer? components = null;

    // ─── Control declarations ─────────────────────────────────────────────────
    private FileDropListView  lstOld                  = null!;
    private FileDropListView  lstNew                  = null!;
    private ListView          lstReport               = null!;
    private NumericUpDown     nudPhrase               = null!;
    private NumericUpDown     nudThreshold            = null!;
    private NumericUpDown     nudSkipLength           = null!;
    private NumericUpDown     nudTolerance            = null!;
    private NumericUpDown     nudPercentage           = null!;
    private CheckBox          chkBriefReport          = null!;
    private CheckBox          chkIgnoreCase           = null!;
    private CheckBox          chkIgnoreNumbers        = null!;
    private CheckBox          chkIgnorePunctuation    = null!;
    private CheckBox          chkIgnoreOuterPunctuation = null!;
    private CheckBox          chkSkipLongWords        = null!;
    private CheckBox          chkSkipNonwords         = null!;
    private CheckBox          chkBasicCharacters      = null!;
    private TextBox           txtReportFolder         = null!;
    private ComboBox          cmbLanguage             = null!;
    private Button            btnRun                  = null!;
    private Button            btnVocabulary           = null!;
    private Button            btnBrowseFolder         = null!;
    private Button            btnViewReport           = null!;
    private Button            btnClose                = null!;
    private Label             lblStatus               = null!;
    private ProgressBar       progressBar             = null!;
    private GroupBox          grpSettings             = null!;
    private SplitContainer    splitLists              = null!;
    private Label             lblOld                  = null!;
    private Label             lblNew                  = null!;
    private ColumnHeader      colOldPath              = null!;
    private ColumnHeader      colNewPath              = null!;
    private ColumnHeader      colPerfect              = null!;
    private ColumnHeader      colOverall              = null!;
    private ColumnHeader      colFileL                = null!;
    private ColumnHeader      colFileR                = null!;

    protected override void Dispose(bool disposing)
    {
        if (disposing) components?.Dispose();
        base.Dispose(disposing);
    }

    // ─── InitializeComponent ──────────────────────────────────────────────────

    private void InitializeComponent()
    {
        components = new System.ComponentModel.Container();

        // ── Old document list ─────────────────────────────────────────────────
        lblOld = new Label
        {
            Text      = "Old Documents",
            Dock      = DockStyle.Top,
            TextAlign = ContentAlignment.MiddleLeft,
            Font      = new Font("Segoe UI", 9F, FontStyle.Bold),
            Height    = 20,
        };
        colOldPath = new ColumnHeader { Text = "Path", Width = 400 };
        lstOld = new FileDropListView
        {
            Dock = DockStyle.Fill,
            Name = "lstOld",
        };
        lstOld.Columns.Add(colOldPath);
        lstOld.MouseUp += (s, e) => { if (e.Button == MouseButtons.Right) ShowListContextMenu(lstOld, e); };
        lstOld.DoubleClick += (s, e) => BrowseDocuments(lstOld);

        var pnlOld = new Panel { Dock = DockStyle.Fill };
        pnlOld.Controls.Add(lstOld);
        pnlOld.Controls.Add(lblOld);

        // ── New document list ─────────────────────────────────────────────────
        lblNew = new Label
        {
            Text      = "New Documents",
            Dock      = DockStyle.Top,
            TextAlign = ContentAlignment.MiddleLeft,
            Font      = new Font("Segoe UI", 9F, FontStyle.Bold),
            Height    = 20,
        };
        colNewPath = new ColumnHeader { Text = "Path", Width = 400 };
        lstNew = new FileDropListView
        {
            Dock = DockStyle.Fill,
            Name = "lstNew",
        };
        lstNew.Columns.Add(colNewPath);
        lstNew.MouseUp += (s, e) => { if (e.Button == MouseButtons.Right) ShowListContextMenu(lstNew, e); };
        lstNew.DoubleClick += (s, e) => BrowseDocuments(lstNew);

        var pnlNew = new Panel { Dock = DockStyle.Fill };
        pnlNew.Controls.Add(lstNew);
        pnlNew.Controls.Add(lblNew);

        // ── SplitContainer for the two lists (stacked vertically, equal height) ──
        splitLists = new SplitContainer
        {
            Dock             = DockStyle.Top,
            Height           = 300,
            Orientation      = Orientation.Horizontal,
            SplitterDistance = 148,   // equal halves
            FixedPanel       = FixedPanel.None,
        };
        splitLists.Panel1.Controls.Add(pnlOld);
        splitLists.Panel2.Controls.Add(pnlNew);

        // ── Settings group ────────────────────────────────────────────────────
        grpSettings = new GroupBox
        {
            Text    = "Settings",
            Dock    = DockStyle.Top,
            Height  = 285,
            Padding = new Padding(8),
        };

        // Numeric row 1: Phrase Length, Word Threshold, Skip Length
        var lblPhrase    = MakeLabel("Phrase Length:");
        nudPhrase        = MakeNud(1, 999, 6);
        var lblThreshold = MakeLabel("Word Threshold:");
        nudThreshold     = MakeNud(1, 9999, 100);
        var lblSkip      = MakeLabel("Skip Length:");
        nudSkipLength    = MakeNud(1, 999, 20);

        // Numeric row 2: Mismatch Tolerance, Mismatch Percentage
        var lblTolerance  = MakeLabel("Mismatch Tolerance:");
        nudTolerance      = MakeNud(0, 9, 2);
        var lblPct        = MakeLabel("Min. Match %:");
        nudPercentage     = MakeNud(0, 100, 80);

        // Checkboxes row 1
        chkBriefReport           = MakeCheck("Brief Report");
        chkIgnoreCase            = MakeCheck("Ignore Case");
        chkIgnoreNumbers         = MakeCheck("Ignore Numbers");
        chkIgnorePunctuation     = MakeCheck("Ignore Punctuation");

        // Checkboxes row 2
        chkIgnoreOuterPunctuation = MakeCheck("Ignore Outer Punct.");
        chkSkipLongWords          = MakeCheck("Skip Long Words");
        chkSkipNonwords           = MakeCheck("Skip Nonwords");
        chkBasicCharacters        = MakeCheck("Basic Characters");

        // Language
        var lblLang = MakeLabel("Language:");
        cmbLanguage = new ComboBox
        {
            DropDownStyle = ComboBoxStyle.DropDownList,
            Width = 160,
        };
        cmbLanguage.Items.AddRange(new object[]
        {
            "English", "French", "German", "Spanish", "Italian",
            "Portuguese", "Dutch", "Russian", "Polish", "Czech",
            "Hungarian", "Romanian", "Swedish", "Danish", "Norwegian",
            "Finnish", "Turkish", "Greek", "Bulgarian", "Croatian",
        });

        // Report folder
        var lblFolder = MakeLabel("Report Folder:");
        txtReportFolder = new TextBox { Width = 380 };
        btnBrowseFolder = new Button { Text = "Browse…", Width = 80 };
        btnBrowseFolder.Click += btnBrowseFolder_Click;

        // Buttons
        btnRun = new Button
        {
            Text    = "Run",
            Width   = 80,
            Height  = 28,
            Font    = new Font("Segoe UI", 10F, FontStyle.Bold),
            BackColor = Color.FromArgb(0, 122, 204),
            ForeColor = Color.White,
            FlatStyle = FlatStyle.Flat,
        };
        btnRun.FlatAppearance.BorderSize = 0;
        btnRun.Click += btnRun_Click;

        btnVocabulary = new Button { Text = "Vocabulary", Width = 90, Height = 28 };
        btnVocabulary.Click += btnVocabulary_Click;

        btnViewReport = new Button { Text = "View Report", Width = 90, Height = 28 };
        btnViewReport.Click += btnViewReport_Click;

        btnClose = new Button { Text = "Close", Width = 80, Height = 28 };
        btnClose.Click += (_, _) => Close();

        // Layout settings group using a TableLayoutPanel
        var tbl = new TableLayoutPanel
        {
            Dock        = DockStyle.Fill,
            RowCount    = 7,
            ColumnCount = 1,
            AutoSize    = false,
        };
        tbl.RowStyles.Add(new RowStyle(SizeType.AutoSize));  // numeric row 1
        tbl.RowStyles.Add(new RowStyle(SizeType.AutoSize));  // numeric row 2
        tbl.RowStyles.Add(new RowStyle(SizeType.AutoSize));  // checkboxes row 1
        tbl.RowStyles.Add(new RowStyle(SizeType.AutoSize));  // checkboxes row 2
        tbl.RowStyles.Add(new RowStyle(SizeType.AutoSize));  // language
        tbl.RowStyles.Add(new RowStyle(SizeType.AutoSize));  // report folder
        tbl.RowStyles.Add(new RowStyle(SizeType.AutoSize));  // buttons

        var rowNum1 = MakeFlowRow(lblPhrase, nudPhrase, new Label { Width = 8 },
            lblThreshold, nudThreshold, new Label { Width = 8 }, lblSkip, nudSkipLength);
        var rowNum2 = MakeFlowRow(lblTolerance, nudTolerance, new Label { Width = 8 },
            lblPct, nudPercentage);
        var rowChk1 = MakeFlowRow(chkBriefReport, chkIgnoreCase, chkIgnoreNumbers, chkIgnorePunctuation);
        var rowChk2 = MakeFlowRow(chkIgnoreOuterPunctuation, chkSkipLongWords, chkSkipNonwords, chkBasicCharacters);
        var rowLang = MakeFlowRow(lblLang, cmbLanguage);
        var rowFolder = MakeFlowRow(lblFolder, txtReportFolder, btnBrowseFolder);
        var rowBtns  = MakeFlowRow(btnRun, new Label { Width = 8 }, btnVocabulary,
                                   new Label { Width = 8 }, btnViewReport,
                                   new Label { Width = 8 }, btnClose);

        tbl.Controls.Add(rowNum1,   0, 0);
        tbl.Controls.Add(rowNum2,   0, 1);
        tbl.Controls.Add(rowChk1,   0, 2);
        tbl.Controls.Add(rowChk2,   0, 3);
        tbl.Controls.Add(rowLang,   0, 4);
        tbl.Controls.Add(rowFolder, 0, 5);
        tbl.Controls.Add(rowBtns,   0, 6);

        grpSettings.Controls.Add(tbl);

        // ── Status / progress row ─────────────────────────────────────────────
        progressBar = new ProgressBar
        {
            Dock    = DockStyle.Right,
            Width   = 200,
            Visible = false,
            Minimum = 0,
            Maximum = 100,
        };
        lblStatus = new Label
        {
            Dock      = DockStyle.Fill,
            TextAlign = ContentAlignment.MiddleLeft,
            Text      = "",
            Visible   = true,
        };
        var pnlStatus = new Panel
        {
            Dock   = DockStyle.Top,
            Height = 26,
        };
        pnlStatus.Controls.Add(lblStatus);
        pnlStatus.Controls.Add(progressBar);

        // ── Report list ───────────────────────────────────────────────────────
        colPerfect = new ColumnHeader { Text = "Perfect Match",  Width = 160 };
        colOverall = new ColumnHeader { Text = "Overall Match",  Width = 200 };
        colFileL   = new ColumnHeader { Text = "File L",         Width = 220 };
        colFileR   = new ColumnHeader { Text = "File R",         Width = 220 };

        lstReport = new ListView
        {
            Dock          = DockStyle.Fill,
            View          = View.Details,
            FullRowSelect = true,
            GridLines     = true,
            Name          = "lstReport",
        };
        lstReport.Columns.AddRange(new[] { colPerfect, colOverall, colFileL, colFileR });
        lstReport.DoubleClick += lstReport_DoubleClick;
        lstReport.MouseClick  += (s, e) => { if (e.Button == MouseButtons.Right) ShowReportContextMenu(e); };

        var lblReport = new Label
        {
            Text      = "Results",
            Dock      = DockStyle.Top,
            TextAlign = ContentAlignment.MiddleLeft,
            Font      = new Font("Segoe UI", 9F, FontStyle.Bold),
            Height    = 20,
        };

        // ── Form assembly ─────────────────────────────────────────────────────
        // Add in reverse DockStyle.Top order so they stack correctly
        SuspendLayout();

        Controls.Add(lstReport);      // Fill — takes remaining space
        Controls.Add(lblReport);      // Top
        Controls.Add(pnlStatus);      // Top
        Controls.Add(grpSettings);    // Top
        Controls.Add(splitLists);     // Top

        Text            = "WCopyfind 5.0";
        ClientSize      = new Size(920, 820);
        MinimumSize     = new Size(700, 560);
        StartPosition   = FormStartPosition.CenterScreen;
        Font            = new Font("Segoe UI", 9F);

        ResumeLayout(false);
        PerformLayout();
    }

    // ─── Layout helpers ───────────────────────────────────────────────────────

    private static Label MakeLabel(string text) => new()
    {
        Text      = text,
        AutoSize  = true,
        TextAlign = ContentAlignment.MiddleLeft,
    };

    private static NumericUpDown MakeNud(int min, int max, int value) => new()
    {
        Minimum = min,
        Maximum = max,
        Value   = value,
        Width   = 60,
    };

    private static CheckBox MakeCheck(string text) => new()
    {
        Text     = text,
        AutoSize = true,
    };

    private static FlowLayoutPanel MakeFlowRow(params Control[] controls)
    {
        var panel = new FlowLayoutPanel
        {
            AutoSize        = true,
            FlowDirection   = FlowDirection.LeftToRight,
            WrapContents    = false,
            Dock            = DockStyle.Top,
            Margin          = new Padding(0, 2, 0, 2),
        };
        foreach (var ctl in controls)
        {
            ctl.Margin = new Padding(2, 3, 2, 0);
            panel.Controls.Add(ctl);
        }
        return panel;
    }
}
