#include "..\stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <afxinet.h>
#include <locale.h>
#include <vector>
#include <string>
#include "HeapSort.h"
#include "InputDocument.h"
#include "Words.h"
#include "CompareDocuments.h"

#define _UNICODE 1

	CCompareDocuments::CCompareDocuments(int ndocuments)	// a constructor method
	{								
		m_Documents = ndocuments;
		if((m_pDocs = new Document[m_Documents]) == NULL) return; // allocate the documents
		for(int i=0; i<m_Documents; i++)
		{
			(m_pDocs+i)->m_DocumentType=DOC_TYPE_UNDEFINED;
			(m_pDocs+i)->m_pWordHash=NULL;
			(m_pDocs+i)->m_pSortedWordHash=NULL;
			(m_pDocs+i)->m_pSortedWordNumber=NULL;
			(m_pDocs+i)->m_WordsTotal=0;
			(m_pDocs+i)->m_FirstHash=0;
			(m_pDocs+i)->m_bLoaded=false;
		}

		m_fMatch=NULL;
		m_fLog=NULL;
		m_Anchors=0;
		m_debug=false;

		m_WordsAllocated=0;

		m_pQWordHash=NULL;
		m_pXWordHash=NULL;

		m_Compares=0;												// zero the count of comparisons completed
		m_MatchingDocumentPairs=0;									// zero the count of matching document pairs

		m_PhraseLength = 6;
		m_FilterPhraseLength = 6;
		m_WordThreshold = 100;
		m_SkipLength = 20;
		m_MismatchTolerance = 2;
		m_MismatchPercentage = 80;

		m_bBriefReport = false;
		m_bIgnoreCase = false;
		m_bIgnoreNumbers = false;
		m_bIgnoreOuterPunctuation = false;
		m_bIgnorePunctuation = false;
		m_bSkipLongWords = false;
		m_bSkipNonwords = false;
		m_bBasic_Characters = false;
		_wsetlocale(LC_ALL, L"English");

		m_szSoftwareName=L"";

	}

	CCompareDocuments::~CCompareDocuments()							// a destructor method
	{
		if(m_pDocs != NULL)
		{
			for(int i=0; i<m_Documents; i++)
			{
				if((m_pDocs+i)->m_pWordHash != NULL) {delete [] (m_pDocs+i)->m_pWordHash; (m_pDocs+i)->m_pWordHash=NULL;}	// if allocated, delete hash-coded word list
				if((m_pDocs+i)->m_pSortedWordHash != NULL) {delete [] (m_pDocs+i)->m_pSortedWordHash; (m_pDocs+i)->m_pSortedWordHash=NULL;}	// if allocated, delete sorted hash-coded word list
				if((m_pDocs+i)->m_pSortedWordNumber != NULL) {delete [] (m_pDocs+i)->m_pSortedWordNumber; (m_pDocs+i)->m_pSortedWordNumber=NULL;}	// if allocated, delete sorted word number list
			}
			delete[] m_pDocs; m_pDocs=NULL;
		}

		if(m_fMatch != NULL) {fclose(m_fMatch); m_fMatch=NULL;}			// close matches file
		if(m_fLog != NULL)  {fclose(m_fLog); m_fLog=NULL;}	// close log file

		if(m_pQWordHash != NULL) {delete [] m_pQWordHash; m_pQWordHash=NULL;}	// if allocated, delete temporary hash-coded word list
		if(m_pXWordHash != NULL) {delete [] m_pXWordHash; m_pXWordHash=NULL;}	// if allocated, delete temporary hash-coded word list
	}

int CCompareDocuments::LoadDocument(Document *inDocument)
{
	int	Words,WordNumber;								// total number of words in document and running count
	wchar_t Word[256];									// container for the current word
	int DelimiterType;									// type of word-ending delimiter
	int Wordinc=10000;									// step size is 10000 words when allocating more word entries
	int i;
	int iReturn;

	CInputDocument indoc;								// CInputDocument class to handle inputting the document
	indoc.m_bBasic_Characters = m_bBasic_Characters;	// inform the input document about whether we're using Basic Characters only
	indoc.m_fLog = m_fLog;								// inform the input document about where the log file is
	indoc.m_debug = m_debug;							// inform the input document about the debug choice

	fwprintf(m_fLog,L"Loading %s\n",(LPCWSTR)inDocument->m_szDocumentName);
	fflush(m_fLog);

	iReturn = indoc.OpenDocument(inDocument->m_szDocumentName); // open the next document for word input
	if(iReturn > -1)
	{
		indoc.CloseDocument();							// close document
		return iReturn;
	}
		
	WordNumber=0;										// set count of words in document to zero
	DelimiterType=DEL_TYPE_NONE;

	while( DelimiterType != DEL_TYPE_EOF )				// loop until an eof
	{
		indoc.GetWord(Word,DelimiterType);				// get the next word
		if(m_bIgnorePunctuation) WordRemovePunctuation(Word);	// if ignore punctuation is active, remove punctuation
		if(m_bIgnoreOuterPunctuation) wordxouterpunct(Word);	// if ignore outer punctuation is active, remove outer punctuation
		if(m_bIgnoreNumbers) WordRemoveNumbers(Word);			// if ignore numbers is active, remove numbers
		if(m_bIgnoreCase) WordToLowerCase(Word);				// if ignore case is active, remove case
		if(m_bSkipLongWords && (wcslen(Word) > m_SkipLength) ) continue;	// if skip too-long words is active, skip them
		if(m_bSkipNonwords && (!WordCheck(Word)) ) continue;		// if skip nonwords is active, skip them

		if (WordNumber==m_WordsAllocated)							// if hash-coded word entries are full (or don't exist)
		{
			m_WordsAllocated += Wordinc;							// increase maximum number of words
			if(m_pXWordHash != NULL) delete [] m_pXWordHash;		// if allocated, delete temporary hash-coded word list
			m_pXWordHash = new unsigned long[m_WordsAllocated];		// allocate new, larger array of entries
			if( m_pXWordHash == NULL ) return ERR_CANNOT_ALLOCATE_WORKING_HASH_ARRAY;

			if(m_pQWordHash != NULL)
			{
				for(i=0;i<WordNumber;i++) m_pXWordHash[i]=m_pQWordHash[i];	// copy hash-coded word entries to new array
				delete [] m_pQWordHash;	// if allocated, delete temporary hash-coded word list
			}

			m_pQWordHash=m_pXWordHash;							// set normal pointer to new, larger array
			m_pXWordHash=NULL;									// null out temporary pointer
		}
					
		m_pQWordHash[WordNumber]=WordHash(Word);					// hash-code the word and save that hash
		WordNumber++;											// increment count of words
	}

	Words=WordNumber;											// save number of words
	inDocument->m_WordsTotal=Words;							// save number of words in document entry

	inDocument->m_pWordHash =	new unsigned long[Words];		// allocate array for hash-coded words in doc entry
	if( inDocument->m_pWordHash == NULL ) return ERR_CANNOT_ALLOCATE_HASH_ARRAY;
	inDocument->m_pSortedWordHash = new unsigned long[Words];	// allocate array for sorted hash-coded words
	if( inDocument->m_pSortedWordHash == NULL ) return ERR_CANNOT_ALLOCATE_SORTED_HASH_ARRAY;
	inDocument->m_pSortedWordNumber =	new int[Words];			// allocate array for sorted word numbers
	if( inDocument->m_pSortedWordNumber == NULL ) return ERR_CANNOT_ALLOCATE_SORTED_NUMBER_ARRAY;
		
	for (i=0;i<Words;i++)			// loop for all the words in the document
	{
		inDocument->m_pWordHash[i]=m_pQWordHash[i];				// copy over hash-coded words
		inDocument->m_pSortedWordNumber[i]=i;					// copy over word numbers
		inDocument->m_pSortedWordHash[i]=m_pQWordHash[i];		// copy over hash-coded words
	}

	HeapSort(inDocument->m_pSortedWordHash,				// sort hash-coded words (and word numbers)
		inDocument->m_pSortedWordNumber,Words);

	if(m_PhraseLength == 1)	inDocument->m_FirstHash = 0;		// if phraselength is 1 word, compare even the shortest words
	else														// if phrase length is > 1 word, start at first word with more than 3 chars
	{															
		int FirstLong=0;
		for (i=0;i<Words;i++)									// loop for all the words in the document
		{
			if( (inDocument->m_pSortedWordHash[i] & 0xFFC00000) != 0 )	// if the word is longer than 3 letters, break
				{
					FirstLong=i;
					break;
				}
		}
		inDocument->m_FirstHash = FirstLong;					// save the number of the first >3 letter word, or the first word
	}

	indoc.CloseDocument();										// close this document
	inDocument->m_bLoaded = true;
	return -1;
}

