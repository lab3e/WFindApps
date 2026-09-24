// RepeatFinder.h : the engine that finds repeated phrases within one document or one multi-document work
//
// The approach follows WCopyfind's CCompareDocuments: every word is hash-coded, the hash codes are sorted
// so identical words sit together, and each pair of identical words is used as a seed from which a matching
// phrase is grown forward and backward (tolerating small imperfections). Because both copies of a repeated
// phrase live in the same word list, the growth is constrained so that the earlier copy never runs into the
// later copy, and a word can be claimed as the later copy ("echo") of only one earlier passage. Pairs of
// copies that share words are then gathered into repeat groups, so a passage that appears three times is
// reported once, as a group with three copies.

#pragma once
#include <vector>
#include <string>
#include <atomic>
#include <functional>

#define REPEAT_ERR_ABORT 0
#define REPEAT_ERR_NO_WORDS 1
#define REPEAT_ERR_CANNOT_WRITE_REPORT 2

#define REPEAT_WORD_UNMATCHED -1
#define REPEAT_WORD_PERFECT 0
#define REPEAT_WORD_FLAW 1

struct RepeatSettings
{
	int PhraseLength = 6;				// shortest phrase (in perfectly matching words) to report
	int MismatchTolerance = 2;			// most consecutive imperfections to bridge
	int MismatchPercentage = 80;		// minimum percentage of perfectly matching words in an imperfect phrase
	bool IgnoreCase = true;
	bool IgnorePunctuation = true;
	bool IgnoreOuterPunctuation = false;
	bool IgnoreNumbers = false;
	bool SkipLongWords = false;
	int SkipLength = 20;
	bool SkipNonwords = false;
	bool BasicCharacters = false;
	bool OneWork = true;				// true: documents are parts of one work, so repeats may span documents
};

struct RepeatToken							// one word of the original text, as read from the document
{
	std::wstring Text;						// the word exactly as it appears
	int Delimiter;							// DEL_TYPE_WHITE, DEL_TYPE_NEWLINE, or DEL_TYPE_EOF after the word
	int Word;								// index into the compared word list, or -1 if filtered out
};

struct RepeatDocument
{
	std::wstring Path;
	std::wstring Title;						// file name without folder
	int FirstToken = 0, TokenEnd = 0;		// range of tokens [FirstToken, TokenEnd)
	int FirstWord = 0, WordEnd = 0;			// range of compared words [FirstWord, WordEnd)
};

struct RepeatCopy							// one appearance of a repeated passage
{
	int FirstWord, LastWord;				// inclusive range of compared words
};

struct RepeatGroup							// a passage and all of its copies
{
	int Number;								// 1-based, in reading order of the first copy
	std::vector<RepeatCopy> Copies;			// in reading order
	int Words;								// length of the longest copy, in words
	unsigned int Key;						// stable identifier derived from the first copy's words
};

class CRepeatFinder
{
public:
	RepeatSettings Settings;
	std::atomic<bool>* Abort = nullptr;
	std::function<void(int percent, const std::wstring& status)> Progress;

	int AddDocument(const std::wstring& path);	// returns -1 on success, else an error code
	int FindRepeats();							// returns -1 on success, else an error code
	int WriteReport(const std::wstring& reportPath, const std::wstring& softwareName) const;

	const std::vector<RepeatDocument>& Documents() const { return m_Docs; }
	const std::vector<RepeatGroup>& Groups() const { return m_Groups; }
	int WordsTotal() const { return (int)m_Hash.size(); }
	int RepeatedWords() const;					// words that belong to a later copy of some passage
	std::wstring CopyLocation(const RepeatCopy& copy, bool withTitle) const;
	std::wstring CopyText(const RepeatCopy& copy, int maxWords) const;
	std::wstring WorkTitle() const;

	static std::wstring ErrorMessage(int code);

private:
	struct Span { int FirstL, LastL, FirstR, LastR, Perfect; };

	bool Extend(int seedL, int seedR, Span& span);
	void Commit(const Span& span);
	void BuildGroups();
	bool FreeR(int w) const { return m_Echo[w] < 0; }
	static int PercentMatching(int firstL, int firstR, int lastL, int lastR, int perfect)
		{ return (200 * perfect) / (lastL - firstL + lastR - firstR + 2); }

	std::vector<RepeatDocument> m_Docs;
	std::vector<RepeatToken> m_Tokens;
	std::vector<unsigned long> m_Hash;		// hash code of each compared word
	std::vector<int> m_TokenOf;				// token index of each compared word
	std::vector<int> m_DocOf;				// document index of each compared word
	std::vector<int> m_ParaOf;				// paragraph number (1-based, within its document) of each compared word

	std::vector<Span> m_Pairs;				// committed pairs of copies
	std::vector<int> m_Echo;				// pair that claimed this word as its later copy, or -1
	std::vector<int> m_Mark;				// REPEAT_WORD_* for each compared word
	std::vector<int> m_TempL, m_TempR;		// markup while a phrase is being grown
	std::vector<RepeatGroup> m_Groups;
	std::vector<int> m_GroupOf;				// index into m_Groups for each word, or -1
	std::vector<int> m_CopyOf;				// index into that group's Copies for each word
};
