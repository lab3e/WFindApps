// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Louis A. Bloomfield
// RepeatFinder.cpp : finds repeated phrases within one document or one multi-document work

#include "stdafx.h"
#include <afxinet.h>
#include <algorithm>
#include <numeric>
#include "InputDocument.h"
#include "Words.h"
#include "HeapSort.h"
#include "RepeatFinder.h"

// Function: AddDocument
// Purpose: Reads a document word by word, keeping every word as written (for the report) and hash-coding
//			the words that survive the filters (for the comparison).

int CRepeatFinder::AddDocument(const std::wstring& path)
{
	CInputDocument indoc;
	indoc.m_bBasic_Characters = Settings.BasicCharacters;
	indoc.m_fLog = nullptr;
	indoc.m_debug = false;

	int iReturn = indoc.OpenDocument(CString(path.c_str()));
	if(iReturn > -1)
	{
		indoc.CloseDocument();
		return iReturn;
	}

	RepeatDocument doc;
	doc.Path = path;
	size_t slash = path.find_last_of(L"\\/");
	doc.Title = (slash == std::wstring::npos) ? path : path.substr(slash + 1);
	doc.FirstToken = (int)m_Tokens.size();
	doc.FirstWord = (int)m_Hash.size();
	int docIndex = (int)m_Docs.size();

	wchar_t word[WORDBUFFERLENGTH];
	wchar_t filtered[WORDBUFFERLENGTH];
	int DelimiterType = DEL_TYPE_NONE;
	int paragraph = 1;

	while(DelimiterType != DEL_TYPE_EOF)
	{
		iReturn = indoc.GetWord(word, DelimiterType);
		if(iReturn > -1)
		{
			indoc.CloseDocument();
			return iReturn;
		}
		if(word[0] == 0) continue;							// only happens at the end of the document

		wcscpy_s(filtered, word);							// apply the same filters, in the same order, as WCopyfind
		if(Settings.IgnorePunctuation) WordRemovePunctuation(filtered);
		if(Settings.IgnoreOuterPunctuation) wordxouterpunct(filtered);
		if(Settings.IgnoreNumbers) WordRemoveNumbers(filtered);
		if(Settings.IgnoreCase) WordToLowerCase(filtered);
		bool skip = (filtered[0] == 0)						// a word that was nothing but punctuation or digits
			|| (Settings.SkipLongWords && ((int)wcslen(filtered) > Settings.SkipLength))
			|| (Settings.SkipNonwords && !WordCheck(filtered));

		RepeatToken token;
		token.Text = word;
		token.Delimiter = DelimiterType;
		token.Word = skip ? -1 : (int)m_Hash.size();
		if(!skip)
		{
			m_Hash.push_back(WordHash(filtered));
			m_TokenOf.push_back((int)m_Tokens.size());
			m_DocOf.push_back(docIndex);
			m_ParaOf.push_back(paragraph);
		}
		m_Tokens.push_back(std::move(token));
		if(DelimiterType == DEL_TYPE_NEWLINE) paragraph++;
	}
	indoc.CloseDocument();

	doc.TokenEnd = (int)m_Tokens.size();
	doc.WordEnd = (int)m_Hash.size();
	m_Docs.push_back(doc);
	return -1;
}

// Function: FindRepeats
// Purpose: Seeds a comparison at every pair of identical words (earlier copy, later copy), grows each seed
//			into the longest phrase it can, and keeps the phrase if it has enough perfectly matching words.
//			For each later word, every earlier copy of that word is tried and the longest phrase wins, so a
//			passage is paired with its true twin rather than with a short coincidental match.