int CCompareDocuments::ComparePair(Document *DocL,Document *DocR)
{
	int WordNumberL,WordNumberR;						// word number for left document and right document
	int WordNumberRedundantL,WordNumberRedundantR;		// word number of end of redundant words
	int iWordNumberL,iWordNumberR;						// word number counter, for loops
	int FirstL,FirstR;									// first matching word in left document and right document
	int LastL,LastR;									// last matching word in left document and right document
	int FirstLp,FirstRp;								// first perfectly matching word in left document and right document
	int LastLp,LastRp;									// last perfectlymatching word in left document and right document
	int FirstLx,FirstRx;								// first original perfectly matching word in left document and right document
	int LastLx,LastRx;									// last original perfectlymatching word in left document and right document
	int Flaws;											// flaw count
	unsigned long Hash;
	int MatchingWordsPerfect;							// count of perfect matches within a single phrase
	int Anchor;											// number of current match anchor
	int i;

	m_MatchingWordsPerfect=0;
	m_MatchingWordsTotalL=0;
	m_MatchingWordsTotalR=0;

	for(WordNumberL=0;WordNumberL<DocL->m_WordsTotal;WordNumberL++)	// loop for all left words
	{
		m_MatchMarkL[WordNumberL]=WORD_UNMATCHED;		// set the left match markers to "WORD_UNMATCHED"
		m_MatchAnchorL[WordNumberL]=0;					// zero the left match anchors
	}
	for(WordNumberR=0;WordNumberR<DocR->m_WordsTotal;WordNumberR++)	// loop for all right words
	{
		m_MatchMarkR[WordNumberR]=WORD_UNMATCHED;		// set the right match markers to "WORD_UNMATCHED"
		m_MatchAnchorR[WordNumberR]=0;					// zero the right match anchors
	}

	WordNumberL=DocL->m_FirstHash;						// start left at first >3 letter word
	WordNumberR=DocR->m_FirstHash;						// start right at first >3 letter word

	Anchor=0;											// start with no html anchors assigned
						
	while ( (WordNumberL < DocL->m_WordsTotal)			// loop while there are still words to check
			&& (WordNumberR < DocR->m_WordsTotal) )
	{
		// if the next word in the left sorted hash-coded list has been matched
		if( m_MatchMarkL[DocL->m_pSortedWordNumber[WordNumberL]] != WORD_UNMATCHED )
		{
			WordNumberL++;								// advance to next left sorted hash-coded word
			continue;
		}

		// if the next word in the right sorted hash-coded list has been matched
		if( m_MatchMarkR[DocR->m_pSortedWordNumber[WordNumberR]] != WORD_UNMATCHED )
		{
			WordNumberR++;								// skip to next right sorted hash-coded word
			continue;
		}

		// check for left word less than right word
		if( DocL->m_pSortedWordHash[WordNumberL] < DocR->m_pSortedWordHash[WordNumberR] )
		{
			WordNumberL++;								// advance to next left word
			if ( WordNumberL >= DocL->m_WordsTotal) break;
			continue;									// and resume looping
		}

		// check for right word less than left word
		if( DocL->m_pSortedWordHash[WordNumberL] > DocR->m_pSortedWordHash[WordNumberR] )
		{
			WordNumberR++;								// advance to next right word
			if ( WordNumberR >= DocR->m_WordsTotal) break;
			continue;									// and resume looping
		}

		// we have a match, so check redundancy of this words and compare all copies of this word
		Hash=DocL->m_pSortedWordHash[WordNumberL];
		WordNumberRedundantL=WordNumberL;
		WordNumberRedundantR=WordNumberR;
		while(WordNumberRedundantL < (DocL->m_WordsTotal - 1))
		{
			if( DocL->m_pSortedWordHash[WordNumberRedundantL + 1] == Hash ) WordNumberRedundantL++;
			else break;
		}
		while(WordNumberRedundantR < (DocR->m_WordsTotal - 1))
		{
			if( DocR->m_pSortedWordHash[WordNumberRedundantR + 1] == Hash ) WordNumberRedundantR++;
			else break;
		}
		for(iWordNumberL=WordNumberL;iWordNumberL<=WordNumberRedundantL;iWordNumberL++)	// loop for each copy of this word on the left
		{
			if( m_MatchMarkL[DocL->m_pSortedWordNumber[iWordNumberL]] != WORD_UNMATCHED ) continue;	// skip words that have been matched already
			for(iWordNumberR=WordNumberR;iWordNumberR<=WordNumberRedundantR;iWordNumberR++)	// loop for each copy of this word on the right
			{
				if( m_MatchMarkR[DocR->m_pSortedWordNumber[iWordNumberR]] != WORD_UNMATCHED ) continue;	// skip words that have been matched already

				// look up and down the hash-coded (not sorted) lists for matches
				m_MatchMarkTempL[DocL->m_pSortedWordNumber[iWordNumberL]]=WORD_PERFECT;	// markup word in temporary list at perfection quality
				m_MatchMarkTempR[DocR->m_pSortedWordNumber[iWordNumberR]]=WORD_PERFECT;	// markup word in temporary list at perfection quality

				FirstL=DocL->m_pSortedWordNumber[iWordNumberL]-1;	// start left just before current word
				LastL=DocL->m_pSortedWordNumber[iWordNumberL]+1;	// end left just after current word
				FirstR=DocR->m_pSortedWordNumber[iWordNumberR]-1;	// start right just before current word
				LastR=DocR->m_pSortedWordNumber[iWordNumberR]+1;	// end right just after current word

				while( (FirstL >= 0) && (FirstR >= 0) )		// if we aren't at the start of either document,
				{

					// Note: when we leave this loop, FirstL and FirstR will always point one word before the first match
					
					// make sure that left and right words haven't been used in a match before and
					// that the two words actually match. If so, move up another word and repeat the test.

					if( m_MatchMarkL[FirstL] != WORD_UNMATCHED ) break;
					if( m_MatchMarkR[FirstR] != WORD_UNMATCHED ) break;

					if( DocL->m_pWordHash[FirstL] == DocR->m_pWordHash[FirstR] )
					{
						m_MatchMarkTempL[FirstL]=WORD_PERFECT;		// markup word in temporary list
						m_MatchMarkTempR[FirstR]=WORD_PERFECT;		// markup word in temporary list
						FirstL--;									// move up on left
						FirstR--;									// move up on right
						continue;
					}
					break;
				}

				while( (LastL < DocL->m_WordsTotal) && (LastR < DocR->m_WordsTotal) ) // if we aren't at the end of either document
				{

					// Note: when we leave this loop, LastL and LastR will always point one word after last match
					
					// make sure that left and right words haven't been used in a match before and
					// that the two words actually match. If so, move up another word and repeat the test.

					if( m_MatchMarkL[LastL] != WORD_UNMATCHED ) break;
					if( m_MatchMarkR[LastR] != WORD_UNMATCHED ) break;
					if( DocL->m_pWordHash[LastL] == DocR->m_pWordHash[LastR] )
					{
						m_MatchMarkTempL[LastL]=WORD_PERFECT;	// markup word in temporary list
						m_MatchMarkTempR[LastR]=WORD_PERFECT;	// markup word in temporary list
						LastL++;								// move down on left
						LastR++;								// move down on right
						continue;
					}
					break;
				}

				FirstLp=FirstL+1;						// pointer to first perfect match left
				FirstRp=FirstR+1;						// pointer to first perfect match right
				LastLp=LastL-1;							// pointer to last perfect match left
				LastRp=LastR-1;							// pointer to last perfect match right
				MatchingWordsPerfect=LastLp-FirstLp+1;	// save number of perfect matches

				if(m_MismatchTolerance > 0)				// are we accepting imperfect matches?
				{

					FirstLx=FirstLp;					// save pointer to word before first perfect match left
					FirstRx=FirstRp;					// save pointer to word before first perfect match right
					LastLx=LastLp;						// save pointer to word after last perfect match left
					LastRx=LastRp;						// save pointer to word after last perfect match right

					Flaws=0;							// start with zero flaws
					while( (FirstL >= 0) && (FirstR >= 0) )		// if we aren't at the start of either document,
					{

						// Note: when we leave this loop, FirstL and FirstR will always point one word before the first reportable match
						
						// make sure that left and right words haven't been used in a match before and
						// that the two words actually match. If so, move up another word and repeat the test.
						if( m_MatchMarkL[FirstL] != WORD_UNMATCHED ) break;
						if( m_MatchMarkR[FirstR] != WORD_UNMATCHED ) break;
						if( DocL->m_pWordHash[FirstL] == DocR->m_pWordHash[FirstR] )
						{
							MatchingWordsPerfect++;				// increment perfect match count;
							Flaws=0;							// having just found a perfect match, we're back to perfect matching
							m_MatchMarkTempL[FirstL]=WORD_PERFECT;			// markup word in temporary list
							m_MatchMarkTempR[FirstR]=WORD_PERFECT;			// markup word in temporary list
							FirstLp=FirstL;						// save pointer to first left perfect match
							FirstRp=FirstR;						// save pointer to first right perfect match
							FirstL--;							// move up on left
							FirstR--;							// move up on right
							continue;
						}

						// we're at a flaw, so increase the flaw count
						Flaws++;
						if( Flaws > m_MismatchTolerance ) break;	// check for maximum flaws reached
						
						if( (FirstL-1) >= 0 )					// check one word earlier on left (if it exists)
						{
							if( m_MatchMarkL[FirstL-1] != WORD_UNMATCHED ) break;	// make sure we haven't already matched this word
							
							if( DocL->m_pWordHash[FirstL-1] == DocR->m_pWordHash[FirstR] )
							{
								if( PercentMatching(FirstL-1,FirstR,LastLx,LastRx,MatchingWordsPerfect+1) < m_MismatchPercentage ) break;	// are we getting too imperfect?
								m_MatchMarkTempL[FirstL]=WORD_FLAW;	// markup non-matching word in left temporary list
								FirstL--;						// move up on left to skip over the flaw
								MatchingWordsPerfect++;			// increment perfect match count;
								Flaws=0;						// having just found a perfect match, we're back to perfect matching
								m_MatchMarkTempL[FirstL]=WORD_PERFECT;		// markup word in left temporary list
								m_MatchMarkTempR[FirstR]=WORD_PERFECT;		// markup word in right temporary list
								FirstLp=FirstL;					// save pointer to first left perfect match
								FirstRp=FirstR;					// save pointer to first right perfect match
								FirstL--;						// move up on left
								FirstR--;						// move up on right
								continue;
							}
						}

						if( (FirstR-1) >= 0 )					// check one word earlier on right (if it exists)
						{
							if( m_MatchMarkR[FirstR-1] != WORD_UNMATCHED ) break;	// make sure we haven't already matched this word

							if( DocL->m_pWordHash[FirstL] == DocR->m_pWordHash[FirstR-1] )
							{
								if( PercentMatching(FirstL,FirstR-1,LastLx,LastRx,MatchingWordsPerfect+1) < m_MismatchPercentage ) break;	// are we getting too imperfect?
								m_MatchMarkTempR[FirstR]=WORD_FLAW;	// markup non-matching word in right temporary list
								FirstR--;						// move up on right to skip over the flaw
								MatchingWordsPerfect++;			// increment perfect match count;
								Flaws=0;						// having just found a perfect match, we're back to perfect matching
								m_MatchMarkTempL[FirstL]=WORD_PERFECT;		// markup word in left temporary list
								m_MatchMarkTempR[FirstR]=WORD_PERFECT;		// markup word in right temporary list
								FirstLp=FirstL;					// save pointer to first left perfect match
								FirstRp=FirstR;					// save pointer to first right perfect match
								FirstL--;						// move up on left
								FirstR--;						// move up on right
								continue;
							}
						}

						if( PercentMatching(FirstL-1,FirstR-1,LastLx,LastRx,MatchingWordsPerfect) < m_MismatchPercentage ) break;	// are we getting too imperfect?
						m_MatchMarkTempL[FirstL]=WORD_FLAW;		// markup word in left temporary list
						m_MatchMarkTempR[FirstR]=WORD_FLAW;		// markup word in right temporary list
						FirstL--;								// move up on left
						FirstR--;								// move up on right
					}
		
					Flaws=0;							// start with zero flaws
					while( (LastL < DocL->m_WordsTotal) && (LastR < DocR->m_WordsTotal) ) // if we aren't at the end of either document
					{

						// Note: when we leave this loop, LastL and LastR will always point one word after last match
						
						// make sure that left and right words haven't been used in a match before and
						// that the two words actually match. If so, move up another word and repeat the test.
						if( m_MatchMarkL[LastL] != WORD_UNMATCHED ) break;
						if( m_MatchMarkR[LastR] != WORD_UNMATCHED ) break;
						if( DocL->m_pWordHash[LastL] == DocR->m_pWordHash[LastR] )
						{
							MatchingWordsPerfect++;				// increment perfect match count;
							Flaws=0;							// having just found a perfect match, we're back to perfect matching
							m_MatchMarkTempL[LastL]=WORD_PERFECT;	// markup word in temporary list
							m_MatchMarkTempR[LastR]=WORD_PERFECT;	// markup word in temporary list
							LastLp=LastL;						// save pointer to last left perfect match
							LastRp=LastR;						// save pointer to last right perfect match
							LastL++;							// move down on left
							LastR++;							// move down on right
							continue;
						}

						Flaws++;
						if( Flaws > m_MismatchTolerance ) break;	// check for maximum flaws reached
							
						if( (LastL+1) < DocL->m_WordsTotal )		// check one word later on left (if it exists)
						{
							if( m_MatchMarkL[LastL+1] != WORD_UNMATCHED ) break;	// make sure we haven't already matched this word
							
							if( DocL->m_pWordHash[LastL+1] == DocR->m_pWordHash[LastR] )
							{
								if( PercentMatching(FirstLx,FirstRx,LastL+1,LastR,MatchingWordsPerfect+1) < m_MismatchPercentage ) break;	// are we getting too imperfect?
								m_MatchMarkTempL[LastL]=WORD_FLAW;		// markup non-matching word in left temporary list
								LastL++;						// move down on left to skip over the flaw
								MatchingWordsPerfect++;			// increment perfect match count;
								Flaws=0;						// having just found a perfect match, we're back to perfect matching
								m_MatchMarkTempL[LastL]=WORD_PERFECT;	// markup word in lefttemporary list
								m_MatchMarkTempR[LastR]=WORD_PERFECT;	// markup word in right temporary list
								LastLp=LastL;					// save pointer to last left perfect match
								LastRp=LastR;					// save pointer to last right perfect match
								LastL++;						// move down on left
								LastR++;						// move down on right
								continue;
							}
						}

						if( (LastR+1) < DocR->m_WordsTotal )	// check one word later on right (if it exists)
						{
							if( m_MatchMarkR[LastR+1] != WORD_UNMATCHED ) break;	// make sure we haven't already matched this word

							if( DocL->m_pWordHash[LastL] == DocR->m_pWordHash[LastR+1] )
							{
								if( PercentMatching(FirstLx,FirstRx,LastL,LastR+1,MatchingWordsPerfect+1) < m_MismatchPercentage ) break;	// are we getting too imperfect?
								m_MatchMarkTempR[LastR]=WORD_FLAW;		// markup non-matching word in right temporary list
								LastR++;						// move down on right to skip over the flaw
								MatchingWordsPerfect++;			// increment perfect match count;
								Flaws=0;						// having just found a perfect match, we're back to perfect matching
								m_MatchMarkTempL[LastL]=WORD_PERFECT;	// markup word in left temporary list
								m_MatchMarkTempR[LastR]=WORD_PERFECT;	// markup word in right temporary list
								LastLp=LastL;					// save pointer to last left perfect match
								LastRp=LastR;					// save pointer to last right perfect match
								LastL++;						// move down on left
								LastR++;						// move down on right
								continue;
							}
						}

						if( PercentMatching(FirstLx,FirstRx,LastL+1,LastR+1,MatchingWordsPerfect) < m_MismatchPercentage ) break;	// are we getting too imperfect?
						m_MatchMarkTempL[LastL]=WORD_FLAW;		// markup word in left temporary list
						m_MatchMarkTempR[LastR]=WORD_FLAW;		// markup word in right temporary list
						LastL++;								// move down on left
						LastR++;								// move down on right
					}
				}
				if( MatchingWordsPerfect >= m_PhraseLength )	// check that phrase has enough perfect matches in it to mark
				{
					Anchor++;									// increment anchor count
					for(i=FirstLp;i<=LastLp;i++)				// loop for all left matched words
					{
						m_MatchMarkL[i]=m_MatchMarkTempL[i];	// copy over left matching markup
						if(m_MatchMarkTempL[i]==WORD_PERFECT) m_MatchingWordsPerfect++;	// count the number of perfect matching words (same as for right document)
						m_MatchAnchorL[i]=Anchor;				// identify the anchor for this phrase
					}
					m_MatchingWordsTotalL += LastLp-FirstLp+1;	// add the number of words in the matching phrase, whether perfect or flawed matches
					for(i=FirstRp;i<=LastRp;i++)				// loop for all right matched words
					{
						m_MatchMarkR[i]=m_MatchMarkTempR[i];	// copy over right matching markup
						m_MatchAnchorR[i]=Anchor;				// identify the anchor for this phrase
					}
					m_MatchingWordsTotalR += LastRp-FirstRp+1;	// add the number of words in the matching phrase, whether perfect or flawed matches
				}
			}
		}
		WordNumberL=WordNumberRedundantL + 1;			// continue searching after the last redundant word on left
		WordNumberR=WordNumberRedundantR + 1;			// continue searching after the last redundant word on right
	}

	m_Anchors=Anchor;									// remember how many matching phrases were found
	m_Compares++;										// increment count of comparisons
	if( (m_Compares%m_CompareStep)	== 0 )				// if count is divisible by 1000,
	{
		fwprintf(m_fLog,L"Comparing Documents, %lld Completed\n",m_Compares);
		fflush(m_fLog);
	}
	return -1;
}

