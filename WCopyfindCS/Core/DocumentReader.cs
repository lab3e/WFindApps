using System.Diagnostics;
using System.IO.Compression;
using System.Net.Http;
using System.Text;

namespace WCopyfindCS.Core;

// ─── Delimiter constants ──────────────────────────────────────────────────────
public static class DelimType
{
    public const int None    = 0;
    public const int White   = 1;
    public const int Newline = 2;
    public const int Eof     = 3;
}

// ─── Word token ───────────────────────────────────────────────────────────────
public readonly struct WordToken(string word, int delimType)
{
    public string Word      { get; } = word;
    public int    DelimType { get; } = delimType;
}

/// <summary>
/// Stateless document reader. Every call to ReadWords() re-reads the file from scratch,
/// so it can be called twice — once to build hashes and once to generate HTML.
/// Port of InputDocument.cpp.
/// </summary>
public static class DocumentReader
{
    private const int MaxWord = 255;

    private static readonly Lazy<HttpClient> Http = new(() => new HttpClient());

    private static string? FindPdftotext()
    {
        string p = Path.Combine(AppContext.BaseDirectory, "pdftotext.exe");
        return File.Exists(p) ? p : null;
    }

    // ── Public entry point ───────────────────────────────────────────────────

    public static IEnumerable<WordToken> ReadWords(string path, bool basicChars)
    {
        string ext = Path.GetExtension(path).ToLowerInvariant();
        return ext switch
        {
            ".txt"          => ReadTxt(path),
            ".html" or ".htm" => ReadHtml(path),
            ".docx"         => ReadDocx(path),
            ".doc"          => ReadDoc(path, basicChars),
            ".pdf"          => ReadPdf(path),
            ".url"          => ReadUrl(path),
            _               => ReadUnknown(path, basicChars),
        };
    }

    // ── TXT ──────────────────────────────────────────────────────────────────

    private static IEnumerable<WordToken> ReadTxt(string path)
    {
        using var sr = new StreamReader(path, Encoding.GetEncoding(1252),
                                        detectEncodingFromByteOrderMarks: true);
        foreach (var tok in TextTokenize(sr, skipControlChars: true, basicChars: false))
            yield return tok;
    }

    // ── HTML ─────────────────────────────────────────────────────────────────

    private static IEnumerable<WordToken> ReadHtml(string path)
    {
        using var sr = new StreamReader(path, Encoding.UTF8,
                                        detectEncodingFromByteOrderMarks: true);
        foreach (var tok in HtmlTokenize(sr, isDocx: false))
            yield return tok;
    }

    // ── DOCX — System.IO.Compression, replaces all 31 zlib C files ───────────

    private static IEnumerable<WordToken> ReadDocx(string path)
    {
        string xml;
        try
        {
            using var zip = ZipFile.OpenRead(path);
            var entry = zip.GetEntry("word/document.xml")
                ?? throw new InvalidDataException();
            using var es  = entry.Open();
            using var sr  = new StreamReader(es, Encoding.UTF8);
            xml = sr.ReadToEnd();
        }
        catch { yield break; }

        using var reader = new StringReader(xml);
        foreach (var tok in HtmlTokenize(reader, isDocx: true))
            yield return tok;
    }

    // ── DOC (legacy binary) — raw byte fallback ──────────────────────────────
    // Note: NPOI's HWPF (.doc) support is not available for net8.0 targets.
    // We use the same raw-byte approach the original C++ fallback used.

    private static IEnumerable<WordToken> ReadDoc(string path, bool basicChars)
        => ReadUnknown(path, basicChars);

    // ── PDF — pdftotext.exe pipe, or manual zlib fallback ────────────────────

    private static IEnumerable<WordToken> ReadPdf(string path)
    {
        string? pdftotextExe = FindPdftotext();
        return pdftotextExe is not null
            ? ReadPdfProcess(path, pdftotextExe)
            : ReadPdfManual(path);
    }

