using System.Diagnostics;
using System.Text;

namespace WCopyfindCS.Core;

// ─── Public types used by MainForm ────────────────────────────────────────────

public enum DocType { Undefined = 0, Old = 1, New = 2 }

public sealed class Document
{
    public string   Name             = "";
    public DocType  Type;
    public uint[]   WordHash         = [];
    public uint[]   SortedWordHash   = [];
    public int[]    SortedWordNumber = [];
    public int      WordsTotal;
    public int      FirstHash;
}

public sealed class MatchResult
{
    public string PerfectMatch = "";
    public string OverallMatch = "";
    public string FileL        = "";
    public string FileR        = "";
}

public readonly struct ComparisonProgress(string status, int percent)
{
    public string Status  { get; } = status;
    public int    Percent { get; } = percent;
}

// ─── DocumentComparer ─────────────────────────────────────────────────────────

/// <summary>
/// Orchestrates document loading, pairwise comparison, and HTML report generation.
/// All comparison logic is a faithful port of CompareDocuments.cpp.
/// </summary>
public sealed class DocumentComparer : IDisposable
{
    // ─── Match mark constants (same values as C++ #defines) ──────────────────
    private const int WordUnmatched = -1;
    private const int WordPerfect   =  0;
    private const int WordFlaw      =  1;

    private readonly ComparisonSettings _settings;
    private readonly Stopwatch _sw = Stopwatch.StartNew();

    // Working arrays (allocated by SetupComparisons)
    private int[]? _matchMarkL, _matchMarkR;
    private int[]? _matchAnchorL, _matchAnchorR;
    private int[]? _matchMarkTempL, _matchMarkTempR;

    // Report files
    private StreamWriter? _fLog, _fMatch, _fMatchHtml;

    // State for current pair
    private int    _matchingWordsPerfect;
    private int    _matchingWordsTotalL;
    private int    _matchingWordsTotalR;
    private string _docNameL = "";
    private string _docNameR = "";

    public DocumentComparer(ComparisonSettings settings)
    {
        _settings = settings;
    }

    public void Dispose()
    {
        _fLog?.Dispose();       _fLog = null;
        _fMatch?.Dispose();     _fMatch = null;
        _fMatchHtml?.Dispose(); _fMatchHtml = null;
    }

    // ═════════════════════════════════════════════════════════════════════════
    //  SetupReports — port of CCompareDocuments::SetupReports
    // ═════════════════════════════════════════════════════════════════════════

    public void SetupReports()
    {
        _sw.Restart();
        var utf8 = new UTF8Encoding(false);
        string folder = _settings.ReportFolder;
        Directory.CreateDirectory(folder);

        _fLog       = new StreamWriter(Path.Combine(folder, "log.txt"),     false, utf8);
        _fMatch     = new StreamWriter(Path.Combine(folder, "matches.txt"), false, utf8);
        _fMatchHtml = new StreamWriter(Path.Combine(folder, "matches.html"),false, utf8);

        // Bug fix: original fwprintf(fLog, "Starting Report Files\n", m_Time) had a stale arg
        Log("Starting Report Files");
        WriteMatchesHtmlHeader();
    }

