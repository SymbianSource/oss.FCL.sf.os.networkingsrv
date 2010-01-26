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

#if !defined(__CHAT_H__)
#define __CHAT_H__

#ifndef	_NOECONS
#define	_NOECONS
#endif

#include <e32base.h>

class CCommChatString;

class MCommChatNotify
/**
@internalComponent
*/
	{
public:
	virtual void ChatStringMatch(CCommChatString* aString)=0;
	virtual void ChatTimeout()=0;
	};

class CCommChatter : public CTimer
/**
@internalComponent
*/
	{
public:
	CCommChatter(MCommChatNotify* aNotify, TInt aPriority);
	virtual ~CCommChatter();	
	void CreateL(TInt aBufferSize);
	TBool Match(const CCommChatString* aString) const;
	TBool MatchF(const CCommChatString* aString) const;
	void AddChar(TText8 aChar);
	void ClearHistory();
	void AddString(CCommChatString* aString);
	void RemoveString(CCommChatString* aString);
	void StartTimer(TTimeIntervalMicroSeconds32 aTimeout);
	void StopTimer();
	void DeleteAllAndStop();
protected:
	virtual void RunL();
protected:
	MCommChatNotify* iNotify;
	TDblQue<CCommChatString> iList;
	TText8* iBuffer;
	TText8* iBufferEnd;
	TText8* iLastChar;	// Last character added
	TInt iCount;		// No of chars added to buffer
	};

class CCommChatString : public CBase
/**
@internalComponent
*/
	{
public:
	static CCommChatString* NewL(const TDesC8& aDes, TBool aIsFolded);
	CCommChatString();
	virtual ~CCommChatString();
	void CreateL(const TDesC8& aDes, TBool aIsFolded);
	inline TText8 LastChar() const { return *iLastChar; }
	inline TInt Length() const { return (iLastChar-iString)+1; }
	inline const TText8* Ptr() const { return iString; }
	inline const TText8* EndPtr() const { return iLastChar; }
	inline TBool IsFolded() const { return iIsFolded; }
	inline TPtrC8 Des() { return TPtrC8(Ptr(), Length()); }
protected:
	TBool iIsFolded;
	TText8* iString;
	TText8* iLastChar;
private:
	friend class CCommChatter;
	TDblQueLink iLink;
	};

#endif