int CRepeatFinder::FindRepeats()
{
	int words = (int)m_Hash.size();
	if(words == 0) return REPEAT_ERR_NO_WORDS;

	m_Pairs.clear();
	m_Groups.clear();
	m_Echo.assign(words, -1);
	m_Mark.assign(words, REPEAT_WORD_UNMATCHED);
	m_TempL.assign(words, REPEAT_WORD_UNMATCHED);
	m_TempR.assign(words, REPEAT_WORD_UNMATCHED);

	std::vector<unsigned long> sortedHash(m_Hash);
	std::vector<int> sortedWord(words);
	std::iota(sortedWord.begin(), sortedWord.end(), 0);
	HeapSort(sortedHash.data(), sortedWord.data(), words);		// sorts by hash, then by word number

	int start = 0;
	if(Settings.PhraseLength > 1)								// as in WCopyfind, don't seed on words of 3 or fewer letters
		while((start < words) && ((sortedHash[start] & 0xFFC00000) == 0)) start++;

	int reportStep = (std::max)(1, words / 100);
	int nextReport = 0;

	for(int a = start; a < words; )
	{
		int b = a + 1;											// [a, b) is a run of identical words, in reading order
		while((b < words) && (sortedHash[b] == sortedHash[a])) b++;

		for(int jj = a + 1; jj < b; jj++)
		{
			int q = sortedWord[jj];								// the later word
			if(!FreeR(q)) continue;								// already part of a later copy
			if(Abort && *Abort) return REPEAT_ERR_ABORT;

			Span best = {0, 0, 0, 0, 0};
			int bestP = -1;
			for(int ii = a; ii < jj; ii++)
			{
				int p = sortedWord[ii];							// an earlier copy of the same word
				if(!Settings.OneWork && (m_DocOf[p] != m_DocOf[q])) continue;
				Span span;
				Extend(p, q, span);
				if(span.Perfect > best.Perfect)
				{
					best = span;
					bestP = p;
				}
			}
			if((bestP >= 0) && (best.Perfect >= Settings.PhraseLength))
			{
				Extend(bestP, q, best);							// regrow the winner to restore its markup
				Commit(best);
			}
		}

		if(a >= nextReport)
		{
			if(Abort && *Abort) return REPEAT_ERR_ABORT;
			if(Progress) Progress((int)(100LL * a / words), L"Looking for repeated phrases");
			nextReport = a + reportStep;
		}
		a = b;
	}

	BuildGroups();
	return -1;
}

// Function: Extend
// Purpose: Grows a matching phrase outward from the seed words seedL (earlier) and seedR (later).
// Details: The logic mirrors CCompareDocuments::ComparePair: first grow over perfectly matching words, then,
//			if imperfections are allowed, continue over flaws (including a word inserted on either side) as long
//			as no more than MismatchTolerance flaws occur in a row and the phrase stays at least
//			MismatchPercentage perfect. Two constraints are new:
//			  - the earlier copy must end before the later copy begins (LastLp < FirstRp), so a phrase can't
//			    match an overlapping version of itself;
//			  - a later-copy word must not already be the later copy of another passage (FreeR).
//			Neither copy crosses a document boundary.