    private void WriteMatchesHtmlHeader()
    {
        var s = _settings;
        _fMatchHtml!.WriteLine("<html><title>File Comparison Report</title><body><H2>File Comparison Report</H2>");
        _fMatchHtml.WriteLine($"<H3>Produced by {s.SoftwareName} with These Settings:</H3><br><blockquote>Shortest Phrase to Match: {s.PhraseLength}");
        _fMatchHtml.WriteLine($"<br>Fewest Matches to Report: {s.WordThreshold}");
        _fMatchHtml.WriteLine($"<br>Ignore Punctuation: {(s.IgnorePunctuation ? "Yes" : "No")}");
        _fMatchHtml.WriteLine($"<br>Ignore Outer Punctuation: {(s.IgnoreOuterPunctuation ? "Yes" : "No")}");
        _fMatchHtml.WriteLine($"<br>Ignore Numbers: {(s.IgnoreNumbers ? "Yes" : "No")}");
        _fMatchHtml.WriteLine($"<br>Ignore Letter Case: {(s.IgnoreCase ? "Yes" : "No")}");
        _fMatchHtml.WriteLine($"<br>Skip Non-Words: {(s.SkipNonwords ? "Yes" : "No")}");
        if (s.SkipLongWords)
            _fMatchHtml.WriteLine($"<br>Skip Words Longer Than {s.SkipLength} Characters: Yes");
        else
            _fMatchHtml.WriteLine("<br>Skip Long Words: No");
        _fMatchHtml.WriteLine($"<br>Most Imperfections to Allow: {s.MismatchTolerance}");
        _fMatchHtml.WriteLine($"<br>Minimum % of Matching Words: {s.MismatchPercentage}");
        _fMatchHtml.WriteLine("</blockquote><br><br><table border='1' cellpadding='5'>" +
            "<tr><td align='center'>Perfect Match</td>" +
            "<td align='center'>Overall Match</td>" +
            "<td align='center'>View Both Files</td>" +
            "<td align='center'>File L</td>" +
            "<td align='center'>File R</td></tr>");
    }

    // ═════════════════════════════════════════════════════════════════════════
    //  LoadDocument — port of CCompareDocuments::LoadDocument
    // ═════════════════════════════════════════════════════════════════════════

    public Document LoadDocument(string path, DocType type)
    {
        Log($"Loading {path}");
        var hashes = new List<uint>(10000);

        foreach (WordToken tok in DocumentReader.ReadWords(path, _settings.BasicCharacters))
        {
            if (tok.DelimType == DelimType.Eof) break;
            string? filtered = WordProcessor.ApplyFilters(tok.Word, _settings);
            if (filtered is null) continue;
            hashes.Add(WordProcessor.WordHash(filtered));
        }

        int n = hashes.Count;
        var doc = new Document
        {
            Name             = path,
            Type             = type,
            WordsTotal       = n,
            WordHash         = hashes.ToArray(),
            SortedWordNumber = Enumerable.Range(0, n).ToArray(),
            SortedWordHash   = hashes.ToArray(),
        };

        // Bug fix: Array.Sort replaces HeapSort(&array[-1], ...) — no undefined behavior
        Array.Sort(doc.SortedWordHash, doc.SortedWordNumber);

        // Find first sorted word whose hash has high bits set (≥ 3 letters)
        if (_settings.PhraseLength > 1)
        {
            for (int i = 0; i < n; i++)
            {
                if ((doc.SortedWordHash[i] & 0xFFC00000u) != 0)
                {
                    doc.FirstHash = i;
                    break;
                }
            }
        }

        return doc;
    }

    // ═════════════════════════════════════════════════════════════════════════
    //  SetupComparisons — allocate working arrays sized for largest document
    // ═════════════════════════════════════════════════════════════════════════

    public void SetupComparisons(int maxWords)
    {
        _matchMarkL     = new int[maxWords];
        _matchMarkR     = new int[maxWords];
        _matchAnchorL   = new int[maxWords];
        _matchAnchorR   = new int[maxWords];
        _matchMarkTempL = new int[maxWords];
        _matchMarkTempR = new int[maxWords];
    }

    // ═════════════════════════════════════════════════════════════════════════
    //  ComparePair — faithful port of CCompareDocuments::ComparePair
    // ═════════════════════════════════════════════════════════════════════════

