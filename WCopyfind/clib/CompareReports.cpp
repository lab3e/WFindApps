// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Louis A. Bloomfield
// CompareReports.cpp : writes WCopyfind's reports
//
// The report folder receives:
//   matches.html		an index of the matching pairs, sortable and filterable
//   matches.txt		the same list, tab-separated, in the format WCopyfind has always used
//   log.txt			a log of the run
//   pairs\pair-NNNNN.html	one page per matching pair, showing the two documents side by side
//   pairs\pairs.js		the number of pairs, so each pair page can link to the previous and next pair

#include "..\stdafx.h"
#include <afxinet.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include "InputDocument.h"
#include "Words.h"
#include "CompareDocuments.h"
#include "ReportAssets.h"

static void AppendEscaped(std::wstring& out, const wchar_t* text)
{
	for(; *text; text++)
	{
		switch(*text)
		{
		case L'&': out += L"&amp;"; break;
		case L'<': out += L"&lt;"; break;
		case L'>': out += L"&gt;"; break;
		case L'"': out += L"&quot;"; break;
		default: out += *text;
		}
	}
}

static std::wstring Escaped(const CString& text)
{
	std::wstring out;
	AppendEscaped(out, text);
	return out;
}

static std::wstring Number(long long value)			// 12345 -> "12,345"
{
	std::wstring digits = std::to_wstring(value < 0 ? -value : value), out;
	for(size_t i = 0; i < digits.size(); i++)
	{
		if(i > 0 && (digits.size() - i) % 3 == 0) out += L',';
		out += digits[i];
	}
	return value < 0 ? L"-" + out : out;
}

static int Percent(int part, int whole)
{
	return whole > 0 ? (int)((100LL * part) / whole) : 0;
}

static std::wstring PercentText(int part, int whole)		// "12%", or "&lt;1%" when there is a match too small to round up to 1%
{
	int percent = Percent(part, whole);
	return (percent == 0 && part > 0 ? std::wstring(L"&lt;1") : std::to_wstring(percent)) + L"%";
}

static std::string ToUtf8(const std::wstring& text)
{
	int bytes = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), nullptr, 0, nullptr, nullptr);
	std::string utf8(bytes, '\0');
	WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), utf8.data(), bytes, nullptr, nullptr);
	return utf8;
}

static bool WriteUtf8File(const CString& path, const std::wstring& text)
{
	FILE* file = nullptr;
	if((_wfopen_s(&file, path, L"wb") != 0) || (file == nullptr)) return false;
	std::string utf8 = ToUtf8(text);
	bool ok = fwrite(utf8.data(), 1, utf8.size(), file) == utf8.size();
	fclose(file);
	return ok;
}

static void WriteUtf8Line(FILE* file, const CString& line)		// for the text files, which have no byte-order mark
{
	if(file == nullptr) return;
	std::string utf8 = ToUtf8(std::wstring(line) + L"\r\n");
	fwrite(utf8.data(), 1, utf8.size(), file);
	fflush(file);
}

static CString FolderOf(const CString& path)
{
	int slash = path.ReverseFind(L'\\');
	return slash < 0 ? CString() : path.Left(slash);
}

CString CCompareDocuments::FileNameOf(const CString& path)
{
	int slash = path.ReverseFind(L'\\');
	return slash < 0 ? path : path.Mid(slash + 1);
}

static std::wstring PageStart(const std::wstring& title, const wchar_t* style)
{
	return L"<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n"
		L"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n<title>" + title + L"</title>\n<style>"
		+ REPORT_COMMON_STYLE + style + L"</style>\n</head>\n";
}