bool CRepeatFinder::Extend(int seedL, int seedR, Span& span)
{
	const RepeatDocument& docL = m_Docs[m_DocOf[seedL]];
	const RepeatDocument& docR = m_Docs[m_DocOf[seedR]];
	const int loL = docL.FirstWord, hiL = docL.WordEnd;
	const int loR = docR.FirstWord, hiR = docR.WordEnd;
	const int tolerance = Settings.MismatchTolerance;
	const int percentage = Settings.MismatchPercentage;
	const unsigned long* h = m_Hash.data();
	int* tL = m_TempL.data();
	int* tR = m_TempR.data();

	tL[seedL] = REPEAT_WORD_PERFECT;
	tR[seedR] = REPEAT_WORD_PERFECT;
	int FirstLp = seedL, LastLp = seedL, FirstRp = seedR, LastRp = seedR;	// current extent of each copy
	int L, R;

	// grow backward over perfect matches
	for(L = seedL - 1, R = seedR - 1; (L >= loL) && (R >= loR) && (R > LastLp) && FreeR(R) && (h[L] == h[R]); L--, R--)
	{
		tL[L] = tR[R] = REPEAT_WORD_PERFECT;
		FirstLp = L;
		FirstRp = R;
	}

	// grow forward over perfect matches
	for(L = seedL + 1, R = seedR + 1; (L < hiL) && (R < hiR) && (L < FirstRp) && FreeR(R) && (h[L] == h[R]); L++, R++)
	{
		tL[L] = tR[R] = REPEAT_WORD_PERFECT;
		LastLp = L;
		LastRp = R;
	}

	int perfect = LastLp - FirstLp + 1;

	if(tolerance > 0)
	{
		const int FirstLx = FirstLp, FirstRx = FirstRp, LastLx = LastLp, LastRx = LastRp;	// the perfect core
		int flaws = 0;

		// grow backward, bridging flaws
		L = FirstLp - 1;
		R = FirstRp - 1;
		while((L >= loL) && (R >= loR))
		{
			if((R <= LastLp) || !FreeR(R)) break;
			if(h[L] == h[R])
			{
				perfect++;
				flaws = 0;
				tL[L] = tR[R] = REPEAT_WORD_PERFECT;
				FirstLp = L; FirstRp = R;
				L--; R--;
				continue;
			}

			if(++flaws > tolerance) break;

			if((L - 1 >= loL) && (h[L - 1] == h[R]))			// an extra word in the earlier copy
			{
				if(PercentMatching(L - 1, R, LastLx, LastRx, perfect + 1) < percentage) break;
				tL[L] = REPEAT_WORD_FLAW;
				L--;
				perfect++;
				flaws = 0;
				tL[L] = tR[R] = REPEAT_WORD_PERFECT;
				FirstLp = L; FirstRp = R;
				L--; R--;
				continue;
			}

			if((R - 1 >= loR) && (R - 1 > LastLp) && FreeR(R - 1) && (h[L] == h[R - 1]))	// an extra word in the later copy
			{
				if(PercentMatching(L, R - 1, LastLx, LastRx, perfect + 1) < percentage) break;
				tR[R] = REPEAT_WORD_FLAW;
				R--;
				perfect++;
				flaws = 0;
				tL[L] = tR[R] = REPEAT_WORD_PERFECT;
				FirstLp = L; FirstRp = R;
				L--; R--;
				continue;
			}

			if(PercentMatching(L - 1, R - 1, LastLx, LastRx, perfect) < percentage) break;	// a changed word
			tL[L] = tR[R] = REPEAT_WORD_FLAW;
			L--; R--;
		}

		// grow forward, bridging flaws
		flaws = 0;
		L = LastLp + 1;
		R = LastRp + 1;
		while((L < hiL) && (R < hiR))
		{
			if((L >= FirstRp) || !FreeR(R)) break;
			if(h[L] == h[R])
			{
				perfect++;
				flaws = 0;
				tL[L] = tR[R] = REPEAT_WORD_PERFECT;
				LastLp = L; LastRp = R;
				L++; R++;
				continue;
			}

			if(++flaws > tolerance) break;

			if((L + 1 < hiL) && (L + 1 < FirstRp) && (h[L + 1] == h[R]))	// an extra word in the earlier copy
			{
				if(PercentMatching(FirstLx, FirstRx, L + 1, R, perfect + 1) < percentage) break;
				tL[L] = REPEAT_WORD_FLAW;
				L++;
				perfect++;
				flaws = 0;
				tL[L] = tR[R] = REPEAT_WORD_PERFECT;
				LastLp = L; LastRp = R;
				L++; R++;
				continue;
			}

			if((R + 1 < hiR) && FreeR(R + 1) && (h[L] == h[R + 1]))	// an extra word in the later copy
			{
				if(PercentMatching(FirstLx, FirstRx, L, R + 1, perfect + 1) < percentage) break;
				tR[R] = REPEAT_WORD_FLAW;
				R++;
				perfect++;
				flaws = 0;
				tL[L] = tR[R] = REPEAT_WORD_PERFECT;
				LastLp = L; LastRp = R;
				L++; R++;
				continue;
			}

			if(PercentMatching(FirstLx, FirstRx, L + 1, R + 1, perfect) < percentage) break;	// a changed word
			tL[L] = tR[R] = REPEAT_WORD_FLAW;
			L++; R++;
		}
	}

	span.FirstL = FirstLp;
	span.LastL = LastLp;
	span.FirstR = FirstRp;
	span.LastR = LastRp;
	span.Perfect = perfect;
	return perfect >= Settings.PhraseLength;
}

