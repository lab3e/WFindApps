// RepeatReport.cpp : writes the HTML report for CRepeatFinder
//
// The report is one self-contained page: a summary table of repeat groups, followed by the full text with
// each group's copies highlighted in the group's color. Clicking a highlighted passage jumps to the next copy
// of it. Each group can be dismissed (for intentional repeats); dismissals are remembered by the browser for
// this work, keyed by the text of the passage, so they survive re-running WRepeatfind after edits.

#include "stdafx.h"
#include <afxinet.h>
#include "InputDocument.h"
#include "RepeatFinder.h"

static const int PALETTE_SIZE = 8;

static void AppendEscaped(std::wstring& out, const std::wstring& text)
{
	for(wchar_t ch : text)
	{
		switch(ch)
		{
		case L'&': out += L"&amp;"; break;
		case L'<': out += L"&lt;"; break;
		case L'>': out += L"&gt;"; break;
		case L'"': out += L"&quot;"; break;
		default: out += ch;
		}
	}
}

static std::wstring Escaped(const std::wstring& text)
{
	std::wstring out;
	AppendEscaped(out, text);
	return out;
}

static const wchar_t* REPORT_STYLE = LR"CSS(
:root {
  color-scheme: light dark;
  --bg: #fdfcf9; --fg: #1f1f1f; --muted: #6b6b6b; --line: #dedbd3; --panel: #f4f2ec; --accent: #2458b3;
  --c0: #ffe08a; --c1: #b9e3ff; --c2: #c6efbd; --c3: #ffc9dd; --c4: #dcd0ff; --c5: #ffd2ad; --c6: #b5eee3; --c7: #e9e79c;
}
@media (prefers-color-scheme: dark) {
  :root {
    --bg: #1b1c1e; --fg: #e6e4df; --muted: #a09d97; --line: #3a3b3e; --panel: #242528; --accent: #8fb3ff;
    --c0: #6b5516; --c1: #1d4f6e; --c2: #2c5a24; --c3: #6d2945; --c4: #43337a; --c5: #6e4219; --c6: #1b5a50; --c7: #545218;
  }
}
* { box-sizing: border-box; }
body { margin: 0; background: var(--bg); color: var(--fg); font: 15px/1.5 "Segoe UI", system-ui, sans-serif; }
.wrap { max-width: 52rem; margin: 0 auto; padding: 1.5rem 1rem 4rem; }
h1 { font-size: 1.5rem; margin: 0 0 .25rem; }
h2.doc { font-size: 1.15rem; margin: 2.5rem 0 1rem; padding-bottom: .3rem; border-bottom: 1px solid var(--line); }
.stats { font-size: 1.05rem; margin: .25rem 0; }
.settings { color: var(--muted); font-size: .85rem; margin: .25rem 0 1rem; }
.bar { position: sticky; top: 0; z-index: 2; background: var(--panel); border: 1px solid var(--line); border-radius: 6px;
       padding: .5rem .75rem; display: flex; flex-wrap: wrap; gap: .5rem 1.5rem; align-items: center; font-size: .9rem; }
.bar label { cursor: pointer; }
#disCount { color: var(--muted); margin-left: auto; }
table { width: 100%; border-collapse: collapse; margin: 1rem 0 0; font-size: .9rem; }
th { text-align: left; font-weight: 600; color: var(--muted); border-bottom: 1px solid var(--line); padding: .35rem .4rem; }
td { border-bottom: 1px solid var(--line); padding: .35rem .4rem; vertical-align: top; }
td.num { text-align: right; white-space: nowrap; }
td.where a { color: var(--accent); white-space: nowrap; }
td.snip { color: var(--fg); }
tr.dis td { opacity: .5; }
.sw { display: inline-block; min-width: 1.8em; padding: 0 .3em; border-radius: 4px; text-align: center; font-weight: 600; cursor: pointer; }
button.dismiss { font: inherit; font-size: .8rem; padding: .15rem .6rem; border: 1px solid var(--line); border-radius: 4px;
                 background: var(--bg); color: var(--fg); cursor: pointer; }
