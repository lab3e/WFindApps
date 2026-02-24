namespace WCopyfindCS.Core;

/// <summary>
/// Plain-data object holding all comparison settings with the same defaults as the original C++ constructor.
/// </summary>
public sealed class ComparisonSettings
{
    public int PhraseLength { get; set; } = 6;
    public int FilterPhraseLength { get; set; } = 6;
    public int WordThreshold { get; set; } = 100;
    public int SkipLength { get; set; } = 20;
    public int MismatchTolerance { get; set; } = 2;
    public int MismatchPercentage { get; set; } = 80;

    public bool BriefReport { get; set; } = false;
    public bool IgnoreCase { get; set; } = false;
    public bool IgnoreNumbers { get; set; } = false;
    public bool IgnorePunctuation { get; set; } = false;
    public bool IgnoreOuterPunctuation { get; set; } = false;
    public bool SkipLongWords { get; set; } = false;
    public bool SkipNonwords { get; set; } = false;
    public bool BasicCharacters { get; set; } = false;

    public string ReportFolder { get; set; } = @"C:\WCopyfind\Report";
    public string Language { get; set; } = "English";
    public string SoftwareName { get; set; } = "WCopyfind";

    public bool SortOnLoadOld { get; set; } = true;
    public bool SortOnLoadNew { get; set; } = true;
}
