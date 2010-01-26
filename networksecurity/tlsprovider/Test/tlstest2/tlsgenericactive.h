// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file tlsgenericactive.h
 @internalTechnology	
*/
#ifndef __TLSGENERICACTIVE_H__
#define __TLSGENERICACTIVE_H__

class CGenericActive : public CActive
	{
public:
	CGenericActive()
		: CActive(EPriorityNormal)
		{
		CActiveScheduler::Add(this);
		};
		
	void Start()
		{
		SetActive();
		CActiveScheduler::Start();
		};
	
	void RunL()
		{
		CActiveScheduler::Stop();
		};
	
	void DoCancel()
		{
		// do nothing, just wait for completion
		};
	};

#endif /* __TLSGENERICACTIVE_H__ */