int CCompareDocuments::ComparePairFiltered(Document *DocL,Document *DocR,Document *DocF)
{
	int WordNumberL,WordNumberR,WordNumberF;			// word number for left document and right document
	int WordNumberRedundantL,WordNumberRedundantR;		// word number of end of redundant words
	int WordNumberRedundantF;
	int iWordNumberL,iWordNumberR,iWordNumberF;			// word number counter, for loops
	int FirstL,FirstR,FirstF;							// first matching word in left document and right document
	int LastL,LastR,LastF;								// last matching word in left document and right document
	int FirstLp,FirstRp,FirstFp;						// first perfectly matching word in left document and right document
	int LastLp,LastRp,LastFp;							// last perfectlymatching word in left document and right document
	int FirstLx,FirstRx;								// first original perfectly matching word in left document and right document
	int LastLx,LastRx;									// last original perfectlymatching word in left document and right document
	int Flaws;											// flaw count
	unsigned long Hash;
	int MatchingWordsPerfect;							// count of perfect matches within a single phrase
	int Anchor;											// number of current match anchor
	int i;

	m_MatchingWordsPerfect=0;
	m_MatchingWordsTotalL=0;
	m_MatchingWordsTotalR=0;

	for(WordNumberL=0;WordNumberL<DocL->m_WordsTotal;WordNumberL++)	// loop for all left words
	{
		m_MatchMarkL[WordNumberL]=WORD_UNMATCHED;		// set the left match markers to "WORD_UNMATCHED"
		m_MatchAnchorL[WordNumberL]=0;					// zero the left match anchors
	}
	for(WordNumberR=0;WordNumberR<DocR->m_WordsTotal;WordNumberR++)	// loop for all right words
	{
		m_MatchMarkR[WordNumberR]=WORD_UNMATCHED;		// set the right match markers to "WORD_UNMATCHED"
		m_MatchAnchorR[WordNumberR]=0;					// zero the right match anchors
	}
//
// filter left document
//
	WordNumberL=DocL->m_FirstHash;						// start left at first >3 letter word
	WordNumberF=DocF->m_FirstHash;						// start filter at first >3 letter word
						
	while ( (WordNumberL < DocL->m_WordsTotal)			// loop while there are still words to check
			&& (WordNumberF < DocF->m_WordsTotal) )
	{
		// if the next word in the left sorted hash-coded list has been matched
		if( m_MatchMarkL[DocL->m_pSortedWordNumber[WordNumberL]] != WORD_UNMATCHED )
		{
			WordNumberL++;								// advance to next left sorted hash-coded word
			continue;
		}

		// check for left word less than filter word
		if( DocL->m_pSortedWordHash[WordNumberL] < DocF->m_pSortedWordHash[WordNumberF] )
		{
			WordNumberL++;								// advance to next left word
			if ( WordNumberL >= DocL->m_WordsTotal) break;
			continue;									// and resume looping
		}

		// check for filter word less than left word
		if( DocL->m_pSortedWordHash[WordNumberL] > DocF->m_pSortedWordHash[WordNumberF] )
		{
			WordNumberF++;								// advance to next filter word
			if ( WordNumberF >= DocF->m_WordsTotal) break;
			continue;									// and resume looping
		}

		// we have a match, so check redundancy of this words and compare all copies of this word
		Hash=DocL->m_pSortedWordHash[WordNumberL];
		WordNumberRedundantL=WordNumberL;
		WordNumberRedundantF=WordNumberF;
		while(WordNumberRedundantL < (DocL->m_WordsTotal - 1))
		{
			if( DocL->m_pSortedWordHash[WordNumberRedundantL + 1] == Hash ) WordNumberRedundantL++;
			else break;
		}
		while(WordNumberRedundantF < (DocF->m_WordsTotal - 1))
		{
			if( DocF->m_pSortedWordHash[WordNumberRedundantF + 1] == Hash ) WordNumberRedundantF++;
			else break;
		}
		for(iWordNumberL=WordNumberL;iWordNumberL<=WordNumberRedundantL;iWordNumberL++)	// loop for each copy of this word on the left
		{
			if( m_MatchMarkL[DocL->m_pSortedWordNumber[iWordNumberL]] != WORD_UNMATCHED ) continue;	// skip words that have been matched already
			for(iWordNumberF=WordNumberF;iWordNumberF<=WordNumberRedundantF;iWordNumberF++)	// loop for each copy of this word on the filter
			{
				// look up and down the hash-coded (not sorted) lists for matches
				FirstL=DocL->m_pSortedWordNumber[iWordNumberL]-1;	// start left just before current word
				LastL=DocL->m_pSortedWordNumber[iWordNumberL]+1;	// end left just after current word
				FirstF=DocF->m_pSortedWordNumber[iWordNumberF]-1;	// start filter just before current word
				LastF=DocF->m_pSortedWordNumber[iWordNumberF]+1;	// end filter just after current word

				while( (FirstL >= 0) && (FirstF >= 0) )		// if we aren't at the start of either document,
				{

					// Note: when we leave this loop, FirstL and FirstF will always point one word before the first match
					
					// make sure that left word hasn't been used in a match before and
					// that the two words actually match. If so, move up another word and repeat the test.

					if( m_MatchMarkL[FirstL] != WORD_UNMATCHED ) break;

					if( DocL->m_pWordHash[FirstL] == DocF->m_pWordHash[FirstF] )
					{
						FirstL--;									// move up on left
						FirstF--;									// move up on filter
						continue;
					}
					break;
				}

				while( (LastL < DocL->m_WordsTotal) && (LastF < DocF->m_WordsTotal) ) // if we aren't at the end of either document
				{

					// Note: when we leave this loop, LastL and LastF will always point one word after last match
					
					// make sure that left word hasn't been used in a match before and
					// that the two words actually match. If so, move up another word and repeat the test.

					if( m_MatchMarkL[LastL] != WORD_UNMATCHED ) break;
					if( DocL->m_pWordHash[LastL] == DocF->m_pWordHash[LastF] )
					{
						LastL++;								// move down on left
						LastF++;								// move down on filter
						continue;
					}
					break;
				}

				FirstLp=FirstL+1;						// pointer to first perfect match left
				FirstFp=FirstF+1;						// pointer to first perfect match filter
				LastLp=LastL-1;							// pointer to last perfect match left
				LastFp=LastF-1;							// pointer to last perfect match filter
				MatchingWordsPerfect=LastLp-FirstLp+1;	// save number of perfect matches

				if( MatchingWordsPerfect >= m_FilterPhraseLength )	// check that phrase has enough perfect matches in it to mark
				{
					for(i=FirstLp;i<=LastLp;i++)				// loop for all left matched words
					{
						m_MatchMarkL[i]=WORD_FILTERED;	// mark word as filtered
					}
				}
			}
		}
		WordNumberL=WordNumberRedundantL + 1;			// continue searching after the last redundant word on left
		WordNumberF=WordNumberRedundantF + 1;			// continue searching after the last redundant word on filter
	}
//
// filter right document
//
	WordNumberR=DocR->m_FirstHash;						// start right at first >3 letter word
	WordNumberF=DocF->m_FirstHash;						// start filter at first >3 letter word
						
	while ( (WordNumberR < DocR->m_WordsTotal)			// loop while there are still words to check
			&& (WordNumberF < DocF->m_WordsTotal) )
	{
		// if the next word in the right sorted hash-coded list has been matched
		if( m_MatchMarkR[DocR->m_pSortedWordNumber[WordNumberR]] != WORD_UNMATCHED )
		{
			WordNumberR++;								// advance to next right sorted hash-coded word
			continue;
		}

		// check for right word less than filter word
		if( DocR->m_pSortedWordHash[WordNumberR] < DocF->m_pSortedWordHash[WordNumberF] )
		{
			WordNumberR++;								// advance to next right word
			if ( WordNumberR >= DocR->m_WordsTotal) break;
			continue;									// and resume looping
		}

		// check for filter word less than right word
		if( DocR->m_pSortedWordHash[WordNumberR] > DocF->m_pSortedWordHash[WordNumberF] )
		{
			WordNumberF++;								// advance to next filter word
			if ( WordNumberF >= DocF->m_WordsTotal) break;
			continue;									// and resume looping
		}

		// we have a match, so check redundancy of this words and compare all copies of this word
		Hash=DocR->m_pSortedWordHash[WordNumberR];
		WordNumberRedundantR=WordNumberR;
		WordNumberRedundantF=WordNumberF;
		while(WordNumberRedundantR < (DocR->m_WordsTotal - 1))
		{
			if( DocR->m_pSortedWordHash[WordNumberRedundantR + 1] == Hash ) WordNumberRedundantR++;
			else break;
		}
		while(WordNumberRedundantF < (DocF->m_WordsTotal - 1))
		{
			if( DocF->m_pSortedWordHash[WordNumberRedundantF + 1] == Hash ) WordNumberRedundantF++;
			else break;
		}
		for(iWordNumberR=WordNumberR;iWordNumberR<=WordNumberRedundantR;iWordNumberR++)	// loop for each copy of this word on the right
		{
			if( m_MatchMarkR[DocR->m_pSortedWordNumber[iWordNumberR]] != WORD_UNMATCHED ) continue;	// skip words that have been matched already
			for(iWordNumberF=WordNumberF;iWordNumberF<=WordNumberRedundantF;iWordNumberF++)	// loop for each copy of this word on the filter
			{
				// look up and down the hash-coded (not sorted) lists for matches
				FirstR=DocR->m_pSortedWordNumber[iWordNumberR]-1;	// start right just before current word
				LastR=DocR->m_pSortedWordNumber[iWordNumberR]+1;	// end right just after current word
				FirstF=DocF->m_pSortedWordNumber[iWordNumberF]-1;	// start filter just before current word
				LastF=DocF->m_pSortedWordNumber[iWordNumberF]+1;	// end filter just after current word

				while( (FirstR >= 0) && (FirstF >= 0) )		// if we aren't at the start of either document,
				{

					// Note: when we leave this loop, FirstR and FirstF will always point one word before the first match
					
					// make sure that right word hasn't been used in a match before and
					// that the two words actually match. If so, move up another word and repeat the test.

					if( m_MatchMarkR[FirstR] != WORD_UNMATCHED ) break;

					if( DocR->m_pWordHash[FirstR] == DocF->m_pWordHash[FirstF] )
					{
						FirstR--;									// move up on right
						FirstF--;									// move up on filter
						continue;
					}
					break;
				}

				while( (LastR < DocR->m_WordsTotal) && (LastF < DocF->m_WordsTotal) ) // if we aren't at the end of either document
				{

					// Note: when we leave this loop, LastR and LastF will always point one word after last match
					
					// make sure that right word hasn't been used in a match before and
					// that the two words actually match. If so, move up another word and repeat the test.

					if( m_MatchMarkR[LastR] != WORD_UNMATCHED ) break;
					if( DocR->m_pWordHash[LastR] == DocF->m_pWordHash[LastF] )
					{
						LastR++;								// move down on right
						LastF++;								// move down on filter
						continue;
					}
					break;
				}

				FirstRp=FirstR+1;						// pointer to first perfect match right
				FirstFp=FirstF+1;						// pointer to first perfect match filter
				LastRp=LastR-1;							// pointer to last perfect match right
				LastFp=LastF-1;							// pointer to last perfect match filter
				MatchingWordsPerfect=LastRp-FirstRp+1;	// save number of perfect matches

				if( MatchingWordsPerfect >= m_FilterPhraseLength )	// check that phrase has enough perfect matches in it to mark
				{
					for(i=FirstRp;i<=LastRp;i++)				// loop for all right matched words
					{
						m_MatchMarkR[i]=WORD_FILTERED;	// mark word as filtered
					}
				}
			}
		}
		WordNumberR=WordNumberRedundantR + 1;			// continue searching after the last redundant word on right
		WordNumberF=WordNumberRedundantF + 1;			// continue searching after the last redundant word on filter
	}
//
// now do the actual comparison between left and right documents
//
	WordNumberL=DocL->m_FirstHash;						// start left at first >3 letter word
	WordNumberR=DocR->m_FirstHash;						// start right at first >3 letter word

	Anchor=0;											// start with no html anchors assigned
						
	while ( (WordNumberL < DocL->m_WordsTotal)			// loop while there are still words to check
			&& (WordNumberR < DocR->m_WordsTotal) )
	{
		// if the next word in the left sorted hash-coded list has been matched
		if( m_MatchMarkL[DocL->m_pSortedWordNumber[WordNumberL]] != WORD_UNMATCHED )
		{
			WordNumberL++;								// advance to next left sorted hash-coded word
			continue;
		}

		// if the next word in the right sorted hash-coded list has been matched
		if( m_MatchMarkR[DocR->m_pSortedWordNumber[WordNumberR]] != WORD_UNMATCHED )
		{
			WordNumberR++;								// skip to next right sorted hash-coded word
			continue;
		}

		// check for left word less than right word
		if( DocL->m_pSortedWordHash[WordNumberL] < DocR->m_pSortedWordHash[WordNumberR] )
		{
			WordNumberL++;								// advance to next left word
			if ( WordNumberL >= DocL->m_WordsTotal) break;
			continue;									// and resume looping
		}

		// check for right word less than left word
		if( DocL->m_pSortedWordHash[WordNumberL] > DocR->m_pSortedWordHash[WordNumberR] )
		{
			WordNumberR++;								// advance to next right word
			if ( WordNumberR >= DocR->m_WordsTotal) break;
			continue;									// and resume looping
		}

		// we have a match, so check redundancy of this words and compare all copies of this word
		Hash=DocL->m_pSortedWordHash[WordNumberL];
		WordNumberRedundantL=WordNumberL;
		WordNumberRedundantR=WordNumberR;
		while(WordNumberRedundantL < (DocL->m_WordsTotal - 1))
		{
			if( DocL->m_pSortedWordHash[WordNumberRedundantL + 1] == Hash ) WordNumberRedundantL++;
			else break;
		}
		while(WordNumberRedundantR < (DocR->m_WordsTotal - 1))
		{
			if( DocR->m_pSortedWordHash[WordNumberRedundantR + 1] == Hash ) WordNumberRedundantR++;
			else break;
		}
		for(iWordNumberL=WordNumberL;iWordNumberL<=WordNumberRedundantL;iWordNumberL++)	// loop for each copy of this word on the left
		{
			if( m_MatchMarkL[DocL->m_pSortedWordNumber[iWordNumberL]] != WORD_UNMATCHED ) continue;	// skip words that have been matched already
			for(iWordNumberR=WordNumberR;iWordNumberR<=WordNumberRedundantR;iWordNumberR++)	// loop for each copy of this word on the right
			{
				if( m_MatchMarkR[DocR->m_pSortedWordNumber[iWordNumberR]] != WORD_UNMATCHED ) continue;	// skip words that have been matched already

				// look up and down the hash-coded (not sorted) lists for matches
				m_MatchMarkTempL[DocL->m_pSortedWordNumber[iWordNumberL]]=WORD_PERFECT;	// markup word in temporary list at perfection quality
				m_MatchMarkTempR[DocR->m_pSortedWordNumber[iWordNumberR]]=WORD_PERFECT;	// markup word in temporary list at perfection quality

				FirstL=DocL->m_pSortedWordNumber[iWordNumberL]-1;	// start left just before current word
				LastL=DocL->m_pSortedWordNumber[iWordNumberL]+1;	// end left just after current word
				FirstR=DocR->m_pSortedWordNumber[iWordNumberR]-1;	// start right just before current word
				LastR=DocR->m_pSortedWordNumber[iWordNumberR]+1;	// end right just after current word

				while( (FirstL >= 0) && (FirstR >= 0) )		// if we aren't at the start of either document,
				{

					// Note: when we leave this loop, FirstL and FirstR will always point one word before the first match
					
					// make sure that left and right words haven't been used in a match before and
					// that the two words actually match. If so, move up another word and repeat the test.

					if( m_MatchMarkL[FirstL] != WORD_UNMATCHED ) break;
					if( m_MatchMarkR[FirstR] != WORD_UNMATCHED ) break;

					if( DocL->m_pWordHash[FirstL] == DocR->m_pWordHash[FirstR] )
					{
						m_MatchMarkTempL[FirstL]=WORD_PERFECT;		// markup word in temporary list
						m_MatchMarkTempR[FirstR]=WORD_PERFECT;		// markup word in temporary list
						FirstL--;									// move up on left
						FirstR--;									// move up on right
						continue;
					}
					break;
				}

				while( (LastL < DocL->m_WordsTotal) && (LastR < DocR->m_WordsTotal) ) // if we aren't at the end of either document
				{

					// Note: when we leave this loop, LastL and LastR will always point one word after last match
					
					// make sure that left and right words haven't been used in a match before and
					// that the two words actually match. If so, move up another word and repeat the test.

					if( m_MatchMarkL[LastL] != WORD_UNMATCHED ) break;
					if( m_MatchMarkR[LastR] != WORD_UNMATCHED ) break;
					if( DocL->m_pWordHash[LastL] == DocR->m_pWordHash[LastR] )
					{
						m_MatchMarkTempL[LastL]=WORD_PERFECT;	// markup word in temporary list
						m_MatchMarkTempR[LastR]=WORD_PERFECT;	// markup word in temporary list
						LastL++;								// move down on left
						LastR++;								// move down on right
						continue;
					}
					break;
				}

				FirstLp=FirstL+1;						// pointer to first perfect match left
				FirstRp=FirstR+1;						// pointer to first perfect match right
				LastLp=LastL-1;							// pointer to last perfect match left
				LastRp=LastR-1;							// pointer to last perfect match right
				MatchingWordsPerfect=LastLp-FirstLp+1;	// save number of perfect matches

				if(m_MismatchTolerance > 0)				// are we accepting imperfect matches?
				{

					FirstLx=FirstLp;					// save pointer to word before first perfect match left
					FirstRx=FirstRp;					// save pointer to word before first perfect match right
					LastLx=LastLp;						// save pointer to word after last perfect match left
					LastRx=LastRp;						// save pointer to word after last perfect match right

					Flaws=0;							// start with zero flaws
					while( (FirstL >= 0) && (FirstR >= 0) )		// if we aren't at the start of either document,
					{

						// Note: when we leave this loop, FirstL and FirstR will always point one word before the first reportable match
						
						// make sure that left and right words haven't been used in a match before and
						// that the two words actually match. If so, move up another word and repeat the test.
						if( m_MatchMarkL[FirstL] != WORD_UNMATCHED ) break;
						if( m_MatchMarkR[FirstR] != WORD_UNMATCHED ) break;
						if( DocL->m_pWordHash[FirstL] == DocR->m_pWordHash[FirstR] )
						{
							MatchingWordsPerfect++;				// increment perfect match count;
							Flaws=0;							// having just found a perfect match, we're back to perfect matching
							m_MatchMarkTempL[FirstL]=WORD_PERFECT;			// markup word in temporary list
							m_MatchMarkTempR[FirstR]=WORD_PERFECT;			// markup word in temporary list
							FirstLp=FirstL;						// save pointer to first left perfect match
							FirstRp=FirstR;						// save pointer to first right perfect match
							FirstL--;							// move up on left
							FirstR--;							// move up on right
							continue;
						}

						// we're at a flaw, so increase the flaw count
						Flaws++;
						if( Flaws > m_MismatchTolerance ) break;	// check for maximum flaws reached
						
						if( (FirstL-1) >= 0 )					// check one word earlier on left (if it exists)
						{
							if( m_MatchMarkL[FirstL-1] != WORD_UNMATCHED ) break;	// make sure we haven't already matched this word
							
							if( DocL->m_pWordHash[FirstL-1] == DocR->m_pWordHash[FirstR] )
							{
								if( PercentMatching(FirstL-1,FirstR,LastLx,LastRx,MatchingWordsPerfect+1) < m_MismatchPercentage ) break;	// are we getting too imperfect?
								m_MatchMarkTempL[FirstL]=WORD_FLAW;	// markup non-matching word in left temporary list
								FirstL--;						// move up on left to skip over the flaw
								MatchingWordsPerfect++;			// increment perfect match count;
								Flaws=0;						// having just found a perfect match, we're back to perfect matching
								m_MatchMarkTempL[FirstL]=WORD_PERFECT;		// markup word in left temporary list
								m_MatchMarkTempR[FirstR]=WORD_PERFECT;		// markup word in right temporary list
								FirstLp=FirstL;					// save pointer to first left perfect match
								FirstRp=FirstR;					// save pointer to first right perfect match
								FirstL--;						// move up on left
								FirstR--;						// move up on right
								continue;
							}
						}

						if( (FirstR-1) >= 0 )					// check one word earlier on right (if it exists)
						{
							if( m_MatchMarkR[FirstR-1] != WORD_UNMATCHED ) break;	// make sure we haven't already matched this word

							if( DocL->m_pWordHash[FirstL] == DocR->m_pWordHash[FirstR-1] )
							{
								if( PercentMatching(FirstL,FirstR-1,LastLx,LastRx,MatchingWordsPerfect+1) < m_MismatchPercentage ) break;	// are we getting too imperfect?
								m_MatchMarkTempR[FirstR]=WORD_FLAW;	// markup non-matching word in right temporary list
								FirstR--;						// move up on right to skip over the flaw
								MatchingWordsPerfect++;			// increment perfect match count;
								Flaws=0;						// having just found a perfect match, we're back to perfect matching
								m_MatchMarkTempL[FirstL]=WORD_PERFECT;		// markup word in left temporary list
								m_MatchMarkTempR[FirstR]=WORD_PERFECT;		// markup word in right temporary list
								FirstLp=FirstL;					// save pointer to first left perfect match
								FirstRp=FirstR;					// save pointer to first right perfect match
								FirstL--;						// move up on left
								FirstR--;						// move up on right
								continue;
							}
						}

						if( PercentMatching(FirstL-1,FirstR-1,LastLx,LastRx,MatchingWordsPerfect) < m_MismatchPercentage ) break;	// are we getting too imperfect?
						m_MatchMarkTempL[FirstL]=WORD_FLAW;		// markup word in left temporary list
						m_MatchMarkTempR[FirstR]=WORD_FLAW;		// markup word in right temporary list
						FirstL--;								// move up on left
						FirstR--;								// move up on right
					}
		
					Flaws=0;							// start with zero flaws
					while( (LastL < DocL->m_WordsTotal) && (LastR < DocR->m_WordsTotal) ) // if we aren't at the end of either document
					{

						// Note: when we leave this loop, LastL and LastR will always point one word after last match
						
						// make sure that left and right words haven't been used in a match before and
						// that the two words actually match. If so, move up another word and repeat the test.
						if( m_MatchMarkL[LastL] != WORD_UNMATCHED ) break;
						if( m_MatchMarkR[LastR] != WORD_UNMATCHED ) break;
						if( DocL->m_pWordHash[LastL] == DocR->m_pWordHash[LastR] )
						{
							MatchingWordsPerfect++;				// increment perfect match count;
							Flaws=0;							// having just found a perfect match, we're back to perfect matching
							m_MatchMarkTempL[LastL]=WORD_PERFECT;	// markup word in temporary list
							m_MatchMarkTempR[LastR]=WORD_PERFECT;	// markup word in temporary list
							LastLp=LastL;						// save pointer to last left perfect match
							LastRp=LastR;						// save pointer to last right perfect match
							LastL++;							// move down on left
							LastR++;							// move down on right
							continue;
						}

						Flaws++;
						if( Flaws > m_MismatchTolerance ) break;	// check for maximum flaws reached
							
						if( (LastL+1) < DocL->m_WordsTotal )		// check one word later on left (if it exists)
						{
							if( m_MatchMarkL[LastL+1] != WORD_UNMATCHED ) break;	// make sure we haven't already matched this word
							
							if( DocL->m_pWordHash[LastL+1] == DocR->m_pWordHash[LastR] )
							{
								if( PercentMatching(FirstLx,FirstRx,LastL+1,LastR,MatchingWordsPerfect+1) < m_MismatchPercentage ) break;	// are we getting too imperfect?
								m_MatchMarkTempL[LastL]=WORD_FLAW;		// markup non-matching word in left temporary list
								LastL++;						// move down on left to skip over the flaw
								MatchingWordsPerfect++;			// increment perfect match count;
								Flaws=0;						// having just found a perfect match, we're back to perfect matching
								m_MatchMarkTempL[LastL]=WORD_PERFECT;	// markup word in lefttemporary list
								m_MatchMarkTempR[LastR]=WORD_PERFECT;	// markup word in right temporary list
								LastLp=LastL;					// save pointer to last left perfect match
								LastRp=LastR;					// save pointer to last right perfect match
								LastL++;						// move down on left
								LastR++;						// move down on right
								continue;
							}
						}

						if( (LastR+1) < DocR->m_WordsTotal )	// check one word later on right (if it exists)
						{
							if( m_MatchMarkR[LastR+1] != WORD_UNMATCHED ) break;	// make sure we haven't already matched this word

							if( DocL->m_pWordHash[LastL] == DocR->m_pWordHash[LastR+1] )
							{
								if( PercentMatching(FirstLx,FirstRx,LastL,LastR+1,MatchingWordsPerfect+1) < m_MismatchPercentage ) break;	// are we getting too imperfect?
								m_MatchMarkTempR[LastR]=WORD_FLAW;		// markup non-matching word in right temporary list
								LastR++;						// move down on right to skip over the flaw
								MatchingWordsPerfect++;			// increment perfect match count;
								Flaws=0;						// having just found a perfect match, we're back to perfect matching
								m_MatchMarkTempL[LastL]=WORD_PERFECT;	// markup word in left temporary list
								m_MatchMarkTempR[LastR]=WORD_PERFECT;	// markup word in right temporary list
								LastLp=LastL;					// save pointer to last left perfect match
								LastRp=LastR;					// save pointer to last right perfect match
								LastL++;						// move down on left
								LastR++;						// move down on right
								continue;
							}
						}

						if( PercentMatching(FirstLx,FirstRx,LastL+1,LastR+1,MatchingWordsPerfect) < m_MismatchPercentage ) break;	// are we getting too imperfect?
						m_MatchMarkTempL[LastL]=WORD_FLAW;		// markup word in left temporary list
						m_MatchMarkTempR[LastR]=WORD_FLAW;		// markup word in right temporary list
						LastL++;								// move down on left
						LastR++;								// move down on right
					}
				}
				if( MatchingWordsPerfect >= m_PhraseLength )	// check that phrase has enough perfect matches in it to mark
				{
					Anchor++;									// increment anchor count
					for(i=FirstLp;i<=LastLp;i++)				// loop for all left matched words
					{
						m_MatchMarkL[i]=m_MatchMarkTempL[i];	// copy over left matching markup
						if(m_MatchMarkTempL[i]==WORD_PERFECT) m_MatchingWordsPerfect++;	// count the number of perfect matching words (same as for right document)
						m_MatchAnchorL[i]=Anchor;				// identify the anchor for this phrase
					}
					m_MatchingWordsTotalL += LastLp-FirstLp+1;	// add the number of words in the matching phrase, whether perfect or flawed matches
					for(i=FirstRp;i<=LastRp;i++)				// loop for all right matched words
					{
						m_MatchMarkR[i]=m_MatchMarkTempR[i];	// copy over right matching markup
						m_MatchAnchorR[i]=Anchor;				// identify the anchor for this phrase
					}
					m_MatchingWordsTotalR += LastRp-FirstRp+1;	// add the number of words in the matching phrase, whether perfect or flawed matches
				}
			}
		}
		WordNumberL=WordNumberRedundantL + 1;			// continue searching after the last redundant word on left
		WordNumberR=WordNumberRedundantR + 1;			// continue searching after the last redundant word on right
	}

	m_Anchors=Anchor;									// remember how many matching phrases were found
	m_Compares++;										// increment count of comparisons
	if( (m_Compares%m_CompareStep)	== 0 )				// if count is divisible by 1000,
	{
		fwprintf(m_fLog,L"Comparing Documents, %lld Completed\n",m_Compares);
		fflush(m_fLog);
	}
	return -1;
}

