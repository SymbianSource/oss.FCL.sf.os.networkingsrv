// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This contains TS_QoS TestCase Open and Close Server
// 
//

// EPOC includes
#include <e32base.h>

// Test system includes
#include "TS_QoSSocketServer.h"

/*
 * Open Socket Server
 *
 */
CTS_QoSOpenServer::CTS_QoSOpenServer()
	{
	// store the name of this test case
	iTestStepName = _L("OpenServer");
	}

CTS_QoSOpenServer::~CTS_QoSOpenServer()
	{
	}

enum TVerdict CTS_QoSOpenServer::doTestStepL(void)
	{
	TRAPD(ret, iQoSSuite->iSocketServer.Connect());
	if (ret==KErrNone)
		return EPass;
	else
		{
		Log(_L("Connecting to socket server failed with %d"), ret);
		return EFail;
		}
	}

/*
 * Close Socket Server
 *
 */
CTS_QoSCloseServer::CTS_QoSCloseServer()
{
	// store the name of this test case
	iTestStepName = _L("CloseServer");
}

CTS_QoSCloseServer::~CTS_QoSCloseServer()
{
}

enum TVerdict CTS_QoSCloseServer::doTestStepL( void )
{
	iQoSSuite->iSocketServer.Close();

	return EPass;
}