    private static IEnumerable<WordToken> ReadPdfProcess(string path, string exe)
    {
        var psi = new ProcessStartInfo
        {
            FileName               = exe,
            UseShellExecute        = false,
            CreateNoWindow         = true,
            RedirectStandardOutput = true,
            StandardOutputEncoding = Encoding.UTF8,
        };
        // Bug fix #12: use ArgumentList (no shell injection)
        psi.ArgumentList.Add("-enc"); psi.ArgumentList.Add("UTF-8");
        psi.ArgumentList.Add(path);   psi.ArgumentList.Add("-");

        Process? proc = null;
        try { proc = Process.Start(psi)!; } catch { yield break; }

        using (proc)
        using (var sr = proc.StandardOutput)
            foreach (var tok in TextTokenize(sr, skipControlChars: true, basicChars: false))
                yield return tok;
    }

    private static IEnumerable<WordToken> ReadPdfManual(string path)
    {
        byte[] data;
        try { data = File.ReadAllBytes(path); } catch { yield break; }

        var text = new List<byte>(data.Length * 2);
        ExtractPdfText(data, text);
        if (text.Count == 0) yield break;

        using var sr = new StringReader(Encoding.ASCII.GetString(text.ToArray()));
        foreach (var tok in TextTokenize(sr, skipControlChars: true, basicChars: false))
            yield return tok;
    }

    // Port of ProcessStreamPdf — extracts text bytes from decompressed PDF stream
    private static void ExtractPdfText(byte[] data, List<byte> output)
    {
        bool inText = false, inParen = false, inLit = false;
        var saved = new char[15];
        for (int j = 0; j < 15; j++) saved[j] = ' ';

        void Put(char c) => output.Add((byte)c);

        bool Token(char a, char b) =>
            (saved[11] is ' ' or '\n' or '\r') &&
             saved[12] == a && saved[13] == b &&
            (saved[14] is ' ' or '\n' or '\r');

        void Shift(char c) { for (int j = 0; j < 14; j++) saved[j] = saved[j + 1]; saved[14] = c; }

        int pos = 0;
        while (pos < data.Length)
        {
            int ss = IndexOf(data, pos, "stream"u8);
            int se = IndexOf(data, pos, "endstream"u8);
            if (ss < 0 || se <= ss) break;

            ss += 6;
            if (ss + 1 < data.Length && data[ss] == 0x0D && data[ss + 1] == 0x0A) ss += 2;
            else if (ss < data.Length && data[ss] == 0x0A) ss++;

            if (se >= 2 && data[se - 2] == 0x0D && data[se - 1] == 0x0A) se -= 2;
            else if (se >= 1 && data[se - 1] == 0x0A) se--;

            int len = se - ss;
            if (len > 2)
            {
                try
                {
                    using var ms  = new MemoryStream(data, ss + 2, len - 2);
                    using var ds  = new DeflateStream(ms, CompressionMode.Decompress);
                    using var bms = new MemoryStream();
                    ds.CopyTo(bms);
                    byte[] buf = bms.ToArray();

                    for (int i = 0; i < buf.Length; i++)
                    {
                        char c = (char)buf[i];
                        if (inText)
                        {
                            if (inParen)
                            {
                                if (c == ')' && !inLit) inParen = false;
                                else if (!inLit && c == '\\') inLit = true;
                                else { inLit = false; if (c >= ' ' && c <= '~') Put(c); }
                            }
                            else
                            {
                                if (Token('E', 'T')) { inText = false; Put('\n'); }
                                else if (Token('T', 'm')) Put(' ');
                                if (c == '(') { inParen = true; Put(' '); }
                            }
                        }
                        else if (Token('B', 'T')) { inText = true; Put('\n'); }
                        Shift(c);
                    }
                }
                catch { /* skip malformed stream */ }
            }
            pos = se + 9;
        }
    }

    private static int IndexOf(byte[] data, int start, ReadOnlySpan<byte> needle)
    {
        for (int i = start; i <= data.Length - needle.Length; i++)
        {
            bool ok = true;
            for (int j = 0; j < needle.Length; j++)
                if (data[i + j] != needle[j]) { ok = false; break; }
            if (ok) return i;
        }
        return -1;
    }

    // ── URL ───────────────────────────────────────────────────────────────────