    /// <summary>
    /// Compare a pair of documents. Returns true if MatchingWordsPerfect &gt;= WordThreshold.
    /// Bug fix: CancellationToken replaces the g_abort global data race.
    /// </summary>
    public bool ComparePair(Document docL, Document docR, CancellationToken ct)
    {
        // All six arrays are allocated together in SetupComparisons; _matchMarkL being non-null guarantees the rest.
        if (_matchMarkL is null || _matchMarkR is null || _matchAnchorL is null || _matchAnchorR is null) return false;

        _matchingWordsPerfect = 0;
        _matchingWordsTotalL  = 0;
        _matchingWordsTotalR  = 0;

        Array.Fill(_matchMarkL,   WordUnmatched, 0, docL.WordsTotal);
        Array.Fill(_matchAnchorL, 0,             0, docL.WordsTotal);
        Array.Fill(_matchMarkR,   WordUnmatched, 0, docR.WordsTotal);
        Array.Fill(_matchAnchorR, 0,             0, docR.WordsTotal);

        int wnL = docL.FirstHash;
        int wnR = docR.FirstHash;
        int anchor = 0;
        int ctCheck = 0;

        while (wnL < docL.WordsTotal && wnR < docR.WordsTotal)
        {
            // Check cancellation every 256 iterations — cheap volatile read, avoids per-step overhead
            if ((ctCheck++ & 0xFF) == 0) ct.ThrowIfCancellationRequested();

            if (_matchMarkL[docL.SortedWordNumber[wnL]] != WordUnmatched) { wnL++; continue; }
            if (_matchMarkR[docR.SortedWordNumber[wnR]] != WordUnmatched) { wnR++; continue; }

            uint hL = docL.SortedWordHash[wnL];
            uint hR = docR.SortedWordHash[wnR];

            if (hL < hR) { wnL++; continue; }
            if (hL > hR) { wnR++; continue; }

            // Equal hashes — find extent of duplicate words with this hash
            uint hash = hL;
            int redL = wnL, redR = wnR;
            while (redL < docL.WordsTotal - 1 && docL.SortedWordHash[redL + 1] == hash) redL++;
            while (redR < docR.WordsTotal - 1 && docR.SortedWordHash[redR + 1] == hash) redR++;

            for (int iL = wnL; iL <= redL; iL++)
            {
                if (_matchMarkL[docL.SortedWordNumber[iL]] != WordUnmatched) continue;
                for (int iR = wnR; iR <= redR; iR++)
                {
                    if (_matchMarkR[docR.SortedWordNumber[iR]] != WordUnmatched) continue;

                    int posL = docL.SortedWordNumber[iL];
                    int posR = docR.SortedWordNumber[iR];
                    _matchMarkTempL![posL] = WordPerfect;
                    _matchMarkTempR![posR] = WordPerfect;

                    int firstL = posL - 1, lastL = posL + 1;
                    int firstR = posR - 1, lastR = posR + 1;

                    // Extend backward — perfect matches only
                    while (firstL >= 0 && firstR >= 0 &&
                           _matchMarkL[firstL] == WordUnmatched &&
                           _matchMarkR[firstR] == WordUnmatched &&
                           docL.WordHash[firstL] == docR.WordHash[firstR])
                    {
                        _matchMarkTempL[firstL] = WordPerfect;
                        _matchMarkTempR[firstR] = WordPerfect;
                        firstL--; firstR--;
                    }

                    // Extend forward — perfect matches only
                    while (lastL < docL.WordsTotal && lastR < docR.WordsTotal &&
                           _matchMarkL[lastL] == WordUnmatched &&
                           _matchMarkR[lastR] == WordUnmatched &&
                           docL.WordHash[lastL] == docR.WordHash[lastR])
                    {
                        _matchMarkTempL[lastL] = WordPerfect;
                        _matchMarkTempR[lastR] = WordPerfect;
                        lastL++; lastR++;
                    }

                    int firstLp = firstL + 1, firstRp = firstR + 1;
                    int lastLp  = lastL  - 1, lastRp  = lastR  - 1;
                    int perfectCount = lastLp - firstLp + 1;

                    if (_settings.MismatchTolerance > 0)
                    {
                        int firstLx = firstLp, firstRx = firstRp;
                        int lastLx  = lastLp,  lastRx  = lastRp;
                        int flaws = 0;

                        // Extend backward with mismatch tolerance
                        while (firstL >= 0 && firstR >= 0 &&
                               _matchMarkL[firstL] == WordUnmatched &&
                               _matchMarkR[firstR] == WordUnmatched)
                        {
                            if (docL.WordHash[firstL] == docR.WordHash[firstR])
                            {
                                perfectCount++; flaws = 0;
                                _matchMarkTempL[firstL] = WordPerfect;
                                _matchMarkTempR[firstR] = WordPerfect;
                                firstLp = firstL; firstRp = firstR;
                                firstL--; firstR--; continue;
                            }
                            flaws++;
                            if (flaws > _settings.MismatchTolerance) break;

                            if (firstL - 1 >= 0 && _matchMarkL[firstL - 1] == WordUnmatched &&
                                docL.WordHash[firstL - 1] == docR.WordHash[firstR])
                            {
                                if (Pct(firstL - 1, firstR, lastLx, lastRx, perfectCount + 1) < _settings.MismatchPercentage) break;
                                _matchMarkTempL[firstL] = WordFlaw; firstL--;
                                perfectCount++; flaws = 0;
                                _matchMarkTempL[firstL] = WordPerfect;
                                _matchMarkTempR[firstR] = WordPerfect;
                                firstLp = firstL; firstRp = firstR;
                                firstL--; firstR--; continue;
                            }

                            if (firstR - 1 >= 0 && _matchMarkR[firstR - 1] == WordUnmatched &&
                                docL.WordHash[firstL] == docR.WordHash[firstR - 1])
                            {
                                if (Pct(firstL, firstR - 1, lastLx, lastRx, perfectCount + 1) < _settings.MismatchPercentage) break;
                                _matchMarkTempR[firstR] = WordFlaw; firstR--;
                                perfectCount++; flaws = 0;
                                _matchMarkTempL[firstL] = WordPerfect;
                                _matchMarkTempR[firstR] = WordPerfect;
                                firstLp = firstL; firstRp = firstR;
                                firstL--; firstR--; continue;
                            }

                            if (Pct(firstL - 1, firstR - 1, lastLx, lastRx, perfectCount) < _settings.MismatchPercentage) break;
                            _matchMarkTempL[firstL] = WordFlaw;
                            _matchMarkTempR[firstR] = WordFlaw;
                            firstL--; firstR--;
                        }

                        flaws = 0;

                        // Extend forward with mismatch tolerance
                        while (lastL < docL.WordsTotal && lastR < docR.WordsTotal &&
                               _matchMarkL[lastL] == WordUnmatched &&
                               _matchMarkR[lastR] == WordUnmatched)
                        {
                            if (docL.WordHash[lastL] == docR.WordHash[lastR])
                            {
                                perfectCount++; flaws = 0;
                                _matchMarkTempL[lastL] = WordPerfect;
                                _matchMarkTempR[lastR] = WordPerfect;
                                lastLp = lastL; lastRp = lastR;
                                lastL++; lastR++; continue;
                            }
                            flaws++;
                            if (flaws == _settings.MismatchTolerance) break;

                            if (lastL + 1 < docL.WordsTotal && _matchMarkL[lastL + 1] == WordUnmatched &&
                                docL.WordHash[lastL + 1] == docR.WordHash[lastR])
                            {
                                if (Pct(firstLx, firstRx, lastL + 1, lastR, perfectCount + 1) < _settings.MismatchPercentage) break;
                                _matchMarkTempL[lastL] = WordFlaw; lastL++;
                                perfectCount++; flaws = 0;
                                _matchMarkTempL[lastL] = WordPerfect;
                                _matchMarkTempR[lastR] = WordPerfect;
                                lastLp = lastL; lastRp = lastR;
                                lastL++; lastR++; continue;
                            }

                            if (lastR + 1 < docR.WordsTotal && _matchMarkR[lastR + 1] == WordUnmatched &&
                                docL.WordHash[lastL] == docR.WordHash[lastR + 1])
                            {
                                if (Pct(firstLx, firstRx, lastL, lastR + 1, perfectCount + 1) < _settings.MismatchPercentage) break;
                                _matchMarkTempR[lastR] = WordFlaw; lastR++;
                                perfectCount++; flaws = 0;
                                _matchMarkTempL[lastL] = WordPerfect;
                                _matchMarkTempR[lastR] = WordPerfect;
                                lastLp = lastL; lastRp = lastR;
                                lastL++; lastR++; continue;
                            }

                            if (Pct(firstLx, firstRx, lastL + 1, lastR + 1, perfectCount) < _settings.MismatchPercentage) break;
                            _matchMarkTempL[lastL] = WordFlaw;
                            _matchMarkTempR[lastR] = WordFlaw;
                            lastL++; lastR++;
                        }
                    }

                    if (perfectCount >= _settings.PhraseLength)
                    {
                        anchor++;
                        for (int k = firstLp; k <= lastLp; k++)
                        {
                            _matchMarkL[k] = _matchMarkTempL![k];
                            if (_matchMarkTempL[k] == WordPerfect) _matchingWordsPerfect++;
                            _matchAnchorL![k] = anchor;
                        }
                        _matchingWordsTotalL += lastLp - firstLp + 1;
                        for (int k = firstRp; k <= lastRp; k++)
                        {
                            _matchMarkR[k] = _matchMarkTempR![k];
                            _matchAnchorR![k] = anchor;
                        }
                        _matchingWordsTotalR += lastRp - firstRp + 1;
                    }

                    ClearTempMarks(docL.WordsTotal, docR.WordsTotal);
                }
            }

            wnL = redL + 1;
            wnR = redR + 1;
        }

        return _matchingWordsPerfect >= _settings.WordThreshold;
    }