int CCompareDocuments::SetupReports()
{
	m_StartTicks = clock();
	m_PairRecords.clear();
	m_Unreadable.clear();

	SHCreateDirectoryExW(nullptr, m_szReportFolder, nullptr);
	SHCreateDirectoryExW(nullptr, m_szReportFolder + L"\\pairs", nullptr);
	if(!PathIsDirectoryW(m_szReportFolder + L"\\pairs")) return ERR_CANNOT_CREATE_REPORT_FOLDER;

	if((_wfopen_s(&m_fLog, m_szReportFolder + L"\\log.txt", L"w") != 0) || (m_fLog == NULL)) return ERR_CANNOT_OPEN_LOG_FILE;
	fwprintf(m_fLog, L"Starting Report Files\n");

	if((_wfopen_s(&m_fMatch, m_szReportFolder + L"\\matches.txt", L"wb") != 0) || (m_fMatch == NULL)) return ERR_CANNOT_OPEN_COMPARISON_REPORT_TXT_FILE;

	m_szIndexPath = m_szReportFolder + L"\\matches.html";
	return -1;
}

// Function: ReportMatchedPair
// Purpose: Records a pair that shares enough words, in matches.txt and in its own side-by-side page.

int CCompareDocuments::ReportMatchedPair()
{
	m_MatchingDocumentPairs++;

	PairRecord record;
	record.Number = m_MatchingDocumentPairs;
	record.Perfect = m_MatchingWordsPerfect;
	record.TotalL = m_MatchingWordsTotalL;
	record.TotalR = m_MatchingWordsTotalR;
	record.WordsL = m_pDocL->m_WordsTotal;
	record.WordsR = m_pDocR->m_WordsTotal;
	record.Passages = m_Anchors;
	record.PathL = m_pDocL->m_szDocumentName;
	record.PathR = m_pDocR->m_szDocumentName;
	record.File.Format(L"pairs/pair-%05d.html", record.Number);

	CString line;
	line.Format(L"%d\t%d\t%d\t%s\t%s", record.Perfect, record.TotalL, record.TotalR, (LPCWSTR)record.PathL, (LPCWSTR)record.PathR);
	WriteUtf8Line(m_fMatch, line);
	fwprintf(m_fLog, L"Match: %d\t%d\t%d\t%s\t%s\n", record.Perfect, record.TotalL, record.TotalR, (LPCWSTR)record.PathL, (LPCWSTR)record.PathR);
	fflush(m_fLog);

	m_szDocL = FileNameOf(record.PathL);
	m_szDocR = FileNameOf(record.PathR);

	std::wstring nameL = Escaped(m_szDocL), nameR = Escaped(m_szDocR);
	std::wstring out;
	out.reserve(size_t(record.WordsL + record.WordsR) * 12 + 32768);
	out += PageStart(nameL + L" &harr; " + nameR + L" &ndash; WCopyfind", PAIR_STYLE);
	out += L"<body data-pair=\"" + std::to_wstring(record.Number) + L"\">\n<header>\n";
	out += L"<div class=\"nav\"><a href=\"../matches.html\">&larr; All matching pairs</a><span id=\"pairOf\" class=\"muted\">Pair "
		+ std::to_wstring(record.Number) + L"</span><span class=\"spacer\"></span>"
		L"<button id=\"prevPair\">&lsaquo; Previous pair</button><button id=\"nextPair\">Next pair &rsaquo;</button></div>\n";
	out += L"<h1>" + nameL + L" &nbsp;&harr;&nbsp; " + nameR + L"</h1>\n";
	out += L"<p class=\"stats\"><strong>" + Number(record.Perfect) + L" matching words</strong> &middot; "
		+ PercentText(record.Perfect, record.WordsL) + L" of A (" + Number(record.WordsL) + L" words) &middot; "
		+ PercentText(record.Perfect, record.WordsR) + L" of B (" + Number(record.WordsR) + L" words) &middot; "
		+ Number(record.Passages) + (record.Passages == 1 ? L" matching passage" : L" matching passages");
	if(m_MismatchTolerance > 0)
		out += L"<br><span class=\"muted\">Including imperfect matches: " + Number(record.TotalL) + L" words of A ("
			+ PercentText(record.TotalL, record.WordsL) + L"), " + Number(record.TotalR) + L" words of B ("
			+ PercentText(record.TotalR, record.WordsR) + L"). Imperfect words are shown in <span class=\"fl\">italics</span>.</span>";
	out += L"</p>\n";
	out += L"<div class=\"tools\"><button id=\"prevMatch\">&lsaquo; Previous match</button><button id=\"nextMatch\">Next match &rsaquo;</button>"
		L"<span id=\"where\"></span>";
	if(!m_bBriefReport) out += L"<label><input type=\"checkbox\" id=\"onlyMatches\"> Show only paragraphs with matches</label>";
	else out += L"<input type=\"checkbox\" id=\"onlyMatches\" hidden><span class=\"muted\">Brief report: only the matching passages are shown.</span>";
	out += L"<span class=\"muted\">Click a highlighted passage to find it in the other document. Keys: N next, P previous.</span></div>\n</header>\n";
	out += L"<div class=\"cols\">\n";

	for(int side = 0; side < 2; side++)
	{
		const Document* doc = side ? m_pDocR : m_pDocL;
		const CString& path = doc->m_szDocumentName;
		const wchar_t letter = side ? L'B' : L'A';
		out += L"<section class=\"doc\" id=\"";
		out += letter;
		out += L"\"><h2><span class=\"side\">";
		out += letter;
		out += L"</span>" + Escaped(FileNameOf(path)) + L"<span class=\"folder\">" + Escaped(FolderOf(path)) + L"</span></h2>\n<div class=\"text\">\n";

		CInputDocument indoc;
		indoc.m_bBasic_Characters = m_bBasic_Characters;
		int iReturn = indoc.OpenDocument(path);
		if(iReturn > -1)
		{
			indoc.CloseDocument();
			return side ? ERR_CANNOT_OPEN_RIGHT_DOCUMENT_FILE : ERR_CANNOT_OPEN_LEFT_DOCUMENT_FILE;
		}
		DocumentToHtml(indoc, side ? m_MatchMarkR.data() : m_MatchMarkL.data(), side ? m_MatchAnchorR.data() : m_MatchAnchorL.data(),
			doc->m_WordsTotal, letter, out);
		indoc.CloseDocument();
		out += L"</div>\n</section>\n";
	}

	out += L"</div>\n<script src=\"pairs.js\"></script>\n<script>";
	out += PAIR_SCRIPT;
	out += L"</script>\n</body>\n</html>\n";

	CString pagePath = m_szReportFolder + L"\\" + record.File;
	pagePath.Replace(L'/', L'\\');
	if(!WriteUtf8File(pagePath, out)) return ERR_CANNOT_OPEN_SIDE_BY_SIDE_HTML_FILE;

	m_PairRecords.push_back(record);
	return -1;
}

