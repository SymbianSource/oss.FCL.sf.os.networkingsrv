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
// ExampleTest.cpp
// This file contains an example Test step implementation 
// This demonstrates the various functions provided
// by the CTestStep base class which are available within
// a test step 
// 
//


#if (!defined __QOSSOCKETSECTION_H__)
#define __QOSSOCKETSECTION_H__

#include "TS_QoSStep.h"
#include "TS_QoSSuite.h"


/* QoS TestCase 1_3 Class Definition
 */
class CTS_QoSSocketSection1_3 : public CTS_QoSStep
{
public:
	CTS_QoSSocketSection1_3();
	~CTS_QoSSocketSection1_3();
	virtual enum TVerdict doTestStepL( void );

};


/* QoS TestCase CEsockSendAndRecvData Class Definition
 */
class CTS_CEsockSendAndRecvData : public CTS_QoSStep
{
public:
	CTS_CEsockSendAndRecvData();
	~CTS_CEsockSendAndRecvData();
	virtual enum TVerdict doTestStepL( void );

};


#endif (__QOSSOCKETSECTION_H__)
