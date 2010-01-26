// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
//

#include "chat.h"

CCommChatString* CCommChatString::NewL(const TDesC8& aDes, TBool aIsFolded)
//
//
//
	{
	CCommChatString* cs = new (ELeave) CCommChatString;
	CleanupStack::PushL(cs);
	cs->CreateL(aDes, aIsFolded);
	CleanupStack::Pop();
	return cs;
	}

CCommChatString::CCommChatString()
//
//
//
	{
	__DECLARE_NAME(_S("CCommChatString"));
	}

CCommChatString::~CCommChatString()
//
//
//
	{
	User::Free(iString);
	}

void CCommChatString::CreateL(const TDesC8& aDes, TBool aIsFolded)
//
//
//
	{
	iIsFolded = aIsFolded;
	iString = (TText8*) User::AllocL(aDes.Length());
	iLastChar = (iString+aDes.Length())-1;
	Mem::Copy(iString, (TUint8*)aDes.Ptr(), aDes.Length());
	}

CCommChatter::CCommChatter(MCommChatNotify* aNotify, TInt aPriority)
//
//
//
	: CTimer(aPriority), iNotify(aNotify)
	{
	__DECLARE_NAME(_S("CCommChatter"));
	CActiveScheduler::Add(this);
	iList.SetOffset(_FOFF(CCommChatString,iLink));
	}

CCommChatter::~CCommChatter()
//
//
//
	{
	DeleteAllAndStop();
	User::Free(iBuffer);
	}

void CCommChatter::CreateL(TInt aBufSize)
//
//
//
	{
	CTimer::ConstructL();
	iBuffer = (TText8*)User::AllocL(aBufSize);
	iBufferEnd = (iBuffer+aBufSize)-1;
	ClearHistory();
	}

void CCommChatter::ClearHistory()
//
// Empty history
//
	{
	iLastChar = iBuffer;
	iCount = 0;
	}

void CCommChatter::AddChar(TText8 aChar)
//
// Add a character to the history buffer
// Scan all strings to find any matches that may
// be completed.
//
	{
	// Add char to buffer
	if (++iLastChar>iBufferEnd)
		iLastChar = iBuffer;
	*iLastChar = aChar;
	++iCount;

	TText8 fchar = (TText8)User::Fold(aChar);

	// Scan for matching last character	
	CCommChatString* cs;
	TDblQueIter<CCommChatString> iter(iList);
	
	while (cs = iter++, cs!=NULL)
		{
		if (cs->IsFolded()
			? (cs->LastChar()==fchar && MatchF(cs))
			: (cs->LastChar()==aChar && Match(cs)) )
			{
			iNotify->ChatStringMatch(cs);
			cs = iter;	// In case user removed cs;
			}
		}
	}


TBool CCommChatter::Match(const CCommChatString* aString) const
//
// Match a chat sgring against the history buffer
// (Case sensitive)
//
	{
	const TText8* s = aString->Ptr();
	const TText8* sp = aString->EndPtr();
	const TText8* bp = iLastChar;

	if (iCount<aString->Length())
		return EFalse;

	while (*bp==*sp && sp>=s)
		{
		--sp;
		if (--bp<iBuffer)
			bp = iBufferEnd;
		}

	return sp<s;
	}

TBool CCommChatter::MatchF(const CCommChatString* aString) const
//
// Match a folded chat sgring against the history buffer.
// (Case insensitive)
//
	{
	const TText8* s = aString->Ptr();
	const TText8* sp = aString->EndPtr();
	const TText8* bp = iLastChar;

	if (iCount<aString->Length())
		return EFalse;

	while (User::Fold(*bp)==*sp && sp>=s)
		{
		--sp;
		if (--bp<iBuffer)
			bp = iBufferEnd;
		}

	return sp<s;
	}

void CCommChatter::RunL()
//
// Timer completed
//
	{
	iNotify->ChatTimeout();
	}

void CCommChatter::AddString(CCommChatString* aString)
//
// Add a string to the list
//
	{
	iList.AddLast(*aString);
	}

void CCommChatter::RemoveString(CCommChatString* aString)
//
// Remove a string
//
	{
	aString->iLink.Deque();
	}

void CCommChatter::DeleteAllAndStop()
//
// Remove a string
//
	{
	StopTimer();
	CCommChatString* cs;
	while (!iList.IsEmpty())
		{
		cs = iList.First();
		RemoveString(cs);
		delete cs;
		}
	}

void CCommChatter::StartTimer(TTimeIntervalMicroSeconds32 aTimeout)
//
//
//
	{
	if (IsActive())
		Cancel();
	After(aTimeout);
	}

void CCommChatter::StopTimer()
//
//
//
	{
	Cancel();
	}