button.dismiss:hover { border-color: var(--muted); }
#text { font: 17px/1.65 Georgia, "Times New Roman", serif; margin-top: 2rem; }
#text p { margin: 0 0 .9em; }
mark.rp { color: inherit; border-radius: 3px; padding: 0 1px; cursor: pointer; }
mark.rp.dis { background: none; cursor: default; }
mark.rp.dis sup.tag { display: none; }
mark.rp.flash { outline: 3px solid var(--accent); outline-offset: 1px; }
sup.tag { font: 600 .65rem "Segoe UI", system-ui, sans-serif; color: var(--muted); margin-right: .2em; }
.fl { font-style: italic; text-decoration: underline dotted; }
.c0 { background: var(--c0); } .c1 { background: var(--c1); } .c2 { background: var(--c2); } .c3 { background: var(--c3); }
.c4 { background: var(--c4); } .c5 { background: var(--c5); } .c6 { background: var(--c6); } .c7 { background: var(--c7); }
.none { color: var(--muted); font-style: italic; }
@media print { .bar, button.dismiss { display: none; } mark.rp.dis { background: none; } }
)CSS";

static const wchar_t* REPORT_SCRIPT = LR"JS(
(function () {
  var storeKey = 'WRepeatfind|' + document.body.getAttribute('data-work');
  var dismissed = {};
  try { (JSON.parse(localStorage.getItem(storeKey) || '[]')).forEach(function (k) { dismissed[k] = true; }); } catch (e) {}
  function save() { try { localStorage.setItem(storeKey, JSON.stringify(Object.keys(dismissed))); } catch (e) {} }

  var rows = Array.prototype.slice.call(document.querySelectorAll('#summary tbody tr'));
  var keyOf = {};
  rows.forEach(function (r) { keyOf[r.getAttribute('data-g')] = r.getAttribute('data-key'); });
  var marks = Array.prototype.slice.call(document.querySelectorAll('mark.rp'));
  var onlyRep = document.getElementById('onlyRep');
  var showDis = document.getElementById('showDis');

  function apply() {
    var count = 0;
    rows.forEach(function (r) {
      var d = !!dismissed[r.getAttribute('data-key')];
      if (d) count++;
      r.classList.toggle('dis', d);
      r.hidden = d && !showDis.checked;
      var b = r.querySelector('button.dismiss');
      if (b) b.textContent = d ? 'Restore' : 'Dismiss';
    });
    marks.forEach(function (m) { m.classList.toggle('dis', !!dismissed[keyOf[m.getAttribute('data-g')]]); });
    document.getElementById('disCount').textContent = count ? count + ' dismissed' : '';
    document.querySelectorAll('#text p').forEach(function (p) {
      p.hidden = onlyRep.checked && !p.querySelector('mark.rp:not(.dis)');
    });
  }

  function flash(g, c) {
    var parts = document.querySelectorAll('mark.rp[data-g="' + g + '"][data-c="' + c + '"]');
    parts.forEach(function (m) { m.classList.add('flash'); });
    setTimeout(function () { parts.forEach(function (m) { m.classList.remove('flash'); }); }, 1600);
  }

  function go(g, c) {
    var el = document.getElementById('g' + g + 'c' + c);
    if (!el) return;
    var p = el.closest('p');
    if (p && p.hidden) p.hidden = false;
    el.scrollIntoView({ block: 'center', behavior: 'smooth' });
    flash(g, c);
    if (history.replaceState) history.replaceState(null, '', '#g' + g + 'c' + c);
  }

  document.addEventListener('click', function (e) {
    var b = e.target.closest('button.dismiss');
    if (b) {
      var key = b.closest('tr').getAttribute('data-key');
      if (dismissed[key]) delete dismissed[key]; else dismissed[key] = true;
      save(); apply();
      return;
    }
    var link = e.target.closest('a[data-g]');
    if (link) { e.preventDefault(); go(link.getAttribute('data-g'), link.getAttribute('data-c')); return; }
    var sw = e.target.closest('.sw');
    if (sw) { go(sw.getAttribute('data-g'), 0); return; }
    var m = e.target.closest('mark.rp');
    if (m && !m.classList.contains('dis')) {
      var k = parseInt(m.getAttribute('data-k'), 10);
      go(m.getAttribute('data-g'), (parseInt(m.getAttribute('data-c'), 10) + 1) % k);
    }
  });
  onlyRep.addEventListener('change', apply);
  showDis.addEventListener('change', apply);
  apply();

  var h = /^#g(\d+)c(\d+)$/.exec(location.hash);
  if (h) setTimeout(function () { go(h[1], h[2]); }, 50);
})();
)JS";

