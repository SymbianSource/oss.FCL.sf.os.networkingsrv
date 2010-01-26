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
 @file tlstest2server.h
 @internalTechnology	
*/
#ifndef __TLSTEST2SERVER_H__
#define __TLSTEST2SERVER_H__

#include <e32base.h>
#include <testexecuteserverbase.h>
#include <hash.h>

_LIT(KTlsTest2Server, "tlstest2");

class CTlsTest2Server : public CTestServer
	{
public:
	static CTlsTest2Server* NewLC();
	~CTlsTest2Server();
	
	CTestStep* CreateTestStep(const TDesC& aStepName);
private:
	CSHA1 *iSha;
	};


#endif /* __TLSTEST2SERVER_H__ */