void CCompareDocuments::SetupProgressReports(int Group1,int Group2,int Group3)
{
	int i;												// local index counter
	int Group1Count=0;
	int Group2Count=0;
	int Group3Count=0;

	for (i=0;i<m_Documents;i++)
	{
		if( (m_pDocs+i)->m_DocumentType == Group1 ) Group1Count++;
		if( (m_pDocs+i)->m_DocumentType == Group2 ) Group2Count++;
		if( (m_pDocs+i)->m_DocumentType == Group3 ) Group3Count++;
	}
	if(Group1 == DOC_TYPE_UNDEFINED) Group1Count=0;	// ignore this group
	if(Group2 == DOC_TYPE_UNDEFINED) Group2Count=0;	// ignore this group
	if(Group3 == DOC_TYPE_UNDEFINED) Group3Count=0;	// ignore this group

	m_TotalCompares=(Group1Count * Group2Count) + ((Group3Count * (Group3Count-1))/2);

	if(m_TotalCompares<100) m_CompareStep=1;
	else if(m_TotalCompares<1000) m_CompareStep=10;
	else if(m_TotalCompares<10000) m_CompareStep=100;
	else m_CompareStep=1000;
}

void CCompareDocuments::SetupLoading()
{
	fwprintf(m_fLog,L"Starting to Load and Hash-Code Documents\n");
	fflush(m_fLog);
}

