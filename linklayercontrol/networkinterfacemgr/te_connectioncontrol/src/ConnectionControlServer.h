/**
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Defines the testexecute server for the Nifman configuration control mechanism.
* 
*
*/



/**
 @file
 @internalComponent
*/

#ifndef __CONNECTION_CONTROL_SERVER_H__
#define __CONNECTION_CONTROL_SERVER_H__


#include <testexecuteserverbase.h>


class CConnectionControlServer : public CTestServer
/**
 * Connection control test server class.
 *
 * @internalComponent
 */
	{
public:
	static CConnectionControlServer* NewL(void);
	virtual CTestStep* CreateTestStep(const TDesC& aStepName);
	};

	
#endif // __CONNECTION_CONTROL_SERVER_H__