    private void ClearTempMarks(int nL, int nR)
    {
        if (_matchMarkTempL is null) return;
        Array.Fill(_matchMarkTempL,  WordUnmatched, 0, nL);
        Array.Fill(_matchMarkTempR!, WordUnmatched, 0, nR);
    }

    // PercentMatching — port of CCompareDocuments::PercentMatching
    private static int Pct(int fl, int fr, int ll, int lr, int perfect)
        => (200 * perfect) / (ll - fl + lr - fr + 2);

    // ═════════════════════════════════════════════════════════════════════════
    //  ReportMatchedPair — port of CCompareDocuments::ReportMatchedPair
    // ═════════════════════════════════════════════════════════════════════════

    public MatchResult ReportMatchedPair(Document docL, Document docR, int pairIndex)
    {
        _docNameL = Path.GetFileName(docL.Name);
        _docNameR = Path.GetFileName(docR.Name);

        // Bug fix: safe string ops replace wcsncat_s(..., 255) truncation
        string hrefL = $"{_docNameL}.{_docNameR}.html";
        string hrefR = $"{_docNameR}.{_docNameL}.html";
        string snL   = _docNameL.Length > 8 ? _docNameL[..8] : _docNameL;
        string snR   = _docNameR.Length > 8 ? _docNameR[..8] : _docNameR;
        string hrefB = $"SBS.{snR}.{snL}.{pairIndex}.html";

        string pm = FormatPerfectMatch(docL, docR);
        string om = FormatOverallMatch(docL, docR);

        _fMatch?.WriteLine($"{_matchingWordsPerfect}\t{_matchingWordsTotalL}\t{_matchingWordsTotalR}\t{docL.Name}\t{docR.Name}");
        Log($"Match: {_matchingWordsPerfect}\t{_matchingWordsTotalL}\t{_matchingWordsTotalR}\t{docL.Name}\t{docR.Name}");

        _fMatchHtml?.WriteLine(
            $"<tr><td>{pm}</td><td>{om}</td>" +
            $"<td><a href=\"{hrefB}\" target=\"_blank\">Side-by-Side</a></td>" +
            $"<td><a href=\"{hrefL}\" target=\"_blank\">{HtmlEscape(_docNameL)}</a></td>" +
            $"<td><a href=\"{hrefR}\" target=\"_blank\">{HtmlEscape(_docNameR)}</a></td></tr>");

        string folder = _settings.ReportFolder;
        WriteDocumentHtml(Path.Combine(folder, hrefL), docL, _matchMarkL!, _matchAnchorL!, hrefR);
        WriteDocumentHtml(Path.Combine(folder, hrefR), docR, _matchMarkR!, _matchAnchorR!, hrefL);
        WriteSideBySideHtml(Path.Combine(folder, hrefB), hrefL, hrefR);

        return new MatchResult
        {
            PerfectMatch = pm,
            OverallMatch = om,
            FileL        = _docNameL,
            FileR        = _docNameR,
        };
    }