    private static IEnumerable<WordToken> ReadUrl(string path)
    {
        string? url = null;
        try
        {
            foreach (string line in File.ReadLines(path))
                if (line.StartsWith("URL=", StringComparison.OrdinalIgnoreCase))
                { url = line.Substring(4).Trim(); break; }
        }
        catch { yield break; }

        if (string.IsNullOrEmpty(url)) yield break;

        string content;
        bool isHtml;
        try
        {
            var resp = Http.Value.GetAsync(url).GetAwaiter().GetResult();
            resp.EnsureSuccessStatusCode();
            string ct = resp.Content.Headers.ContentType?.MediaType ?? "";
            isHtml  = ct.Contains("html", StringComparison.OrdinalIgnoreCase);
            content = resp.Content.ReadAsStringAsync().GetAwaiter().GetResult();
        }
        catch { yield break; }

        using var sr = new StringReader(content);
        IEnumerable<WordToken> tokens = isHtml
            ? HtmlTokenize(sr, isDocx: false)
            : TextTokenize(sr, skipControlChars: true, basicChars: false);
        foreach (var tok in tokens) yield return tok;
    }

    // ── Unknown / raw bytes ───────────────────────────────────────────────────

    private static IEnumerable<WordToken> ReadUnknown(string path, bool basicChars)
    {
        using var fs = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
        foreach (var tok in RawBytesTokenize(fs, basicChars))
            yield return tok;
    }

    // ═════════════════════════════════════════════════════════════════════════
    //  Core tokenizers
    // ═════════════════════════════════════════════════════════════════════════

    /// <summary>
    /// Generic text tokenizer for TXT, PDF output, and DOC NPOI text.
    /// skipControlChars=true → skip control chars (TXT/PDF).
    /// skipControlChars=false → reset state on control chars (DOC).
    /// </summary>
    private static IEnumerable<WordToken> TextTokenize(
        TextReader rdr, bool skipControlChars, bool basicChars)
    {
        var  word     = new StringBuilder(MaxWord + 1);
        int  delim    = DelimType.None;
        bool gotWord  = false, gotDelim = false;
        int  pending  = -1;

        while (true)
        {
            int ch;
            if (pending >= 0) { ch = pending; pending = -1; }
            else ch = rdr.Read();

            if (ch < 0) { yield return new WordToken(word.ToString(), DelimType.Eof); yield break; }

            bool nl   = ch == '\n' || ch == '\r';
            bool ws   = !nl && char.IsWhiteSpace((char)ch);
            bool ctrl = (char.IsControl((char)ch) && ch < 0x80) || ch == 0xFF;

            if (nl)       { delim = Math.Max(delim, DelimType.Newline); gotDelim = true; }
            else if (ws)  { delim = Math.Max(delim, DelimType.White);   gotDelim = true; }
            else if (ctrl)
            {
                if (skipControlChars) continue;
                word.Clear(); gotWord = false; gotDelim = false; delim = DelimType.None;
            }
            else if (gotDelim)
            {
                if (gotWord)
                {
                    yield return new WordToken(word.ToString(), delim);
                    word.Clear(); gotWord = false; gotDelim = false; delim = DelimType.None;
                    pending = ch;
                }
                else { delim = DelimType.None; gotDelim = false; pending = ch; }
            }
            else
            {
                if (basicChars && ch > 0x7F)
                    { word.Clear(); gotWord = false; gotDelim = false; delim = DelimType.None; }
                else if (word.Length < MaxWord) { word.Append((char)ch); gotWord = true; }
            }
        }
    }

    /// <summary>
    /// Raw-byte tokenizer for Unknown and DOC fallback — handles single-null skipping
    /// and control-char state-reset (port of original C++ Unknown/DOC handler).
    /// </summary>
    private static IEnumerable<WordToken> RawBytesTokenize(Stream stream, bool basicChars)
    {
        var  word    = new StringBuilder(MaxWord + 1);
        int  delim   = DelimType.None;
        bool gotWord = false, gotDelim = false;
        int  pending = -1;

        while (true)
        {
            int ch;
            if (pending >= 0) { ch = pending; pending = -1; }
            else
            {
                ch = stream.ReadByte();
                if (ch == 0) ch = stream.ReadByte(); // skip single null
            }

            if (ch < 0) { yield return new WordToken(word.ToString(), DelimType.Eof); yield break; }

            bool nl  = ch == '\n' || ch == '\r';
            bool tab = ch == '\t' || ch == ' ';
            bool ctrl = ch < 0x20 || ch == 0xFF;

            if (nl)      { delim = Math.Max(delim, DelimType.Newline); gotDelim = true; }
            else if (tab){ delim = Math.Max(delim, DelimType.White);   gotDelim = true; }
            else if (ctrl)
                { word.Clear(); gotWord = false; gotDelim = false; delim = DelimType.None; }
            else if (basicChars && ch > 0x7F)
                { word.Clear(); gotWord = false; gotDelim = false; delim = DelimType.None; }
            else if (gotDelim)
            {
                if (gotWord)
                {
                    yield return new WordToken(word.ToString(), delim);
                    word.Clear(); gotWord = false; gotDelim = false; delim = DelimType.None;
                    pending = ch;
                }
                else { delim = DelimType.None; gotDelim = false; pending = ch; }
            }
            else if (word.Length < MaxWord) { word.Append((char)ch); gotWord = true; }
        }
    }