// Function: DocumentToHtml
// Purpose: Rereads a document exactly as LoadDocument read it and writes its text as HTML paragraphs, with each
//			matching phrase wrapped in a <mark> that carries the phrase's anchor number (shared with the other
//			document). Words that the filters skipped are still shown; inside a matching phrase they stay inside
//			its highlight. In a brief report only the matching phrases are written, one to a paragraph.

void CCompareDocuments::DocumentToHtml(CInputDocument& indoc, const int *MatchMark, const int *MatchAnchor, long words, wchar_t side, std::wstring& out)
{
	std::vector<bool> anchored(size_t(m_Anchors) + 2, false);
	int openAnchor = 0;				// the phrase whose highlight is open, or 0
	bool inParagraph = false;
	bool pendingSpace = false;

	auto closeMark = [&]()
	{
		if(openAnchor > 0) out += L"</mark>";
		openAnchor = 0;
	};
	auto closeParagraph = [&]()
	{
		closeMark();
		if(inParagraph) out += L"</p>\n";
		inParagraph = false;
		pendingSpace = false;
	};

	auto emit = [&](const wchar_t* text, int delimiter, int anchor, int mark)
	{
		if(m_bBriefReport)
		{
			if(anchor <= 0) return;									// a brief report shows only matching phrases
			if(anchor != openAnchor) closeParagraph();
		}
		if(!inParagraph)
		{
			out += L"<p>";
			inParagraph = true;
		}
		if(anchor != openAnchor)
		{
			closeMark();
			if(pendingSpace) out += L" ";
			pendingSpace = false;
			if(anchor > 0)
			{
				std::wstring a = std::to_wstring(anchor);
				out += L"<mark class=\"m c" + std::to_wstring(anchor % 4) + L"\" data-a=\"" + a + L"\"";
				if(anchor < (int)anchored.size() && !anchored[anchor])
				{
					out += L" id=\"";
					out += side;
					out += a + L"\"";
					anchored[anchor] = true;
				}
				out += L">";
				openAnchor = anchor;
			}
		}
		if(pendingSpace) out += L" ";
		pendingSpace = false;

		if(mark == WORD_FLAW && anchor > 0)
		{
			out += L"<span class=\"fl\">";
			AppendEscaped(out, text);
			out += L"</span>";
		}
		else AppendEscaped(out, text);

		if(delimiter == DEL_TYPE_WHITE) pendingSpace = true;
		else if(delimiter == DEL_TYPE_NEWLINE)
		{
			if(m_bBriefReport) pendingSpace = true;					// keep a brief report's phrase in one paragraph
			else closeParagraph();
		}
	};

	struct Skipped { std::wstring Text; int Delimiter; };
	std::vector<Skipped> skipped;					// filtered-out words waiting to learn which phrase they fall in
	wchar_t word[WORDBUFFERLENGTH], tword[WORDBUFFERLENGTH];
	int DelimiterType = DEL_TYPE_WHITE;
	long wordNumber = 0;

	while(DelimiterType != DEL_TYPE_EOF)
	{
		if(indoc.GetWord(word, DelimiterType) > -1) break;
		wcscpy_s(tword, word);
		if(m_bIgnorePunctuation) WordRemovePunctuation(tword);	// the same filters, in the same order, as LoadDocument
		if(m_bIgnoreOuterPunctuation) wordxouterpunct(tword);
		if(m_bIgnoreNumbers) WordRemoveNumbers(tword);
		if(m_bIgnoreCase) WordToLowerCase(tword);
		if((m_bSkipLongWords && (wcslen(tword) > m_SkipLength)) || (m_bSkipNonwords && !WordCheck(tword)))
		{
			skipped.push_back({word, DelimiterType});
			continue;
		}
		if(wordNumber >= words) break;								// only if the document changed since it was loaded

		int anchor = MatchAnchor[wordNumber];
		int mark = MatchMark[wordNumber];
		if(mark != WORD_PERFECT && mark != WORD_FLAW) anchor = 0;
		int between = (wordNumber > 0 && anchor > 0 && MatchAnchor[wordNumber - 1] == anchor) ? anchor : 0;
		for(const Skipped& s : skipped) emit(s.Text.c_str(), s.Delimiter, between, WORD_PERFECT);
		skipped.clear();

		if(word[0] != 0) emit(word, DelimiterType, anchor, mark);
		wordNumber++;
	}
	for(const Skipped& s : skipped) emit(s.Text.c_str(), s.Delimiter, 0, WORD_UNMATCHED);
	closeParagraph();
}

