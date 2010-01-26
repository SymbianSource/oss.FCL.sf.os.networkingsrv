// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file
 @internalTechnology
*/

#ifndef __TE_UNITTEST_IDNASERVER_H__
#define __TE_UNITTEST_IDNASERVER_H__

#include <testexecuteserverbase.h>

class CIdnaTestServer : public CTestServer
	{
public:
	static CIdnaTestServer* NewL();
	virtual CTestStep* CreateTestStep(const TDesC& aStepName);
	};

#endif //__TE_UNITTEST_IDNASERVER_H__
