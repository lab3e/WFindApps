// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Louis A. Bloomfield
//
#include "stdafx.h"	// the including project's stdafx.h, found via its include path
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <afx.h>
#include <afxwin.h>
#include <afxinet.h>
#include <wininet.h>
#include <locale.h>
#include "InputDocument.h"
#include "words.h"

void WordRemovePunctuation(wchar_t *word)
{
	int wordlen;
	int ccnt;
	int icnt;
	
	wordlen=(int)wcslen(word);
	for(ccnt=0;ccnt<wordlen;ccnt++)
	{
		if(iswpunct(word[ccnt]))
		{
			for(icnt=ccnt;icnt<wordlen;icnt++) word[icnt]=word[icnt+1]; // move the null, too.
			wordlen--;
			ccnt--;
		}
	}
}

void wordxouterpunct(wchar_t *word)
{
	int wordlen;
	int ccnt;
	int icnt;
	
	wordlen=(int)wcslen(word);
	for(ccnt=0;ccnt<wordlen;ccnt++)
	{
		if(iswpunct(word[ccnt]))
		{
			for(icnt=ccnt;icnt<wordlen;icnt++)	word[icnt]=word[icnt+1]; // move the null, too.
			wordlen--;
			ccnt--;
		}
		else break;
	}
	for(ccnt=wordlen-1;ccnt>=0;ccnt--)
	{
		if(iswpunct(word[ccnt]))
		{
			for(icnt=ccnt;icnt<wordlen;icnt++) word[icnt]=word[icnt+1];	// move the null, too.
			wordlen--;
		}
		else break;
	}
}

void WordRemoveNumbers(wchar_t *word)
{
	int wordlen;
	int ccnt;
	int icnt;
	
	wordlen=(int)wcslen(word);
	for(ccnt=0;ccnt<wordlen;ccnt++)
	{
		if(iswdigit(word[ccnt]))
		{
			for(icnt=ccnt;icnt<wordlen;icnt++) word[icnt]=word[icnt+1];	// move the null, too.
			wordlen--;
			ccnt--;
		}
	}
}

void WordToLowerCase(wchar_t *word)
{
	int wordlen;
	int ccnt;

	wordlen=(int)wcslen(word);
	for(ccnt=0;ccnt<wordlen;ccnt++)
	{
		if(iswupper(word[ccnt])) word[ccnt]=towlower(word[ccnt]);
	}
}

bool WordCheck(wchar_t *word)
{
	int wordlen;
	int ccnt;

	wordlen=(int)wcslen(word);

	if(wordlen < 1) return false;
	if( !iswalpha(word[0]) ) return false;
	if( !iswalpha(word[wordlen-1]) ) return false;

	for(ccnt=1;ccnt<wordlen-1;ccnt++)
	{
		if( iswalpha(word[ccnt]) ) continue;
		if( word[ccnt] == '-' ) continue;
		if( word[ccnt] == '\'' ) continue;
		return false;
	}
	return true;
}

unsigned long WordHash(wchar_t *word)
{
	unsigned long inhash;
	inhash = 0;

	int charcount;
	charcount=0;

	if(word[0] == 0) return 1;	// if word is null, return 1 as hash value
	else while(word[charcount] != 0)
	{
		inhash=	((inhash << 7)|(inhash >> 25)) ^ word[charcount];	// xor into the rotateleft(7) of inhash
		charcount++;							// and increment the count of characters in the word
	}
	return inhash;
}