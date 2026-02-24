namespace WCopyfindCS.Forms;

/// <summary>
/// Simple modal-less "Abort" dialog shown during a comparison run.
/// Clicking Abort sets the shared CancellationTokenSource.
/// Closing via X is ignored unless Abort was clicked.
/// </summary>
public sealed class ProgressForm : Form
{
    private readonly CancellationTokenSource _cts;
    private bool _abortClicked;

    public ProgressForm(CancellationTokenSource cts)
    {
        _cts = cts;
        InitializeComponent();
    }

    private Button btnAbort = null!;

    private void InitializeComponent()
    {
        btnAbort = new Button
        {
            Text     = "Abort",
            Dock     = DockStyle.Fill,
            Font     = new Font("Segoe UI", 12F, FontStyle.Bold),
        };
        btnAbort.Click += (_, _) =>
        {
            _abortClicked = true;
            _cts.Cancel();
            Close();
        };

        Text            = "Running…";
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MinimizeBox     = false;
        MaximizeBox     = false;
        ClientSize      = new Size(200, 60);
        Controls.Add(btnAbort);
        StartPosition   = FormStartPosition.Manual;
    }

    protected override void OnFormClosing(FormClosingEventArgs e)
    {
        // Block only user-initiated close via X; allow programmatic Close() when run finishes
        if (!_abortClicked && e.CloseReason == CloseReason.UserClosing)
            e.Cancel = true;
        base.OnFormClosing(e);
    }
}
