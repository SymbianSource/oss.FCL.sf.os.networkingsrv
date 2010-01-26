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
// STEPBASE.H
// 
//

/**
 @file cachestepbase.h
 @internalTechnology	
*/

#ifndef __CACHESTEPBASE_H__
#define __CACHESTEPBASE_H__

#include <x509cert.h>
#include "tlscacheclient.h"
#include "tlsstepbase.h"


//class CCacheStepBase : public CTestStep
class CCacheStepBase : public CTlsStepBase
	{
	
public:
	void InitializeL();
	
	inline const CX509Certificate& Certificate();
	HBufC* SubjectLC();
	inline RTlsCacheClient& Session();
	
	~CCacheStepBase();
private:
	CX509Certificate* iCertificate;
	RTlsCacheClient iSession;
	
	};

inline const CX509Certificate& CCacheStepBase::Certificate()
	{
	return *iCertificate;
	}
	
inline RTlsCacheClient& CCacheStepBase::Session()
	{
	return iSession;
	}

#endif /* __CACHESTEPBASE_H__ */
