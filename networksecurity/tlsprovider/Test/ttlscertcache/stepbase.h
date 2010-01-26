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
//

#ifndef __STEPBASE_H__
#define __STEPBASE_H__

#include <x509cert.h>
#include <testexecuteserverbase.h>
#include "tlscacheclient.h"


class CStepBase : public CTestStep
	{
	
public:
	void InitializeL();
	
	inline const CX509Certificate& Certificate();
	HBufC* SubjectLC();
	inline RTlsCacheClient& Session();
	
	~CStepBase();
private:
	CX509Certificate* iCertificate;
	RTlsCacheClient iSession;
	
	};

inline const CX509Certificate& CStepBase::Certificate()
	{
	return *iCertificate;
	}
	
inline RTlsCacheClient& CStepBase::Session()
	{
	return iSession;
	}

#endif /* __STEPBASE_H__ */
