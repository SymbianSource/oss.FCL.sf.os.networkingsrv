// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Interface for the CTlsCacheServer class
// 
//

/**
 @file 
 @internalTechnology
*/
 
#ifndef __TLSCACHESERVER_H__
#define __TLSCACHESERVER_H__

#include <e32base.h>

class CTlsCache;
class CShutdown;

class CTlsCacheServer : public CServer2
	{
	
public:
	static CTlsCacheServer* NewLC();
	
	void AddSession();
	void DropSession();
	
	~CTlsCacheServer();
private:
	CTlsCacheServer();
	void ConstructL();

	CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMessage) const;
	
private:
	CTlsCache* iCache;
	CShutdown* iShutdown;
	
	TInt iSessionCount;
	};
	
const TInt KTlsCacheShutdownDelay = 0x200000;// approx 2s

class CShutdown : public CTimer
	{
public:
	CShutdown();
	void ConstructL();
	void Start();
	
private:
	void RunL();
	};

inline CShutdown::CShutdown()
	:CTimer(EPriorityStandard)
	{
	CActiveScheduler::Add(this);
	}
	
inline void CShutdown::ConstructL()
	{
	CTimer::ConstructL();
	}
	
inline void CShutdown::Start()
	{
	After(KTlsCacheShutdownDelay);
	}

#endif /* __TLSCACHESERVER_H__ */