    /// <summary>
    /// HTML/DOCX tag-stripping tokenizer. State (binScript etc.) persists across yields.
    /// isDocx=false → HTML: handles P/BR/SCRIPT/STYLE/HEAD, HTML entities.
    /// isDocx=true  → DOCX: handles w:p/w:tab/, XML entities only.
    /// Port of the GetWord() HTML and DOCX branches in InputDocument.cpp.
    /// </summary>
    private static IEnumerable<WordToken> HtmlTokenize(TextReader rdr, bool isDocx)
    {
        var  word      = new StringBuilder(MaxWord + 1);
        int  delim     = DelimType.None;
        bool gotWord   = false, gotDelim = false;
        int  pending   = -1;

        // HTML section flags (persist across word boundaries, unlike original C++ bug)
        bool inScript = false, inStyle = false, inHeader = false;

        while (true)
        {
            int ch;
            if (pending >= 0) { ch = pending; pending = -1; }
            else ch = rdr.Read();

            if (ch < 0) { yield return new WordToken(word.ToString(), DelimType.Eof); yield break; }

            if (ch == '<') // ── XML/HTML tag ─────────────────────────────────
            {
                var tagName = new StringBuilder(32);
                bool needName = true;
                bool consumed = false;

                while (true)
                {
                    int tc = rdr.Read();
                    if (tc < 0) { yield return new WordToken(word.ToString(), DelimType.Eof); yield break; }
                    if (tc == '>') break;
                    if (tc == ' ') needName = false;
                    else if (needName) tagName.Append((char)tc);

                    // HTML comment: <!-- ... -->
                    if (!isDocx && tagName.ToString() == "!--")
                    {
                        int dashes = 0;
                        while (true)
                        {
                            int cc = rdr.Read(); if (cc < 0) break;
                            if (cc == '-') dashes++;
                            else if (cc == '>' && dashes >= 2) break;
                            else dashes = 0;
                        }
                        consumed = true; break;
                    }
                }

                if (consumed) continue;
                string tag = tagName.ToString();

                if (isDocx)
                {
                    if      (tag.Equals("w:p",    StringComparison.OrdinalIgnoreCase))
                        { delim = DelimType.Newline; gotDelim = true; }
                    else if (tag.Equals("w:tab/", StringComparison.OrdinalIgnoreCase))
                        { delim = Math.Max(delim, DelimType.White); gotDelim = true; }
                }
                else
                {
                    if      (tag.Equals("P",        StringComparison.OrdinalIgnoreCase) ||
                             tag.Equals("BR",        StringComparison.OrdinalIgnoreCase) ||
                             tag.Equals("BR/",       StringComparison.OrdinalIgnoreCase) ||
                             tag.Equals("UL",        StringComparison.OrdinalIgnoreCase) ||
                             tag.Equals("TD",        StringComparison.OrdinalIgnoreCase))
                        { delim = DelimType.Newline; gotDelim = true; }
                    else if (tag.Equals("SCRIPT",   StringComparison.OrdinalIgnoreCase)) inScript = true;
                    else if (tag.Equals("/SCRIPT",  StringComparison.OrdinalIgnoreCase)) inScript = false;
                    else if (tag.Equals("STYLE",    StringComparison.OrdinalIgnoreCase)) inStyle  = true;
                    else if (tag.Equals("/STYLE",   StringComparison.OrdinalIgnoreCase)) inStyle  = false;
                    else if (tag.Equals("HEAD",     StringComparison.OrdinalIgnoreCase)) inHeader = true;
                    else if (tag.Equals("/HEAD",    StringComparison.OrdinalIgnoreCase)) inHeader = false;
                }
                continue;
            }

            // Skip script/style/header content (HTML only)
            if (!isDocx && (inScript || inStyle || inHeader)) continue;

            bool isNl = ch == '\n' || ch == '\r';
            bool isWs = !isNl && char.IsWhiteSpace((char)ch);
            bool isCtrl = char.IsControl((char)ch) && ch != '\n' && ch != '\r';

            if (isNl)      { delim = Math.Max(delim, DelimType.Newline); gotDelim = true; continue; }
            if (isWs)      { delim = Math.Max(delim, DelimType.White);   gotDelim = true; continue; }
            if (isCtrl) continue;

            if (ch == '&') // ── Entity ────────────────────────────────────────
            {
                int resolved = ReadEntity(rdr);
                if (resolved < 0) continue;
                ch = resolved;
                isNl = ch == '\n' || ch == '\r';
                isWs = !isNl && ch <= 127 && char.IsWhiteSpace((char)ch);
                if (isNl) { delim = Math.Max(delim, DelimType.Newline); gotDelim = true; continue; }
                if (isWs) { delim = Math.Max(delim, DelimType.White);   gotDelim = true; continue; }
            }

            if (gotDelim)
            {
                if (gotWord)
                {
                    yield return new WordToken(word.ToString(), delim);
                    word.Clear(); gotWord = false; gotDelim = false; delim = DelimType.None;
                    pending = ch;
                }
                else { delim = DelimType.None; gotDelim = false; pending = ch; }
                continue;
            }

            if (word.Length < MaxWord) { word.Append((char)ch); gotWord = true; }
        }
    }