    // ═════════════════════════════════════════════════════════════════════════
    //  FinishReports — port of CCompareDocuments::FinishReports
    // ═════════════════════════════════════════════════════════════════════════

    public void FinishReports(int matchingPairs)
    {
        _fMatchHtml?.WriteLine("</table>");
        if (matchingPairs == 0)
            _fMatchHtml?.WriteLine($"<br>{_settings.SoftwareName} found no matching pairs of documents." +
                "<br>You may want to lower the thresholds for matching and try again.<br>");
        else
            _fMatchHtml?.WriteLine($"<br>{_settings.SoftwareName} found {matchingPairs} matching pairs of documents.<br>");
        _fMatchHtml?.WriteLine("</body></html>");

        Log($"Done. Total CPU Time: {_sw.Elapsed.TotalSeconds:F3} seconds");
        Dispose();
    }

    // ═════════════════════════════════════════════════════════════════════════
    //  HTML file generation helpers
    // ═════════════════════════════════════════════════════════════════════════

    private void WriteDocumentHtml(string filePath, Document doc,
        int[] matchMark, int[] matchAnchor, string hrefOther)
    {
        var utf8 = new UTF8Encoding(false);
        using var sw = new StreamWriter(filePath, false, utf8);

        sw.WriteLine("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" " +
                     "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">");
        sw.WriteLine("<html xmlns=\"http://www.w3.org/1999/xhtml\">");
        sw.WriteLine("<head>");
        sw.WriteLine("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />");
        sw.WriteLine($"<title>Comparison of {HtmlEscape(_docNameL)} with {HtmlEscape(_docNameR)} " +
                     $"(Matched Words = {_matchingWordsPerfect})</title>");
        sw.WriteLine("<base target='right'>");
        sw.WriteLine("</head>");
        sw.WriteLine("<body>");

        DocumentToHtml(sw, doc.Name, matchMark, matchAnchor, doc.WordsTotal, hrefOther);

        sw.WriteLine("</body></html>");
    }

    private void WriteSideBySideHtml(string filePath, string hrefL, string hrefR)
    {
        var utf8 = new UTF8Encoding(false);
        using var sw = new StreamWriter(filePath, false, utf8);
        sw.WriteLine("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" " +
                     "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">");
        sw.WriteLine($"<html><title>Comparison of {HtmlEscape(_docNameR)} with {HtmlEscape(_docNameL)} " +
                     $"(Matched Words = {_matchingWordsPerfect})</title>");
        sw.WriteLine($"<frameset cols=\"*,*\" frameborder=\"YES\" border=\"1\" framespacing=\"0\">");
        sw.WriteLine($"<frame src=\"{hrefL}\" name=\"left\">");
        sw.WriteLine($"<frame src=\"{hrefR}\" name=\"right\">");
        sw.WriteLine("</frameset><body></body></html>");
    }

    // ═════════════════════════════════════════════════════════════════════════
    //  DocumentToHtml — port of CCompareDocuments::DocumentToHtml
    // ═════════════════════════════════════════════════════════════════════════

    private void DocumentToHtml(StreamWriter sw, string docPath,
        int[] matchMark, int[] matchAnchor, int words, string href)
    {
        int lastMatch  = WordUnmatched;
        int lastAnchor = 0;
        int wordCount  = 0;

        using var enumerator = DocumentReader.ReadWords(docPath, _settings.BasicCharacters).GetEnumerator();

        for (wordCount = 0; wordCount < words; wordCount++)
        {
            // Advance to next accepted word (skipping filtered-out tokens)
            string originalWord;
            int    delimType;

            while (true)
            {
                if (!enumerator.MoveNext()) goto done;
                WordToken tok = enumerator.Current;
                if (tok.DelimType == DelimType.Eof) goto done;
                string? filtered = WordProcessor.ApplyFilters(tok.Word, _settings);
                if (filtered is null) continue; // filtered word — don't count it
                originalWord = tok.Word;
                delimType    = tok.DelimType;
                break;
            }

            int xMatch  = matchMark[wordCount];
            int xAnchor = matchAnchor[wordCount];

            // Manage markup transitions
            if (lastMatch != xMatch || lastAnchor != xAnchor)
            {
                // Close prior markup
                if (lastMatch == WordPerfect) sw.Write("</font>");
                else if (lastMatch == WordFlaw) sw.Write("</font></i>");

                // Manage anchor transitions
                if (lastAnchor != xAnchor)
                {
                    if (lastAnchor > 0) sw.Write("</a>");
                    if (xAnchor > 0)
                    {
                        if (_settings.BriefReport && wordCount > 0)
                            sw.Write("</P>\n<P>");
                        sw.Write($"<a name='{xAnchor}' href='{href}#{xAnchor}'>");
                    }
                }

                // Open new markup
                if (xMatch == WordPerfect)   sw.Write("<font color='#FF0000'>");
                else if (xMatch == WordFlaw) sw.Write("<i><font color='#007F00'>");
            }

            lastMatch  = xMatch;
            lastAnchor = xAnchor;

            // Emit word in full-report mode, or always if matched
            if (!_settings.BriefReport || xMatch == WordPerfect || xMatch == WordFlaw)
            {
                foreach (char c in originalWord)
                    WriteHtmlChar(sw, c);

                if (delimType == DelimType.White)
                    sw.Write(' ');
                else if (delimType == DelimType.Newline)
                    sw.Write("<br>");
            }
        }

        done:
        // Close any open markup
        if (lastMatch == WordPerfect)   sw.Write("</font>");
        else if (lastMatch == WordFlaw) sw.Write("</font></i>");
        if (lastAnchor > 0) sw.Write("</a>");
    }

    // ═════════════════════════════════════════════════════════════════════════
    //  Vocabulary builder — port of OnButtonVocabulary logic
    // ═════════════════════════════════════════════════════════════════════════

    public Dictionary<string, int> BuildVocabulary(IEnumerable<string> paths)
    {
        var vocab = new Dictionary<string, int>(StringComparer.Ordinal);
        foreach (string path in paths)
        {
            foreach (WordToken tok in DocumentReader.ReadWords(path, _settings.BasicCharacters))
            {
                if (tok.DelimType == DelimType.Eof) break;
                string? filtered = WordProcessor.ApplyFilters(tok.Word, _settings);
                if (filtered is null || filtered.Length == 0) continue;
                vocab.TryGetValue(filtered, out int count);
                vocab[filtered] = count + 1;
            }
        }
        return vocab;
    }

    // ═════════════════════════════════════════════════════════════════════════
    //  Helpers
    // ═════════════════════════════════════════════════════════════════════════

    private void Log(string message) => _fLog?.WriteLine(message);

    private string FormatPerfectMatch(Document docL, Document docR)
    {
        int pctL = docL.WordsTotal > 0 ? 100 * _matchingWordsPerfect / docL.WordsTotal : 0;
        int pctR = docR.WordsTotal > 0 ? 100 * _matchingWordsPerfect / docR.WordsTotal : 0;
        return $"{_matchingWordsPerfect} ({pctL}% L, {pctR}% R)";
    }

    private string FormatOverallMatch(Document docL, Document docR)
    {
        int pctL = docL.WordsTotal > 0 ? 100 * _matchingWordsTotalL / docL.WordsTotal : 0;
        int pctR = docR.WordsTotal > 0 ? 100 * _matchingWordsTotalR / docR.WordsTotal : 0;
        return $"{_matchingWordsTotalL} ({pctL}%) L; {_matchingWordsTotalR} ({pctR}%) R";
    }

    /// <summary>Writes a single character to HTML, escaping &amp; and &lt;. Port of PrintWCharAsHtmlUTF8.</summary>
    private static void WriteHtmlChar(StreamWriter sw, char c)
    {
        if      (c == '&') sw.Write("&amp;");
        else if (c == '<') sw.Write("&lt;");
        else               sw.Write(c); // StreamWriter handles UTF-8 encoding automatically
    }

    private static string HtmlEscape(string s)
        => s.Replace("&", "&amp;").Replace("<", "&lt;").Replace(">", "&gt;");
}