int CRepeatFinder::WriteReport(const std::wstring& reportPath, const std::wstring& softwareName) const
{
	std::wstring out;
	out.reserve(m_Tokens.size() * 12 + 65536);

	std::wstring work = WorkTitle();
	std::wstring workKey;
	for(const RepeatDocument& d : m_Docs) workKey += d.Title + L"|";

	SYSTEMTIME now;
	GetLocalTime(&now);
	wchar_t date[64];
	GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_LONGDATE, &now, nullptr, date, 64, nullptr);

	int repeated = RepeatedWords();
	int total = WordsTotal();
	wchar_t percent[32];
	swprintf_s(percent, L"%.1f", total ? 100.0 * repeated / total : 0.0);

	out += L"<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n";
	out += L"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
	out += L"<title>Repeats in " + Escaped(work) + L"</title>\n<style>" + REPORT_STYLE + L"</style>\n</head>\n";
	out += L"<body data-work=\"" + Escaped(workKey) + L"\">\n<div class=\"wrap\">\n";

	out += L"<h1>Repeated phrases in " + Escaped(work) + L"</h1>\n<p class=\"stats\">";
	if(m_Groups.empty()) out += L"No repeated phrases of " + std::to_wstring(Settings.PhraseLength) + L" or more words were found.";
	else out += std::to_wstring(m_Groups.size()) + (m_Groups.size() == 1 ? L" repeated passage" : L" repeated passages")
		+ L" · " + std::to_wstring(repeated) + L" of " + std::to_wstring(total) + L" words (" + percent + L"%) are in repeat copies";
	out += L"</p>\n";

	out += L"<p class=\"settings\">Shortest phrase: " + std::to_wstring(Settings.PhraseLength) + L" words";
	out += L" · Up to " + std::to_wstring(Settings.MismatchTolerance) + L" differing words in a row, with at least "
		+ std::to_wstring(Settings.MismatchPercentage) + L"% matching";
	if(Settings.IgnoreCase) out += L" · Ignoring letter case";
	if(Settings.IgnorePunctuation) out += L" · Ignoring punctuation";
	else if(Settings.IgnoreOuterPunctuation) out += L" · Ignoring outer punctuation";
	if(Settings.IgnoreNumbers) out += L" · Ignoring numbers";
	if(Settings.SkipNonwords) out += L" · Skipping non-words";
	if(Settings.SkipLongWords) out += L" · Skipping words longer than " + std::to_wstring(Settings.SkipLength) + L" characters";
	if(m_Docs.size() > 1) out += Settings.OneWork ? L" · Documents checked together as one work" : L" · Documents checked separately";
	out += L"<br>Produced by " + Escaped(softwareName) + L" on " + date + L"</p>\n";

	out += L"<div class=\"bar\"><label><input type=\"checkbox\" id=\"onlyRep\"> Show only paragraphs with repeats</label>";
	out += L"<label><input type=\"checkbox\" id=\"showDis\"> Show dismissed repeats</label><span id=\"disCount\"></span></div>\n";

	// summary table
	out += L"<section id=\"summary\">";
	if(!m_Groups.empty())
	{
		out += L"<table><thead><tr><th>#</th><th>Words</th><th>Copies</th><th>Where</th><th>Begins with</th><th></th></tr></thead><tbody>\n";
		for(const RepeatGroup& g : m_Groups)
		{
			std::wstring n = std::to_wstring(g.Number);
			wchar_t key[16];
			swprintf_s(key, L"%08x", g.Key);
			out += L"<tr data-g=\"" + n + L"\" data-key=\"" + key + L"\"><td><span class=\"sw c" + std::to_wstring(g.Number % PALETTE_SIZE)
				+ L"\" data-g=\"" + n + L"\" title=\"Go to the first copy\">" + n + L"</span></td>";
			out += L"<td class=\"num\">" + std::to_wstring(g.Words) + L"</td><td class=\"num\">" + std::to_wstring(g.Copies.size()) + L"</td><td class=\"where\">";
			for(size_t c = 0; c < g.Copies.size(); c++)
			{
				if(c) out += L", ";
				out += L"<a href=\"#g" + n + L"c" + std::to_wstring(c) + L"\" data-g=\"" + n + L"\" data-c=\"" + std::to_wstring(c) + L"\">"
					+ Escaped(CopyLocation(g.Copies[c], true)) + L"</a>";
			}
			out += L"</td><td class=\"snip\">" + Escaped(CopyText(g.Copies[0], 14)) + L"</td>";
			out += L"<td><button class=\"dismiss\" title=\"Hide this repeat if it is intentional\">Dismiss</button></td></tr>\n";
		}
		out += L"</tbody></table>";
	}
	out += L"</section>\n<main id=\"text\">\n";

	// document text, with repeats highlighted
	std::vector<std::vector<bool>> anchored(m_Groups.size());
	for(size_t g = 0; g < m_Groups.size(); g++) anchored[g].assign(m_Groups[g].Copies.size(), false);

	for(const RepeatDocument& doc : m_Docs)
	{
		if(m_Docs.size() > 1) out += L"<h2 class=\"doc\">" + Escaped(doc.Title) + L"</h2>\n";

		std::vector<int> nextWord(doc.TokenEnd - doc.FirstToken + 1, -1);	// next compared word at or after each token
		for(int t = doc.TokenEnd - 1; t >= doc.FirstToken; t--)
			nextWord[t - doc.FirstToken] = (m_Tokens[t].Word >= 0) ? m_Tokens[t].Word : nextWord[t - doc.FirstToken + 1];

		int openGroup = -1, openCopy = -1;		// the highlight currently open, if any
		int prevWord = -1;
		bool inParagraph = false;
		bool pendingSpace = false;

		for(int t = doc.FirstToken; t < doc.TokenEnd; t++)
		{
			const RepeatToken& token = m_Tokens[t];
			int group = -1, copy = -1, mark = REPEAT_WORD_UNMATCHED;
			if(token.Word >= 0)
			{
				group = m_GroupOf[token.Word];
				copy = m_CopyOf[token.Word];
				mark = m_Mark[token.Word];
				prevWord = token.Word;
			}
			else													// a filtered-out word inside a copy stays inside it
			{
				int next = nextWord[t - doc.FirstToken];
				if((prevWord >= 0) && (next >= 0) && (m_GroupOf[prevWord] >= 0)
					&& (m_GroupOf[prevWord] == m_GroupOf[next]) && (m_CopyOf[prevWord] == m_CopyOf[next]))
				{
					group = m_GroupOf[prevWord];
					copy = m_CopyOf[prevWord];
					mark = REPEAT_WORD_PERFECT;
				}
			}

			if(!inParagraph)
			{
				out += L"<p>";
				inParagraph = true;
			}
			if((group != openGroup) || (copy != openCopy))
			{
				if(openGroup >= 0) out += L"</mark>";
				if(pendingSpace) out += L" ";
				pendingSpace = false;
				if(group >= 0)
				{
					const RepeatGroup& g = m_Groups[group];
					std::wstring n = std::to_wstring(g.Number);
					std::wstring c = std::to_wstring(copy);
					out += L"<mark class=\"rp c" + std::to_wstring(g.Number % PALETTE_SIZE) + L"\" data-g=\"" + n + L"\" data-c=\"" + c
						+ L"\" data-k=\"" + std::to_wstring(g.Copies.size()) + L"\"";
					if(!anchored[group][copy]) out += L" id=\"g" + n + L"c" + c + L"\"";
					out += L" title=\"Repeat " + n + L": copy " + std::to_wstring(copy + 1) + L" of " + std::to_wstring(g.Copies.size())
						+ L". Click to go to the next copy.\">";
					if(!anchored[group][copy]) out += L"<sup class=\"tag\">" + n + L"</sup>";
					anchored[group][copy] = true;
				}
				openGroup = group;
				openCopy = copy;
			}
			if(pendingSpace) out += L" ";
			pendingSpace = false;

			if(mark == REPEAT_WORD_FLAW && group >= 0) out += L"<span class=\"fl\">" + Escaped(token.Text) + L"</span>";
			else AppendEscaped(out, token.Text);

			if(token.Delimiter == DEL_TYPE_WHITE) pendingSpace = true;
			else if(token.Delimiter == DEL_TYPE_NEWLINE)
			{
				if(openGroup >= 0) out += L"</mark>";
				openGroup = openCopy = -1;
				out += L"</p>\n";
				inParagraph = false;
			}
		}
		if(openGroup >= 0) out += L"</mark>";
		if(inParagraph) out += L"</p>\n";
	}

	out += L"</main>\n</div>\n<script>";
	out += REPORT_SCRIPT;
	out += L"</script>\n</body>\n</html>\n";

	// write as UTF-8
	int bytes = WideCharToMultiByte(CP_UTF8, 0, out.c_str(), (int)out.size(), nullptr, 0, nullptr, nullptr);
	std::string utf8(bytes, '\0');
	WideCharToMultiByte(CP_UTF8, 0, out.c_str(), (int)out.size(), utf8.data(), bytes, nullptr, nullptr);

	FILE* file = nullptr;
	if((_wfopen_s(&file, reportPath.c_str(), L"wb") != 0) || (file == nullptr)) return REPEAT_ERR_CANNOT_WRITE_REPORT;
	size_t written = fwrite(utf8.data(), 1, utf8.size(), file);
	fclose(file);
	return (written == utf8.size()) ? -1 : REPEAT_ERR_CANNOT_WRITE_REPORT;
}