    // ─── HTML entity decoder ──────────────────────────────────────────────────

    private static int ReadEntity(TextReader rdr)
    {
        var buf = new StringBuilder(16);
        while (true)
        {
            int c = rdr.Read();
            if (c < 0 || c == ';') break;
            if (!char.IsLetterOrDigit((char)c) && c != '#') break;
            buf.Append((char)c);
        }
        string e = buf.ToString().TrimEnd();
        if (e.Length == 0) return -1;

        if (e[0] == '#')
        {
            if (e.Length > 1 && e[1] == 'x')
                return int.TryParse(e.AsSpan(2),
                    System.Globalization.NumberStyles.HexNumber, null, out int hv) ? hv : -1;
            return int.TryParse(e.AsSpan(1), out int dv) ? dv : -1;
        }

        return e switch
        {
            "nbsp"=>160,"iexcl"=>161,"cent"=>162,"pound"=>163,"yen"=>165,"sect"=>167,
            "copy"=>169,"reg"=>174,"plusmn"=>177,"micro"=>181,"para"=>182,"middot"=>183,
            "raquo"=>187,"iquest"=>191,"Agrave"=>192,"Aacute"=>193,"Acirc"=>194,
            "Atilde"=>195,"Auml"=>196,"Aring"=>197,"AElig"=>198,"Ccedil"=>199,
            "Egrave"=>200,"Eacute"=>201,"Ecirc"=>202,"Euml"=>203,"Igrave"=>204,
            "Iacute"=>205,"Icirc"=>206,"Iuml"=>207,"Ntilde"=>209,"Ograve"=>210,
            "Oacute"=>211,"Ocirc"=>212,"Otilde"=>213,"Ouml"=>214,"Oslash"=>216,
            "Ugrave"=>217,"Uacute"=>218,"Ucirc"=>219,"Uuml"=>220,"szlig"=>223,
            "agrave"=>224,"aacute"=>225,"acirc"=>226,"atilde"=>227,"auml"=>228,
            "aring"=>229,"aelig"=>230,"ccedil"=>231,"egrave"=>232,"eacute"=>233,
            "ecirc"=>234,"euml"=>235,"igrave"=>236,"iacute"=>237,"icirc"=>238,
            "iuml"=>239,"ntilde"=>241,"ograve"=>242,"oacute"=>243,"ocirc"=>244,
            "otilde"=>245,"ouml"=>246,"divide"=>247,"oslash"=>248,"ugrave"=>249,
            "uacute"=>250,"ucirc"=>251,"uuml"=>252,"yuml"=>255,
            "quot"=>34,"amp"=>38,"lt"=>60,"gt"=>62,"trade"=>8482,
            "ndash"=>8211,"mdash"=>8212,"lsquo"=>8216,"rsquo"=>8217,
            "ldquo"=>8220,"rdquo"=>8221,"euro"=>8364,
            _ => -1
        };
    }
}