// Function: Commit
// Purpose: Records a pair of copies. The later copy's words are claimed so they can't be the later copy of
//			anything else; the earlier copy's words stay available, so one passage can be the source of many.

void CRepeatFinder::Commit(const Span& span)
{
	int pair = (int)m_Pairs.size();
	m_Pairs.push_back(span);

	auto combine = [](int existing, int incoming)
	{
		if((existing == REPEAT_WORD_PERFECT) || (incoming == REPEAT_WORD_PERFECT)) return REPEAT_WORD_PERFECT;
		return REPEAT_WORD_FLAW;
	};
	for(int w = span.FirstL; w <= span.LastL; w++) m_Mark[w] = combine(m_Mark[w], m_TempL[w]);
	for(int w = span.FirstR; w <= span.LastR; w++)
	{
		m_Echo[w] = pair;
		m_Mark[w] = combine(m_Mark[w], m_TempR[w]);
	}
}

// Function: BuildGroups
// Purpose: Gathers pairs whose copies share words into groups (union-find), merges overlapping copies within
//			each group, and numbers the groups in reading order.

void CRepeatFinder::BuildGroups()
{
	int pairs = (int)m_Pairs.size();
	int words = (int)m_Hash.size();

	std::vector<int> parent(pairs);
	std::iota(parent.begin(), parent.end(), 0);
	auto find = [&](int x)
	{
		while(parent[x] != x) x = parent[x] = parent[parent[x]];
		return x;
	};

	std::vector<int> owner(words, -1);
	for(int k = 0; k < pairs; k++)
	{
		const Span& s = m_Pairs[k];
		for(int side = 0; side < 2; side++)
		{
			int first = side ? s.FirstR : s.FirstL;
			int last = side ? s.LastR : s.LastL;
			for(int w = first; w <= last; w++)
			{
				if(owner[w] < 0) owner[w] = k;
				else parent[find(owner[w])] = find(k);
			}
		}
	}

	std::vector<std::vector<RepeatCopy>> spans(pairs);
	for(int k = 0; k < pairs; k++)
	{
		int root = find(k);
		spans[root].push_back({m_Pairs[k].FirstL, m_Pairs[k].LastL});
		spans[root].push_back({m_Pairs[k].FirstR, m_Pairs[k].LastR});
	}

	for(int root = 0; root < pairs; root++)
	{
		std::vector<RepeatCopy>& list = spans[root];
		if(list.empty()) continue;
		std::sort(list.begin(), list.end(), [](const RepeatCopy& x, const RepeatCopy& y) { return x.FirstWord < y.FirstWord; });

		RepeatGroup group;
		for(const RepeatCopy& c : list)
		{
			if(!group.Copies.empty() && (c.FirstWord <= group.Copies.back().LastWord))
				group.Copies.back().LastWord = (std::max)(group.Copies.back().LastWord, c.LastWord);
			else group.Copies.push_back(c);
		}
		if(group.Copies.size() < 2)								// copies chained into one block (text repeated back to back);
			group.Copies = {{m_Pairs[root].FirstL, m_Pairs[root].LastL}, {m_Pairs[root].FirstR, m_Pairs[root].LastR}};

		group.Words = 0;
		for(const RepeatCopy& c : group.Copies) group.Words = (std::max)(group.Words, c.LastWord - c.FirstWord + 1);

		group.Key = 2166136261u;								// FNV-1a over the first copy's word hashes
		for(int w = group.Copies[0].FirstWord; w <= group.Copies[0].LastWord; w++)
			group.Key = (group.Key ^ (unsigned int)m_Hash[w]) * 16777619u;

		m_Groups.push_back(std::move(group));
	}

	std::sort(m_Groups.begin(), m_Groups.end(),
		[](const RepeatGroup& x, const RepeatGroup& y) { return x.Copies[0].FirstWord < y.Copies[0].FirstWord; });

	m_GroupOf.assign(words, -1);
	m_CopyOf.assign(words, -1);
	for(int g = 0; g < (int)m_Groups.size(); g++)
	{
		m_Groups[g].Number = g + 1;
		for(int c = 0; c < (int)m_Groups[g].Copies.size(); c++)
			for(int w = m_Groups[g].Copies[c].FirstWord; w <= m_Groups[g].Copies[c].LastWord; w++)
			{
				m_GroupOf[w] = g;
				m_CopyOf[w] = c;
			}
	}
}

