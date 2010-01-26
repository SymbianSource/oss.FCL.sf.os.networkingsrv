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
// QoSSocketServer.cpp
// 
//


#if (!defined __QOSSOCKETSERVER_H__)
#define __QOSSOCKETSERVER_H__

#include "TS_QoSStep.h"
#include "TS_QoSSuite.h"

/* Open Socket Server Class Definition
 */
class CTS_QoSOpenServer : public CTS_QoSStep
{
public:
	CTS_QoSOpenServer();
	~CTS_QoSOpenServer();

	virtual enum TVerdict doTestStepL( void );

};

/* Close Socket Server Class Definition
 */
class CTS_QoSCloseServer : public CTS_QoSStep
{
public:
	CTS_QoSCloseServer();
	~CTS_QoSCloseServer();

	virtual enum TVerdict doTestStepL( void );

};

#endif (__QOSSOCKETSERVER_H__)
