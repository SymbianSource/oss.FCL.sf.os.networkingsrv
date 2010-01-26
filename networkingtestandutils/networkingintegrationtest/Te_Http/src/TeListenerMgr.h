// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __TELISTENERMGR_H__
#define __TELISTENERMGR_H__

#include "TeSocketListener.h"
#include "TeSocketConnector.h"
#include <es_sock.h>
#include <e32std.h>

class CTestSocketListener;
class CTestSocketConnector;

class CTestListenerMgr : public CBase
{
public:

	CTestListenerMgr();
	~CTestListenerMgr();

	CTestSocketListener*	CreateListenerL(TPtrC aListener, CTestStepBase* aTestStepBase);
	CTestSocketConnector*	CreateConnectorL(TPtrC aConnector, CTestStepBase* aTestStepBase);


public:
/**	The CTestSocketListener object for listener socket
*/
	CTestSocketListener*			iListener;

/**	The CTestSocketConnector object for connector socket
*/
	CTestSocketConnector*			iConnector;

/**	The RSocketServ object for listener socket
*/
	RSocketServ						iSocketServOne;

/**	The RSocketServ object for connector socket
*/
	RSocketServ						iSocketServTwo;
};

#endif // __TELISTENERMGR_H__