int CRepeatFinder::RepeatedWords() const
{
	int total = 0;
	for(const RepeatGroup& g : m_Groups)
		for(size_t c = 1; c < g.Copies.size(); c++) total += g.Copies[c].LastWord - g.Copies[c].FirstWord + 1;
	return total;
}

std::wstring CRepeatFinder::CopyLocation(const RepeatCopy& copy, bool withTitle) const
{
	std::wstring location;
	if(withTitle && (m_Docs.size() > 1)) location = m_Docs[m_DocOf[copy.FirstWord]].Title + L", ";
	location += L"¶" + std::to_wstring(m_ParaOf[copy.FirstWord]);
	return location;
}

std::wstring CRepeatFinder::CopyText(const RepeatCopy& copy, int maxWords) const
{
	std::wstring text;
	int first = m_TokenOf[copy.FirstWord];
	int last = m_TokenOf[copy.LastWord];
	for(int t = first; t <= last; t++)
	{
		if(t - first >= maxWords)
		{
			text += L"…";
			break;
		}
		if(t > first) text += L" ";
		text += m_Tokens[t].Text;
	}
	return text;
}

std::wstring CRepeatFinder::WorkTitle() const
{
	if(m_Docs.empty()) return L"";
	if(m_Docs.size() == 1) return m_Docs[0].Title;
	return m_Docs[0].Title + L" and " + std::to_wstring(m_Docs.size() - 1) + (m_Docs.size() == 2 ? L" other document" : L" other documents");
}

std::wstring CRepeatFinder::ErrorMessage(int code)
{
	switch(code)
	{
	case REPEAT_ERR_ABORT: return L"Stopped.";
	case REPEAT_ERR_NO_WORDS: return L"The documents contain no words to compare.";
	case REPEAT_ERR_CANNOT_WRITE_REPORT: return L"The report could not be written. Check that the report folder exists and that you can save files there.";
	case ERR_CANNOT_ACCESS_URL: return L"The web address could not be reached.";
	case ERR_CANNOT_FIND_FILE: return L"The file could not be found.";
	case ERR_CANNOT_FIND_FILE_EXTENSION: return L"The file has no extension, so its type can't be determined.";
	case ERR_BAD_DOCX_FILE: return L"This .docx file can't be read.";
	case ERR_BAD_PDF_FILE: return L"This .pdf file can't be read. (PDF reading needs pdftotext.exe in the same folder as WRepeatfind.exe.)";
	case ERR_CANNOT_FIND_URL_LINK: return L"The web link could not be found.";
	case ERR_CANNOT_OPEN_INPUT_FILE: return L"The file can't be opened. It may be damaged, or another program may have it open.";
	default: return L"Error " + std::to_wstring(code) + L" occurred.";
	}
}
