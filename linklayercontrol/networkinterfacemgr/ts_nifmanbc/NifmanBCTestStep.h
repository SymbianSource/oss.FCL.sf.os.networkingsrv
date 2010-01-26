// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __NIFMANBCTESTSTEP_H__
#define __NIFMANBCTESTSTEP_H__

#include <es_sock.h>
#include <nifman.h>
#include <agentclient.h>
#include <networking/teststep.h>

class CNifmanBCTestSuite;
class CNifmanBCTestStep : public CTestStep
	{
public:
	CNifmanBCTestSuite* iTestSuite;
	};

#endif