void CCompareDocuments::FinishLoading()
{
	fwprintf(m_fLog,L"Done Loading Documents\n");
	fflush(m_fLog);
}

int	CCompareDocuments::SetupComparisons()
{
	fwprintf(m_fLog,L"Starting to Compare Documents\n");
	fflush(m_fLog);

	m_MatchMarkL.resize(m_WordsAllocated);			// allocate array for left match markers
	m_MatchMarkR.resize(m_WordsAllocated);			// allocate array for right match markers
	m_MatchAnchorL.resize(m_WordsAllocated);		// allocate array for left match anchors
	m_MatchAnchorR.resize(m_WordsAllocated);		// allocate array for right match anchors
	m_MatchMarkTempL.resize(m_WordsAllocated);		// allocate array for left match markers - temporary
	m_MatchMarkTempR.resize(m_WordsAllocated);		// allocate array for right match markers - temporary

	return -1;
}

void CCompareDocuments::FinishComparisons()
{
	fwprintf(m_fLog,L"Done Comparing Documents\n");
	fflush(m_fLog);
}

int CCompareDocuments::PercentMatching(int FirstL,int FirstR,int LastL,int LastR,int PerfectMatchingWords)
{
	return (200*PerfectMatchingWords)/(LastL-FirstL+LastR-FirstR+2);
}