// Function: FinishReports
// Purpose: Writes the index page and the pair count, and closes the text files.

int CCompareDocuments::FinishReports(bool stopped)
{
	m_Time = float((clock() - m_StartTicks) * (1.0 / CLOCKS_PER_SEC));
	if(m_fMatch != NULL) { fclose(m_fMatch); m_fMatch = NULL; }

	std::wstring count = L"window.WCOPYFIND_PAIRS = " + std::to_wstring(m_PairRecords.size()) + L";\n";
	WriteUtf8File(m_szReportFolder + L"\\pairs\\pairs.js", count);

	SYSTEMTIME now;
	GetLocalTime(&now);
	wchar_t date[64], time[64];
	GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_LONGDATE, &now, nullptr, date, 64, nullptr);
	GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, TIME_NOSECONDS, &now, nullptr, time, 64);
	wchar_t seconds[32];
	swprintf_s(seconds, L"%.1f", m_Time);

	int loaded = 0;
	for(int i = 0; i < m_Documents; i++) if(m_pDocs[i].m_bLoaded) loaded++;

	std::wstring out = PageStart(L"WCopyfind report", INDEX_STYLE);
	out += L"<body>\n<div class=\"wrap\">\n<h1>WCopyfind report</h1>\n<p class=\"stats\">";
	if(m_PairRecords.empty())
		out += L"No pairs of documents share " + Number(m_WordThreshold) + L" or more matching words. You may want to lower the thresholds and try again.";
	else
		out += L"<strong>" + Number((long long)m_PairRecords.size()) + (m_PairRecords.size() == 1 ? L" matching pair" : L" matching pairs")
			+ L"</strong> among " + Number(loaded) + L" documents";
	out += L" &middot; " + Number(m_Compares) + (m_Compares == 1 ? L" pair compared" : L" pairs compared") + L" in " + seconds + L" seconds</p>\n";

	out += L"<p class=\"settings\">Matching phrases of at least " + std::to_wstring(m_PhraseLength) + L" words &middot; Reporting pairs that share at least "
		+ Number(m_WordThreshold) + L" matching words &middot; ";
	if(m_MismatchTolerance > 0)
		out += L"Up to " + std::to_wstring(m_MismatchTolerance) + (m_MismatchTolerance == 1 ? L" imperfection" : L" imperfections")
			+ L" in a row, with at least " + std::to_wstring(m_MismatchPercentage) + L"% matching";
	else out += L"Exact matches only";
	if(m_bIgnoreCase) out += L" &middot; Ignoring letter case";
	if(m_bIgnorePunctuation) out += L" &middot; Ignoring punctuation";
	else if(m_bIgnoreOuterPunctuation) out += L" &middot; Ignoring outer punctuation";
	if(m_bIgnoreNumbers) out += L" &middot; Ignoring numbers";
	if(m_bSkipNonwords) out += L" &middot; Skipping non-words";
	if(m_bSkipLongWords) out += L" &middot; Skipping words longer than " + std::to_wstring(m_SkipLength) + L" characters";
	if(m_bBasic_Characters) out += L" &middot; Basic characters only";
	if(m_bBriefReport) out += L" &middot; Brief report";

	CString commonFolder;										// when every document is in one folder, say so once
	bool oneFolder = !m_PairRecords.empty();
	for(const PairRecord& r : m_PairRecords)
		for(const CString* path : {&r.PathL, &r.PathR})
		{
			if(commonFolder.IsEmpty()) commonFolder = FolderOf(*path);
			else if(FolderOf(*path).CompareNoCase(commonFolder) != 0) oneFolder = false;
		}
	if(oneFolder) out += L"<br>All the matching documents are in " + Escaped(commonFolder);
	out += L"<br>Produced by " + Escaped(m_szSoftwareName) + L" on " + date + L" at " + time + L"</p>\n";

	if(stopped) out += L"<p class=\"notice\">The comparison was stopped before it finished, so this report lists only the pairs found until then.</p>\n";
	if(!m_Unreadable.empty())
	{
		out += L"<div class=\"notice\"><strong>" + Number((long long)m_Unreadable.size())
			+ (m_Unreadable.size() == 1 ? L" document couldn't be read" : L" documents couldn't be read") + L"</strong> and " +
			(m_Unreadable.size() == 1 ? L"was" : L"were") + L" left out:<ul>";
		for(const auto& u : m_Unreadable) out += L"<li>" + Escaped(u.first) + L" <span class=\"muted\">&ndash; " + Escaped(u.second) + L"</span></li>";
		out += L"</ul></div>\n";
	}

	if(!m_PairRecords.empty())
	{
		bool imperfect = m_MismatchTolerance > 0;
		out += L"<div class=\"bar\"><input id=\"filter\" type=\"search\" placeholder=\"Filter by document name\" aria-label=\"Filter by document name\">"
			L"<span class=\"muted\">Click a column heading to sort.</span><span id=\"shown\"></span></div>\n";
		out += L"<div class=\"tablewrap\"><table id=\"pairs\"><thead><tr><th class=\"num\">#</th><th class=\"num\">Matching words</th><th class=\"num\">Share of A</th><th class=\"num\">Share of B</th>";
		if(imperfect) out += L"<th class=\"num\" title=\"Words in matching phrases, including imperfect matches\">Including imperfect (A / B)</th>";
		out += L"<th>Document A</th><th>Document B</th></tr></thead>\n<tbody>\n";
		for(const PairRecord& r : m_PairRecords)
		{
			CString nameL = FileNameOf(r.PathL), nameR = FileNameOf(r.PathR);
			CString names = nameL + L" " + nameR;
			names.MakeLower();
			std::wstring link = L"<a href=\"" + std::wstring(r.File) + L"\">";
			out += L"<tr data-names=\"" + Escaped(names) + L"\">";
			out += L"<td class=\"num\" data-v=\"" + std::to_wstring(r.Number) + L"\">" + link + std::to_wstring(r.Number) + L"</a></td>";
			out += L"<td class=\"num\" data-v=\"" + std::to_wstring(r.Perfect) + L"\">" + Number(r.Perfect) + L"</td>";
			for(int side = 0; side < 2; side++)
			{
				int words = side ? r.WordsR : r.WordsL;
				wchar_t share[32];
				swprintf_s(share, L"%.4f", words ? 100.0 * r.Perfect / words : 0.0);
				out += L"<td class=\"num\" data-v=\"" + std::wstring(share) + L"\" title=\"of " + Number(words) + L" words\">" + PercentText(r.Perfect, words)
					+ L"<span class=\"meter\"><i style=\"width:" + std::to_wstring(Percent(r.Perfect, words)) + L"%\"></i></span></td>";
			}
			if(imperfect)
				out += L"<td class=\"num\" data-v=\"" + std::to_wstring((std::max)(r.TotalL, r.TotalR)) + L"\">" + Number(r.TotalL) + L" / " + Number(r.TotalR) + L"</td>";
			std::wstring folderL = oneFolder ? L"" : L"<span class=\"folder\">" + Escaped(FolderOf(r.PathL)) + L"</span>";
			std::wstring folderR = oneFolder ? L"" : L"<span class=\"folder\">" + Escaped(FolderOf(r.PathR)) + L"</span>";
			out += L"<td class=\"doc\" data-v=\"" + Escaped(nameL) + L"\">" + link + Escaped(nameL) + L"</a>" + folderL + L"</td>";
			out += L"<td class=\"doc\" data-v=\"" + Escaped(nameR) + L"\">" + link + Escaped(nameR) + L"</a>" + folderR + L"</td>";
			out += L"</tr>\n";
		}
		out += L"</tbody></table></div>\n";
	}
	out += L"</div>\n<script>";
	out += INDEX_SCRIPT;
	out += L"</script>\n</body>\n</html>\n";

	bool written = WriteUtf8File(m_szIndexPath, out);

	fwprintf(m_fLog, L"Finishing Report Files\nDone. Total CPU Time: %.3f seconds\n", m_Time);
	fclose(m_fLog);
	m_fLog = NULL;
	return written ? -1 : ERR_CANNOT_OPEN_COMPARISON_REPORT_HTML_FILE;
}
